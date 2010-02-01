// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c) 2007, 2009 The HDF Group, Inc. and OPeNDAP, Inc.
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 1901 South First Street,
// Suite C-2, Champaign, IL 61820  

///////////////////////////////////////////////////////////////////////////////
/// \file h5dds.cc
/// \brief DDS/DODS request processing source
///
/// This file is part of h5_dap_handler, a C++ implementation of the DAP
/// handler for HDF5 data.
///
/// This file contains functions which use depth-first search to walk through
/// an HDF5 file and build the in-memory DDS.
///
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
/// \author Muqun Yang    <myang6@hdfgroup.org>
///

#include "config_hdf5.h"

#include <InternalErr.h>
#include <util.h>
#include <debug.h>
#include <mime_util.h> 

#include "h5dds.h"
#include "HDF5Int32.h"
#include "HDF5UInt32.h"
#include "HDF5UInt16.h"
#include "HDF5Int16.h"
#include "HDF5Byte.h"
#include "HDF5Array.h"
#include "HDF5ArrayEOS.h"
#include "HDF5Str.h"
#include "HDF5Float32.h"
#include "HDF5Float64.h"
#include "HDF5Grid.h"
#include "HDF5GridEOS.h"
#include "HDF5Url.h"
#include "HDF5Structure.h"
#include "h5get.h"
#include "HE5Parser.h"

extern HE5Parser eos;

/// An instance of DS_t structure defined in hdf5_handler.h.
static DS_t dt_inst; 

extern string get_hardlink(hid_t, const string &);
///////////////////////////////////////////////////////////////////////////////
/// \fn depth_first(hid_t pid, char *gname, DDS & dds, const char *fname)
/// will fill DDS table.
///
/// This function will walk through hdf5 \a gname group
/// using depth-first approach to obtain data information
/// (data type and data pattern) of all hdf5 datasets and then
/// put them into dds table.
///
/// \param pid group id
/// \param gname group name (absolute name from root group)
/// \param dds reference of DDS object
/// \param fname file name
///
/// \return 0, if failed.
/// \return 1, if succeeded.
///
/// \remarks hard link is treated as a dataset.
/// \remarks will return error message to the DAP interface.
/// \see depth_first(hid_t pid, char *gname, DAS & das, const char *fname)
///////////////////////////////////////////////////////////////////////////////
bool depth_first(hid_t pid, char *gname, DDS & dds, const char *fname)
{
    DBG(cerr
        << ">depth_first()" 
        << " pid: " << pid
        << " gname: " << gname
        << " fname: " << fname
        << endl);
    
    // Iterate through the file to see the members of the group from the root.
    hsize_t nelems = 0;
    if (H5Gget_num_objs(pid, &nelems) < 0) {
        string msg =
            "h5_dds handler: counting hdf5 group elements error for ";
        msg += gname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    DBG(cerr << " nelems = " << nelems << endl);

    for (int i = 0; i < nelems; i++) {
        
        char *oname = NULL;
        int type = -1;
        ssize_t oname_size = 0;

        try{
            // Query the length of object name.
            oname_size =
                H5Gget_objname_by_idx(pid, (hsize_t) i, NULL,
                                      (size_t) DODS_NAMELEN);

            if (oname_size <= 0) {
                string msg = "Error getting the size of hdf5 the object: ";
                msg += gname;
                throw InternalErr(__FILE__, __LINE__, msg);
            }
            // Obtain the name of the object 
            oname = new char[(size_t) oname_size + 1];
            if (H5Gget_objname_by_idx
                (pid, (hsize_t) i, oname, (size_t) (oname_size + 1)) < 0) {
                string msg =
                    "h5_dds handler: getting the hdf5 object name error from";
                msg += gname;
                throw InternalErr(__FILE__, __LINE__, msg);
            }

            type = H5Gget_objtype_by_idx(pid, (hsize_t) i);
            if (type < 0) {
                string msg =
                    "h5_dds handler: getting the hdf5 object type error from";
                msg += gname;
                throw InternalErr(__FILE__, __LINE__, msg);
            }

            switch (type) {  
            case H5G_GROUP: {
                string full_path_name =
                    string(gname) + string(oname) + "/";

                DBG(cerr << "=depth_first():H5G_GROUP " << full_path_name
                    << endl);

                // Check the hard link loop and break the loop if it exists.

                char *t_fpn = new char[full_path_name.length() + 1];
              
                (void)full_path_name.copy(t_fpn, full_path_name.length());
                t_fpn[full_path_name.length()] = '\0';
                hid_t cgroup = H5Gopen(pid, t_fpn);

                if (cgroup < 0){
		   throw InternalErr(__FILE__, __LINE__, "H5Gopen() failed.");
		}
		string oid = get_hardlink(pid, oname);
                if (oid == "") {
                    depth_first(cgroup, t_fpn, dds, fname);
                }

                if (H5Gclose(cgroup) < 0){
		   throw InternalErr(__FILE__, __LINE__, "Could not close the group.");
		}
                if(t_fpn) {delete[]t_fpn; t_fpn = NULL;}                
                break;
            }

            case H5G_DATASET:{
                string full_path_name = string(gname) + string(oname);
#ifdef CF
                // Skip the processing of coreMetadata because many netCDF
                // clients throw error in handling this metadata.
                if (eos.is_valid()) {
                    if (full_path_name.find("coremetadata") != string::npos) {
                        break;
                    }
                    if (full_path_name.find("CoreMetadata") != string::npos) {
                        break;
                    }
                }
#endif

                // Obtain hdf5 dataset handle. 
                get_dataset(pid, full_path_name, &dt_inst);
                // Put hdf5 dataset structure into DODS dds.
                // read_objects throws InternalErr.

                read_objects(dds, full_path_name, fname);
                break;
            }

            case H5G_TYPE:
            default:
                break;
            }
        } // try
        catch(...) {
            if(oname) {delete[]oname; oname = NULL;}

            throw;
            
        } // catch
        if(oname) {delete[]oname; oname = NULL;}
    } // for i is 0 ... nelems

    DBG(cerr << "<depth_first() " << endl);
    return true;
}


///////////////////////////////////////////////////////////////////////////////
/// \fn Get_bt(const string &varname,  const string  &dataset, hid_t datatype)
/// returns the pointer to the base type
///
/// This function will create a new DODS object that corresponds to HDF5
/// dataset and return the pointer of a new object of DODS datatype. If an
/// error is found, an exception of type InternalErr is thrown. 
///
/// \param varname object name
/// \param dataset name of dataset where this object comes from
/// \param datatype datatype id
/// \return pointer to BaseType
///////////////////////////////////////////////////////////////////////////////
static BaseType *Get_bt(const string &vname,
                        const string &dataset,
                        hid_t datatype)
{
    BaseType *btp = NULL;
    
    try {

        DBG(cerr << ">Get_bt varname=" << vname << " datatype=" << datatype
            << endl);

        size_t size = 0;
        int sign = -2;
        switch (H5Tget_class(datatype)) {

        case H5T_INTEGER:
            size = H5Tget_size(datatype);
            sign = H5Tget_sign(datatype);
            DBG(cerr << "=Get_bt() H5T_INTEGER size = " << size << " sign = "
                << sign << endl);

	    if (sign == H5T_SGN_ERROR) {
                throw InternalErr(__FILE__, __LINE__, "cannot retrieve the sign type of the integer");
            }
            if (size == 0) {
		throw InternalErr(__FILE__, __LINE__, "cannot return the size of the datatype");
	    }
	    else if (size == 1) {
                if (sign == H5T_SGN_2)
                    btp = new HDF5Int16(vname, dataset);
                else
                    btp = new HDF5Byte(vname, dataset);
            }
            else if (size == 2) {
                if (sign == H5T_SGN_2)
                    btp = new HDF5Int16(vname, dataset);
                else
                    btp = new HDF5UInt16(vname, dataset);
            }
            else if (size == 4) {
                if (sign == H5T_SGN_2)
                    btp = new HDF5Int32(vname, dataset);
                else
                    btp = new HDF5UInt32(vname, dataset);
            }
            else if (size == 8) {
                throw
                    InternalErr(__FILE__, __LINE__,
                                string("Unsupported HDF5 64-bit Integer type:")
                                + vname);
            }
            break;

        case H5T_FLOAT:
            size = H5Tget_size(datatype);
            DBG(cerr << "=Get_bt() H5T_FLOAT size = " << size << endl);

	    if (size == 0) {
                throw InternalErr(__FILE__, __LINE__, "cannot return the size of the datatype");
            }
            else if (size == 4) {
                btp = new HDF5Float32(vname, dataset);
            }
            else if (size == 8) {
                btp = new HDF5Float64(vname, dataset);
            }
            break;

        case H5T_STRING:
            btp = new HDF5Str(vname, dataset);
            break;

        case H5T_ARRAY: {
            BaseType *ar_bt = 0;
            try {
                DBG(cerr <<
                    "=Get_bt() H5T_ARRAY datatype = " << datatype
                    << endl);

                // Get the base datatype of the array
                hid_t dtype_base = H5Tget_super(datatype);
                ar_bt = Get_bt(vname, dataset, dtype_base);
                btp = new HDF5Array(vname, dataset, ar_bt);
                delete ar_bt; ar_bt = 0;

                // Set the size of the array.
                int ndim = H5Tget_array_ndims(datatype);
                size = H5Tget_size(datatype);
                int nelement = 1;

		if (dtype_base < 0) {
                throw InternalErr(__FILE__, __LINE__, "cannot return the base datatype");
 	        }
		if (ndim < 0) {
                throw InternalErr(__FILE__, __LINE__, "cannot return the rank of the array datatype");
                }
		if (size == 0) {
                throw InternalErr(__FILE__, __LINE__, "cannot return the size of the datatype");
                }
                DBG(cerr
                    << "=Get_bt()" << " Dim = " << ndim
                    << " Size = " << size
                    << endl);

                hsize_t size2[DODS_MAX_RANK];
                int perm[DODS_MAX_RANK];              // not used
                if(H5Tget_array_dims(datatype, size2, perm) < 0){
                    throw
                        InternalErr(__FILE__, __LINE__,
                                    string("Could not get array dims for: ")
                                      + vname);
                }


                HDF5Array &h5_ar = dynamic_cast < HDF5Array & >(*btp);
                for (int dim_index = 0; dim_index < ndim; dim_index++) {
                    h5_ar.append_dim(size2[dim_index]);
                    DBG(cerr << "=Get_bt() " << size2[dim_index] << endl);
                    nelement = nelement * size2[dim_index];
                }

                h5_ar.set_did(dt_inst.dset);
                // Assign the array datatype id.
                h5_ar.set_tid(datatype);
                h5_ar.set_memneed(size);
                h5_ar.set_numdim(ndim);
                h5_ar.set_numelm(nelement);
                h5_ar.set_length(nelement);
                h5_ar.d_type = H5Tget_class(dtype_base); 
		if (h5_ar.d_type == H5T_NO_CLASS){
		    throw InternalErr(__FILE__, __LINE__, "cannot return the datatype class identifier");
		}
            }
            catch (...) {
                if( ar_bt ) delete ar_bt;
                if( btp ) delete btp;
                throw;
            }
            break;
        }

        case H5T_REFERENCE:
            btp = new HDF5Url(vname, dataset);
            break;
        
        default:
            throw InternalErr(__FILE__, __LINE__,
                              string("Unsupported HDF5 type:  ") + vname);
        }
    }
    catch (...) {
        if( btp ) delete btp;
        throw;
    }

    if (!btp)
        throw InternalErr(__FILE__, __LINE__,
                          string("Could not make a DAP variable for: ")
                          + vname);
                                                  
    switch (btp->type()) {

    case dods_byte_c: {
        HDF5Byte &v = dynamic_cast < HDF5Byte & >(*btp);
        v.set_did(dt_inst.dset);
        v.set_tid(dt_inst.type);
        break;
    }
        
    case dods_int16_c: {
        HDF5Int16 &v = dynamic_cast < HDF5Int16 & >(*btp);
        v.set_did(dt_inst.dset);
        v.set_tid(dt_inst.type);
        break;
    }
    case dods_uint16_c: {
        HDF5UInt16 &v = dynamic_cast < HDF5UInt16 & >(*btp);
        v.set_did(dt_inst.dset);
        v.set_tid(dt_inst.type);
        break;
    }
    case dods_int32_c: {
        HDF5Int32 &v = dynamic_cast < HDF5Int32 & >(*btp);
        v.set_did(dt_inst.dset);
        v.set_tid(dt_inst.type);
        break;
    }
    case dods_uint32_c: {
        HDF5UInt32 &v = dynamic_cast < HDF5UInt32 & >(*btp);
        v.set_did(dt_inst.dset);
        v.set_tid(dt_inst.type);
        break;
    }
    case dods_float32_c: {
        HDF5Float32 &v = dynamic_cast < HDF5Float32 & >(*btp);
        v.set_did(dt_inst.dset);
        v.set_tid(dt_inst.type);
        break;
    }
    case dods_float64_c: {
        HDF5Float64 &v = dynamic_cast < HDF5Float64 & >(*btp);
        v.set_did(dt_inst.dset);
        v.set_tid(dt_inst.type);
        break;
    }
    case dods_str_c: {
        HDF5Str &v = dynamic_cast < HDF5Str & >(*btp);
        v.set_did(dt_inst.dset);
        v.set_tid(dt_inst.type);
        break;
    }
    case dods_array_c:
        break;
        
    case dods_url_c: {
        HDF5Url &v = dynamic_cast < HDF5Url & >(*btp);
        v.set_did(dt_inst.dset);
        v.set_tid(dt_inst.type);
        break;
    }
    default:
        delete btp;
        throw InternalErr(__FILE__, __LINE__,
                          string("error counting hdf5 group elements for ") 
                          + vname);
    }
    DBG(cerr << "<Get_bt()" << endl);
    return btp;
}


///////////////////////////////////////////////////////////////////////////////
/// \fn Get_structure(const string& varname, const string &dataset,
///     hid_t datatype)
/// returns a pointer of structure type. An exception is thrown if an error
/// is encountered.
/// 
/// This function will create a new DODS object that corresponds to HDF5
/// compound dataset and return a pointer of a new structure object of DODS.
///
/// \param varname object name
/// \param dataset name of the dataset from which this structure created
/// \param datatype datatype id
/// \return pointer to Structure type
///
///////////////////////////////////////////////////////////////////////////////
static Structure *Get_structure(const string &varname,
                                const string &dataset,
                                hid_t datatype)
{
    HDF5Structure *structure_ptr = NULL;

    string vname = get_short_name(varname);

    DBG(cerr << ">Get_structure()" << datatype << endl);

    if (H5Tget_class(datatype) != H5T_COMPOUND)
        throw InternalErr(__FILE__, __LINE__,
                          string("Compound-to-structure mapping error for ")
                          + vname);

    try {
        structure_ptr = new HDF5Structure(vname, dataset);
        structure_ptr->set_did(dt_inst.dset);
        structure_ptr->set_tid(dt_inst.type);

        // Retrieve member types
        int nmembs = H5Tget_nmembers(datatype);
        DBG(cerr << "=Get_structure() has " << nmembs << endl);
	if (nmembs < 0){
	   throw InternalErr(__FILE__, __LINE__, "cannot retrieve the number of elements");
	}
        for (int i = 0; i < nmembs; i++) {
            char *memb_name = H5Tget_member_name(datatype, i);
            H5T_class_t memb_cls = H5Tget_member_class(datatype, i);
            hid_t memb_type = H5Tget_member_type(datatype, i);
	    if (memb_name == NULL){
		throw InternalErr(__FILE__, __LINE__, "cannot retrieve the name of the member");
	    }
            if (memb_cls < 0 | memb_type < 0) {
                // structure_ptr is deleted in the catch ... block
                // below. So if this exception is thrown, it will
                // get caught below and the ptr deleted.
                // pwest Mar 18, 2009
                //delete structure_ptr;
                throw InternalErr(__FILE__, __LINE__,
                                  string("Type mapping error for ")
                                  + string(memb_name) );
            }
            
            // ~Structure() will delete these if they are added.
            if (memb_cls == H5T_COMPOUND) {
                Structure *s = Get_structure(memb_name, dataset, memb_type);
                structure_ptr->add_var(s);
                delete s; s = 0;
            } 
            else {
                BaseType *bt = Get_bt(memb_name, dataset, memb_type);
                structure_ptr->add_var(bt);
                delete bt; bt = 0;
            }
        }
    }
    catch (...) {
        // if memory allocation exception thrown it will be caught
        // here, so should check if structure ptr exists before
        // deleting it. pwest Mar 18, 2009
        if( structure_ptr ) delete structure_ptr;
        throw;
    }

    DBG(cerr << "<Get_structure()" << endl);

    return structure_ptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \fn process_grid_matching_dimscale(Array* array, Grid * gr,
///     hid_t* dimids)
///  builds a DAP Grid for dataset that has matching dimension scales.
/// 
/// This function constructs Grid object that corresponds to HDF5
/// dataset that has dimension scales. Mapping such dataset to DAP Grid
/// enables OPeNDAP clients to visualize the dataset. The h4toh5 conversion
/// tool preserves dimension scale information from HDF4 files and we want
/// to visualize the converted HDF5 files easily through OPeNDAP.
///
/// In this function, the \a gr is modified to have map arrays that match
/// dimension names.
///
/// \param array dataset array
/// \param gr grid
/// \param dimids dimension ids
///
///////////////////////////////////////////////////////////////////////////////
static void process_grid_matching_dimscale(Array* array, Grid *gr,
					   hid_t *dimids)
{ 
        // char *dimname_buf;
        // ssize_t bufsize;
        // hid_t temp_dtype;
        // hid_t temp_dspace;
        // hid_t temp_nelm; Not used. jhrg 1/19/2010

        for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++) {
            
            ssize_t bufsize = H5Iget_name(dimids[dim_index],NULL,0);
            if(bufsize<=0){
              throw 
                 InternalErr(__FILE__,__LINE__,
			     "Fail to get the dimension name size");
           }
#if 0
            try {
#endif
        	vector<char> dimname_buf(bufsize+1);
#if 0
            }
            catch(...) {
               if(dimname_buf) {
                  delete [] dimname_buf;
                  dimname_buf = NULL;
               }
               throw InternalErr(__FILE__,__LINE__, "Fail to allocate memory");
            }
#endif
            if((H5Iget_name(dimids[dim_index], (char *)&dimname_buf[0], (size_t)(bufsize+1))) < 0) {
                throw
                    InternalErr(__FILE__, __LINE__,
                            "Fail to get the dimension name");
            }
            string each_dim_name(&dimname_buf[0]);
            each_dim_name = get_short_name(each_dim_name);
            // delete [] dimname_buf; dimname_buf = NULL;

            hid_t temp_dspace = H5Dget_space(dimids[dim_index]);
 	    if (temp_dspace < 0){
                throw InternalErr(__FILE__, __LINE__, "H5Dget_space() failed.");
            }
            hsize_t temp_nelm_dim = H5Sget_simple_extent_npoints(temp_dspace);
	    if (temp_nelm_dim == 0){
		throw InternalErr(__FILE__, __LINE__, "cannot determine the number of elements in the dataspace");
	    }
            DBG(cerr << "nelem = " << temp_nelm_dim << endl);
            hid_t temp_dtype = H5Dget_type(dimids[dim_index]);
	    if (temp_dtype < 0){
                throw InternalErr(__FILE__, __LINE__, "H5Dget_type() failed.");
            }
            hid_t memtype = H5Tget_native_type(temp_dtype, H5T_DIR_ASCEND);
	    if (memtype < 0) {
		throw InternalErr(__FILE__, __LINE__, "cannot return the native datatype");
	    }
            size_t temp_tsize = H5Tget_size(memtype);
	    if (temp_tsize == 0){
		throw InternalErr(__FILE__, __LINE__, "cannot return the size of datatype");
	    }

            BaseType *bt = 0;
            HDF5Array *map = 0;
            try {
                bt = Get_bt(each_dim_name, gr->dataset(), memtype);
                map = new HDF5Array(each_dim_name, gr->dataset(), bt);
                delete bt; bt = 0;
                map->set_did(dimids[dim_index]);
                map->set_tid(memtype);
                map->set_memneed(temp_tsize * temp_nelm_dim);
                map->set_numdim(1);
                // temp_nelm is never set in this code. jhrg 1/19/2010
                //map->set_numelm(temp_nelm);
                map->set_numelm(temp_nelm_dim);
                map->append_dim(temp_nelm_dim, each_dim_name);
                array->append_dim(temp_nelm_dim, each_dim_name);
                gr->add_var(map, maps);
                delete map; map = 0;
            }
            catch(...) {
                if( bt ) delete bt;
                if( map ) delete map;
                throw;
            }
        
        } // for ()

}
#ifdef CF
///////////////////////////////////////////////////////////////////////////////
/// \fn write_grid_shared_dimensions(DDS& dds_table, const string & filename)
///
/// inserts shared dimension variables into \a dds_table table.
/// 
/// NASA AURA grid files do not have shared dimension variables and we
/// need to infer them from StructMetadata. We compute the values of
/// geo-location dimensions like lat / lon from the StructMetadata information
/// through parser. For the rest, we simply put dummy values from 0,1,..n
/// where n is the size of dimension.
///
/// \param dds_table the destination for the shared dimension Array object
/// \param filename the file name of NASA AURA Grid
///////////////////////////////////////////////////////////////////////////////
static void write_grid_shared_dimensions(DDS & dds_table,
                                         const string & filename) {

    // Add all shared dimension data.
    if (!eos.get_shared_dimension()) {
        map < string, int >   grid_dimension_processed;
        int j;
        BaseType *bt = 0;
        Array *ar = 0;    
        vector < string > dimension_names;
        eos.get_grid_dimension_list(dimension_names);

        for(j=0; j < dimension_names.size(); j++){
            bool new_dim = false;
            int shared_dim_size =
                eos.get_grid_dimension_size(dimension_names.at(j));
            string str_grid_name = eos.get_grid_name(dimension_names.at(j));
            string str_cf_name =
                eos.get_CF_name((char*) dimension_names.at(j).c_str());
            // Check for duplicate dimension names.
            int old_dim_size = grid_dimension_processed[str_cf_name];
            if(old_dim_size > 0) {
                // If a duplicate entry is found, check its size.
                if(old_dim_size != shared_dim_size){
                    // If the size doesn't match, thrown an error.
                    // CF convention doesn't allow multiple dimensions.
                    throw InternalErr(__FILE__, __LINE__,
                                      "detected multiple shared dimensions with different sizes");
                }
                else{
                    // If the size matches, skip adding it.
                    new_dim = false;
                }
            }
            else{
                new_dim = true;
            }
            
            if(new_dim){
                
                bt = new HDF5Float32(str_cf_name, filename);
                ar = new HDF5ArrayEOS(str_cf_name, filename, bt);

                ar->add_var(bt);
                int dim_id =
                    eos.get_grid_dimension_data_location(dimension_names.at(j));
                ((HDF5ArrayEOS*)ar)->set_dim_id(dim_id);
                delete bt; bt = 0;
                ar->append_dim(shared_dim_size, str_cf_name);
                dds_table.add_var(ar);
                delete ar; ar = 0;
                // Remember that it's processed.
                grid_dimension_processed[str_cf_name] = shared_dim_size;
            }

        } // for
        // Set the flag for "shared dimension" true.        
        eos.set_shared_dimension();        
    } // if
}
#endif

/// processes the NASA EOS AURA Grids.
static void process_grid_nasa_eos(const string &varname, 
                                  Array *array, Grid *gr, DDS &dds_table) {
    // Fill the map part of the grid.
    // Retrieve the dimension lists from the parsed metadata.
    vector < string > tokens;
    // Retrieve the full path to the each dimension name.
    string str_grid_name = eos.get_grid_name(varname);

    eos.get_grid_variable_dimensions(varname, tokens);
    DBG(cerr << "=read_objects_base_type():Number of dimensions "
        << dt_inst.ndims << endl);

    for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++) {
        DBG(cerr << "=read_objects_base_type():Dim name " <<
            tokens.at(dim_index) << endl);

        string str_dim_name = tokens.at(dim_index);

        string str_dim_full_name = str_grid_name + str_dim_name;
        int dim_size = eos.get_grid_dimension_size(str_dim_full_name);
        int dim_id =            
            eos.get_grid_dimension_data_location(str_dim_full_name);

#ifdef CF
        // Rename dimension name according to CF convention.
        str_dim_full_name =
            eos.get_CF_name((char *) str_dim_full_name.c_str());
#endif

        BaseType *bt = 0;
        Array *ar = 0;
        try {
            bt = new HDF5Float32(str_dim_full_name, gr->dataset());
            ar = new HDF5ArrayEOS(str_dim_full_name, gr->dataset(), bt);
            delete bt; bt = 0;
            ((HDF5ArrayEOS*)ar)->set_dim_id(dim_id);
            ar->append_dim(dim_size, str_dim_full_name);
            array->append_dim(dim_size, str_dim_full_name);
            gr->add_var(ar, maps);
            delete ar; ar = 0;
            
        }
        catch (...) {
            // Memory allocation exceptions could be thrown in
            // creating one of these two pointers, so check if they exist
            // before deleting them.
            if( bt ) delete bt;
            if( ar ) delete ar;
            throw;
        }
    }

}


///////////////////////////////////////////////////////////////////////////////
/// \fn write_swath(DDS& dds_table, string varname, const string & filename)
///
/// inserts a HDF-EOS5 Swath dataset into \a dds_table table.
/// 
/// We can map Swath variables into DAP Arrays, not Grids. We must add
/// dimension names to make CF compatible.
///
/// \param dds_table the destination for the Swath  Array object
/// \param varname the full path name of Swath dataset
/// \param filename the file name of Swath file
///////////////////////////////////////////////////////////////////////////////
static void write_swath(DDS & dds_table,  
                        string varname, 
                        const string & filename) 
                    
{
    string sname = varname;
    DBG(cerr << ">write_swath():sname=" << sname 
        << " varname=" << varname 
        << " filename=" << filename
        << endl);

    if(eos.get_swath_variable(varname)) {
        // Rename the variable if necessary.
        sname = eos.get_CF_name((char*) varname.c_str());
#ifdef SHORT_PATH
        if(sname == varname){   // It means it's not one of lat/lon/lev/time.
            sname = eos.get_short_name(varname);
        }
#endif         
    }

    // Get a base type. It should be int, float, double, etc. -- atomic
    // datatype. 
    BaseType *bt = Get_bt(sname, filename, dt_inst.type);
    
    if (!bt) {
        // NB: We're throwing InternalErr even though it's possible that
        // someone might ask for an HDF5 varaible which this server cannot
        // handle.
        throw
            InternalErr(__FILE__, __LINE__,
                        "Unable to convert hdf5 datatype to dods basetype");
    }

    // First deal with scalar data. 
    if (dt_inst.ndims == 0) {
        dds_table.add_var(bt);
        delete bt; bt = 0;
    }
    else {
        HDF5Array *ar = new HDF5Array(sname, filename, bt);
        vector < string > dimension_names;
        eos.get_swath_variable_dimensions(varname, dimension_names);

        delete bt; bt = 0;
        ar->set_did(dt_inst.dset);
        ar->set_tid(dt_inst.type);
        ar->set_memneed(dt_inst.need);
        ar->set_numdim(dt_inst.ndims); 
        ar->set_numelm((int) (dt_inst.nelmts));
        for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++){
            if(dimension_names.size() > 0){
                string str_dim_name = dimension_names.at(dim_index);
                DBG(cerr << "=write_swath():dimname=" << str_dim_name << endl);
                ar->append_dim(dt_inst.size[dim_index], str_dim_name);
            }
            else{
                ar->append_dim(dt_inst.size[dim_index]);
            }
        }

        dds_table.add_var(ar);
        delete ar; ar = 0;
    }

    DBG(cerr << "<write_swath():variable=" << sname  << endl);
}


///////////////////////////////////////////////////////////////////////////////
/// \fn read_objects_base_type(DDS & dds_table,
///                            const string & varname,
///                            const string & filename)
/// fills in information of a dataset (name, data type, data space) into one
/// DDS table.
/// 
/// Given a reference to an instance of class DDS and a filename that refers
/// to an hdf5 file, read hdf5 file and extract all the dimensions of
/// each of its variables. Add the variables and their dimensions to the
/// instance of DDS.
///
/// It will use dynamic cast toput necessary information into subclass of dods
/// datatype. 
///
///    \param dds_table Destination for the HDF5 objects. 
///    \param varname Absolute name of either a dataset or a group
///    \param filename Added to the DDS (dds_table).
///    \throw error a string of error message to the dods interface.
///////////////////////////////////////////////////////////////////////////////
void
read_objects_base_type(DDS & dds_table, const string & a_name,
                       const string & filename)
{
    dds_table.set_dataset_name(name_path(filename)); 
#ifdef CF
    // If the array is part of HDF-EOS5 swath, returrn arrays.
    if(eos.get_swath()){
        write_swath(dds_table, a_name, filename);
        return;
    }
#endif
    bool has_dim_scale = false;
    bool has_dim_group = false;
#ifdef NASA_EOS_GRID
    bool is_eos_grid = false;
#endif
    string varname = a_name;
    string sname = varname;
    hid_t *dimids;
    try {
        dimids = new hid_t[dt_inst.ndims];
    }
    catch (...) {
        if(dimids) {
            delete [] dimids;
            dimids=0;
        }
        throw;
    }
    // Check if this variable is part of HDF4_DIMGROUP.
    if(varname.find("/HDF4_DIMGROUP/") != string::npos){
        has_dim_group = true;
        // Remove any '/' for GrADS compatibility.
        sname = get_short_name_dimscale(sname);
    }
         
    // Check if this array can be a Grid through dimension maps.
    if (has_matching_grid_dimscale(dt_inst.dset, 
                                   dt_inst.ndims, 
                                   dt_inst.size,dimids)) {
        has_dim_scale = true;
        // Remove any '/' for GrADS compatibility.
        sname = get_short_name_dimscale(sname);
    }

#ifdef NASA_EOS_GRID
    // Check if this array is EOS array can be a Grid.
    // This EOS variable needs to be treated as an array.
    if (eos.is_valid() && eos.get_grid_variable(varname)) {
        is_eos_grid = true;
#ifdef CF        
        write_grid_shared_dimensions(dds_table, filename);
#ifdef SHORT_PATH        
        sname = eos.get_short_name(varname);
#endif        
#endif
    }
#endif
    

    // Get a base type. It should be int, float, double, etc. -- atomic
    // datatype. 
    BaseType *bt = Get_bt(sname, filename, dt_inst.type);
    
    if (!bt) {
        // NB: We're throwing InternalErr even though it's possible that
        // someone might ask for an HDF5 varaible which this server cannot
        // handle.
        throw
            InternalErr(__FILE__, __LINE__,
                        "Unable to convert hdf5 datatype to dods basetype");
    }

    // First deal with scalar data. 
    if (dt_inst.ndims == 0) {
        dds_table.add_var(bt);
        delete bt; bt = 0;
    }
    else {
        // Next, deal with Array and Grid data. This 'else clause' runs to
        // the end of the method. jhrg

        HDF5Array *ar = new HDF5Array(sname, filename, bt);
        delete bt; bt = 0;
        ar->set_did(dt_inst.dset);
        ar->set_tid(dt_inst.type);
        ar->set_memneed(dt_inst.need);
        ar->set_numdim(dt_inst.ndims);
        ar->set_numelm((int) (dt_inst.nelmts));
#ifndef DODSGRID // DODSGRID is defined in common.h by default.
        // If DODSGRID is not defined, it always has to be an array.
	for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++)
            ar->append_dim(dt_inst.size[dim_index]); 
        dds_table.add_var(ar);
        delete ar; ar = 0;
#else // DODSGRID is defined
        // Check whether this HDF5 dataset can be mapped to the grid data
        // For pure HDF5 dataset,
        // We only need to check if the HDF5 dataset
        // is following the HDF5 dimension scale specification.
        // http://www.hdfgroup.org/HDF5/doc/HL/H5DS_Spec.pdf
        // Of course, for each dimension, only one dimension scale needs to
        // be applied. 
        // For HDF-EOS grid, we need to check if we have HDF-EOS5 grid data
        // If the dataset has dimension scale following HDF5 dimension scale 
        // specification
        // Please also be aware that only one scale should attach to one 
        // dimension.
        // We don't support multiple scales applying to one dimension.
       {
           if (has_dim_scale) {
               // Construct a grid instead of returning a simple array.
               Grid *gr = new HDF5Grid(sname, filename);
               process_grid_matching_dimscale(ar, gr, dimids);
               gr->add_var(ar, array);
               delete ar; ar = 0;
               dds_table.add_var(gr);
               delete gr; gr = 0;
           }
#ifdef NASA_EOS_GRID
           else if (is_eos_grid) {
               DBG(cerr << "EOS Grid: " << varname << endl);
               // Generate grid based on the parsed StructMetada.
               Grid *gr = new HDF5GridEOS(sname, filename);
               process_grid_nasa_eos(varname, ar, gr, dds_table);
               gr->add_var(ar, array);
               delete ar; ar = 0;

               dds_table.add_var(gr);
               delete gr; gr = 0;
           }
#endif // #ifdef  NASA_EOS_GRID
           else {              // cannot be mapped to grid, must be an array.
               for (int dim_index = 0; dim_index < dt_inst.ndims; dim_index++){

                   if(has_dim_group){
                       // IDV and Panoply requires named dimensions for shared
                       // dimension map variables.
                       ar->append_dim(dt_inst.size[dim_index], sname); 
                   }
                   else{
                       ar->append_dim(dt_inst.size[dim_index]);
                   }
               }
               dds_table.add_var(ar);
               delete ar; ar = 0;
           }
           delete [] dimids;
       } // end of DODS Grid case
#endif                          // #ifndef DODSGRID
    }

    DBG(cerr << "<read_objects_base_type(dds)" << endl);
}

///////////////////////////////////////////////////////////////////////////////
/// \fn read_objects_structure(DDS & dds_table,const string & varname,
///                  const string & filename)
/// fills in information of a structure dataset (name, data type, data space)
/// into a DDS table.
/// 
///    \param dds_table Destination for the HDF5 objects. 
///    \param varname Absolute name of structure
///    \param filename Added to the DDS (dds_table).
///    \throw error a string of error message to the dods interface.
///////////////////////////////////////////////////////////////////////////////
void
read_objects_structure(DDS & dds_table, const string & varname,
                       const string & filename)
{
    dds_table.set_dataset_name(name_path(filename));

    Structure *structure = Get_structure(varname, filename, dt_inst.type);
    try {
        // Assume Get_structure() uses exceptions to signal an error. jhrg
        DBG(cerr << "=read_objects_structure(): Dimension is " 
            << dt_inst.ndims << endl);

        if (dt_inst.ndims != 0) {   // Array of Structure
            int dim_index;
            DBG(cerr << "=read_objects_structure(): array of size " <<
                dt_inst.nelmts << endl);
            DBG(cerr << "=read_objects_structure(): memory needed = " <<
                dt_inst.need << endl);
            HDF5Array *ar = new HDF5Array(varname, filename, structure);
            delete structure; structure = 0;
            try {
                ar->set_did(dt_inst.dset);
                ar->set_tid(dt_inst.type);
                ar->set_memneed(dt_inst.need);
                ar->set_numdim(dt_inst.ndims);
                ar->set_numelm((int) (dt_inst.nelmts));
                ar->set_length((int) (dt_inst.nelmts));

                for (dim_index = 0; dim_index < dt_inst.ndims; dim_index++) {
                    ar->append_dim(dt_inst.size[dim_index]);
                    DBG(cerr << "=read_objects_structure(): append_dim = " <<
                        dt_inst.size[dim_index] << endl);
                }

                dds_table.add_var(ar);
                delete ar; ar = 0;
            } // try Array *ar
            catch (...) {
                delete ar;
                throw;
            }
        } else {
            dds_table.add_var(structure);
            delete structure; structure = 0;

        }
    } // try     Structure *structure = Get_structure(...)
    catch (...) {
        delete structure;
        throw;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \fn read_objects(DDS & dds_table,const string & varname,
///                  const string & filename)
/// fills in information of a dataset (name, data type, data space) into one
/// DDS table.
/// 
///    \param dds_table Destination for the HDF5 objects. 
///    \param varname Absolute name of either a dataset or a group
///    \param filename Added to the DDS (dds_table).
///    \throw error a string of error message to the dods interface.
///////////////////////////////////////////////////////////////////////////////
void
read_objects(DDS & dds_table, const string &varname, const string &filename)
{

    switch (H5Tget_class(dt_inst.type)) {

    case H5T_COMPOUND:
        read_objects_structure(dds_table, varname, filename);
        break;

    default:
        read_objects_base_type(dds_table, varname, filename);
        break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \fn get_short_name(string varname)
/// returns a short name.
///
/// This function returns a short name from \a varname.
///
/// For HDF-EOS5 case, 
/// Short name is defined as a string from the last '/' to the end of \a
/// varname excluding the '/'. Then, the extracted string is prefixed with
/// "A<number>". Finally, the prefixed string is shortened to the first 15
/// characters. This function is meaningful only for HDF-EOS5 files
/// when --enable-cf and --enable-short-name configuration options are
/// set to be "yes".
///
///
/// For a file converted via h4toh5 tool from a HDF4 file that contains 
/// the dimension scale, short name is defined as a string that doesn't
/// contain any group path information.
/// 
/// \param varname a full object name that has a full group path information
/// \return a shortened string
///////////////////////////////////////////////////////////////////////////////
string get_short_name(string varname)
{
  if(varname.find("/HDF4_DIMGROUP/") != string::npos)
    {
        return get_short_name_dimscale(varname);
    }
    return varname;
}


///////////////////////////////////////////////////////////////////////////////
/// \fn get_short_name_dimscale(string dimname)
/// returns a short name of dimname.
///
/// This function returns a short name from \a dimname.
/// Short name is defined as a string from the last '/' to the end of \a
/// dimname excluding the '/'. For example, "/HDF4_DIMGROUP/time" will be 
/// shortened to "time".
/// 
/// \param dimname a full object name that has a full group path information
/// \return a shortened string
///////////////////////////////////////////////////////////////////////////////
string get_short_name_dimscale(string dimname)
{
  int pos = dimname.find_last_of('/', dimname.length() - 1);
  return dimname.substr(pos + 1);
}

