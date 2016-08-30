// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org> and Muqun Yang
// <myang6@hdfgroup.org> 

// Copyright (c) 2009-2016 The HDF Group, Inc. and OPeNDAP, Inc.
//
// This is free software; you can redistribute it and/or modify it under the
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
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  
/// \file HDF5Array.cc
/// \brief This file contains the implementation on data array reading for the default option.
///
/// It includes the methods to read data array into DAP buffer from an HDF5 dataset.
/// \author Hyo-Kyung Lee   (hyoklee@hdfgroup.org)
/// \author Kent Yang       (myang6@hdfgroup.org)
/// \author James Gallagher (jgallagher@opendap.org)


#include "config_hdf5.h"

#include <iostream>
#include <memory>
#include <sstream>
#include <algorithm>
#include <ctype.h>

#include <BESDebug.h>
#include <Error.h>
#include <InternalErr.h>

#include "HDF5Array.h"
#include "HDF5Structure.h"
#include "HDF5Str.h"

using namespace std;

BaseType *HDF5Array::ptr_duplicate() {
    return new HDF5Array(*this);
}

HDF5Array::HDF5Array(const string & n, const string &d, BaseType * v) :
    Array(n, d, v) {
    d_num_dim = 0;
    d_num_elm = 0;
    d_memneed = 0;
}

HDF5Array::~HDF5Array() {
}

int HDF5Array::format_constraint(int *offset, int *step, int *count) {

    // For the 0-length array case, just return 0.
    if(length() == 0)
        return 0;

    long nels = 1;
    int id = 0;

    Dim_iter p = dim_begin();

    while (p != dim_end()) {

        int start = dimension_start(p, true);
        int stride = dimension_stride(p, true);
        int stop = dimension_stop(p, true);

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

    BESDEBUG("h5","after H5Fopen "<<endl);
    BESDEBUG("h5","variable name is "<<name() <<endl);
    BESDEBUG("h5","variable path is  "<<var_path <<endl);

    hid_t dset_id = -1;

    if(true == is_dap4()) 
        dset_id = H5Dopen2(file_id,var_path.c_str(),H5P_DEFAULT);
    else 
        dset_id = H5Dopen2(file_id,name().c_str(),H5P_DEFAULT);

    BESDEBUG("h5","after H5Dopen2 "<<endl);
    // Leave the following code. We may replace the struct DS and DSattr(see hdf5_handler.h)
#if 0
    hid_t dspace_id = H5Dget_space(dset_id);
    if(dspace_id < 0) {
        H5Dclose(dset_id);
        H5Fclose(file_id);
        throw InternalErr(__FILE__,__LINE__, "Fail to obtain the dataspace .");
    }

    int num_dim = H5Sget_simple_extent_ndims(dspace_id);
    if(num_dim < 0) {
        H5Sclose(dspace_id);
        H5Dclose(dset_id);
        H5Fclose(file_id);
        throw InternalErr(__FILE__,__LINE__, "Fail to obtain the datatype .");
    }

    H5Sclose(dspace_id);
#endif

    hid_t dtype_id = H5Dget_type(dset_id);
    if(dtype_id < 0) {
        H5Dclose(dset_id);
        H5Fclose(file_id);
        throw InternalErr(__FILE__,__LINE__, "Fail to obtain the datatype .");
    }

 
    vector<int> offset(d_num_dim);
    vector<int> count(d_num_dim);
    vector<int> step(d_num_dim);
    int nelms = format_constraint(&offset[0], &step[0], &count[0]); // Throws Error.
    vector<char>values;
 
    // The dataset ID and datatype ID should be replaced by using H5Fopen
    //hid_t dset_id = d_dset_id;
    //hid_t dtype_id = d_ty_id;

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
        do_array_read(dset_id,dtype_id,values,false,0,nelms,&offset[0],&count[0],&step[0]);
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

void HDF5Array::do_array_read(hid_t dset_id,hid_t dtype_id,vector<char>&values,bool has_values,int values_offset,
                                   int nelms,int* offset,int* count, int* step)
{

    H5T_class_t  tcls = H5Tget_class(dtype_id);

    if(H5T_COMPOUND == tcls)
        m_array_of_structure(dset_id,values,has_values,values_offset,nelms,offset,count,step);
    else if(H5T_INTEGER == tcls || H5T_FLOAT == tcls || H5T_STRING == tcls) 
        m_array_of_atomic(dset_id,dtype_id,nelms,offset,count,step);
    else {
        throw InternalErr(__FILE__,__LINE__,"Fail to read the data for Unsupported datatype.");
    }
    
}

void HDF5Array:: m_array_of_atomic(hid_t dset_id, hid_t dtype_id, 
                                   int nelms,int* offset,int* count, int* step)
{
    
    hid_t memtype = -1;
    if((memtype = H5Tget_native_type(dtype_id, H5T_DIR_ASCEND))<0) {
        throw InternalErr (__FILE__, __LINE__, "Fail to obtain memory datatype.");
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

        vector<string>finstrval;
        finstrval.resize(nelms);
        try {
	    read_vlen_string(dset_id, nelms, &hoffset[0], &hstep[0], &hcount[0],finstrval);
        }
        catch(...) {
            H5Tclose(memtype);
            throw InternalErr(__FILE__,__LINE__,"Fail to read variable-length string.");
        }
        set_value(finstrval,nelms);
        H5Tclose(memtype);
	return ;
    }

    try {
        if (nelms == d_num_elm) {

	    vector<char> convbuf(d_memneed);
	    get_data(dset_id, (void *) &convbuf[0]);

	    // Check if a Signed Byte to Int16 conversion is necessary, this is only valid for DAP2.
          if(false == is_dap4()) {
            if (1 == H5Tget_size(memtype) && H5T_SGN_2 == H5Tget_sign(memtype)) 
            {
	        vector<short> convbuf2(nelms);
	        for (int i = 0; i < nelms; i++) {
		    convbuf2[i] = (signed char) (convbuf[i]);
		    BESDEBUG("h5", "convbuf[" << i << "]="
		        	<< (signed char)convbuf[i] << endl);
		    BESDEBUG("h5", "convbuf2[" << i << "]="
		        	<< convbuf2[i] << endl)
		    ;
	        }
	        // Libdap will generate the wrong output.
	        m_intern_plain_array_data((char*) &convbuf2[0],memtype);
	    }
	    else 
                m_intern_plain_array_data(&convbuf[0],memtype);
          }
          else 
              m_intern_plain_array_data(&convbuf[0],memtype);
        } // if (nelms == d_num_elm)
        else {
    	    size_t data_size = nelms * H5Tget_size(memtype);
	    if (data_size == 0) {
	        throw InternalErr(__FILE__, __LINE__, "get_size failed");
            }
	    vector<char> convbuf(data_size);
	    get_slabdata(dset_id, &offset[0], &step[0], &count[0], d_num_dim, &convbuf[0]);

	    // Check if a Signed Byte to Int16 conversion is necessary.
          if(false == is_dap4()){
            if (1 == H5Tget_size(memtype) && H5T_SGN_2 == H5Tget_sign(memtype)) {
	    //if (get_dap_type(memtype,false) == "Int8") {// bug, get_dap_type never returns Int8 type.
	        vector<short> convbuf2(data_size);
	        for (int i = 0; i < (int)data_size; i++) {
	    	    convbuf2[i] = static_cast<signed char> (convbuf[i]);
	        }
	        m_intern_plain_array_data((char*) &convbuf2[0],memtype);
	    }
	    else {
	        m_intern_plain_array_data(&convbuf[0],memtype);
	    }
          }
          else 
	      m_intern_plain_array_data(&convbuf[0],memtype);

        }
        H5Tclose(memtype);
    }
    catch (...) {
        H5Tclose(memtype);
        throw;
    }

}

bool HDF5Array::m_array_of_structure(hid_t dsetid, vector<char>&values,bool has_values,int values_offset,
                                   int nelms,int* offset,int* count, int* step) {

    BESDEBUG("h5", "=read() Array of Structure length=" << length() << endl);

    hid_t mspace   = -1;
    hid_t memtype  = -1;
    hid_t dtypeid  = -1;
    size_t ty_size = -1;

    if((dtypeid = H5Dget_type(dsetid)) < 0) 
        throw InternalErr (__FILE__, __LINE__, "Cannot obtain the datatype.");

    if((memtype = H5Tget_native_type(dtypeid, H5T_DIR_ASCEND))<0) {
        H5Tclose(dtypeid);
        throw InternalErr (__FILE__, __LINE__, "Fail to obtain memory datatype.");
    }

    ty_size = H5Tget_size(memtype);
    if (ty_size == 0) {
            H5Tclose(memtype);
            H5Tclose(dtypeid);
            throw InternalErr (__FILE__, __LINE__,"Fail to obtain the size of HDF5 compound datatype.");
    }

    if(false == has_values) {

        hid_t dspace = -1;

        if ((dspace = H5Dget_space(dsetid))<0) {
            H5Tclose(memtype);
            H5Tclose(dtypeid);
            throw InternalErr (__FILE__, __LINE__, "Cannot obtain data space.");
        }

        d_num_dim = H5Sget_simple_extent_ndims(dspace);
        if(d_num_dim < 0) {
            H5Tclose(memtype);
            H5Tclose(dtypeid);
            H5Sclose(dspace);
            throw InternalErr (__FILE__, __LINE__, "Cannot obtain the number of dimensions of the data space.");
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
                                &hoffset[0], &hstep[0],
                                &hcount[0], NULL) < 0) {
            H5Tclose(memtype);
            H5Tclose(dtypeid);
            H5Sclose(dspace);
            throw InternalErr (__FILE__, __LINE__, "Cannot generate the hyperslab of the HDF5 dataset.");
        }

        mspace = H5Screate_simple(d_num_dim, &hcount[0],NULL);
        if (mspace < 0) {
            H5Sclose(dspace);
            H5Tclose(memtype);
            H5Tclose(dtypeid);
            throw InternalErr (__FILE__, __LINE__, "Cannot create the memory space.");
        }

        values.resize(nelms*ty_size);
        hid_t read_ret = -1;
        read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,(void*)&values[0]);
        if (read_ret < 0) {
            H5Tclose(memtype);
            H5Tclose(dtypeid);
            H5Sclose(dspace);
            throw InternalErr (__FILE__, __LINE__, "Fail to read the HDF5 compound datatype dataset.");
        }

        H5Sclose(dspace);
        has_values = true;
    } // end if(false == has_values)

    HDF5Structure *h5s = NULL;
    hid_t  memb_id  = -1;      
    char* memb_name = NULL;

    try {

        // Loop through all the elements in this compound datatype array.
        for (int element = 0; element < nelms; ++element) { 

            h5s = dynamic_cast<HDF5Structure*>(var()->ptr_duplicate());
            H5T_class_t         memb_cls         = H5T_NO_CLASS;
            int                 nmembs           = 0;
            size_t              memb_offset      = 0;

            if((nmembs = H5Tget_nmembers(memtype)) < 0) 
                throw InternalErr (__FILE__, __LINE__, "Fail to obtain number of HDF5 compound datatype.");

            for(unsigned int u = 0; u < (unsigned)nmembs; u++) {

                // Get member type ID 
                if((memb_id = H5Tget_member_type(memtype, u)) < 0) 
                    throw InternalErr (__FILE__, __LINE__, "Fail to obtain the datatype of an HDF5 compound datatype member.");

                // Get member type class 
                if((memb_cls = H5Tget_member_class (memtype, u)) < 0) 
                    throw InternalErr (__FILE__, __LINE__, "Fail to obtain the datatype class of an HDF5 compound datatype member.");

                // Get member offset,H5Tget_member_offset only fails
                // when H5Tget_memeber_class fails. Sinc H5Tget_member_class
                // is checked above. So no need to check the return value.
                memb_offset= H5Tget_member_offset(memtype,u);

                // Get member name
                memb_name = H5Tget_member_name(memtype,u);
                if(memb_name == NULL) 
                    throw InternalErr (__FILE__, __LINE__, "Fail to obtain the name of an HDF5 compound datatype member.");

                BaseType *field = h5s->var(memb_name);
                if (memb_cls == H5T_COMPOUND) {  
                    HDF5Structure &memb_h5s = dynamic_cast<HDF5Structure&>(*field);
                    memb_h5s.do_structure_read(dsetid, memb_id,values,has_values,memb_offset+values_offset+ty_size*element);
                }

                else if(memb_cls == H5T_ARRAY) {

                    // memb_id, obtain the number of dimensions
                    int at_ndims = H5Tget_array_ndims(memb_id);
                    if(at_ndims <= 0)  
                        throw InternalErr (__FILE__, __LINE__, "Fail to obtain number of dimensions of the array datatype.");

                    //HDF5Array &h5_array_type = dynamic_cast<HDF5Array&>(*h5s->var(memb_name));
                    HDF5Array &h5_array_type = dynamic_cast<HDF5Array&>(*field);
                    vector<int> at_offset(at_ndims,0);
                    vector<int> at_count(at_ndims,0);
                    vector<int> at_step(at_ndims,0);

                    int at_nelms = h5_array_type.format_constraint(&at_offset[0],&at_step[0],&at_count[0]);

                    // Read the array data
                    h5_array_type.do_h5_array_type_read(dsetid,memb_id,values,has_values,memb_offset+values_offset+ty_size*element,
                                                        at_nelms,&at_offset[0],&at_count[0],&at_step[0]);

                }
                else if(memb_cls == H5T_INTEGER || memb_cls == H5T_FLOAT) {

                    if(true == promote_char_to_short(memb_cls,memb_id)) {
                        void *src = (void*)(&values[0] + (element*ty_size) + values_offset +memb_offset);
                        char val_int8;
                        memcpy(&val_int8,src,1);
                        short val_short=(short)val_int8;
                        field->val2buf(&val_short);
                    }
                    else {
                        field->val2buf(&values[0] + (element*ty_size) + values_offset +memb_offset);
                    }

                }
                else if(memb_cls == H5T_STRING) {

                    // distinguish between variable length and fixed length
                    if(true == H5Tis_variable_str(memb_id)) {
                        void *src = (void*)(&values[0]+(element*ty_size)+values_offset + memb_offset);
                        string final_str;
                        try {
                            get_vlen_str_data((char*)src,final_str);
                        }
                        catch(...) {
                            throw;
                        }
                        field->val2buf(&final_str);
                    }
                    else {// Obtain fixed-size string value
                        void *src = (void*)(&values[0]+(element*ty_size)+values_offset + memb_offset);
                        vector<char> str_val;
                        size_t memb_size = H5Tget_size(memb_id);
                        if (memb_size == 0) {
                            H5Tclose(memb_id);
                            free(memb_name);
                            delete h5s;
                            throw InternalErr (__FILE__, __LINE__,"Fail to obtain the size of HDF5 compound datatype.");
                        }
                        str_val.resize(memb_size);
                        memcpy(&str_val[0],src,memb_size);
                        string temp_string(str_val.begin(),str_val.end());
                        field->val2buf(&temp_string);
                    }
                }
                else {
                    free(memb_name);
                    H5Tclose(memb_id);
                    delete h5s;
                    throw InternalErr (__FILE__, __LINE__, 
                     "Only support the field of compound datatype when the field type class is integer, float, string, array or compound..");

                }

                // Close member type ID 
                H5Tclose(memb_id);
                free(memb_name);
                field->set_read_p(true);
            } // end for(unsigned u = 0)
            h5s->set_read_p(true);
            set_vec(element,h5s);
            delete h5s;
        } // end for (int element=0

        if(true == has_values) {
            if(-1 == mspace) 
                throw InternalErr(__FILE__, __LINE__, "memory type and memory space for this compound datatype should be valid.");

            if(H5Dvlen_reclaim(memtype,mspace,H5P_DEFAULT,(void*)&values[0])<0) 
                throw InternalErr(__FILE__, __LINE__, "Unable to reclaim the compound datatype array.");
            H5Sclose(mspace);
        
        }
    
        H5Tclose(dtypeid);
        H5Tclose(memtype);

    }
    catch(...) {

        if(memb_id != -1)
            H5Tclose(memb_id);
        if(memb_name != NULL)
            free(memb_name);
        if(h5s != NULL)
            delete h5s;
        if(true == has_values) {
            if(H5Dvlen_reclaim(memtype,mspace,H5P_DEFAULT,(void*)(&values[0]))<0) {
                H5Tclose(memtype);
                H5Sclose(mspace);
            }
            H5Sclose(mspace);
        }
        H5Tclose(memtype);
        H5Tclose(dtypeid);
        throw;
    }
    
    set_read_p(true);

    return false;
}

// Haven't checked the codes and comments
// Haven't added the close handles routines for error handlings yet. KY 2011-11-18
bool HDF5Array::m_array_of_reference(hid_t dset_id,hid_t dtype_id)
{

    hid_t memtype = H5Tget_native_type(dtype_id, H5T_DIR_ASCEND);
    if (memtype < 0)
        throw InternalErr(__FILE__, __LINE__, "cannot obtain the memory data type for the dataset.");
    
    hid_t d_ty_id = memtype;
    hid_t d_dset_id = dset_id;
    hdset_reg_ref_t *rbuf = NULL;

    try {
	vector<int> offset(d_num_dim);
	vector<int> count(d_num_dim);
	vector<int> step(d_num_dim);


	int nelms = format_constraint(&offset[0], &step[0], &count[0]); // Throws Error.
	vector<string> v_str(nelms);

	BESDEBUG("h5", "=read() URL type is detected. "
		<< "nelms=" << nelms << " full_size=" << d_num_elm << endl);

	// Handle regional reference.
	if (H5Tequal(d_ty_id, H5T_STD_REF_DSETREG) < 0) {
	    throw InternalErr(__FILE__, __LINE__, "H5Tequal() failed");
	}

	if (H5Tequal(d_ty_id, H5T_STD_REF_DSETREG) > 0) {
	    BESDEBUG("h5", "=read() Got regional reference. " << endl);
            // Vector doesn't work for this case. somehow it doesn't support the type.
            rbuf = new hdset_reg_ref_t[d_num_elm];
//            vector<hdset_reg_ref_t> rbuf;
 //           rbuf.resize(d_num_elm);
            if(rbuf == NULL){
                throw InternalErr(__FILE__, __LINE__, "new() failed.");
            }
	    if (H5Dread(d_dset_id, H5T_STD_REF_DSETREG, H5S_ALL, H5S_ALL, H5P_DEFAULT, &rbuf[0]) < 0) {
		throw InternalErr(__FILE__, __LINE__, "H5Dread() failed.");
	    }

	    for (int i = 0; i < nelms; i++) {
		// Let's assume that URL array is always 1 dimension.
		BESDEBUG("h5", "=read() rbuf[" << i << "]" <<
			rbuf[offset[0] + i * step[0]] << endl);

		if (rbuf[offset[0] + i * step[0]][0] != '\0') {
		    char r_name[DODS_NAMELEN];

		    hid_t did_r = H5Rdereference(d_dset_id, H5R_DATASET_REGION, rbuf[offset[0] + i * step[0]]);
		    if (did_r < 0) {
			throw InternalErr(__FILE__, __LINE__, "H5Rdereference() failed.");

		    }

		    if (H5Iget_name(did_r, (char *) r_name, DODS_NAMELEN) < 0) {
			throw InternalErr(__FILE__, __LINE__, "H5Iget_name() failed.");
		    }
		    BESDEBUG("h5", "=read() dereferenced name is " << r_name
			    << endl);

		    string varname(r_name);
		    hid_t space_id = H5Rget_region(did_r, H5R_DATASET_REGION, rbuf[offset[0] + i * step[0]]);
		    if (space_id < 0) {
			throw InternalErr(__FILE__, __LINE__, "H5Rget_region() failed.");

		    }

		    int ndim = H5Sget_simple_extent_ndims(space_id);
		    if (ndim < 0) {
			throw InternalErr(__FILE__, __LINE__, "H5Sget_simple_extent_ndims() failed.");
		    }

		    BESDEBUG("h5", "=read() dim is " << ndim << endl);

		    string expression;
		    switch (H5Sget_select_type(space_id)) {

		    case H5S_SEL_NONE:
			BESDEBUG("h5", "=read() None selected." << endl);
			break;

		    case H5S_SEL_POINTS: {
			BESDEBUG("h5", "=read() Points selected." << endl);
			hssize_t npoints = H5Sget_select_npoints(space_id);
			if (npoints < 0) {
			    throw InternalErr(__FILE__, __LINE__,
				    "Cannot determine number of elements in the dataspace selection");
			}

			BESDEBUG("h5", "=read() npoints are " << npoints
				<< endl);
			vector<hsize_t> buf(npoints * ndim);
			if (H5Sget_select_elem_pointlist(space_id, 0, npoints, &buf[0]) < 0) {
			    throw InternalErr(__FILE__, __LINE__, "H5Sget_select_elem_pointlist() failed.");
			}

#ifdef DODS_DEBUG
			for (int j = 0; j < npoints * ndim; j++) {
                            "h5", "=read() npoints buf[0] =" << buf[j] <<endl;
			}
#endif

			for (int j = 0; j < (int) npoints; j++) {
			    // Name of the dataset.
			    expression.append(varname);
			    for (int k = 0; k < ndim; k++) {
				ostringstream oss;
				oss << "[" << (int) buf[j * ndim + k] << "]";
				expression.append(oss.str());
			    }
			    if (j != (int) (npoints - 1)) {
				expression.append(",");
			    }
			}
			v_str[i].append(expression);

			break;
		    }
		    case H5S_SEL_HYPERSLABS: {
			vector<hsize_t> start(ndim);
			vector<hsize_t> end(ndim);

			BESDEBUG("h5", "=read() Slabs selected." << endl);
			BESDEBUG("h5", "=read() nblock is " <<
				H5Sget_select_hyper_nblocks(space_id) << endl);

			if (H5Sget_select_bounds(space_id, &start[0], &end[0]) < 0) {
			    throw InternalErr(__FILE__, __LINE__, "H5Sget_select_bounds() failed.");
			}

			for (int j = 0; j < ndim; j++) {
			    ostringstream oss;
			    BESDEBUG("h5", "=read() start is " << start[j]
				    << "=read() end is " << end[j] << endl);
			    oss << "[" << (int) start[j] << ":" << (int) end[j] << "]";
			    expression.append(oss.str());
			    BESDEBUG("h5", "=read() expression is "
				    << expression << endl)
			    ;
			}
			v_str[i] = varname;
			if (!expression.empty()) {
			    v_str[i].append(expression);
			}
			// Constraint expression. [start:1:end]
			break;
		    }
		    case H5S_SEL_ALL:
			BESDEBUG("h5", "=read() All selected." << endl);
			break;

		    default:
			BESDEBUG("h5", "Unknown space type." << endl);
			break;
		    }

		}
		else {
		    v_str[i] = "";
		}
	    }
            delete[] rbuf;
	}

	// Handle object reference.
	if (H5Tequal(d_ty_id, H5T_STD_REF_OBJ) < 0) {
	    throw InternalErr(__FILE__, __LINE__, "H5Tequal() failed.");
	}

	if (H5Tequal(d_ty_id, H5T_STD_REF_OBJ) > 0) {
	    BESDEBUG("h5", "=read() Got object reference. " << endl);
            vector<hobj_ref_t> orbuf;
            orbuf.resize(d_num_elm);
	    if (H5Dread(d_dset_id, H5T_STD_REF_OBJ, H5S_ALL, H5S_ALL, H5P_DEFAULT, &orbuf[0]) < 0) {
		throw InternalErr(__FILE__, __LINE__, "H5Dread failed()");
	    }

	    for (int i = 0; i < nelms; i++) {
		// Let's assume that URL array is always 1 dimension.
		hid_t did_r = H5Rdereference(d_dset_id, H5R_OBJECT, &orbuf[offset[0] + i * step[0]]);
		if (did_r < 0) {
		    throw InternalErr(__FILE__, __LINE__, "H5Rdereference() failed.");
		}
		char r_name[DODS_NAMELEN];
		if (H5Iget_name(did_r, (char *) r_name, DODS_NAMELEN) < 0) {
		    throw InternalErr(__FILE__, __LINE__, "H5Iget_name() failed.");
		}

		// Shorten the dataset name
		string varname(r_name);

		BESDEBUG("h5", "=read() dereferenced name is " << r_name <<endl);
		v_str[i] = varname;
	    }
	}
	set_value(&v_str[0], nelms);
        H5Tclose(memtype);
	return false;
    }
    catch (...) {
        if(rbuf!= NULL)
            delete[] rbuf;
        if(memtype != -1)
            H5Tclose(memtype);
	throw;
    }
}

void HDF5Array::m_intern_plain_array_data(char *convbuf,hid_t memtype)
{
    if (check_h5str(memtype)) {
	vector<string> v_str(d_num_elm);
	size_t elesize = H5Tget_size(memtype);
	if (elesize == 0) {
	    throw InternalErr(__FILE__, __LINE__, "H5Tget_size() failed.");
	}
	vector<char> strbuf(elesize + 1);
	BESDEBUG("h5", "=read()<check_h5str()  element size=" << elesize
		<< " d_num_elm=" << d_num_elm << endl);

	for (int strindex = 0; strindex < d_num_elm; strindex++) {
	    get_strdata(strindex, &convbuf[0], &strbuf[0], elesize);
	    BESDEBUG("h5", "=read()<get_strdata() strbuf=" << &strbuf[0] << endl);
	    v_str[strindex] = &strbuf[0];
	}
	set_read_p(true);
	val2buf((void *) &v_str[0]);
    }
    else {
	set_read_p(true);
	val2buf((void *) convbuf);
    }
}


bool HDF5Array::do_h5_array_type_read(hid_t dsetid, hid_t memb_id,vector<char>&values,bool has_values,int values_offset,
                                                   int at_nelms,int* at_offset,int* at_count, int* at_step){
    //1. Call do array first(datatype must be derived) and the value must be set. We don't support Array datatype 
    //   unless it is inside a compound datatype
    if(has_values != true) 
        throw InternalErr (__FILE__, __LINE__, "Only support the retrieval of HDF5 Array datatype values from the parent compound datatype read.");

    hid_t at_base_type = H5Tget_super(memb_id);
    if(at_base_type < 0) {
        throw InternalErr (__FILE__, __LINE__, "Fail to obtain the basetype of the array datatype.");
    }

    // memb_id, obtain the number of dimensions
    int at_ndims = H5Tget_array_ndims(memb_id);
    if(at_ndims <= 0) { 
        H5Tclose(at_base_type);
        throw InternalErr (__FILE__, __LINE__, "Fail to obtain number of dimensions of the array datatype.");
    }

    vector<hsize_t>at_dims_h(at_ndims,0);

    // Obtain the number of elements for each dims
    if(H5Tget_array_dims(memb_id,&at_dims_h[0])<0) {
        H5Tclose(at_base_type);
        throw InternalErr (__FILE__, __LINE__, "Fail to obtain dimensions of the array datatype.");
    }
    vector<int>at_dims(at_ndims,0);
    for(int i = 0;i<at_ndims;i++) {
        at_dims[i] = (int)at_dims_h[i];
    }
    int at_total_nelms = 1;
    for (int i = 0; i <at_ndims; i++) 
        at_total_nelms = at_total_nelms*at_dims[i];

    H5T_class_t array_cls = H5Tget_class(at_base_type);
    if(H5T_NO_CLASS == array_cls) {
        H5Tclose(at_base_type);
        throw InternalErr (__FILE__, __LINE__, "Fail to obtain the datatype class of the array base type.");
    }

    size_t at_base_type_size = H5Tget_size(at_base_type);
    if(0 == at_base_type_size){
        H5Tclose(at_base_type);
        throw InternalErr (__FILE__, __LINE__, "Fail to obtain the size  of the array base type.");
    }

    // H5 Array type, the basetype is COMPOUND.
    if(H5T_COMPOUND == array_cls) { 

        // These vectors are used to handle subset of array datatype
        vector<int>at_end(at_ndims,0);
        vector<int>at_pos(at_ndims,0);
        for (int i = 0; i< at_ndims; i++){
            at_pos[i] = at_offset[i];
            at_end[i] = at_offset[i] + (at_count[i] -1)*at_step[i];
        }
        
        int at_orig_index = INDEX_nD_TO_1D(at_dims,at_pos);

        // To read the array of compound (structure) in DAP, one must read one element each. set_vec is used afterwards.
        for (int array_index = 0; array_index <at_nelms; array_index++) {

            // The basetype of the array datatype is compound,-- check if the following line is valid.
            HDF5Structure *h5s = dynamic_cast<HDF5Structure*>(var()->ptr_duplicate());
            hid_t               child_memb_id;      
            H5T_class_t         child_memb_cls;
            int                 child_nmembs;
            size_t              child_memb_offset;
            unsigned            child_u;

            if((child_nmembs = H5Tget_nmembers(at_base_type)) < 0) {
                H5Tclose(at_base_type);
                delete h5s;
                throw InternalErr (__FILE__, __LINE__, "Fail to obtain number of HDF5 compound datatype.");
            }

            for(child_u = 0; child_u < (unsigned)child_nmembs; child_u++) {

                // Get member type ID 
                if((child_memb_id = H5Tget_member_type(at_base_type, child_u)) < 0) {
                    H5Tclose(at_base_type);
                    delete h5s;
                    throw InternalErr (__FILE__, __LINE__, "Fail to obtain the datatype of an HDF5 compound datatype member.");
                }

                // Get member type class 
                if((child_memb_cls = H5Tget_member_class (at_base_type, child_u)) < 0) {
                    H5Tclose(child_memb_id);
                    H5Tclose(at_base_type);
                    delete h5s;
                    throw InternalErr (__FILE__, __LINE__, "Fail to obtain the datatype class of an HDF5 compound datatype member.");
                }

                // Get member offset
                child_memb_offset= H5Tget_member_offset(at_base_type,child_u); 

                // Get member name
                char *child_memb_name = H5Tget_member_name(at_base_type,child_u);
                if(child_memb_name == NULL) {
                    H5Tclose(child_memb_id);
                    H5Tclose(at_base_type);
                    delete h5s;
                    throw InternalErr (__FILE__, __LINE__, "Fail to obtain the name of an HDF5 compound datatype member.");
                }

                BaseType *field = h5s->var(child_memb_name);
                if (child_memb_cls == H5T_COMPOUND) {  

                    HDF5Structure &memb_h5s = dynamic_cast<HDF5Structure&>(*field);
                    //
                    // Call structure read when reading the whole array. sa1{sa2[100];};
                    // sa2[100] is an array datatype.
                    // If reading the whole buffer, just add the buffer.
                    if(at_total_nelms == at_nelms) {
                        memb_h5s.do_structure_read(dsetid,child_memb_id, values,has_values,values_offset+at_base_type_size*array_index+child_memb_offset);
                    }
                    // Subset of sa2, sa2[10:100:2]; sa2[100] is an array datatype. The whole array sa2[100] is to be read into somewhere in buffer values.
                    else {// The subset should be considered. adjust memb_offset+values_offset+???,make sure only the subset is selected. 
                          // at_total_nelms is 100 but at_nelms is (100-10)/2+1=46. The starting point of the whole array is values+memb_offset_values_offset
                          // When the datatype is structure, we have to obtain the index one by one.  
                        
                        memb_h5s.do_structure_read(dsetid, child_memb_id, values,has_values,values_offset+at_base_type_size*at_orig_index+child_memb_offset);

                    }

                }
                else if(child_memb_cls == H5T_ARRAY) {

                    // memb_id, obtain the number of dimensions
                    int child_at_ndims = H5Tget_array_ndims(child_memb_id);
                    if(child_at_ndims <= 0) { 
                         H5Tclose(at_base_type);
                         H5Tclose(child_memb_id);
                         free(child_memb_name);
                         delete h5s;
                         throw InternalErr (__FILE__, __LINE__, "Fail to obtain number of dimensions of the array datatype.");
 
                    }

                    HDF5Array &h5_array_type = dynamic_cast<HDF5Array&>(*field);
                    vector<int> child_at_offset(child_at_ndims,0);
                    vector<int> child_at_count(child_at_ndims,0);
                    vector<int> child_at_step(child_at_ndims,0);

                    int child_at_nelms = h5_array_type.format_constraint(&child_at_offset[0],&child_at_step[0],&child_at_count[0]);

                    if(at_total_nelms == at_nelms) {
                        h5_array_type.do_h5_array_type_read(dsetid,child_memb_id,values,has_values,child_memb_offset+values_offset+at_base_type_size*array_index,
                                                        child_at_nelms,&child_at_offset[0],&child_at_count[0],&child_at_step[0]);
                    }
                    else {// Adjust memb_offset+values_offset, basically change at_base_type_size*array_index
                        h5_array_type.do_h5_array_type_read(dsetid,child_memb_id,values,has_values,child_memb_offset+values_offset+at_base_type_size*at_orig_index,
                                                        child_at_nelms,&child_at_offset[0],&child_at_count[0],&child_at_step[0]);
 
                    }
                }

                else if(H5T_INTEGER == child_memb_cls || H5T_FLOAT == child_memb_cls){

                    int number_index =((at_total_nelms == at_nelms)?array_index:at_orig_index);
                    if(true == promote_char_to_short(child_memb_cls,child_memb_id)) {
                        void *src = (void*)(&values[0] + (number_index*at_base_type_size) + values_offset +child_memb_offset);
                        char val_int8;
                        memcpy(&val_int8,src,1);
                        short val_short=(short)val_int8;
                        field->val2buf(&val_short);
                    }
                    else 
                        field->val2buf(&values[0] + (number_index * at_base_type_size) + values_offset+child_memb_offset);


                }
                else if(H5T_STRING == child_memb_cls){

                    int string_index =((at_total_nelms == at_nelms)?array_index:at_orig_index);

                    // distinguish between variable length and fixed length
                    if(true == H5Tis_variable_str(child_memb_id)) {

                        // Need to check if the size of variable length array type is right in HDF5 lib.
                        void *src = (void*)(&values[0]+(string_index *at_base_type_size)+values_offset+child_memb_offset);
                        string final_str;
                        char*temp_bp =(char*)src;
                        get_vlen_str_data(temp_bp,final_str);
                        field->val2buf(&final_str[0]); //field->set_value(final_str); 

                    } 
                    else {// Obtain string
                        void *src = (void*)(&values[0]+(string_index *at_base_type_size)+values_offset+child_memb_offset);
                        vector<char> str_val;
                        size_t memb_size = H5Tget_size(child_memb_id);
                        if (memb_size == 0) {
                            H5Tclose(child_memb_id);
                            H5Tclose(at_base_type);
                            free(child_memb_name);
                            delete h5s;
                            throw InternalErr (__FILE__, __LINE__,"Fail to obtain the size of HDF5 compound datatype.");
                        }
                        str_val.resize(memb_size);
                        memcpy(&str_val[0],src,memb_size);
                        field->val2buf(&str_val[0]);
                    
                    }
                }
                else {
                        H5Tclose(child_memb_id);
                        H5Tclose(at_base_type);
                        free(child_memb_name);
                        delete h5s;
                        throw InternalErr (__FILE__, __LINE__, "Unsupported datatype class for the array base type.");
 

                }
                field->set_read_p(true);
                free(child_memb_name);
                H5Tclose(child_memb_id);
               
            } // end for ( child_u = 0)
            h5s->set_read_p(true);

            // Save the value of this element to DAP structure.
            set_vec(array_index,h5s);
            delete h5s;

            vector<int>at_offsetv(at_pos.size(),0);
            vector<int>at_stepv(at_pos.size(),0);
            for (unsigned int at_index = 0; at_index<at_pos.size();at_index++){
                at_offsetv[at_index] = at_offset[at_index];
                at_stepv[at_index] = at_step[at_index];
            }
            //obtain the next position of the selected point based on the offset,end and step.
            obtain_next_pos(at_pos,at_offsetv,at_end,at_stepv,(int)(at_pos.size()));
            at_orig_index = INDEX_nD_TO_1D(at_dims,at_pos);
        }// end for (array_index = 0) for array (compound)datatype

        // Need to check the usage of set_read_p(true);
        //set_read_p(true);
    }
    else if(H5T_INTEGER == array_cls|| H5T_FLOAT == array_cls) {

        // If no subset for the array datatype, just read the whole buffer.
        if(at_total_nelms == at_nelms) {

            // For DAP2 char should be mapped to short
            if( true == promote_char_to_short(array_cls ,at_base_type)) {
                vector<char> val_int8;
                val_int8.resize(at_nelms);
                void*src = (void*)(&values[0] +values_offset);
                memcpy(&val_int8[0],src,at_nelms);
                
                vector<short> val_short;
                for (int i = 0; i<at_nelms; i++)
                    val_short[i] = (short)val_int8[i];
                 
                val2buf(&val_short[0]);

            }
            else // short cut for others
                val2buf(&values[0] + values_offset);

        }
        else { // Adjust the value for the subset of the array datatype

            // Obtain the correponding DAP type of the HDF5 data type
            string dap_type = get_dap_type(at_base_type,is_dap4());

            // The total array type data is read.
            void*src = (void*)(&values[0] + values_offset);

            // set the original position to the starting point
            vector<int>at_pos(at_ndims,0);
            for (int i = 0; i< at_ndims; i++)
                at_pos[i] = at_offset[i];

            if( BYTE == dap_type) {

                vector<unsigned char>total_val;
                total_val.resize(at_total_nelms);
                memcpy(&total_val[0],src,at_total_nelms*at_base_type_size);
 
                vector<unsigned char>final_val;
                subset<unsigned char>(
                                      &total_val[0],
                                      at_ndims,
                                      at_dims,
                                      at_offset,
                                      at_step,
                                      at_count,
                                      &final_val,
                                      at_pos,
                                      0
                                     );
                
                set_value((dods_byte*)&final_val[0],at_nelms);
                //val2buf(&final_val[0]);

            }
            else if( INT16 == dap_type) {

                 // promote char to short,DAP2 doesn't have "char" type
                if(true == promote_char_to_short(array_cls,at_base_type)) {
                    vector<char>total_val;
                    total_val.resize(at_total_nelms);
                    memcpy(&total_val[0],src,at_total_nelms*at_base_type_size);
 
                    vector<char>final_val;
                    subset<char>(
                                      &total_val[0],
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
                    for(int i = 0; i<at_nelms; i++)
                        final_val_short[i] = final_val[i];
                       
                    //field->set_value((dods_byte*)&final_val[0],at_nelms);
                    //field->val2buf(&final_val_short[0]);
                    val2buf(&final_val_short[0]);

                }
                else {// short

                    vector<short>total_val;
                    total_val.resize(at_total_nelms);
                    memcpy(&total_val[0],src,at_total_nelms*at_base_type_size);
 
                    vector<short>final_val;
                    subset<short>(
                                      &total_val[0],
                                      at_ndims,
                                      at_dims,
                                      at_offset,
                                      at_step,
                                      at_count,
                                      &final_val,
                                      at_pos,
                                      0
                                     );
                
                    //field->val2buf(&final_val[0]);
                    val2buf(&final_val[0]);

                }
            } 
            else if( UINT16 == dap_type) {
                    vector<unsigned short>total_val;
                    total_val.resize(at_total_nelms);
                    memcpy(&total_val[0],src,at_total_nelms*at_base_type_size);
 
                    vector<unsigned short>final_val;
                    subset<unsigned short>(
                                      &total_val[0],
                                      at_ndims,
                                      at_dims,
                                      at_offset,
                                      at_step,
                                      at_count,
                                      &final_val,
                                      at_pos,
                                      0
                                     );
                
                    //field->val2buf(&final_val[0]);
                    val2buf(&final_val[0]);
                    
            }
            else if(UINT32 == dap_type) {
                    vector<unsigned int>total_val;
                    total_val.resize(at_total_nelms);
                    memcpy(&total_val[0],src,at_total_nelms*at_base_type_size);
 
                    vector<unsigned int>final_val;
                    subset<unsigned int>(
                                      &total_val[0],
                                      at_ndims,
                                      at_dims,
                                      at_offset,
                                      at_step,
                                      at_count,
                                      &final_val,
                                      at_pos,
                                      0
                                     );
                    //field->val2buf(&final_val[0]);
                    val2buf(&final_val[0]);
                

            }
            else if(INT32 == dap_type) {
                    vector<int>total_val;
                    total_val.resize(at_total_nelms);
                    memcpy(&total_val[0],src,at_total_nelms*at_base_type_size);
 
                    vector<int>final_val;
                    subset<int>(
                                      &total_val[0],
                                      at_ndims,
                                      at_dims,
                                      at_offset,
                                      at_step,
                                      at_count,
                                      &final_val,
                                      at_pos,
                                      0
                                     );
                
                    //field->val2buf(&final_val[0]);
                    val2buf(&final_val[0]);

            }
            else if(FLOAT32 == dap_type) {
                    vector<float>total_val;
                    total_val.resize(at_total_nelms);
                    memcpy(&total_val[0],src,at_total_nelms*at_base_type_size);
 
                    vector<float>final_val;
                    subset<float>(
                                      &total_val[0],
                                      at_ndims,
                                      at_dims,
                                      at_offset,
                                      at_step,
                                      at_count,
                                      &final_val,
                                      at_pos,
                                      0
                                     );
                
                    //field->val2buf(&final_val[0]);
                    val2buf(&final_val[0]);

            }
            else if(FLOAT64 == dap_type) {
                    vector<double>total_val;
                    total_val.resize(at_total_nelms);
                    memcpy(&total_val[0],src,at_total_nelms*at_base_type_size);
 
                    vector<double>final_val;
                    subset<double>(
                                      &total_val[0],
                                      at_ndims,
                                      at_dims,
                                      at_offset,
                                      at_step,
                                      at_count,
                                      &final_val,
                                      at_pos,
                                      0
                                     );
                
                    //field->val2buf(&final_val[0]);
                    val2buf(&final_val[0]);

            }
            else {
                H5Tclose(at_base_type);
                throw InternalErr (__FILE__, __LINE__, 
                      "Non-supported integer or float datatypes");
            }

        }
    }
    else if(H5T_STRING == array_cls) {

        // set the original position to the starting point
        vector<int>at_pos(at_ndims,0);
        for (int i = 0; i< at_ndims; i++)
            at_pos[i] = at_offset[i];

        vector<string>total_strval;
        total_strval.resize(at_total_nelms);

        if(true == H5Tis_variable_str(at_base_type)) {
            void *src = (void*)(&values[0]+values_offset);
            char*temp_bp =(char*)src;
            for(int i = 0;i <at_total_nelms; i++){
                string tempstrval;
                get_vlen_str_data(temp_bp,tempstrval);
                total_strval[i] = tempstrval;
                temp_bp += at_base_type_size;
            }
            if(at_total_nelms == at_nelms) {
                //field->set_value(total_strval,at_total_nelms);
                set_value(total_strval,at_total_nelms);
            }
            else {// obtain subset for variable-length string.
                //field->val2buf(&values[0] + values_offset);
                vector<string>final_val;
                subset<string>(
                                      &total_strval[0],
                                      at_ndims,
                                      at_dims,
                                      at_offset,
                                      at_step,
                                      at_count,
                                      &final_val,
                                      at_pos,
                                      0
                                     );
                
                set_value(final_val,at_nelms);
                //val2buf(&final_val);

           }

        }
        else {// For fixed-size string.
            void *src = (void*)(&values[0]+values_offset);
            for(int i = 0; i <at_total_nelms; i++)
                total_strval[i].resize(at_base_type_size);

            vector<char> str_val;
            str_val.resize(at_total_nelms*at_base_type_size);
            memcpy((void*)&str_val[0],src,at_total_nelms*at_base_type_size);
            string total_in_one_string(str_val.begin(),str_val.end());
            for(int i = 0; i<at_total_nelms;i++)
                total_strval[i] = total_in_one_string.substr(i*at_base_type_size,at_base_type_size);

            if(at_total_nelms == at_nelms)
                set_value(total_strval,at_total_nelms);
            else {
                vector<string>final_val;
                subset<string>(
                               &total_strval[0],
                               at_ndims,
                               at_dims,
                               at_offset,
                               at_step,
                               at_count,
                               &final_val,
                               at_pos,
                               0
                               );
                //field->set_value(final_val,at_nelms);
                set_value(final_val,at_nelms);

            }
        }

    }
    else {
        H5Tclose(at_base_type);
        throw InternalErr (__FILE__, __LINE__, 
                     "Only support the field of compound datatype when the field type class is integer, float, string, array or compound..");

    }
 
    H5Tclose(at_base_type);

    return true;
}

/// This inline routine will translate N dimensions into 1 dimension.
inline int
HDF5Array::INDEX_nD_TO_1D (const std::vector < int > &dims,
                const std::vector < int > &pos)
{
    //
    //  int a[10][20][30];  // & a[1][2][3] == a + (20*30+1 + 30*2 + 1 *3);
    //  int b[10][2]; // &b[1][2] == b + (20*1 + 2);
    // 
    assert (dims.size () == pos.size ());
    int sum = 0;
    int start = 1;

    for (unsigned int p = 0; p < pos.size (); p++) {
        int m = 1;

        for (unsigned int j = start; j < dims.size (); j++)
            m *= dims[j];
        sum += m * pos[p];
        start++;
    }
    return sum;
}

// Obtain the dimension index of the next pos. of the point based on the offset, step and end 
bool HDF5Array::obtain_next_pos(vector<int>& pos, vector<int>&start,vector<int>&end,vector<int>&step,int rank_change) {

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
    vector<int> & dim,
    int start[],
    int stride[],
    int edge[],
    std::vector<T> *poutput,
    vector<int>& pos,
    int index)
{
    for(int k=0; k<edge[index]; k++) 
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

void HDF5Array::set_numelm(int nelms) {
    d_num_elm = nelms;
}

hid_t HDF5Array::mkstr(int size, H5T_str_t pad)
{

    hid_t str_type;

    if ((str_type = H5Tcopy(H5T_C_S1)) < 0)
	return -1;
    if (H5Tset_size(str_type, (size_t) size) < 0)
	return -1;
    if (H5Tset_strpad(str_type, pad) < 0)
	return -1;

    return str_type;
}

// We don't inherit libdap Array Class's transform_to_dap4 method since CF option is still using it.
BaseType* HDF5Array::h5dims_transform_to_dap4(D4Group *grp) {

    Array *dest = static_cast<HDF5Array*>(ptr_duplicate());

    // If there is just a size, don't make
    // a D4Dimension (In DAP4 you cannot share a dimension unless it has
    // a name). jhrg 3/18/14

    for (Array::Dim_iter d = dest->dim_begin(), e = dest->dim_end(); d != e; ++d) {
        if (false == (*d).name.empty()) {

            D4Group *temp_grp   = grp;
            D4Dimension *d4_dim = NULL;
            while(temp_grp) {

                D4Dimensions *temp_dims = temp_grp->dims();

                // Check if the dimension is defined in this group
                d4_dim = temp_dims->find_dim((*d).name);
                if(d4_dim) { 
                  (*d).dim = d4_dim;
                  break;
                }

                if(temp_grp->get_parent()) 
                    temp_grp = static_cast<D4Group*>(temp_grp->get_parent());
                else 
                    temp_grp = 0;

            }

            // Not find this dimension in any of the ancestor groups, add it to this group.
            if(d4_dim == NULL) {
                d4_dim = new D4Dimension((*d).name, (*d).size);
//cerr<<"FQN name is "<<d4_dim->fully_qualified_name() <<endl;
                D4Dimensions * dims = grp->dims();
                dims->add_dim_nocopy(d4_dim);
                (*d).dim = d4_dim;
            }
        }
    }

    dest->set_is_dap4(true);

    return dest;

}
