// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Authors: Hyo-Kyung Lee <hyoklee@hdfgroup.org> and Kent Yang
// <myang6@hdfgroup.org> 

// Copyright (c) 2009-2023 The HDF Group, Inc. and OPeNDAP, Inc.
//
// This is H5free_memory software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
//
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 410 E University Ave,
// Suite 200, Champaign, IL 61820  
/// \file HDF5Array.cc
/// \brief This file contains the implementation on data array reading for the default option.
///
/// It includes the methods to read data array into DAP buffer from an HDF5 dataset.
/// \author Hyo-Kyung Lee   (hyoklee@hdfgroup.org)
/// \author Kent Yang       (myang6@hdfgroup.org)
/// \author James Gallagher (jgallagher@opendap.org)

#include <iostream>
#include <memory>

#include <libdap/Error.h>
#include <libdap/InternalErr.h>

#include <BESDebug.h>
#include <BESInternalError.h>

#include "HDF5Array.h"
#include "HDF5Structure.h"
#include "HDF5Str.h"

using namespace std;
using namespace libdap;

BaseType *HDF5Array::ptr_duplicate() {
    auto HDF5Array_unique = make_unique<HDF5Array>(*this);
    return HDF5Array_unique.release();
}

HDF5Array::HDF5Array(const string & n, const string &d, BaseType * v) :
    Array(n, d, v) {
}

int64_t HDF5Array::format_constraint(int64_t *offset, int64_t *step, int64_t *count) {

    // For the 0-length array case, just return 0.
    if(length() == 0)
        return 0;

    int64_t nels = 1;
    int id = 0;

    Dim_iter p = dim_begin();

    while (p != dim_end()) {

        int64_t start = dimension_start_ll(p, true);
        int64_t stride = dimension_stride_ll(p, true);
        int64_t stop = dimension_stop_ll(p, true);

        // Check for empty constraint
        if (start > stop) {
            ostringstream oss;

            oss << "Array/Grid hyperslab start point "<< start <<
                " is greater than stop point " <<  stop <<".";
            throw Error(malformed_expr, oss.str());
        }

        offset[id] = start;
        step[id] = stride;
        count[id] = ((stop - start) / stride) + 1; // count of elements
        nels *= count[id]; // total number of values for variable

        BESDEBUG("h5",
            "=format_constraint():"
            << "id=" << id << " offset=" << offset[id]
            << " step=" << step[id]
            << " count=" << count[id]
            << endl);

        id++;
        p++;
    }

    return nels;
}


bool HDF5Array::read()
{
    BESDEBUG("h5",
	    ">read() dataset=" << dataset()
	    << " dimension=" << d_num_dim
	    << " data_size=" << d_memneed << " length=" << length()
	    << endl);

    hid_t file_id = H5Fopen(dataset().c_str(),H5F_ACC_RDONLY,H5P_DEFAULT);

    BESDEBUG("h5","variable name is "<<name() <<endl);
    BESDEBUG("h5","variable path is  "<<var_path <<endl);

    hid_t dset_id = -1;

    if (true == is_dap4())
        dset_id = H5Dopen2(file_id,var_path.c_str(),H5P_DEFAULT);
    else 
        dset_id = H5Dopen2(file_id,name().c_str(),H5P_DEFAULT);

    BESDEBUG("h5","after H5Dopen2 "<<endl);

    hid_t dtype_id = H5Dget_type(dset_id);
    if(dtype_id < 0) {
        H5Dclose(dset_id);
        H5Fclose(file_id);
        string msg = "Fail to obtain the datatype .";
        throw InternalErr(__FILE__,__LINE__, msg);
    }

 
    vector<int64_t> offset(d_num_dim);
    vector<int64_t> count(d_num_dim);
    vector<int64_t> step(d_num_dim);
    int64_t nelms = format_constraint(offset.data(), step.data(), count.data()); // Throws Error.
    vector<char>values;

    // We only map the reference to URL when the dataset is an array of reference.
    if (get_dap_type(dtype_id,is_dap4()) == "Url") {
        bool ret_ref = false;
        try {
	    ret_ref = m_array_of_reference(dset_id,dtype_id);
            H5Tclose(dtype_id);
            H5Dclose(dset_id);
            H5Fclose(file_id);
 
        }
        catch(...) {
            H5Tclose(dtype_id);
            H5Dclose(dset_id);
            H5Fclose(file_id);
            throw;
 
        }
        return ret_ref;
    }

    try {
        do_array_read(dset_id,dtype_id,values,nelms,offset.data(),count.data(),step.data());
    }
    catch(...) {
        H5Tclose(dtype_id);
        H5Dclose(dset_id);
        H5Fclose(file_id);
        throw; 
    }

    H5Tclose(dtype_id);
    H5Dclose(dset_id);
    H5Fclose(file_id);
    
    return true;
}

void HDF5Array::do_array_read(hid_t dset_id,hid_t dtype_id,vector<char>&values,
                                   int64_t nelms,const int64_t* offset,const int64_t* count, const int64_t* step)
{

    H5T_class_t  tcls = H5Tget_class(dtype_id);
    bool has_values = false;
    size_t values_offset = 0;

    if (H5T_COMPOUND == tcls)
        m_array_of_structure(dset_id,values,has_values,values_offset,nelms,offset,count,step);
    else if(H5T_INTEGER == tcls || H5T_FLOAT == tcls || H5T_STRING == tcls) 
        m_array_of_atomic(dset_id,dtype_id,nelms,offset,count,step);
    else if (H5T_ENUM == tcls) {
        hid_t basetype = H5Tget_super(dtype_id);
        m_array_of_atomic(dset_id,basetype,nelms,offset,count,step);
        H5Tclose(basetype);
    }
    else {
        string msg = "Fail to read the data for Unsupported datatype.";
        throw InternalErr(__FILE__,__LINE__, msg);
    }
    
}

void HDF5Array:: m_array_of_atomic(hid_t dset_id, hid_t dtype_id, int64_t nelms,const int64_t* offset,
                                   const int64_t* count, const int64_t* step)
{
    
    hid_t memtype = -1;
    if((memtype = H5Tget_native_type(dtype_id, H5T_DIR_ASCEND))<0) {
        string msg = "Fail to obtain memory datatype.";
        throw InternalErr (__FILE__, __LINE__, msg);
    }

    // First handle variable-length string 
    if (H5Tis_variable_str(memtype) && H5Tget_class(memtype) == H5T_STRING) {

        vector<hsize_t> hoffset;
        vector<hsize_t>hcount;
        vector<hsize_t>hstep;
        hoffset.resize(d_num_dim);
        hcount.resize(d_num_dim);
        hstep.resize(d_num_dim);
        for (int i = 0; i <d_num_dim; i++) {
            hoffset[i] = (hsize_t) offset[i];
            hcount[i] = (hsize_t) count[i];
            hstep[i] = (hsize_t) step[i];
        }
        handle_vlen_string(dset_id, memtype, nelms, hoffset, hcount, hstep);
	    return ;
    }

    try {
        if (nelms == (int64_t) d_num_elm)
            handle_array_read_whole(dset_id, memtype, nelms);
        else
            handle_array_read_slab(dset_id, memtype, nelms, offset, step, count);

        H5Tclose(memtype);
    }
    catch (...) {
        H5Tclose(memtype);
        throw;
    }

}

void HDF5Array::handle_array_read_whole(hid_t dset_id, hid_t memtype, int64_t nelms) {

    vector<char> convbuf(d_memneed);
    get_data(dset_id, (void *) convbuf.data());

    // Check if a Signed Byte to Int16 conversion is necessary, this is only valid for DAP2.
    if (false == is_dap4() && (1 == H5Tget_size(memtype) && H5T_SGN_2 == H5Tget_sign(memtype))) {
        vector<short> convbuf2(nelms);
        for (int64_t i = 0; i < nelms; i++) {
            convbuf2[i] = (signed char) (convbuf[i]);
            BESDEBUG("h5", "convbuf[" << i << "]="
                                      << (signed char) convbuf[i] << endl);
            BESDEBUG("h5", "convbuf2[" << i << "]="
                                       << convbuf2[i] << endl);
        }
        m_intern_plain_array_data((char *) convbuf2.data(), memtype);

    } else
        m_intern_plain_array_data(convbuf.data(), memtype);

}

void HDF5Array::handle_array_read_slab(hid_t dset_id, hid_t memtype, int64_t nelms,
                                       const int64_t *offset, const int64_t *step, const int64_t *count)
{
    size_t data_size = nelms * H5Tget_size(memtype);
    if (data_size == 0) {
        string msg = "H5Tget_size failed.";
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    vector<char> convbuf(data_size);
    get_slabdata(dset_id, offset, step, count, d_num_dim, convbuf.data());

    // Check if a Signed Byte to Int16 conversion is necessary.
    if (false == is_dap4() && (1 == H5Tget_size(memtype) && H5T_SGN_2 == H5Tget_sign(memtype))) {
        vector<short> convbuf2(data_size);
        for (int64_t i = 0; i < (int)data_size; i++)
            convbuf2[i] = static_cast<signed char> (convbuf[i]);
        m_intern_plain_array_data((char*) convbuf2.data(),memtype);
    }
    else
        m_intern_plain_array_data(convbuf.data(),memtype);
}

void HDF5Array::handle_vlen_string(hid_t dset_id, hid_t memtype, int64_t nelms, const vector<hsize_t>& hoffset,
                                   const vector<hsize_t>& hcount, const vector<hsize_t>& hstep){

    vector<string>finstrval;
    finstrval.resize(nelms);
    try {
        read_vlen_string(dset_id, nelms, hoffset.data(), hstep.data(), hcount.data(),finstrval);
    }
    catch(...) {
        H5Tclose(memtype);
        string msg = "Fail to read variable-length string.";
        throw InternalErr(__FILE__,__LINE__, msg);
    }
    set_value_ll(finstrval,nelms);
    H5Tclose(memtype);

}

bool HDF5Array::m_array_of_structure(hid_t dsetid, vector<char>&values,bool has_values, size_t values_offset,
                                   int64_t nelms,const int64_t* offset,const int64_t* count, const int64_t* step) {

    BESDEBUG("h5", "=read() Array of Structure length=" << length() << endl);
    hid_t mspace   = -1;
    hid_t memtype  = -1;
    hid_t dtypeid  = -1;
    size_t ty_size = -1;

    if ((dtypeid = H5Dget_type(dsetid)) < 0) {
        string msg = "Cannot obtain the datatype of the variable " + var_path + ".";
        throw InternalErr (__FILE__, __LINE__, msg);
    }

    if ((memtype = H5Tget_native_type(dtypeid, H5T_DIR_ASCEND))<0) {
        H5Tclose(dtypeid);
        string msg = "Cannot obtain the memory datatype of the variable " + var_path + ".";
        throw InternalErr (__FILE__, __LINE__, msg);
    }

    ty_size = H5Tget_size(memtype);

    if (false == has_values) {

        hid_t dspace = -1;

        if ((dspace = H5Dget_space(dsetid))<0) {
            H5Tclose(memtype);
            H5Tclose(dtypeid);
            string msg = "Cannot obtain the data space of the variable " + var_path + ".";
            throw InternalErr (__FILE__, __LINE__, msg);
        }

        d_num_dim = H5Sget_simple_extent_ndims(dspace);
        if (d_num_dim < 0) {
            H5Tclose(memtype);
            H5Tclose(dtypeid);
            H5Sclose(dspace);
            string msg = "Cannot obtain the number of dimensions for the variable " + var_path + ".";
            throw InternalErr (__FILE__, __LINE__, msg);
        }

        vector<hsize_t> hoffset;
        vector<hsize_t>hcount;
        vector<hsize_t>hstep;
        hoffset.resize(d_num_dim);
        hcount.resize(d_num_dim);
        hstep.resize(d_num_dim);
        for (int i = 0; i <d_num_dim; i++) {
            hoffset[i] = (hsize_t) offset[i];
            hcount[i] = (hsize_t) count[i];
            hstep[i] = (hsize_t) step[i];
        }

        if (H5Sselect_hyperslab(dspace, H5S_SELECT_SET,
                                hoffset.data(), hstep.data(),
                                hcount.data(), nullptr) < 0) {
            H5Tclose(memtype);
            H5Tclose(dtypeid);
            H5Sclose(dspace);
            string msg = "Cannot generate the hyperslab for the variable " + var_path + ".";
            throw InternalErr (__FILE__, __LINE__, msg);
        }

        mspace = H5Screate_simple(d_num_dim, hcount.data(),nullptr);
        if (mspace < 0) {
            H5Sclose(dspace);
            H5Tclose(memtype);
            H5Tclose(dtypeid);
            string msg = "Cannot create the memory space for the variable " + var_path + ".";
            throw InternalErr (__FILE__, __LINE__, msg);
        }

        values.resize(nelms*ty_size);
        hid_t read_ret = -1;
        read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,
                           (void*)values.data());
        if (read_ret < 0) {
            H5Tclose(memtype);
            H5Tclose(dtypeid);
            H5Sclose(dspace);
            string msg = "Failed to read the data for the variable " + var_path + ".";
            throw InternalErr (__FILE__, __LINE__, msg);
        }

        H5Sclose(dspace);
        has_values = true;
    } // end of "if(false == has_values)" block

    HDF5Structure *h5s = nullptr;
    hid_t  memb_id  = -1;      
    char* memb_name = nullptr;

    try {

        // Loop through all the elements in this compound datatype array.
        for (int64_t element = 0; element < nelms; ++element) { 

            h5s = dynamic_cast<HDF5Structure*>(var()->ptr_duplicate());
            int                 nmembs           = 0;
            size_t              struct_elem_offset = values_offset + ty_size*element;

            if ((nmembs = H5Tget_nmembers(memtype)) < 0) {
                string msg = "Fail to obtain number of the HDF5 compound datatype members for the variable " + var_path + ".";
                throw InternalErr (__FILE__, __LINE__, msg);
            }

            for (unsigned int u = 0; u < (unsigned)nmembs; u++) {
                // Get member name
                memb_name = H5Tget_member_name(memtype,u);
                if (memb_name == nullptr) {
                    string msg = "Fail to obtain the name of an HDF5 compound datatype member." + var_path + ".";
                    throw InternalErr (__FILE__, __LINE__, msg);
                }

                BaseType *field = h5s->var(memb_name);
                m_array_of_structure_member(field, memtype, u,dsetid, values, has_values, struct_elem_offset );
                H5free_memory(memb_name);

            } // end "for(unsigned u = 0)"
            h5s->set_read_p(true);
            set_vec_ll((uint64_t)element,h5s);
            delete h5s;
        } // end "for (int element=0"

        // Close HDF5-related resources
        m_array_of_structure_close_hdf5_ids(values, has_values, mspace, dtypeid, memtype);
    }
    catch(...) {
        m_array_of_structure_catch_close_hdf5_ids(memb_id, memb_name, values, has_values, mspace, dtypeid, memtype);
        delete h5s;
        throw;
    }
    
    set_read_p(true);

    return false;
}


void HDF5Array:: m_array_of_structure_member(BaseType *field, hid_t memtype, unsigned int u, hid_t dsetid,
                                             vector<char>&values,bool has_values, size_t struct_elem_offset ) const
{

    hid_t  memb_id  = -1;
    H5T_class_t         memb_cls         = H5T_NO_CLASS;
    size_t              memb_offset      = 0;
    // Get member type ID
    if((memb_id = H5Tget_member_type(memtype, u)) < 0) {
        string msg = "Fail to obtain the datatype of an HDF5 compound datatype member for the variable " + var_path + ".";
        throw InternalErr (__FILE__, __LINE__, msg);
    }

    // Get member type class
    if((memb_cls = H5Tget_member_class (memtype, u)) < 0) {
        string msg = "Fail to obtain the datatype class of an HDF5 compound datatype member for the variable " + var_path + ".";
        throw InternalErr (__FILE__, __LINE__, msg);
    }

    // Get member offset,H5Tget_member_offset only fails
    // when H5Tget_memeber_class fails. Sinc H5Tget_member_class
    // is checked above. So no need to check the return value.
    memb_offset= H5Tget_member_offset(memtype,u);

    if (memb_cls == H5T_COMPOUND) {
        HDF5Structure &memb_h5s = dynamic_cast<HDF5Structure&>(*field);
        memb_h5s.do_structure_read(dsetid, memb_id,values,has_values,(int)(memb_offset+struct_elem_offset));
    }
    else if(memb_cls == H5T_ARRAY) {

        // memb_id, obtain the number of dimensions
        int at_ndims = H5Tget_array_ndims(memb_id);
        if(at_ndims <= 0) {
            string msg =  "Fail to obtain number of dimensions of the array datatype for the variable " + var_path + ".";
            throw InternalErr (__FILE__, __LINE__, msg);
        }

        HDF5Array &h5_array_type = dynamic_cast<HDF5Array&>(*field);
        vector<int64_t> at_offset(at_ndims,0);
        vector<int64_t> at_count(at_ndims,0);
        vector<int64_t> at_step(at_ndims,0);

        int64_t at_nelms = h5_array_type.format_constraint(at_offset.data(),at_step.data(),at_count.data());

        // Read the array data
        h5_array_type.do_h5_array_type_read(dsetid,memb_id,values,has_values,(int)(memb_offset+struct_elem_offset),
                                            at_nelms,at_offset.data(),at_count.data(),at_step.data());
    }
    else if(memb_cls == H5T_INTEGER || memb_cls == H5T_FLOAT) {


        if (true == promote_char_to_short(memb_cls,memb_id) && (field->is_dap4() == false)) {

            auto src = (const void*)(values.data() + struct_elem_offset +memb_offset);
            char val_int8;
            memcpy(&val_int8,src,1);
            auto val_short=(short)val_int8;
            field->val2buf(&val_short);
        }
        else {
            field->val2buf(values.data() + struct_elem_offset +memb_offset);
        }
    }
    else if(memb_cls == H5T_STRING) {

        // distinguish between variable length and fixed length
        if (true == H5Tis_variable_str(memb_id)) {
            auto src = (void*)(values.data()+struct_elem_offset + memb_offset);
            string final_str;
            get_vlen_str_data((char*)src,final_str);
            field->val2buf(&final_str);
        }
        else {// Obtain fixed-size string value
            auto src = (const void*)(values.data()+struct_elem_offset + memb_offset);
            vector<char> str_val;
            size_t memb_size = H5Tget_size(memb_id);
            if (memb_size == 0) {
                H5Tclose(memb_id);
                string msg = "Fail to obtain the size of HDF5 compound datatype " + var_path +".";
                throw InternalErr (__FILE__, __LINE__, msg);
            }
            str_val.resize(memb_size);
            memcpy(str_val.data(),src,memb_size);
            string temp_string(str_val.begin(),str_val.end());
            field->val2buf(&temp_string);
        }
    }
    else {
        H5Tclose(memb_id);
        string msg = "Only support the field of compound datatype when the field type class is integer, float, string, array or compound.";
        msg += " The compound datatype of this variable " + var_path + " contains other datatypes.";
        throw InternalErr (__FILE__, __LINE__, msg);
    }

    // Close member type ID
    H5Tclose(memb_id);
    field->set_read_p(true);

}

void HDF5Array::m_array_of_structure_close_hdf5_ids(vector<char> &values, bool has_values, hid_t mspace,
                                                    hid_t dtypeid, hid_t memtype) const
{
    if (true == has_values) {
        if (-1 == mspace) {
            string msg = "Memory type and memory space for this compound datatype should be valid. the variable name is " + var_path +".";
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        if (H5Dvlen_reclaim(memtype,mspace,H5P_DEFAULT,(void*)values.data())<0) {
            string msg = "Unable to reclaim the compound datatype array for the variable " + var_path +".";
            throw InternalErr(__FILE__, __LINE__, msg);
        }
        H5Sclose(mspace);
    }

    H5Tclose(dtypeid);
    H5Tclose(memtype);

}

void HDF5Array:: m_array_of_structure_catch_close_hdf5_ids(hid_t memb_id, char * memb_name, vector<char> &values,
                                                           bool has_values, hid_t mspace, hid_t dtypeid, hid_t memtype) const {

    if (memb_id != -1)
        H5Tclose(memb_id);
    if (memb_name != nullptr)
        H5free_memory(memb_name);
    if (true == has_values) {
        if(H5Dvlen_reclaim(memtype,mspace,H5P_DEFAULT,(void*)(values.data()))<0) {
            H5Tclose(memtype);
            H5Sclose(mspace);
        }
        H5Sclose(mspace);
    }
    H5Tclose(memtype);
    H5Tclose(dtypeid);
}

// Haven't checked the codes and comments
// Haven't added the close handles routines for error handlings yet. KY 2011-11-18
bool HDF5Array::m_array_of_reference(hid_t dset_id,hid_t dtype_id)
{

#if (H5_VERS_MAJOR == 1 && (H5_VERS_MINOR == 10 || H5_VERS_MINOR == 8 || H5_VERS_MINOR == 6))
    hid_t d_dset_id = dset_id;

	vector<int64_t> offset(d_num_dim);
	vector<int64_t> count(d_num_dim);
	vector<int64_t> step(d_num_dim);

	int64_t nelms = format_constraint(offset.data(), step.data(), count.data());
	vector<string> v_str(nelms);

	BESDEBUG("h5", "=read() URL type is detected. "
		<< "nelms=" << nelms << " full_size=" << d_num_elm << endl);

	// Handle regional reference.
	if (H5Tequal(dtype_id, H5T_STD_REF_DSETREG) < 0) { 
            string msg = "H5Tequal() failed for the reference type variable " + var_path + ".";
	    throw InternalErr(__FILE__, __LINE__, msg);
	}

	if (H5Tequal(dtype_id, H5T_STD_REF_DSETREG) > 0)
        m_array_of_region_reference(d_dset_id,v_str, nelms, offset, step);

	// Handle object reference.
	if (H5Tequal(dtype_id, H5T_STD_REF_OBJ) < 0) {
            string msg = "H5Tequal() failed for the reference type variable " + var_path + ".";
	    throw InternalErr(__FILE__, __LINE__, msg);
        }

	if (H5Tequal(dtype_id, H5T_STD_REF_OBJ) > 0)
        m_array_of_object_reference( d_dset_id,  v_str, nelms,offset,step);

	set_value_ll(v_str.data(), nelms);
	return false;

#else
    return m_array_of_reference_new_h5_apis(dset_id,dtype_id);

#endif
}

void HDF5Array:: m_array_of_region_reference(hid_t d_dset_id, vector<string>& v_str,
                                             int64_t nelms, const vector<int64_t>& offset,
                                             const vector<int64_t> &step) {

    hdset_reg_ref_t *rbuf = nullptr;
    BESDEBUG("h5", "=read() Got regional reference. " << endl);

    // Vector doesn't work for this case. somehow it doesn't support the type.
    rbuf = new hdset_reg_ref_t[d_num_elm];
    if (rbuf == nullptr){
        string msg = "memory allocation for region reference variable " + var_path +".";
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    if (H5Dread(d_dset_id, H5T_STD_REF_DSETREG, H5S_ALL, H5S_ALL, H5P_DEFAULT, rbuf) < 0) {
        string msg = "H5Dread() failed for region reference variable " + var_path +".";
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    for (int64_t i = 0; i < nelms; i++) {

        hdset_reg_ref_t *temp_rbuf = rbuf + offset[0]+i*step[0];

        // Let's assume that URL array is always 1 dimension.
        if (temp_rbuf!= nullptr) {

            char r_name[DODS_NAMELEN];

            hid_t did_r = H5RDEREFERENCE(d_dset_id, H5R_DATASET_REGION, (const void*)(temp_rbuf));
            if (did_r < 0) {
                string msg = "H5RDEREFERENCE() failed for region reference variable " + var_path +".";
                throw InternalErr(__FILE__, __LINE__, msg);
            }

            if (H5Iget_name(did_r, (char *) r_name, DODS_NAMELEN) < 0) {
                string msg = "H5Iget_name() failed for region reference variable " + var_path + ".";
                throw InternalErr(__FILE__, __LINE__, msg);
            }

            BESDEBUG("h5", "=read() dereferenced name is " << r_name << endl);

            string varname(r_name);
            hid_t space_id = H5Rget_region(did_r, H5R_DATASET_REGION, (const void*)(temp_rbuf));
            if (space_id < 0) {
                string msg = "H5Rget_region() failed for region reference variable " + var_path + ".";
                throw InternalErr(__FILE__, __LINE__, msg);
            }

            int ndim = H5Sget_simple_extent_ndims(space_id);
            if (ndim < 0) {
                string msg = "H5Sget_simple_extent_ndims() failed for region reference variable " + var_path + ".";
                throw InternalErr(__FILE__, __LINE__, msg);
            }

            BESDEBUG("h5", "=read() dim is " << ndim << endl);

            string expression;
            switch (H5Sget_select_type(space_id)) {

                case H5S_SEL_NONE:
                    BESDEBUG("h5", "=read() None selected." << endl);
                    break;

                case H5S_SEL_POINTS: {
                    m_array_of_region_reference_point_selection(space_id, ndim,varname,v_str, i);
                    break;
                }
                case H5S_SEL_HYPERSLABS: {
                    m_array_of_region_reference_hyperslab_selection(space_id, ndim, varname, v_str, i);
                    break;
                }
                case H5S_SEL_ALL:
                    BESDEBUG("h5", "=read() All selected." << endl);
                    break;

                default:
                    BESDEBUG("h5", "Unknown space type." << endl);
                    break;
            }

        } else {
            v_str[i] = "";
        }
    }
    delete[] rbuf;
}

void HDF5Array:: m_array_of_region_reference_point_selection(hid_t space_id, int ndim, const string &varname,
                                                             vector<string> &v_str,int64_t i) const {

    string expression;

    BESDEBUG("h5", "=read() Points selected." << endl);
    hssize_t npoints = H5Sget_select_npoints(space_id);
    if (npoints < 0) {
        string msg = "Cannot determine number of elements in the dataspace selection for the variable " + var_path + ".";
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    BESDEBUG("h5", "=read() npoints are " << npoints << endl);

    vector<hsize_t> buf(npoints * ndim);
    if (H5Sget_select_elem_pointlist(space_id, 0, npoints, buf.data()) < 0) {
        string msg = "H5Sget_select_elem_pointlist() failed for the variable " + var_path +".";
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    for (int64_t j = 0; j < (int) npoints; j++) {
        // Name of the dataset.
        expression.append(varname);
        for (int k = 0; k < ndim; k++) {
            ostringstream oss;
            oss << "[" << (int) buf[j * ndim + k] << "]";
            expression.append(oss.str());
        }
        if (j != (int64_t) (npoints - 1)) {
            expression.append(",");
        }
    }
    v_str[i].append(expression);

}

void HDF5Array:: m_array_of_region_reference_hyperslab_selection(hid_t space_id, int ndim, const string &varname,
                                                                 vector<string> &v_str,int64_t i) const
{
    string expression;

    vector<hsize_t> start(ndim);
    vector<hsize_t> end(ndim);
    vector<hsize_t> stride(ndim);
    vector<hsize_t> s_count(ndim);
    vector<hsize_t> block(ndim);

    BESDEBUG("h5", "=read() Slabs selected." << endl);
    BESDEBUG("h5", "=read() nblock is " << H5Sget_select_hyper_nblocks(space_id) << endl);

#if (H5_VERS_MAJOR == 1 && H5_VERS_MINOR == 8)
    if (H5Sget_select_bounds(space_id, start.data(), end.data()) < 0) { 
        string msg = "H5Sget_select_bounds() failed for region reference variable "+var_path + ".";
        throw InternalErr(__FILE__, __LINE__, msg);
    }
#else
    if (H5Sget_regular_hyperslab(space_id, start.data(), stride.data(), s_count.data(),
                                 block.data()) < 0) {
        string msg = "H5Sget_regular_hyperslab() failed for region reference variable "+var_path + ".";
        throw InternalErr(__FILE__, __LINE__, msg);
    }
#endif

    for (int j = 0; j < ndim; j++) {
        ostringstream oss;
        BESDEBUG("h5", "start " << start[j]
                                << "stride " << stride[j]
                                << "count " << s_count[j]
                                << "block " << block[j]
                                << endl);

        // Map from HDF5's start,stride,count,block to DAP's start,stride,end.
        end[j] = start[j] + stride[j] * (s_count[j] - 1) + (block[j] - 1);
        BESDEBUG("h5", "=read() start is " << start[j]
                                           << "=read() end is " << end[j] << endl);
        oss << "[" << start[j] << ":" << stride[j] << ":" << end[j] << "]";
        expression.append(oss.str());
        BESDEBUG("h5", "=read() expression is " << expression << endl);
    }
    v_str[i] = varname;
    if (!expression.empty())
        v_str[i].append(expression);

}
void HDF5Array:: m_array_of_object_reference(hid_t d_dset_id, vector<string>& v_str,
                                             int64_t nelms, const vector<int64_t>& offset,
                                             const vector<int64_t> &step) const
{

    BESDEBUG("h5", "=read() Got object reference. " << endl);
    vector<hobj_ref_t> orbuf;
    orbuf.resize(d_num_elm);
    if (H5Dread(d_dset_id, H5T_STD_REF_OBJ, H5S_ALL, H5S_ALL,
                H5P_DEFAULT, orbuf.data()) < 0) {
        string msg = "H5Dread failed() for object reference variable " + var_path + ".";
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    for (int64_t i = 0; i < nelms; i++) {

        // Let's assume that URL array is always 1 dimension.
        hid_t did_r = H5RDEREFERENCE(d_dset_id, H5R_OBJECT, &orbuf[offset[0] + i * step[0]]);
        if (did_r < 0) {
            string msg = "H5RDEREFERENCE() failed for object reference variable " + var_path + ".";
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        char r_name[DODS_NAMELEN];
        if (H5Iget_name(did_r, (char *) r_name, DODS_NAMELEN) < 0) {
            string msg = "H5Iget_name() failed for object reference variable " + var_path + ".";
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        // Shorten the dataset name
        string varname(r_name);

        BESDEBUG("h5", "=read() dereferenced name is " << r_name <<endl);
        v_str[i] = varname;
    }
}

bool HDF5Array::m_array_of_reference_new_h5_apis(hid_t dset_id,hid_t dtype_id) {

#if (H5_VERS_MAJOR == 1 && (H5_VERS_MINOR == 10 || H5_VERS_MINOR == 8 || H5_VERS_MINOR == 6))
    throw BESInternalError(
       "The HDF5 handler compiled with earlier version (<=110)of the HDF5 library should not call method that uses new reference APIs",__FILE__,__LINE__);
    return false;
#else
    
    H5R_ref_t *rbuf = nullptr;
    hid_t  mem_space_id = 0;
    hid_t  file_space_id;
        
    try {

        // First we need to read the reference data from DAP's hyperslab selection.
	vector<int64_t> offset(d_num_dim);
	vector<int64_t> count(d_num_dim);
	vector<int64_t> step(d_num_dim);
        vector<hsize_t> hoffset(d_num_dim);
        vector<hsize_t>hcount(d_num_dim);
        vector<hsize_t>hstep(d_num_dim);

	int64_t nelms = format_constraint(offset.data(), step.data(), count.data());
        for (int i = 0; i <d_num_dim; i++) {
            hoffset[i] = (hsize_t) offset[i];
            hcount[i] = (hsize_t) count[i];
            hstep[i] = (hsize_t) step[i];
        }

	BESDEBUG("h5", "=read() URL type is detected. "
		<< "nelms=" << nelms << endl);

        rbuf = new H5R_ref_t[nelms];

	file_space_id = H5Dget_space(dset_id);
        if (file_space_id < 0) {
            string msg = "Fail to obtain reference dataset file space for varaible " + var_path + ".";
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        if (H5Sselect_hyperslab(file_space_id, H5S_SELECT_SET,
                               hoffset.data(), hstep.data(),
                               hcount.data(), nullptr) < 0) {
            string msg = "Fail to select the hyperslab for reference dataset for the variable " + var_path + ".";
            throw InternalErr (__FILE__, __LINE__, msg);
        }
      

        mem_space_id = H5Screate_simple(d_num_dim,hcount.data(),nullptr);
        if (mem_space_id < 0) {
            string msg = "Fail to obtain reference dataset memory space for the variable " + var_path + ".";
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        if (H5Dread(dset_id,H5T_STD_REF,mem_space_id,file_space_id,H5P_DEFAULT,&rbuf[0])<0) {
            string msg = "Fail to obtain read of eference dataset for the variable " + var_path + ".";
            throw InternalErr(__FILE__, __LINE__, msg);
        }

        H5Sclose(mem_space_id);
        H5Sclose(file_space_id);

        // Now we need to retrieve the reference info. fot the nelms elements.
        vector<string> v_str;

        H5R_type_t ref_type = H5Rget_type((const H5R_ref_t *)&rbuf[0]);

        // The referenced objects can only be either objects or dataset regions.
        if (ref_type != H5R_OBJECT2 && ref_type !=H5R_DATASET_REGION2) {
            string msg = "Unsupported reference: neither object nor region references for the variable " + var_path + "."; 
            throw InternalErr(__FILE__, __LINE__, msg);
        }
           
        for (int64_t i = 0; i < nelms; i++) {

            hid_t obj_id = H5Ropen_object((H5R_ref_t *)&rbuf[i], H5P_DEFAULT, H5P_DEFAULT);
            if (obj_id < 0) {
                string msg = "Cannot open the object the reference points to for the variable " + var_path + ".";
                throw InternalErr(__FILE__, __LINE__, msg);
            }
                
            vector<char> objname;
            ssize_t objnamelen = -1;
            if ((objnamelen= H5Iget_name(obj_id,nullptr,0))<=0) {
                H5Oclose(obj_id);
                string msg = "Cannot obtain the name length of the object the reference points to for the variable " + var_path + ".";
                throw InternalErr(__FILE__, __LINE__, msg);
            }
            objname.resize(objnamelen+1);
            if ((objnamelen= H5Iget_name(obj_id,objname.data(),objnamelen+1))<=0) {
                H5Oclose(obj_id);
                string msg = "Cannot obtain the name of the object the reference points to for the variable " + var_path + ".";
                throw InternalErr(__FILE__, __LINE__, msg);
            }

            string objname_str = string(objname.begin(),objname.end());
            string trim_objname = objname_str.substr(0,objnamelen);
            
            // For object references, we just need to save the object full path.
            if(ref_type == H5R_OBJECT2) 
                v_str.push_back(trim_objname);
            else {// Must be region reference.
                H5O_type_t obj_type;
                if(H5Rget_obj_type3((H5R_ref_t *)&rbuf[i], H5P_DEFAULT, &obj_type) < 0){
                    H5Oclose(obj_id);
                    string msg = "H5Rget_obj_type3() failed for the variable " + var_path + ".";
                    throw InternalErr(__FILE__, __LINE__, msg);
                }
                if(obj_type != H5O_TYPE_DATASET) {
                    H5Oclose(obj_id);
                    string msg = "Region reference must point to a dataset. This is related to the variable " + var_path + ".";
                    throw InternalErr(__FILE__, __LINE__, msg);
                }
                hid_t region_space_id = H5Ropen_region(&rbuf[i],H5P_DEFAULT,H5P_DEFAULT);
                if (region_space_id < 0) {
                    H5Oclose(obj_id);
                    string msg = "Cannot obtain the space ID the reference points to. This is related to the variable " + var_path + ".";
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                int ndim = H5Sget_simple_extent_ndims(region_space_id);
                if (ndim < 0) {
                    H5Sclose(region_space_id);
                    H5Oclose(obj_id);
                    string msg = "H5Sget_simple_extent_ndims() failed. This is related to the variable " + var_path + ".";
                    throw InternalErr(__FILE__, __LINE__, msg);
	        }

		string expression;
		switch (H5Sget_select_type(region_space_id)) {

		    case H5S_SEL_NONE:
			BESDEBUG("h5", "=read() None selected." << endl);
			break;

		    case H5S_SEL_POINTS: {
			BESDEBUG("h5", "=read() Points selected." << endl);
			hssize_t npoints = H5Sget_select_npoints(region_space_id);
			if (npoints < 0) { 
                            H5Sclose(region_space_id);
                            H5Oclose(obj_id);
                            string msg = "Cannot determine number of elements in the dataspace selection. This is related to the variable " + var_path + ".";
			    throw InternalErr(__FILE__, __LINE__, msg);
			}

			BESDEBUG("h5", "=read() npoints are " << npoints
				<< endl);
			vector<hsize_t> buf(npoints * ndim);
			if (H5Sget_select_elem_pointlist(region_space_id, 0, npoints, buf.data()) < 0) {
                            H5Sclose(region_space_id);
                            H5Oclose(obj_id);
                            string msg = "H5Sget_select_elem_pointlist() failed. This is related to the variable " + var_path + ".";
			    throw InternalErr(__FILE__, __LINE__, msg);
			}

#if 0
			for (int j = 0; j < npoints * ndim; j++) {
                            "h5", "=read() npoints buf[0] =" << buf[j] <<endl;
			}
#endif

			for (int64_t j = 0; j < (int) npoints; j++) {
			    // Name of the dataset.
			    expression.append(trim_objname);
			    for (int k = 0; k < ndim; k++) {
				ostringstream oss;
				oss << "[" << (int) buf[j * ndim + k] << "]";
				expression.append(oss.str());
			    }
			    if (j != (int64_t) (npoints - 1)) {
				expression.append(",");
			    }
			}
			v_str.push_back(expression);

			break;
		    }
		    case H5S_SEL_HYPERSLABS: {
			vector<hsize_t> start(ndim);
			vector<hsize_t> end(ndim);
                        vector<hsize_t>stride(ndim);
                        vector<hsize_t>s_count(ndim);
                        vector<hsize_t>block(ndim);

			BESDEBUG("h5", "=read() Slabs selected." << endl);
			BESDEBUG("h5", "=read() nblock is " <<
				H5Sget_select_hyper_nblocks(region_space_id) << endl);

			if (H5Sget_regular_hyperslab(region_space_id, start.data(), stride.data(), s_count.data(), block.data()) < 0) {
	                    H5Sclose(region_space_id);
                            H5Oclose(obj_id);
                            string msg = "H5Sget_regular_hyperslab() failed. This is related to the variable " + var_path + ".";
			    throw InternalErr(__FILE__, __LINE__, msg);
			}

			expression.append(trim_objname);
			for (int j = 0; j < ndim; j++) {
			    ostringstream oss;
			    BESDEBUG("h5", "start " << start[j]
                                     << "stride "<<stride[j] 
                                     << "count "<< s_count[j]
                                     << "block "<< block[j] 
                                     <<endl);

                            // Map from HDF5's start,stride,count,block to DAP's start,stride,end.
                            end[j] = start[j] + stride[j]*(s_count[j]-1)+(block[j]-1);
			    BESDEBUG("h5", "=read() start is " << start[j]
				    << "=read() end is " << end[j] << endl);
			    oss << "[" << start[j] << ":" << stride[j] << ":" << end[j] << "]";
			    expression.append(oss.str());
			    BESDEBUG("h5", "=read() expression is "
				    << expression << endl)
			    ;
			}
			v_str.push_back(expression);
			// Constraint expression. [start:stride:end]
			break;
		    }
		    case H5S_SEL_ALL:
			BESDEBUG("h5", "=read() All selected." << endl);
			break;

		    default:
			BESDEBUG("h5", "Unknown space type." << endl);
			break;
		}
                H5Sclose(region_space_id);
            }
            H5Oclose(obj_id);
        }
        for (int64_t i = 0; i<nelms; i++)
            H5Rdestroy(&rbuf[i]);
        delete[] rbuf;
        H5Sclose(mem_space_id);
        H5Sclose(file_space_id);
	set_value_ll(v_str.data(), nelms);
        return false;
    }
    catch (...) {
        if(rbuf!= nullptr)
            delete[] rbuf;
        H5Sclose(mem_space_id);
        H5Sclose(file_space_id);
	throw;
    }
#endif
} 


void HDF5Array::m_intern_plain_array_data(char *convbuf,hid_t memtype)
{
    if (check_h5str(memtype)) {

        vector<string> v_str(d_num_elm);
        size_t elesize = H5Tget_size(memtype);
        if (elesize == 0) {
            string msg = "H5Tget_size() failed for the variable " + var_path + ".";
            throw InternalErr(__FILE__, __LINE__, msg);
        }
        vector<char> strbuf(elesize + 1);
        BESDEBUG("h5", "=read()<check_h5str()  element size=" << elesize
                 << " d_num_elm=" << d_num_elm << endl);

        for (int64_t strindex = 0; strindex < (int64_t)d_num_elm; strindex++) {
            get_strdata(strindex, convbuf, strbuf.data(), (int)elesize);
            BESDEBUG("h5", "=read()<get_strdata() strbuf=" << strbuf.data() << endl);
            v_str[strindex] = strbuf.data();
        }
        set_read_p(true);
        val2buf((void *) v_str.data());
    }
    else {
	    set_read_p(true);
	    val2buf((void *) convbuf);
    }
}

bool HDF5Array::do_h5_array_type_read(hid_t dsetid, hid_t memb_id,vector<char>&values,bool has_values,size_t values_offset,
                                      int64_t at_nelms,int64_t* at_offset,int64_t* at_count, int64_t* at_step){

    //1. Call do array first(datatype must be derived) and the value must be set. We don't support Array datatype 
    //   unless it is inside a compound datatype
    if (has_values != true) {
        string msg =  "Only support the retrieval of HDF5 Array datatype values from the parent compound datatype read.";
        msg += "This is related to the variable " + var_path + ".";
        throw InternalErr (__FILE__, __LINE__, msg);
    }

    hid_t at_base_type = H5Tget_super(memb_id);
    if (at_base_type < 0) {
        string msg = "Fail to obtain the basetype of the array datatype.";
        msg += "This is related to the variable " + var_path + ".";
        throw InternalErr (__FILE__, __LINE__, msg);
    }

    // memb_id, obtain the number of dimensions
    int at_ndims = H5Tget_array_ndims(memb_id);
    if (at_ndims <= 0) {
        H5Tclose(at_base_type);
        string msg = "Fail to obtain number of dimensions of the array datatype.";
        msg += "This is related to the variable " + var_path + ".";
        throw InternalErr (__FILE__, __LINE__, msg); 
    }

    vector<hsize_t>at_dims_h(at_ndims,0);

    // Obtain the number of elements for each dims
    if (H5Tget_array_dims(memb_id,at_dims_h.data())<0) {
        H5Tclose(at_base_type);
        string msg = "Fail to obtain dimensions of the array datatype.";
        msg += "This is related to the variable " + var_path + ".";
        throw InternalErr (__FILE__, __LINE__, msg); 
    }
    vector<int64_t>at_dims(at_ndims,0);
    for (int64_t i = 0;i<at_ndims;i++)
        at_dims[i] = (int64_t)at_dims_h[i];

    int64_t at_total_nelms = 1;
    for (int i = 0; i <at_ndims; i++) 
        at_total_nelms = at_total_nelms*at_dims[i];

    H5T_class_t array_cls = H5Tget_class(at_base_type);
    size_t at_base_type_size = H5Tget_size(at_base_type);

    // H5 Array type, the basetype is COMPOUND.
    if(H5T_COMPOUND == array_cls) {

        // These vectors are used to handle subset of array datatype
        vector<int64_t> at_end(at_ndims, 0);
        vector<int64_t> at_pos(at_ndims, 0);
        for (int i = 0; i < at_ndims; i++) {
            at_pos[i] = at_offset[i];
            at_end[i] = at_offset[i] + (at_count[i] - 1) * at_step[i];
        }

        int64_t at_orig_index = INDEX_nD_TO_1D(at_dims, at_pos);

        // To read the array of compound (structure) in DAP, one must read one element each. set_vec is used afterwards.
        for (int64_t array_index = 0; array_index < at_nelms; array_index++) {

            // The basetype of the array datatype is compound,-- check if the following line is valid.
            auto h5s = dynamic_cast<HDF5Structure *>(var()->ptr_duplicate());
            hid_t child_memb_id;
            H5T_class_t child_memb_cls;
            int child_nmembs;
            size_t child_memb_offset;

            if ((child_nmembs = H5Tget_nmembers(at_base_type)) < 0) {
                H5Tclose(at_base_type);
                delete h5s;
                string msg = "Fail to obtain number of HDF5 compound datatype.";
                msg += "This is related to the variable " + var_path + ".";
                throw InternalErr (__FILE__, __LINE__, msg); 
            }

            for (unsigned child_u = 0; child_u < (unsigned) child_nmembs; child_u++) {

                // Get member type ID 
                if ((child_memb_id = H5Tget_member_type(at_base_type, child_u)) < 0) {
                    H5Tclose(at_base_type);
                    delete h5s;
                    string msg = "Fail to obtain the datatype of an HDF5 compound datatype member.";
                    msg += "This is related to the variable " + var_path + ".";
                    throw InternalErr (__FILE__, __LINE__, msg); 
                }

                // Get member type class 
                if ((child_memb_cls = H5Tget_member_class(at_base_type, child_u)) < 0) {
                    H5Tclose(child_memb_id);
                    H5Tclose(at_base_type);
                    delete h5s;
                    string msg = "Fail to obtain the datatype class of an HDF5 compound datatype member.";
                    msg += "This is related to the variable " + var_path + ".";
                    throw InternalErr (__FILE__, __LINE__, msg); 
                }

                // Get member offset
                child_memb_offset = H5Tget_member_offset(at_base_type, child_u);

                // Get member name
                char *child_memb_name = H5Tget_member_name(at_base_type, child_u);
                if (child_memb_name == nullptr) {
                    H5Tclose(child_memb_id);
                    H5Tclose(at_base_type);
                    delete h5s;
                    string msg = "Fail to obtain the name of an HDF5 compound datatype member.";
                    msg += "This is related to the variable " + var_path + ".";
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                BaseType *field = h5s->var(child_memb_name);
                H5free_memory(child_memb_name);
                try {
                    do_h5_array_type_read_base_compound_member(dsetid, field, child_memb_id, child_memb_cls, values,
                                           has_values, values_offset, at_nelms, at_total_nelms, at_base_type_size,
                                           array_index, at_orig_index, child_memb_offset);
                }
                catch(...){
                    delete h5s;
                }

            } // end "for ( child_u = 0)"
            h5s->set_read_p(true);

            // Save the value of this element to DAP structure.
            set_vec_ll((uint64_t) array_index, h5s);
            delete h5s;

            vector<int64_t> at_offsetv(at_pos.size(), 0);
            vector<int64_t> at_stepv(at_pos.size(), 0);
            for (int64_t at_index = 0; at_index < (int64_t) (at_pos.size()); at_index++) {
                at_offsetv[at_index] = at_offset[at_index];
                at_stepv[at_index] = at_step[at_index];
            }
            //obtain the next position of the selected point based on the offset,end and step.
            obtain_next_pos(at_pos, at_offsetv, at_end, at_stepv, (int) (at_pos.size()));
            at_orig_index = INDEX_nD_TO_1D(at_dims, at_pos);
        }// end for "(array_index = 0) for array (compound)datatype"

    }
    else if(H5T_INTEGER == array_cls|| H5T_FLOAT == array_cls) {
        do_h5_array_type_read_base_atomic(array_cls, at_base_type, at_base_type_size, values, values_offset, at_nelms,
                                          at_total_nelms,at_ndims, at_dims, at_offset, at_step, at_count);
    }
    else if(H5T_STRING == array_cls) {

        // set the original position to the starting point
        vector<int64_t>at_pos(at_ndims,0);
        for (int i = 0; i< at_ndims; i++)
            at_pos[i] = at_offset[i];

        vector<string>total_strval;
        total_strval.resize(at_total_nelms);

        if (true == H5Tis_variable_str(at_base_type)) {
            auto src = (void*)(values.data()+values_offset);
            auto temp_bp =(char*)src;
            for (int64_t i = 0;i <at_total_nelms; i++){
                string tempstrval;
                get_vlen_str_data(temp_bp,tempstrval);
                total_strval[i] = tempstrval;
                temp_bp += at_base_type_size;
            }
            if (at_total_nelms == at_nelms)
                set_value_ll(total_strval,at_total_nelms);
            else {// obtain subset for variable-length string.
                vector<string>final_val;
                subset<string>(
                              total_strval.data(),
                              at_ndims,
                              at_dims,
                              at_offset,
                              at_step,
                              at_count,
                              &final_val,
                              at_pos,
                              0
                                     );
                
                set_value_ll(final_val,at_nelms);

           }

        }
        else {// For fixed-size string.
            auto src = (void*)(values.data()+values_offset);
            for(int64_t i = 0; i <at_total_nelms; i++)
                total_strval[i].resize(at_base_type_size);

            vector<char> str_val;
            str_val.resize(at_total_nelms*at_base_type_size);
            memcpy((void*)str_val.data(),src,at_total_nelms*at_base_type_size);
            string total_in_one_string(str_val.begin(),str_val.end());
            for (int64_t i = 0; i<at_total_nelms;i++)
                total_strval[i] = total_in_one_string.substr(i*at_base_type_size,at_base_type_size);

            if (at_total_nelms == at_nelms)
                set_value_ll(total_strval,at_total_nelms);
            else {
                vector<string>final_val;
                subset<string>(
                               total_strval.data(),
                               at_ndims,
                               at_dims,
                               at_offset,
                               at_step,
                               at_count,
                               &final_val,
                               at_pos,
                               0
                               );
                set_value_ll(final_val,at_nelms);

            }
        }
    }
    else {
        H5Tclose(at_base_type);
        string msg = "Only support the field of compound datatype when the field type class is integer, float, string, array or compound.";
        msg += "This is related to the variable " + var_path + ".";
        throw InternalErr (__FILE__, __LINE__, msg);
    }
 
    H5Tclose(at_base_type);

    return true;
}

void HDF5Array:: do_h5_array_type_read_base_compound_member(hid_t dsetid, BaseType *field, hid_t child_memb_id,
                                                            H5T_class_t child_memb_cls, vector<char>&values,
                                                            bool has_values, size_t values_offset, int64_t at_nelms,
                                                            int64_t at_total_nelms, size_t at_base_type_size,
                                                            int64_t array_index, int64_t at_orig_index,
                                                            size_t child_memb_offset) const {
    if (child_memb_cls == H5T_COMPOUND) {
        HDF5Structure &memb_h5s = dynamic_cast<HDF5Structure &>(*field);
        //
        // Call structure read when reading the whole array. sa1{sa2[100]}
        // sa2[100] is an array datatype.
        // If reading the whole buffer, just add the buffer.
        if (at_total_nelms == at_nelms) {
            memb_h5s.do_structure_read(dsetid, child_memb_id, values, has_values,
                                       values_offset + at_base_type_size * array_index + child_memb_offset);
        }
        // Subset of sa2, sa2[10:100:2]; sa2[100] is an array datatype.
        // The whole array sa2[100] is to be read into somewhere in buffer values.
        else {
            // The subset should be considered. adjust memb_offset+values_offset+???,make sure only the subset is selected.
            // at_total_nelms is 100 but at_nelms is (100-10)/2+1=46.
            // The starting point of the whole array is values+memb_offset_values_offset
            // When the datatype is structure, we have to obtain the index one by one.

            memb_h5s.do_structure_read(dsetid, child_memb_id, values, has_values,
                                       values_offset + at_base_type_size * at_orig_index +
                                       child_memb_offset);
        }
    }
    else if (child_memb_cls == H5T_ARRAY) {

        // memb_id, obtain the number of dimensions
        int child_at_ndims = H5Tget_array_ndims(child_memb_id);
        if (child_at_ndims <= 0) {
            H5Tclose(child_memb_id);
            string msg = "Fail to obtain number of dimensions of the array datatype.";
            msg += "This is related to the variable " + var_path + ".";
            throw InternalErr (__FILE__, __LINE__, msg); 
        }

        HDF5Array &h5_array_type = dynamic_cast<HDF5Array &>(*field);
        vector<int64_t> child_at_offset(child_at_ndims, 0);
        vector<int64_t> child_at_count(child_at_ndims, 0);
        vector<int64_t> child_at_step(child_at_ndims, 0);

        int64_t child_at_nelms = h5_array_type.format_constraint(child_at_offset.data(),
                                                                 child_at_step.data(),
                                                                 child_at_count.data());
        if (at_total_nelms == at_nelms) {
            h5_array_type.do_h5_array_type_read(dsetid, child_memb_id, values, has_values,
                                                child_memb_offset + values_offset +
                                                at_base_type_size * array_index,
                                                child_at_nelms, child_at_offset.data(),
                                                child_at_count.data(), child_at_step.data());
        } else {// Adjust memb_offset+values_offset, basically change at_base_type_size*array_index
            h5_array_type.do_h5_array_type_read(dsetid, child_memb_id, values, has_values,
                                                child_memb_offset + values_offset +
                                                at_base_type_size * at_orig_index,
                                                child_at_nelms, child_at_offset.data(),
                                                child_at_count.data(), child_at_step.data());

        }
    }
    else if (H5T_INTEGER == child_memb_cls || H5T_FLOAT == child_memb_cls) {

        int64_t number_index = ((at_total_nelms == at_nelms) ? array_index : at_orig_index);
        if (true == promote_char_to_short(child_memb_cls, child_memb_id) && (field->is_dap4() == false)) {
            auto src = (const void *) (values.data() + (number_index * at_base_type_size) + values_offset +
                                  child_memb_offset);
            char val_int8;
            memcpy(&val_int8, src, 1);
            auto val_short = (short) val_int8;
            field->val2buf(&val_short);
        }
        else
            field->val2buf(
                    values.data() + (number_index * at_base_type_size) + values_offset + child_memb_offset);
    }
    else if (H5T_STRING == child_memb_cls) {

        int64_t string_index = ((at_total_nelms == at_nelms) ? array_index : at_orig_index);
        size_t data_offset = (size_t)string_index * at_base_type_size + values_offset +
                                  child_memb_offset;
        do_h5_array_type_read_base_compound_member_string(field, child_memb_id, values, data_offset);

    }
    else {
        H5Tclose(child_memb_id);
        string msg = "Unsupported datatype class for the array base type.";
        msg += "This is related to the variable " + var_path + ".";
        throw InternalErr (__FILE__, __LINE__, msg); 
    }
    field->set_read_p(true);
    H5Tclose(child_memb_id);
}

void HDF5Array:: do_h5_array_type_read_base_compound_member_string(BaseType *field, hid_t child_memb_id,
                                                            const vector<char> &values, size_t data_offset) const
{
    // distinguish between variable length and fixed length
    auto src = (void *) (values.data() + data_offset);
    if (true == H5Tis_variable_str(child_memb_id)) {
        string final_str;
        auto temp_bp = (char *) src;
        get_vlen_str_data(temp_bp, final_str);
        field->val2buf(&final_str[0]);
    }
    else {// Obtain string
        vector<char> str_val;
        size_t memb_size = H5Tget_size(child_memb_id);
        if (memb_size == 0) {
            H5Tclose(child_memb_id);
            string msg =  "Fail to obtain the size of HDF5 compound datatype.";
            msg += "This is related to the variable " + var_path + ".";
            throw InternalErr (__FILE__, __LINE__, msg); 
        }
        str_val.resize(memb_size);
        memcpy(str_val.data(), src, memb_size);
        field->val2buf(str_val.data());
    }
}

void HDF5Array::do_h5_array_type_read_base_atomic(H5T_class_t array_cls, hid_t at_base_type, size_t at_base_type_size,
                                                   vector<char>&values,
                                                    size_t values_offset, int64_t at_nelms,int64_t at_total_nelms,
                                                    int at_ndims, vector<int64_t> &at_dims, int64_t* at_offset,
                                                    int64_t* at_step, int64_t *at_count) {

    // If no subset for the array datatype, just read the whole buffer.
    if (at_total_nelms == at_nelms)
        do_h5_array_type_read_base_atomic_whole_data(array_cls, at_base_type, at_nelms, values, values_offset);
    else {
        // Adjust the value for the subset of the array datatype
        // Obtain the corresponding DAP type of the HDF5 data type
        string dap_type = get_dap_type(at_base_type, is_dap4());

        // The total array type data is read.
        auto src = (void *) (values.data() + values_offset);

        // set the original position to the starting point
        vector<int64_t> at_pos(at_ndims, 0);
        for (int64_t i = 0; i < at_ndims; i++)
            at_pos[i] = at_offset[i];

        if (BYTE == dap_type) {

            vector<unsigned char> total_val;
            total_val.resize(at_total_nelms);
            memcpy(total_val.data(), src, at_total_nelms * at_base_type_size);

            vector<unsigned char> final_val;
            subset<unsigned char>(
                    total_val.data(),
                    at_ndims,
                    at_dims,
                    at_offset,
                    at_step,
                    at_count,
                    &final_val,
                    at_pos,
                    0
            );

            set_value_ll(final_val.data(), at_nelms);

        } else if (INT16 == dap_type) {

            // promote char to short,DAP2 doesn't have "char" type
            if (true == promote_char_to_short(array_cls, at_base_type) && (is_dap4() == false)) {
                vector<char> total_val;
                total_val.resize(at_total_nelms);
                memcpy(total_val.data(), src, at_total_nelms * at_base_type_size);

                vector<char> final_val;
                subset<char>(
                        total_val.data(),
                        at_ndims,
                        at_dims,
                        at_offset,
                        at_step,
                        at_count,
                        &final_val,
                        at_pos,
                        0
                );

                vector<short> final_val_short;
                final_val_short.resize(at_nelms);
                for (int64_t i = 0; i < at_nelms; i++)
                    final_val_short[i] = final_val[i];

                val2buf(final_val_short.data());

            } else {// short

                vector<short> total_val;
                total_val.resize(at_total_nelms);
                memcpy(total_val.data(), src, at_total_nelms * at_base_type_size);

                vector<short> final_val;
                subset<short>(
                        total_val.data(),
                        at_ndims,
                        at_dims,
                        at_offset,
                        at_step,
                        at_count,
                        &final_val,
                        at_pos,
                        0
                );

                val2buf(final_val.data());
            }
        } else if (UINT16 == dap_type) {
            vector<unsigned short> total_val;
            total_val.resize(at_total_nelms);
            memcpy(total_val.data(), src, at_total_nelms * at_base_type_size);

            vector<unsigned short> final_val;
            subset<unsigned short>(
                    total_val.data(),
                    at_ndims,
                    at_dims,
                    at_offset,
                    at_step,
                    at_count,
                    &final_val,
                    at_pos,
                    0
            );

            val2buf(final_val.data());

        } else if (UINT32 == dap_type) {
            vector<unsigned int> total_val;
            total_val.resize(at_total_nelms);
            memcpy(total_val.data(), src, at_total_nelms * at_base_type_size);

            vector<unsigned int> final_val;
            subset<unsigned int>(
                    total_val.data(),
                    at_ndims,
                    at_dims,
                    at_offset,
                    at_step,
                    at_count,
                    &final_val,
                    at_pos,
                    0
            );
            val2buf(final_val.data());


        } else if (INT32 == dap_type) {
            vector<int> total_val;
            total_val.resize(at_total_nelms);
            memcpy(total_val.data(), src, at_total_nelms * at_base_type_size);

            vector<int> final_val;
            subset<int>(
                    total_val.data(),
                    at_ndims,
                    at_dims,
                    at_offset,
                    at_step,
                    at_count,
                    &final_val,
                    at_pos,
                    0
            );

            val2buf(final_val.data());

        } else if (FLOAT32 == dap_type) {
            vector<float> total_val;
            total_val.resize(at_total_nelms);
            memcpy(total_val.data(), src, at_total_nelms * at_base_type_size);

            vector<float> final_val;
            subset<float>(
                    total_val.data(),
                    at_ndims,
                    at_dims,
                    at_offset,
                    at_step,
                    at_count,
                    &final_val,
                    at_pos,
                    0
            );

            val2buf(final_val.data());

        } else if (FLOAT64 == dap_type) {
            vector<double> total_val;
            total_val.resize(at_total_nelms);
            memcpy(total_val.data(), src, at_total_nelms * at_base_type_size);

            vector<double> final_val;
            subset<double>(
                    total_val.data(),
                    at_ndims,
                    at_dims,
                    at_offset,
                    at_step,
                    at_count,
                    &final_val,
                    at_pos,
                    0
            );
            val2buf(final_val.data());

        } else {
            H5Tclose(at_base_type);
            string msg = "Non-supported integer or float datatypes";
            msg += "This is related to the variable " + var_path + ".";
            throw InternalErr (__FILE__, __LINE__, msg); 
        }
    }
}

void HDF5Array::do_h5_array_type_read_base_atomic_whole_data(H5T_class_t array_cls, hid_t at_base_type,int64_t at_nelms,
                                                             vector<char> &values, size_t values_offset)
{
    // For DAP2 char should be mapped to short
    if (true == promote_char_to_short(array_cls, at_base_type) && is_dap4() == false) {

        vector<char> val_int8;
        val_int8.resize(at_nelms);
        auto src = (void *) (values.data() + values_offset);
        memcpy(val_int8.data(), src, at_nelms);

        vector<short> val_short;
        for (int64_t i = 0; i < at_nelms; i++)
            val_short[i] = (short) val_int8[i];

        val2buf(val_short.data());

    }
    else // shortcut for others
        val2buf(values.data() + values_offset);
}
/// This inline routine will translate N dimensions into 1 dimension.
inline int64_t
HDF5Array::INDEX_nD_TO_1D (const std::vector < int64_t > &dims,
                const std::vector < int64_t > &pos) const
{
    //
    //  "int a[10][20][30]  // & a[1][2][3] == a + (20*30+1 + 30*2 + 1 *3)"
    //  "int b[10][2]; // &b[1][2] == b + (20*1 + 2);"
    // 
    assert (dims.size () == pos.size ());
    int64_t sum = 0;
    int start = 1;

    for (const auto &apos:pos) {
        int64_t m = 1;
        for (unsigned int j = start; j < dims.size (); j++)
            m *= dims[j];
        sum += m * apos;
        start++;
    }
    return sum;
}

// Obtain the dimension index of the next pos. of the point based on the offset, step and end 
bool HDF5Array::obtain_next_pos(vector<int64_t>& pos, vector<int64_t>&start,vector<int64_t>&end,vector<int64_t>&step,
                                int rank_change) {

    if((pos[rank_change-1] + step[rank_change-1])<=end[rank_change-1]) {
        pos[rank_change-1] = pos[rank_change-1] + step[rank_change-1];
        return true;
    }
    else {
        if( 1 == rank_change)
            return false;
        pos[rank_change-1] = start[rank_change-1];
        obtain_next_pos(pos,start,end,step,rank_change-1);
    }
    return true;
}

//! Getting a subset of a variable
//
//      \param input Input variable
//       \param dim dimension info of the input
//       \param start start indexes of each dim
//       \param stride stride of each dim
//       \param edge count of each dim
//       \param poutput output variable
// 	\parrm index dimension index
//       \return 0 if successful. -1 otherwise.
//
template<typename T>  
int HDF5Array::subset(
    const T input[],
    int rank,
    vector<int64_t> & dim,
    int64_t start[],
    int64_t stride[],
    int64_t edge[],
    std::vector<T> *poutput,
    vector<int64_t>& pos,
    int index)
{
    for(int64_t k=0; k<edge[index]; k++) 
    {	
        pos[index] = start[index] + k*stride[index];
        if(index+1<rank)
            subset(input, rank, dim, start, stride, edge, poutput,pos,index+1);			
        if(index==rank-1)
        {
            poutput->push_back(input[INDEX_nD_TO_1D( dim, pos)]);
        }
    } // end of for
    return 0;
} // end of template<typename T> static int subset


// public functions to set all parameters needed in read function.


void HDF5Array::set_memneed(size_t need) {
    d_memneed = need;
}

void HDF5Array::set_numdim(int ndims) {
    d_num_dim = ndims;
}

void HDF5Array::set_numelm(hsize_t nelms) {
    d_num_elm = nelms;
}

// We don't inherit libdap Array Class's transform_to_dap4 method since CF option is still using it.
BaseType* HDF5Array::h5dims_transform_to_dap4(D4Group *grp,const vector<string> &dimpath) {

    BESDEBUG("h5", "<h5dims_transform_to_dap4" << endl);

    if (grp == nullptr)
        return nullptr;

    Array *dest = dynamic_cast<HDF5Array*>(ptr_duplicate());

    // If there is just a size, don't make
    // a D4Dimension (In DAP4 you cannot share a dimension unless it has
    // a name). jhrg 3/18/14

    int k = 0;
    for (Array::Dim_iter d = dest->dim_begin(), e = dest->dim_end(); d != e; ++d) {

        if (false == (*d).name.empty()) {

            BESDEBUG("h5", "<coming to the dimension loop, has name " << (*d).name<<endl);
            BESDEBUG("h5", "<coming to the dimension loop, has dimpath " << dimpath[k] <<endl);
            BESDEBUG("h5", "<coming to the dimension loop, has dimpath group "
                        << dimpath[k].substr(0,dimpath[k].find_last_of("/")+1) <<endl);

            D4Group *temp_grp   = grp;
            D4Dimension *d4_dim = nullptr;
            bool is_dim_nonc4_grp = handle_one_dim(d,temp_grp, d4_dim, dimpath, k);

            // Not find this dimension in any of the ancestor groups, add it to this group.
            // The following block is fine, but to avoid the complaint from sonarcloud.
            // Use a bool.
            if (true == is_dim_nonc4_grp) {
                 string msg= "The variable " + var_path +" has dimension ";
                 msg += dimpath[k] + ". This dimension is not under its ancestor or the current group.";
                 msg += " This is not supported.";
                 delete dest;
                 throw InternalErr(__FILE__,__LINE__, msg); 
            }

            bool d4_dim_null = ((d4_dim==nullptr)?true:false);
            if (d4_dim_null == true) {
                auto d4_dim_unique = make_unique<D4Dimension>((*d).name, (*d).size);
                D4Dimensions * dims = grp->dims();
                BESDEBUG("h5", "<Just before adding D4 dimension to group" << grp->FQN() <<endl);
                d4_dim = d4_dim_unique.release();
                dims->add_dim_nocopy(d4_dim);
                (*d).dim = d4_dim;
            }
        }
        k++;
    }

    dest->set_is_dap4(true);

    return dest;

}

bool HDF5Array::handle_one_dim(Array::Dim_iter d, D4Group *temp_grp, D4Dimension * &d4_dim,
                               const vector<string> &dimpath, int k) const
{
    bool is_dim_nonc4_grp = false;
    while (temp_grp) {

        BESDEBUG("h5", "<coming to the group  has name " << temp_grp->name()<<endl);
        BESDEBUG("h5", "<coming to the group  has fullpath " << temp_grp->FQN()<<endl);

        //Obtain all the dimensions of this group.
        D4Dimensions *temp_dims = temp_grp->dims();

        // Check if this dimension is defined in this group
        d4_dim = temp_dims->find_dim((*d).name);

        // Need the full path of the dimension name
        string d4_dim_path = dimpath[k].substr(0,dimpath[k].find_last_of("/")+1);
        BESDEBUG("h5", "d4_dim_path is " << d4_dim_path<<endl);

        bool ancestor_grp = false;

        // If the dim_path is within this group or its ancestor, this is valid.
        if(d4_dim_path.find(temp_grp->FQN())==0 || temp_grp->FQN().find(d4_dim_path)==0)
            ancestor_grp = true;

        // If we find this dimension and the dimension is on the ancestral path,
        // this follows the netCDF-4/DAP4 dimension model, break.
        if(d4_dim && (temp_grp->FQN() == d4_dim_path)) {
            BESDEBUG("h5", "<FInd dimension name " << (*d).name<<endl);
            (*d).dim = d4_dim;
            is_dim_nonc4_grp = false;
            break;
        }
        // If the dimension name is not on the ancestral path, this
        // dimension must be on another path, mark it.
        else if( ancestor_grp == false) {
            is_dim_nonc4_grp = true;
            break;
        }
        else
            d4_dim = nullptr;

        if(temp_grp->get_parent())
            temp_grp = static_cast<D4Group*>(temp_grp->get_parent());
        else
            temp_grp = nullptr;

    }
    return is_dim_nonc4_grp;
}
