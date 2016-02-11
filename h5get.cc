// data server.

// Copyright (c) 2007-2016 The HDF Group, Inc. and OPeNDAP, Inc.
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

///////////////////////////////////////////////////////////////////////////////
/// \file h5get.cc
///  iterates all HDF5 internals.
/// 
///  This file includes all the routines to search HDF5 group, dataset, links,
///  and attributes. since we are using HDF5 C APIs, we include all c functions
///  in this file.
///
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
/// \author Muqun Yang    <myang6@hdfgroup.org>
///
///////////////////////////////////////////////////////////////////////////////

#include "h5get.h"
#include "HDF5Int32.h"
#include "HDF5UInt32.h"
#include "HDF5UInt16.h"
#include "HDF5Int16.h"
#include "HDF5Byte.h"
#include "HDF5Int8.h"
#include "HDF5Int64.h"
#include "HDF5UInt64.h"
#include "HDF5Array.h"
#include "HDF5Str.h"
#include "HDF5Float32.h"
#include "HDF5Float64.h"
#include "HDF5Url.h"
#include "HDF5Structure.h"

#include <BESDebug.h>
#include <math.h>

using namespace libdap;

// H5Ovisit call back function. When finding the dimension scale attributes, return 1. 
static int
visit_obj_cb(hid_t o_id, const char *name, const H5O_info_t *oinfo,
    void *_op_data);

// H5Aiterate2 call back function, check if having the dimension scale attributes.
static herr_t attr_info(hid_t loc_id, const char *name, const H5A_info_t *ainfo, void *opdata);


///////////////////////////////////////////////////////////////////////////////
/// \fn get_attr_info(hid_t dset, int index, bool is_dap4,DSattr_t *attr_inst_ptr,
///                  bool *ignoreptr)
///  will get attribute information.
///
/// This function will get attribute information: datatype, dataspace(dimension
/// sizes) and number of dimensions and put it into a data struct.
///
/// \param[in]  dset  dataset id
/// \param[in]  index  index of attribute
/// \param[in]  is_dap4 is this for DAP4
/// \param[out] attr_inst_ptr an attribute instance pointer
/// \param[out] ignoreptr  a flag to record whether it can be ignored.
/// \return pointer to attribute structure
/// \throw InternalError 
///////////////////////////////////////////////////////////////////////////////
hid_t get_attr_info(hid_t dset, int index, bool is_dap4, DSattr_t * attr_inst_ptr,
                    bool *ignore_attr_ptr)
{

    hid_t attrid = -1;

    // Always assume that we don't ignore any attributes.
    *ignore_attr_ptr = false;

    if ((attrid = H5Aopen_by_idx(dset, ".", H5_INDEX_CRT_ORDER, H5_ITER_INC,(hsize_t)index, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        string msg = "unable to open attribute by index ";
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    // Obtain the size of attribute name.
    ssize_t name_size =  H5Aget_name(attrid, 0, NULL);
    if (name_size < 0) {
        H5Aclose(attrid);
        string msg = "unable to obtain the size of the hdf5 attribute name ";
        throw InternalErr(__FILE__, __LINE__, msg);
    };

    vector<char> attr_name;
    attr_name.resize(name_size+1);
    // Obtain the attribute name.    
    if ((H5Aget_name(attrid, name_size+1, &attr_name[0])) < 0) {
        H5Aclose(attrid);
        string msg = "unable to obtain the hdf5 attribute name  ";
        throw InternalErr(__FILE__, __LINE__, msg);
    }
    
    // Obtain the type of the attribute. 
    hid_t ty_id = -1;
    if ((ty_id = H5Aget_type(attrid)) < 0) {
        string msg = "unable to obtain hdf5 datatype for the attribute  ";
        string attrnamestr(attr_name.begin(),attr_name.end());
        msg += attrnamestr;
        H5Aclose(attrid);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    H5T_class_t ty_class = H5Tget_class(ty_id);
    if (ty_class < 0) {
        string msg = "cannot get hdf5 attribute datatype class for the attribute ";
        string attrnamestr(attr_name.begin(),attr_name.end());
        msg += attrnamestr;
        H5Aclose(attrid);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    // The following datatype will not be supported for mapping to DAS for both DAP2 and DAP4.
    // Note:DAP4 explicitly states that the data should be defined atomic datatype(int, string).
    // 1-D variable length of string can also be mapped to both DAS and DDS.
    // The variable length string class is H5T_STRING rather than H5T_VLEN,
    // So safe here. 
    // We also ignore the mapping of integer 64 bit for DAP2 since DAP2 doesn't
    // support 64-bit integer. In theory, DAP2 doesn't support long double
    // (128-bit or 92-bit floating point type), since this rarely happens
    // in DAP application, we simply don't consider here.
    // However, DAP4 supports 64-bit integer.
    if ((ty_class == H5T_TIME) || (ty_class == H5T_BITFIELD)
        || (ty_class == H5T_OPAQUE) || (ty_class == H5T_ENUM)
        || (ty_class == H5T_REFERENCE) ||(ty_class == H5T_COMPOUND)
        || (ty_class == H5T_VLEN) || (ty_class == H5T_ARRAY)){ 
        
        *ignore_attr_ptr = true;
        H5Tclose(ty_id);
        return attrid;
    }

    // Ignore 64-bit integer for DAP2.
    if (false == is_dap4) {
        if((ty_class == H5T_INTEGER) && (H5Tget_size(ty_id)== 8)) {//64-bit int
            *ignore_attr_ptr = true;
            H5Tclose(ty_id);
            return attrid;
        }
    }       

    hid_t aspace_id = -1;
    if ((aspace_id = H5Aget_space(attrid)) < 0) {
        string msg = "cannot get hdf5 dataspace id for the attribute ";
        string attrnamestr(attr_name.begin(),attr_name.end());
        msg += attrnamestr;
        H5Aclose(attrid);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    // It is better to use the dynamic allocation of the array.
    // However, since the DODS_MAX_RANK is not big and it is also
    // used in other location, we still keep the original code.
    // KY 2011-11-16

    int ndims = H5Sget_simple_extent_ndims(aspace_id);
    if (ndims < 0) {
        string msg = "cannot get hdf5 dataspace number of dimension for attribute ";
        string attrnamestr(attr_name.begin(),attr_name.end());
        msg += attrnamestr;
        H5Sclose(aspace_id);
        H5Aclose(attrid);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    // Check if the dimension size exceeds the maximum number of dimension DAP supports
    if (ndims > DODS_MAX_RANK) {
        string msg = "number of dimensions exceeds allowed for attribute ";
        string attrnamestr(attr_name.begin(),attr_name.end());
        msg += attrnamestr;
        H5Sclose(aspace_id);
        H5Aclose(attrid);
        throw InternalErr(__FILE__, __LINE__, msg);
    }
      
    hsize_t size[DODS_MAX_RANK];
    hsize_t maxsize[DODS_MAX_RANK];

    // DAP applications don't care about the unlimited dimensions 
    // since the applications only care about retrieving the data.
    // So we don't check the maxsize to see if it is the unlimited dimension 
    // attribute.
    if (H5Sget_simple_extent_dims(aspace_id, size, maxsize)<0){
        string msg = "cannot obtain the dim. info for the attribute ";
        string attrnamestr(attr_name.begin(),attr_name.end());
        msg += attrnamestr;
        H5Sclose(aspace_id);
        H5Aclose(attrid);
        throw InternalErr(__FILE__, __LINE__, msg);
    }
      

    // Return ndims and size[ndims]. 
    hsize_t nelmts = 1;
    if (ndims) {
        for (int j = 0; j < ndims; j++)
            nelmts *= size[j];
    }

    
    size_t ty_size = H5Tget_size(ty_id);
    if (ty_size == 0) {
        string msg = "cannot obtain the dtype size for the attribute ";
        string attrnamestr(attr_name.begin(),attr_name.end());
        msg += attrnamestr;
        H5Sclose(aspace_id);
        H5Aclose(attrid);
        throw InternalErr(__FILE__, __LINE__, msg);
    }
     

    size_t need = nelmts * H5Tget_size(ty_id);

    // We want to save memory type in the struct.
    hid_t memtype = H5Tget_native_type(ty_id, H5T_DIR_ASCEND);

    if (memtype < 0){
        string msg = "cannot obtain the memory dtype for the attribute ";
        string attrnamestr(attr_name.begin(),attr_name.end());
        msg += attrnamestr;
        H5Sclose(aspace_id);
        H5Aclose(attrid);
	throw InternalErr(__FILE__, __LINE__, msg);
    }

    // Save the information to the struct
    (*attr_inst_ptr).type = memtype;
    (*attr_inst_ptr).ndims = ndims;
    (*attr_inst_ptr).nelmts = nelmts;
    (*attr_inst_ptr).need = need;
    strncpy((*attr_inst_ptr).name, &attr_name[0], name_size+1);

    for (int j = 0; j < ndims; j++) {
        (*attr_inst_ptr).size[j] = size[j];
    }

    H5Sclose(aspace_id);

    return attrid;
}

///////////////////////////////////////////////////////////////////////////////
/// \fn get_dap_type(hid_t type,bool is_dap4)
/// returns the string representation of HDF5 type.
///
/// This function will get the text representation(string) of the corresponding
/// DODS datatype. DODS-HDF5 subclass method will use this function.
/// Return type is different for DAP2 and DAP4.
///
/// \return string
/// \param type datatype id
/// \param is_dap4 is this for DAP4(for the calls from DMR-related routines)
///////////////////////////////////////////////////////////////////////////////
string get_dap_type(hid_t type, bool is_dap4)
{
    size_t size = 0;
    H5T_sign_t sign;
    BESDEBUG("h5", ">get_dap_type(): type="  << type << endl);
    H5T_class_t class_t = H5Tget_class(type);
    if (H5T_NO_CLASS == class_t)
        throw InternalErr(__FILE__, __LINE__, 
                          "The HDF5 datatype doesn't belong to any Class."); 
    switch (class_t) {

    case H5T_INTEGER:

        size = H5Tget_size(type);
        if (size == 0){
            throw InternalErr(__FILE__, __LINE__,
                              "size of datatype is invalid");
        }

        sign = H5Tget_sign(type);
        if (sign < 0){
            throw InternalErr(__FILE__, __LINE__,
                              "sign of datatype is invalid");
        }

        BESDEBUG("h5", "=get_dap_type(): H5T_INTEGER" <<
            " sign = " << sign <<
            " size = " << size <<
            endl);
        if (size == 1){
            // DAP2 doesn't have signed 8-bit integer, so we need map it to INT16.
            if(true == is_dap4) {
                if (sign == H5T_SGN_NONE)      
                    //return UINT8;    
                    return BYTE;
                else
                    return INT8;
            }
            else {
                if (sign == H5T_SGN_NONE)      
                    return BYTE;    
                else
                    return INT16;
            }
        }

        if (size == 2) {
            if (sign == H5T_SGN_NONE)
                return UINT16;
            else
                return INT16;
        }

        if (size == 4) {
            if (sign == H5T_SGN_NONE)
                return UINT32;
            else
                return INT32;
        }

        if(size == 8) {
            // DAP4 supports 64-bit integer.
            if (true == is_dap4) {
                if (sign == H5T_SGN_NONE)
                    return UINT64;
                else
                    return INT64;
            }
            else
                return INT_ELSE;
        }

        return INT_ELSE;

    case H5T_FLOAT:
        size = H5Tget_size(type);
        if (size == 0){
            throw InternalErr(__FILE__, __LINE__,
                              "size of the datatype is invalid");
        }

        BESDEBUG("h5", "=get_dap_type(): FLOAT size = " << size << endl);
        if (size == 4)
            return FLOAT32;
        if (size == 8)
            return FLOAT64;

        return FLOAT_ELSE;

    case H5T_STRING:
        BESDEBUG("h5", "<get_dap_type(): H5T_STRING" << endl);
        return STRING;

    case H5T_REFERENCE:
        BESDEBUG("h5", "<get_dap_type(): H5T_REFERENCE" << endl);
        return URL;
    // Note: Currently DAP2 and DAP4 only support defined atomic types.
    // So the H5T_COMPOUND and H5_ARRAY cases should never be reached for attribute handling. 
    // However, this function may be used for variable handling.
    case H5T_COMPOUND:
        BESDEBUG("h5", "<get_dap_type(): COMPOUND" << endl);
        return COMPOUND;

    case H5T_ARRAY:
        return ARRAY;

    default:
        BESDEBUG("h5", "<get_dap_type(): Unmappable Type" << endl);
        return "Unmappable Type";
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \fn get_fileid(const char *filename)
/// gets HDF5 file id.
/// 
/// This function is used because H5Fopen cannot be directly used in a C++
/// code.
/// \param filename HDF5 filename
/// \return a file handler id
///////////////////////////////////////////////////////////////////////////////
hid_t get_fileid(const char *filename)
{
    hid_t fileid = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
    if (fileid < 0){
        string msg = "cannot open the HDF5 file  ";
        string filenamestr(filename);
        msg += filenamestr;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    return fileid;
}

///////////////////////////////////////////////////////////////////////////////
/// \fn close_fileid(hid_t fid)
/// closes HDF5 file reffered by \a fid.
/// 
/// This function closes the HDF5 file.
///
/// \param fid HDF5 file id
/// \return throws an error if it can't close the file.
///////////////////////////////////////////////////////////////////////////////
void close_fileid(hid_t fid)
{
    if (H5Fclose(fid) < 0)
        throw Error(unknown_error,
                    string("Could not close the HDF5 file."));

}

///////////////////////////////////////////////////////////////////////////////
/// \fn get_dataset(hid_t pid, const string &dname, DS_t * dt_inst_ptr)
/// obtain data information in a dataset datatype, dataspace(dimension sizes)
/// and number of dimensions and put these information into a pointer of data
/// struct.
///
/// \param[in] pid    parent object id(group id)
/// \param[in] dname  dataset name
/// \param[out] dt_inst_ptr  pointer to the attribute struct(* attr_inst_ptr)
///////////////////////////////////////////////////////////////////////////////
void get_dataset(hid_t pid, const string &dname, DS_t * dt_inst_ptr,bool use_dimscale)
{

    BESDEBUG("h5", ">get_dataset()" << endl);

    // Obtain the dataset ID
    hid_t dset = -1;
    if ((dset = H5Dopen(pid, dname.c_str(),H5P_DEFAULT)) < 0) {
        string msg = "cannot open the HDF5 dataset  ";
        msg += dname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    // Obtain the datatype ID
    hid_t dtype = -1;
    if ((dtype = H5Dget_type(dset)) < 0) {
        H5Dclose(dset);
        string msg = "cannot get the the datatype of HDF5 dataset  ";
        msg += dname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    // Obtain the datatype class 
    H5T_class_t ty_class = H5Tget_class(dtype);
    if (ty_class < 0) {
        H5Tclose(dtype);
        H5Dclose(dset);
        string msg = "cannot get the datatype class of HDF5 dataset  ";
        msg += dname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    // These datatype classes are unsupported. Note we do support
    // variable length string and the variable length string class is
    // H5T_STRING rather than H5T_VLEN.
    if ((ty_class == H5T_TIME) || (ty_class == H5T_BITFIELD)
        || (ty_class == H5T_OPAQUE) || (ty_class == H5T_ENUM) || (ty_class == H5T_VLEN)) {
        string msg = "unexpected datatype of HDF5 dataset  ";
        msg += dname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }
   
    hid_t dspace = -1;
    if ((dspace = H5Dget_space(dset)) < 0) {
        H5Tclose(dtype);
        H5Dclose(dset);
        string msg = "cannot get the the dataspace of HDF5 dataset  ";
        msg += dname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    // It is better to use the dynamic allocation of the array.
    // However, since the DODS_MAX_RANK is not big and it is also
    // used in other location, we still keep the original code.
    // KY 2011-11-17

    int ndims = H5Sget_simple_extent_ndims(dspace);
    if (ndims < 0) {
        H5Tclose(dtype);
        H5Sclose(dspace);
        H5Dclose(dset);
        string msg = "cannot get hdf5 dataspace number of dimension for dataset ";
        msg += dname;
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    // Check if the dimension size exceeds the maximum number of dimension DAP supports
    if (ndims > DODS_MAX_RANK) {
        string msg = "number of dimensions exceeds allowed for dataset ";
        msg += dname;
        H5Tclose(dtype);
        H5Sclose(dspace);
        H5Dclose(dset);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    hsize_t size[DODS_MAX_RANK];
    hsize_t maxsize[DODS_MAX_RANK];

    // DAP applications don't care about the unlimited dimensions 
    // since the applications only care about retrieving the data.
    // So we don't check the maxsize to see if it is the unlimited dimension 
    // attribute.
    if (H5Sget_simple_extent_dims(dspace, size, maxsize)<0){
        string msg = "cannot obtain the dim. info for the dataset ";
        msg += dname;
        H5Tclose(dtype);
        H5Sclose(dspace);
        H5Dclose(dset);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    // return ndims and size[ndims]. 
    hsize_t nelmts = 1;
    if (ndims !=0) {
        for (int j = 0; j < ndims; j++)
            nelmts *= size[j];
    }

    size_t dtype_size = H5Tget_size(dtype);
    if (dtype_size == 0) {
        string msg = "cannot obtain the data type size for the dataset ";
        msg += dname;
        H5Tclose(dtype);
        H5Sclose(dspace);
        H5Dclose(dset);
        throw InternalErr(__FILE__, __LINE__, msg);
    }
 
    size_t need = nelmts * dtype_size;

    hid_t memtype = H5Tget_native_type(dtype, H5T_DIR_ASCEND);
    if (memtype < 0){
        string msg = "cannot obtain the memory data type for the dataset ";
        msg += dname;
        H5Tclose(dtype);
        H5Sclose(dspace);
        H5Dclose(dset);
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    //(*dt_inst_ptr).dset = dset;
    //(*dt_inst_ptr).dataspace = dspace;
    (*dt_inst_ptr).type = memtype;
    (*dt_inst_ptr).ndims = ndims;
    (*dt_inst_ptr).nelmts = nelmts;
    (*dt_inst_ptr).need = need;
    strncpy((*dt_inst_ptr).name, dname.c_str(), dname.length());
    (*dt_inst_ptr).name[dname.length()] = '\0';
    for (int j = 0; j < ndims; j++) 
        (*dt_inst_ptr).size[j] = size[j];

    if(true == use_dimscale) {
        bool is_dimscale = false;

        if(1 == ndims) {

            int count = 0;
            herr_t ret = H5Aiterate2(dset, H5_INDEX_NAME, H5_ITER_INC, NULL, attr_info, &count);
            if(ret < 0) {
                string msg = "cannot interate the attributes of the dataset ";
                msg += dname;
                H5Tclose(dtype);
                H5Sclose(dspace);
                H5Dclose(dset);
                throw InternalErr(__FILE__, __LINE__, msg);
            }

            // Find the dimension scale.
            if (2==count) 
                is_dimscale =true;

        }
 
         if(true == is_dimscale) {// Check Later
//cerr<<"dname is "<<dname <<endl;

            // Only the dimension name.
            (*dt_inst_ptr).dimnames.push_back(dname.substr(dname.find_last_of("/")+1));
            //(*dt_inst_ptr).dimnames.push_back(dname);
         }
         else  
            obtain_dimnames(dset,ndims,dt_inst_ptr);
    }

    H5Tclose(dtype);
    H5Sclose(dspace);
    H5Dclose(dset);
 
    BESDEBUG("h5", "<get_dataset() dimension=" << ndims << " elements=" <<
        nelmts << endl);
}


#if 0
///////////////////////////////////////////////////////////////////////////////
/// \fn get_data(hid_t dset, void *buf)
/// will get all data of a \a dset dataset and put it into \a buf.
/// Note: this routine is only used to access HDF5 integer,float and fixed-size string.
//  variable length string is handled by function read_vlen_string.
///
/// \param[in] dset dataset id(dset)
/// \param[out] buf pointer to a buffer
///////////////////////////////////////////////////////////////////////////////
void get_data(hid_t dset, void *buf)
{
    BESDEBUG("h5", ">get_data()" << endl);

    hid_t dtype = -1;
    if ((dtype = H5Dget_type(dset)) < 0) {
        throw InternalErr(__FILE__, __LINE__, "Failed to get the datatype of the dataset");
    }
    hid_t dspace = -1;
    if ((dspace = H5Dget_space(dset)) < 0) {
        H5Tclose(dtype);
        throw InternalErr(__FILE__, __LINE__, "Failed to get the data space of the dataset");
    }
    //  Use HDF5 H5Tget_native_type API
    hid_t memtype = H5Tget_native_type(dtype, H5T_DIR_ASCEND);
    if (memtype < 0) {
        H5Tclose(dtype);
        H5Sclose(dspace);
        throw InternalErr(__FILE__, __LINE__, "failed to get memory type");
    }

    if (H5Dread(dset, memtype, dspace, dspace, H5P_DEFAULT, buf)
            < 0) {
        H5Tclose(dtype);
        H5Tclose(memtype);
        H5Sclose(dspace);
        throw InternalErr(__FILE__, __LINE__, "failed to read data");
    }

    if (H5Tclose(dtype) < 0){
        H5Tclose(memtype);
        H5Sclose(dspace);
	throw InternalErr(__FILE__, __LINE__, "Unable to release the dtype.");
    }

    if (H5Tclose(memtype) < 0){
        H5Sclose(dspace);
        throw InternalErr(__FILE__, __LINE__, "Unable to release the memtype.");
    }

    if(H5Sclose(dspace)<0) {
        throw InternalErr(__FILE__, __LINE__, "Unable to release the data space.");
    }
#if 0
        // Supposed to release the resource at the release at the HDF5Array destructor.
        //if (H5Dclose(dset) < 0){
	 //  throw InternalErr(__FILE__, __LINE__, "Unable to close the dataset.");
	//}
    }
#endif

    BESDEBUG("h5", "<get_data()" << endl);
}

///////////////////////////////////////////////////////////////////////////////
/// \fn get_strdata(int strindex, char *allbuf, char *buf, int elesize)
/// will get an individual string data from all string data elements and put
/// it into buf. 
///
/// \param[in] strindex index of H5T_STRING array
/// \param[in] allbuf pointer to string buffer that has been built so far
/// \param[in] elesize size of string element in the array
/// \param[out] buf pointer to a buf
/// \return void
///////////////////////////////////////////////////////////////////////////////
void get_strdata(int strindex, char *allbuf, char *buf, int elesize)
{
    char *tempvalue = allbuf;   // The beginning of entire buffer.

    BESDEBUG("h5", ">get_strdata(): "
        << " strindex=" << strindex << " allbuf=" << allbuf << endl);

    // Tokenize the convbuf. 
    for (int i = 0; i < strindex; i++) {
        tempvalue = tempvalue + elesize;
    }

    strncpy(buf, tempvalue, elesize);
    buf[elesize] = '\0';        
}

///////////////////////////////////////////////////////////////////////////////
/// \fn get_slabdata(hid_t dset, int *offset, int *step, int *count, 
///                  int num_dim, void *buf)
/// will get hyperslab data of a dataset and put it into buf.
///
/// \param[in] dset dataset id
/// \param[in] offset starting point
/// \param[in] step  stride
/// \param[in] count  count
/// \param[in] num_dim  number of array dimensions
/// \param[out] buf pointer to a buffer
///////////////////////////////////////////////////////////////////////////////
int
get_slabdata(hid_t dset, int *offset, int *step, int *count, int num_dim,
             void *buf)
{
    BESDEBUG("h5", ">get_slabdata() " << endl);

    hid_t dtype = H5Dget_type(dset);
    if (dtype < 0) {
        throw InternalErr(__FILE__, __LINE__, "could not get data type");
    }
    // Using H5T_get_native_type API
    hid_t memtype = H5Tget_native_type(dtype, H5T_DIR_ASCEND);
    if (memtype < 0) {
        H5Tclose(dtype);
        throw InternalErr(__FILE__, __LINE__, "could not get memory type");
    }

    hid_t dspace = H5Dget_space(dset);
    if (dspace < 0) {
        H5Tclose(dtype);
        H5Tclose(memtype);
        throw InternalErr(__FILE__, __LINE__, "could not get data space");
    }


    vector<hsize_t>dyn_count;
    vector<hsize_t>dyn_step;
    vector<hssize_t>dyn_offset;
    dyn_count.resize(num_dim);
    dyn_step.resize(num_dim);
    dyn_offset.resize(num_dim);

    for (int i = 0; i < num_dim; i++) {
        dyn_count[i] = (hsize_t) (*count);
        dyn_step[i] = (hsize_t) (*step);
        dyn_offset[i] = (hssize_t) (*offset);
        BESDEBUG("h5",
                "count:" << dyn_count[i]
                << " step:" << dyn_step[i]
                << " offset:" << dyn_step[i]
                << endl);
        count++;
        step++;
        offset++;
    }

    if (H5Sselect_hyperslab(dspace, H5S_SELECT_SET, 
                           (const hsize_t *)&dyn_offset[0], &dyn_step[0],
                            &dyn_count[0], NULL) < 0) {
        H5Tclose(dtype);
        H5Tclose(memtype);
        H5Sclose(dspace);
        throw InternalErr(__FILE__, __LINE__, "could not select hyperslab");
    }

    hid_t memspace = H5Screate_simple(num_dim, &dyn_count[0], NULL);
    if (memspace < 0) {
        H5Tclose(dtype);
        H5Tclose(memtype);
        H5Sclose(dspace);
        throw InternalErr(__FILE__, __LINE__, "could not open space");
    }

    if (H5Dread(dset, memtype, memspace, dspace, H5P_DEFAULT,
                (void *) buf) < 0) {
        H5Tclose(dtype);
        H5Tclose(memtype);
        H5Sclose(dspace);
        H5Sclose(memspace);
        throw InternalErr(__FILE__, __LINE__, "could not get data");
    }

    if (H5Sclose(dspace) < 0){
        H5Tclose(dtype);
        H5Tclose(memtype);
        H5Sclose(memspace);
	throw InternalErr(__FILE__, __LINE__, "Unable to close the dspace.");
    }
    if (H5Sclose(memspace) < 0){
        H5Tclose(dtype);
        H5Tclose(memtype);
        throw InternalErr(__FILE__, __LINE__, "Unable to close the memspace.");
    }
    if (H5Tclose(dtype) < 0){
        H5Tclose(memtype);
        throw InternalErr(__FILE__, __LINE__, "Unable to close the dtype.");
    }

    if (H5Tclose(memtype) < 0){
            throw InternalErr(__FILE__, __LINE__, "Unable to close the memtype.");
    }

    BESDEBUG("h5", "<get_slabdata() " << endl);
    return 0;
}

#endif

///////////////////////////////////////////////////////////////////////////////
/// \fn check_h5str(hid_t h5type)
/// checks if type is HDF5 string type
/// 
/// \param h5type data type id
/// \return true if type is string
/// \return false otherwise
///////////////////////////////////////////////////////////////////////////////
bool check_h5str(hid_t h5type)
{
    if (H5Tget_class(h5type) == H5T_STRING)
        return true;
    else
        return false;
}


#if 0
bool read_vlen_string(hid_t dsetid, int nelms, hsize_t *hoffset, hsize_t *hstep, hsize_t *hcount,vector<string> &finstrval)
{

    hid_t dspace = -1;
    hid_t mspace = -1;
    hid_t dtypeid = -1;
    hid_t memtype = -1;
    bool is_scalar = false;


    if ((dspace = H5Dget_space(dsetid))<0) {
        throw InternalErr (__FILE__, __LINE__, "Cannot obtain data space.");
    }

    if(H5S_SCALAR == H5Sget_simple_extent_type(dspace))
        is_scalar = true;


    if (false == is_scalar) {
        if (H5Sselect_hyperslab(dspace, H5S_SELECT_SET,
                               hoffset, hstep,
                               hcount, NULL) < 0) {
            H5Sclose(dspace);
            throw InternalErr (__FILE__, __LINE__, "Cannot generate the hyperslab of the HDF5 dataset.");
        }

        int d_num_dim = H5Sget_simple_extent_ndims(dspace);
        if(d_num_dim < 0) {
            H5Sclose(dspace);
            throw InternalErr (__FILE__, __LINE__, "Cannot obtain the number of dimensions of the data space.");
        }

        mspace = H5Screate_simple(d_num_dim, hcount,NULL);
        if (mspace < 0) {
            H5Sclose(dspace);
            throw InternalErr (__FILE__, __LINE__, "Cannot create the memory space.");
        }
    }


    if ((dtypeid = H5Dget_type(dsetid)) < 0) {
            
        if (false == is_scalar) 
            H5Sclose(mspace);
        H5Sclose(dspace);
        throw InternalErr (__FILE__, __LINE__, "Cannot obtain the datatype.");

    }

    if ((memtype = H5Tget_native_type(dtypeid, H5T_DIR_ASCEND))<0) {

        if (false == is_scalar) 
            H5Sclose(mspace);
        H5Tclose(dtypeid);
        H5Sclose(dspace);
        throw InternalErr (__FILE__, __LINE__, "Fail to obtain memory datatype.");

    }

    size_t ty_size = H5Tget_size(memtype);
    if (ty_size == 0) {
        if (false == is_scalar) 
            H5Sclose(mspace);
        H5Tclose(memtype);
        H5Tclose(dtypeid);
        H5Sclose(dspace);
        throw InternalErr (__FILE__, __LINE__,"Fail to obtain the size of HDF5 string.");
    }

    vector <char> strval;
    strval.resize(nelms*ty_size);
    hid_t read_ret = -1;
    if (true == is_scalar) 
        read_ret = H5Dread(dsetid,memtype,H5S_ALL,H5S_ALL,H5P_DEFAULT,(void*)&strval[0]);
    else 
        read_ret = H5Dread(dsetid,memtype,mspace,dspace,H5P_DEFAULT,(void*)&strval[0]);

    if (read_ret < 0) {
        if (false == is_scalar)  
            H5Sclose(mspace);
        H5Tclose(memtype);
        H5Tclose(dtypeid);
        H5Sclose(dspace);
        throw InternalErr (__FILE__, __LINE__, "Fail to read the HDF5 variable length string dataset.");
    }

    // For scalar, nelms is 1.
    char*temp_bp = &strval[0];
    char*onestring = NULL;
    for (int i =0;i<nelms;i++) {
        onestring = *(char**)temp_bp;
        if(onestring!=NULL ) 
            finstrval[i] =string(onestring);
        else // We will add a NULL if onestring is NULL.
            finstrval[i]="";
        temp_bp +=ty_size;
    }

    if (false == strval.empty()) {
        herr_t ret_vlen_claim;
        if (true == is_scalar) 
            ret_vlen_claim = H5Dvlen_reclaim(memtype,dspace,H5P_DEFAULT,(void*)&strval[0]);
        else 
            ret_vlen_claim = H5Dvlen_reclaim(memtype,mspace,H5P_DEFAULT,(void*)&strval[0]);
        if (ret_vlen_claim < 0){
            if (false == is_scalar) 
                H5Sclose(mspace);
            H5Tclose(memtype);
            H5Tclose(dtypeid);
            H5Sclose(dspace);
            throw InternalErr (__FILE__, __LINE__, "Cannot reclaim the memory buffer of the HDF5 variable length string.");
 
        }
    }

    if (false == is_scalar) 
        H5Sclose(mspace);
    H5Tclose(memtype);
    H5Tclose(dtypeid);
    H5Sclose(dspace);
 
    return true;

}

bool promote_char_to_short(H5T_class_t type_cls, hid_t type_id) {

    bool ret_value = false;
    if(type_cls == H5T_INTEGER) {
        size_t size = H5Tget_size(type_id);
        int sign = H5Tget_sign(type_id);
        if(size == 1 && sign == H5T_SGN_2)
            ret_value = true;
    }

    return ret_value;

}

void get_vlen_str_data(char*temp_bp,string &finalstr_val) {

    char*onestring = NULL;
    onestring = *(char**)temp_bp;
    if(onestring!=NULL )
        finalstr_val =string(onestring);
    else // We will add a NULL is onestring is NULL.
        finalstr_val="";

}
#endif
///////////////////////////////////////////////////////////////////////////////
/// \fn print_attr(hid_t type, int loc, void *sm_buf)
/// will get the printed representation of an attribute.
///
///
/// \param type  HDF5 data type id
/// \param loc    the number of array number
/// \param sm_buf pointer to an attribute
/// \return a string
/// \todo Due to the priority of the handler work, this function will not be 
/// \todo re-written in this re-engineering process. KY 2011-Nov. 14th
///////////////////////////////////////////////////////////////////////////////
string print_attr(hid_t type, int loc, void *sm_buf) {
//static char *print_attr(hid_t type, int loc, void *sm_buf) {
    union {
        unsigned char* ucp;
        char *tcp;
        short *tsp;
        unsigned short *tusp;
        int *tip;
        unsigned int*tuip;
        long *tlp;
        unsigned long*tulp;
        float *tfp;
        double *tdp;
    } gp;

    vector<char> rep;

    switch (H5Tget_class(type)) {

        case H5T_INTEGER: {

            size_t size = H5Tget_size(type);
            if (size == 0){
                throw InternalErr(__FILE__, __LINE__,
                                  "size of datatype is invalid");
            }

            H5T_sign_t sign = H5Tget_sign(type);
            if (sign < 0){
                throw InternalErr(__FILE__, __LINE__,
                                  "sign of datatype is invalid");
            }

            BESDEBUG("h5", "=get_dap_type(): H5T_INTEGER" <<
                        " sign = " << sign <<
                        " size = " << size <<
                        endl);

            // change void pointer into the corresponding integer datatype.
            // 32 should be long enough to hold one integer and one
            // floating point number.
            //rep = new char[32];
            //memset(rep, 0, 32);
            rep.resize(32);

            if (size == 1){
 
                if(sign == H5T_SGN_NONE) {
                    gp.ucp = (unsigned char *) sm_buf;
                    unsigned char tuchar = *(gp.ucp + loc);
                    snprintf(&rep[0], 32, "%u", tuchar);
                }

                else {
                    gp.tcp = (char *) sm_buf;
                    snprintf(&rep[0], 32, "%d", *(gp.tcp + loc));
                }
            }

            else if (size == 2) {

                if(sign == H5T_SGN_NONE) {
                    gp.tusp = (unsigned short *) sm_buf;
                    snprintf(&rep[0], 32, "%hu", *(gp.tusp + loc));
 
                }
                else {
                    gp.tsp = (short *) sm_buf;
                    snprintf(&rep[0], 32, "%hd", *(gp.tsp + loc));
 
                }
            }

            else if (size == 4) {

                if(sign == H5T_SGN_NONE) {
                    gp.tuip = (unsigned int *) sm_buf;
                    snprintf(&rep[0], 32, "%u", *(gp.tuip + loc));
 
                }
                else {
                    gp.tip = (int *) sm_buf;
                    snprintf(&rep[0], 32, "%d", *(gp.tip + loc));
                }
            }
            else if (size == 8) {

                if(sign == H5T_SGN_NONE) {
                    gp.tulp = (unsigned long *) sm_buf;
                    snprintf(&rep[0], 32, "%lu", *(gp.tulp + loc));
                }
                else {
                    gp.tlp = (long *) sm_buf;
                    snprintf(&rep[0], 32, "%ld", *(gp.tlp + loc));
                }
            }
            else 
                throw InternalErr(__FILE__, __LINE__,"Unsupported integer type, check the size of datatype.");

            break;
        }

        case H5T_FLOAT: {
            rep.resize(32);
            char gps[30];

            if (H5Tget_size(type) == 4) {
                
                float attr_val = *(float*)sm_buf;
                bool is_a_fin = isfinite(attr_val);
                // Represent the float number.
                // Some space may be wasted. But it is okay.
                gp.tfp = (float *) sm_buf;
                snprintf(gps, 30, "%.10g", *(gp.tfp + loc));
                int ll = strlen(gps);

                // Add the dot to assure this is a floating number
                if (!strchr(gps, '.') && !strchr(gps, 'e') && !strchr(gps,'E')) {
                    if(true == is_a_fin)
                        gps[ll++] = '.';
                }

                gps[ll] = '\0';
                snprintf(&rep[0], 32, "%s", gps);
            } 
            else if (H5Tget_size(type) == 8) {

                double attr_val = *(double*)sm_buf;
                bool is_a_fin = isfinite(attr_val);
                gp.tdp = (double *) sm_buf;
                snprintf(gps, 30, "%.17g", *(gp.tdp + loc));
                int ll = strlen(gps);
                if (!strchr(gps, '.') && !strchr(gps, 'e')&& !strchr(gps,'E')) {
                    if(true == is_a_fin)
                        gps[ll++] = '.';
                }
                gps[ll] = '\0';
                snprintf(&rep[0], 32, "%s", gps);
            } 
            else if (H5Tget_size(type) == 0){
		throw InternalErr(__FILE__, __LINE__, "H5Tget_size() failed.");
	    }
            break;
        }

        case H5T_STRING: {
            int str_size = H5Tget_size(type);
            if(H5Tis_variable_str(type) == true) {
		throw InternalErr(__FILE__, __LINE__, 
                      "print_attr function doesn't handle variable length string, variable length string should be handled separately.");
            }
	    if (str_size == 0){
		throw InternalErr(__FILE__, __LINE__, "H5Tget_size() failed.");
	    }
            BESDEBUG("h5", "=print_attr(): H5T_STRING sm_buf=" << (char *) sm_buf
                << " size=" << str_size << endl);
            char *buf = NULL;
            // This try/catch block is here to protect the allocation of buf.
            try {
                buf = new char[str_size + 1];
                strncpy(buf, (char *) sm_buf, str_size);
                buf[str_size] = '\0';
                //rep = new char[str_size + 3];
                // Not necessarily allocate 3 more bytes. 
                rep.resize(str_size+3);
                snprintf(&rep[0], str_size + 3, "%s", buf);
                rep[str_size + 2] = '\0';
                delete[] buf; buf = 0;
            }
            catch (...) {
                if( buf ) delete[] buf;
                throw;
            }
            break;
        }

        default:
	    break;
    } // switch(H5Tget_class(type))

    string rep_str(rep.begin(),rep.end());
    return rep_str;
}

D4AttributeType daptype_strrep_to_dap4_attrtype(std::string s){
    
    if (s == "Byte")
        return attr_byte_c;
    else if (s == "Int8")
        return attr_int8_c;
    else if (s == "UInt8") // This may never be used.
        return attr_uint8_c;
    else if (s == "Int16")
        return attr_int16_c;
    else if (s == "UInt16")
        return attr_uint16_c;
    else if (s == "Int32")
        return attr_int32_c;
    else if (s == "UInt32")
        return attr_uint32_c;
    else if (s == "Int64")
        return attr_int64_c;
    else if (s == "UInt64")
        return attr_uint64_c;
    else if (s == "Float32")
        return attr_float32_c;
    else if (s == "Float64")
        return attr_float64_c;
    else if (s == "String")
        return attr_str_c;
    else if (s == "Url")
        return attr_url_c;
    //else if (s == "otherxml")
    //    return attr_otherxml_c;
    else
        return attr_null_c;


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
//static BaseType *Get_bt(const string &vname,
BaseType *Get_bt(const string &vname,
                 const string &vpath,
                 const string &dataset,
                 hid_t datatype, bool is_dap4)
{
    BaseType *btp = NULL;

    try {

        BESDEBUG("h5", ">Get_bt varname=" << vname << " datatype=" << datatype
            << endl);

        size_t size = 0;
        H5T_sign_t sign    = H5T_SGN_ERROR;
        switch (H5Tget_class(datatype)) {

        case H5T_INTEGER:
        {
            size = H5Tget_size(datatype);
            sign = H5Tget_sign(datatype);
            BESDEBUG("h5", "=Get_bt() H5T_INTEGER size = " << size << " sign = "
                << sign << endl);

	    if (sign == H5T_SGN_ERROR) {
                throw InternalErr(__FILE__, __LINE__, "cannot retrieve the sign type of the integer");
            }
            if (size == 0) {
		throw InternalErr(__FILE__, __LINE__, "cannot return the size of the datatype");
	    }
	    else if (size == 1) { // Either signed char or unsigned char
                // DAP2 doesn't support signed char, it maps to DAP int16.
                if (sign == H5T_SGN_2) {
                    if (false == is_dap4) // signed char to DAP2 int16
                        btp = new HDF5Int16(vname, vpath, dataset);
                    else
                        btp = new HDF5Int8(vname,vpath,dataset);
                }
                else
                    btp = new HDF5Byte(vname, vpath,dataset);
            }
            else if (size == 2) {
                if (sign == H5T_SGN_2)
                    btp = new HDF5Int16(vname, vpath,dataset);
                else
                    btp = new HDF5UInt16(vname,vpath, dataset);
            }
            else if (size == 4) {
                if (sign == H5T_SGN_2){
                    btp = new HDF5Int32(vname, vpath,dataset);
                }
                else
                    btp = new HDF5UInt32(vname,vpath, dataset);
            }
            else if (size == 8) {
                if(true == is_dap4) {
                   if(sign == H5T_SGN_2) 
                      btp = new HDF5Int64(vname,vpath, dataset);
                   else
                      btp = new HDF5UInt64(vname,vpath, dataset);
                }
                else {
                    throw
                    InternalErr(__FILE__, __LINE__,
                                string("Unsupported HDF5 64-bit Integer type:")
                                + vname);
                }
            }
        }
            break;

        case H5T_FLOAT:
        {
            size = H5Tget_size(datatype);
            BESDEBUG("h5", "=Get_bt() H5T_FLOAT size = " << size << endl);

	    if (size == 0) {
                throw InternalErr(__FILE__, __LINE__, "cannot return the size of the datatype");
            }
            else if (size == 4) {
                btp = new HDF5Float32(vname,vpath, dataset);
            }
            else if (size == 8) {
                btp = new HDF5Float64(vname,vpath, dataset);
            }
        }
            break;

        case H5T_STRING:
            btp = new HDF5Str(vname, vpath,dataset);
            break;

        // The array datatype is rarely,rarely used. So this
        // part of code is not reviewed.
 #if 0
        case H5T_ARRAY: {
            BaseType *ar_bt = 0;
            try {
                BESDEBUG("h5",
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
                BESDEBUG(cerr
                    << "=Get_bt()" << " Dim = " << ndim
                    << " Size = " << size
                    << endl);

                hsize_t size2[DODS_MAX_RANK];
                if(H5Tget_array_dims(datatype, size2) < 0){
                    throw
                        InternalErr(__FILE__, __LINE__,
                                    string("Could not get array dims for: ")
                                      + vname);
                }


                HDF5Array &h5_ar = static_cast < HDF5Array & >(*btp);
                for (int dim_index = 0; dim_index < ndim; dim_index++) {
                    h5_ar.append_dim(size2[dim_index]);
                    BESDEBUG("h5", "=Get_bt() " << size2[dim_index] << endl);
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
#endif

        // Reference map to DAP URL, check the technical note.
        case H5T_REFERENCE:
            btp = new HDF5Url(vname, vpath,dataset);
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
                                                  
    BESDEBUG("h5", "<Get_bt()" << endl);
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
//static Structure *Get_structure(const string &varname,
Structure *Get_structure(const string &varname,const string &vpath,
                                const string &dataset,
                                hid_t datatype,bool is_dap4)
{
    HDF5Structure *structure_ptr = NULL;
    char* memb_name = NULL;
    hid_t memb_type = -1;

    BESDEBUG("h5", ">Get_structure()" << datatype << endl);

    if (H5Tget_class(datatype) != H5T_COMPOUND)
        throw InternalErr(__FILE__, __LINE__,
                          string("Compound-to-structure mapping error for ")
                          + varname);

    try {
        structure_ptr = new HDF5Structure(varname, vpath, dataset);
        //structure_ptr->set_did(dt_inst.dset);
        //structure_ptr->set_tid(dt_inst.type);

        // Retrieve member types
        int nmembs = H5Tget_nmembers(datatype);
        BESDEBUG("h5", "=Get_structure() has " << nmembs << endl);
	if (nmembs < 0){
	   throw InternalErr(__FILE__, __LINE__, "cannot retrieve the number of elements");
	}
        for (int i = 0; i < nmembs; i++) {
            memb_name = H5Tget_member_name(datatype, i);
            H5T_class_t memb_cls = H5Tget_member_class(datatype, i);
            memb_type = H5Tget_member_type(datatype, i);
	    if (memb_name == NULL){
		throw InternalErr(__FILE__, __LINE__, "cannot retrieve the name of the member");
	    }
            if ((memb_cls < 0) | (memb_type < 0)) {
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
                Structure *s = Get_structure(memb_name, memb_name, dataset, memb_type,is_dap4);
                structure_ptr->add_var(s);
                delete s; s = 0;
            } 
            else if(memb_cls == H5T_ARRAY) {

                BaseType *ar_bt = 0;
                BaseType *btp   = 0;
                Structure *s    = 0;
                hid_t     dtype_base = 0;

                try {

                    // Get the base memb_type of the array
                    dtype_base = H5Tget_super(memb_type);

                    // Set the size of the array.
                    int ndim = H5Tget_array_ndims(memb_type);
                    size_t size = H5Tget_size(memb_type);
                    int nelement = 1;

		    if (dtype_base < 0) {
                        throw InternalErr(__FILE__, __LINE__, "cannot return the base memb_type");
 	            }
		    if (ndim < 0) {
                        throw InternalErr(__FILE__, __LINE__, "cannot return the rank of the array memb_type");
                    }
		    if (size == 0) {
                        throw InternalErr(__FILE__, __LINE__, "cannot return the size of the memb_type");
                    }

                    hsize_t size2[DODS_MAX_RANK];
                    if(H5Tget_array_dims(memb_type, size2) < 0){
                        throw
                        InternalErr(__FILE__, __LINE__,
                                    string("Could not get array dims for: ")
                                      + string(memb_name));
                    }

                    H5T_class_t array_memb_cls = H5Tget_class(dtype_base);
                    if(array_memb_cls == H5T_NO_CLASS) {
                        throw InternalErr(__FILE__, __LINE__,
                                  string("cannot get the correct class for compound type member")
                                  + string(memb_name));
                    }
                    if(H5T_COMPOUND == array_memb_cls) {

                        s = Get_structure(memb_name, memb_name,dataset, dtype_base,is_dap4);
                        HDF5Array *h5_ar = new HDF5Array(memb_name, dataset, s);
                    
                        for (int dim_index = 0; dim_index < ndim; dim_index++) {
                            h5_ar->append_dim(size2[dim_index]);
                            nelement = nelement * size2[dim_index];
                        }

                        // May delete them later since all these can be obtained from the file ID.
                        //h5_ar->set_did(dt_inst.dset);
                        // Assign the array memb_type id.
                        //h5_ar->set_tid(memb_type);
                        h5_ar->set_memneed(size);
                        h5_ar->set_numdim(ndim);
                        h5_ar->set_numelm(nelement);
                        h5_ar->set_length(nelement);

	                structure_ptr->add_var(h5_ar);
                        delete h5_ar;
	
                    }
                    else if (H5T_INTEGER == array_memb_cls || H5T_FLOAT == array_memb_cls || H5T_STRING == array_memb_cls) { 
                        ar_bt = Get_bt(memb_name, memb_name,dataset, dtype_base,is_dap4);
                        HDF5Array *h5_ar = new HDF5Array(memb_name,dataset,ar_bt);
                        //btp = new HDF5Array(memb_name, dataset, ar_bt);
                        //HDF5Array &h5_ar = static_cast < HDF5Array & >(*btp);
                    
                        for (int dim_index = 0; dim_index < ndim; dim_index++) {
                            h5_ar->append_dim(size2[dim_index]);
                            nelement = nelement * size2[dim_index];
                        }

                        // May delete them later
                        //h5_ar->set_did(dt_inst.dset);
                        // Assign the array memb_type id.
                        //h5_ar->set_tid(memb_type);
                        h5_ar->set_memneed(size);
                        h5_ar->set_numdim(ndim);
                        h5_ar->set_numelm(nelement);
                        h5_ar->set_length(nelement);

	                structure_ptr->add_var(h5_ar);
                        delete h5_ar;
                    }
                    if( ar_bt ) delete ar_bt;
                    if( btp ) delete btp;
                    if(s) delete s;
                    H5Tclose(dtype_base);

                }
                catch (...) {
                    if( ar_bt ) delete ar_bt;
                    if( btp ) delete btp;
                    if(s) delete s;
                    H5Tclose(dtype_base);
                    throw;
                }

            }
            else if (memb_cls == H5T_INTEGER || memb_cls == H5T_FLOAT || memb_cls == H5T_STRING)  {
                BaseType *bt = Get_bt(memb_name, memb_name,dataset, memb_type,is_dap4);
                structure_ptr->add_var(bt);
                delete bt; bt = 0;
            }
            else {
                free(memb_name);
		throw InternalErr(__FILE__, __LINE__, "unsupported field datatype inside a compound datatype");
            }
            // Caller needs to free the memory allocated by the library for memb_name.
            free(memb_name);
        }
    }
    catch (...) {
        // if memory allocation exception thrown it will be caught
        // here, so should check if structure ptr exists before
        // deleting it. pwest Mar 18, 2009
        if( structure_ptr ) delete structure_ptr;
        if(memb_name!= NULL) 
            free(memb_name);
        if(memb_type != -1)
            H5Tclose(memb_type);
        throw;
    }

    BESDEBUG("h5", "<Get_structure()" << endl);

    return structure_ptr;
}

#if 0
// H5Ovisit call back function. When finding the dimension scale attributes, return 1. 
static int
visit_obj_cb(hid_t o_id, const char *name, const H5O_info_t *oinfo,
    void *_op_data);

// H5Aiterate2 call back function, check if having the dimension scale attributes.
static herr_t attr_info(hid_t loc_id, const char *name, const H5A_info_t *ainfo, void *opdata);
#endif

// Function to use H5Ovisit to check if dimension scale attributes exist.
bool check_dimscale(hid_t fileid) {

    bool ret_value = false;
    herr_t ret_o= H5Ovisit(fileid, H5_INDEX_NAME, H5_ITER_INC, visit_obj_cb, NULL);
    if(ret_o < 0)
        throw InternalErr(__FILE__, __LINE__, "H5Ovisit fails");
    else 
       ret_value =(ret_o >0)?true:false;

    return ret_value;
}

static int
visit_obj_cb(hid_t  group_id, const char *name, const H5O_info_t *oinfo,
    void *_op_data)
{

    if(oinfo->type == H5O_TYPE_DATASET) {

        hid_t dataset = -1;
        dataset = H5Dopen2(group_id,name,H5P_DEFAULT);
        if(dataset <0) 
            throw InternalErr(__FILE__, __LINE__, "H5Dopen2 fails in the H5Ovisit call back function.");

        hid_t dspace = -1;
        dspace = H5Dget_space(dataset);
        if(dspace <0) {
            H5Dclose(dataset);
            throw InternalErr(__FILE__, __LINE__, "H5Dget_space fails in the H5Ovisit call back function.");
        }

        // We only support netCDF-4 like dimension scales, that is the dimension scale dataset is 1-D dimension.
        if(H5Sget_simple_extent_ndims(dspace) == 1) {

            int count = 0;
            // Check if having "class = DIMENSION_SCALE" and REFERENCE_LIST attributes.
            herr_t ret = H5Aiterate2(dataset, H5_INDEX_NAME, H5_ITER_INC, NULL, attr_info, &count);
            if(ret < 0) {
                H5Sclose(dspace);
                H5Dclose(dataset);
                throw InternalErr(__FILE__, __LINE__, "H5Aiterate2 fails in the H5Ovisit call back function.");
            }

            // Find it.
            if (2==count) {
                if(dspace != -1)
                    H5Sclose(dspace);
                if(dataset != -1)
                    H5Dclose(dataset);
                return 1;
            }

        }
        if(dspace != -1)
            H5Sclose(dspace);
        if(dataset != -1)
            H5Dclose(dataset);
    }
    return 0;
}

static herr_t
attr_info(hid_t loc_id, const char *name, const H5A_info_t *ainfo, void *opdata)
{
    int *count = (int*)opdata;

    hid_t attr_id =   -1;
    hid_t atype_id =  -1;

    // Open the attribute
    attr_id = H5Aopen(loc_id, name, H5P_DEFAULT);
    if(attr_id < 0) 
        throw InternalErr(__FILE__, __LINE__, "H5Aopen fails in the attr_info call back function.");

    // Get attribute datatype and dataspace
    atype_id  = H5Aget_type(attr_id);
    if(atype_id < 0) {
        H5Aclose(attr_id);
        throw InternalErr(__FILE__, __LINE__, "H5Aget_type fails in the attr_info call back function.");
    }

    try {

        // If finding the "REFERENCE_LIST", increases the count.
        if ((H5T_COMPOUND == H5Tget_class(atype_id)) && (strcmp(name,"REFERENCE_LIST")==0)) {
             (*count)++;
        }


        // Check if finding the CLASS attribute.
        if ((H5T_STRING == H5Tget_class(atype_id)) && (strcmp(name,"CLASS") == 0)) {

            H5T_str_t str_pad = H5Tget_strpad(atype_id);

            hid_t aspace_id = -1;
            aspace_id = H5Aget_space(attr_id);
            if(aspace_id < 0) 
                throw InternalErr(__FILE__, __LINE__, "H5Aget_space fails in the attr_info call back function.");

            // CLASS is a variable-length string
            int ndims = H5Sget_simple_extent_ndims(aspace_id);
            hsize_t nelmts = 1;

            // if it is a scalar attribute, just define number of elements to be 1.
            if (ndims != 0) {

                vector<hsize_t> asize;
                vector<hsize_t> maxsize;
                asize.resize(ndims);
                maxsize.resize(ndims);

                // DAP applications don't care about the unlimited dimensions 
                // since the applications only care about retrieving the data.
                // So we don't check the maxsize to see if it is the unlimited dimension 
                // attribute.
                if (H5Sget_simple_extent_dims(aspace_id, &asize[0], &maxsize[0])<0) {
                    H5Sclose(aspace_id);
                    throw InternalErr(__FILE__, __LINE__, "Cannot obtain the dim. info in the H5Aiterate2 call back function.");
                }

                // Return ndims and size[ndims]. 
                for (int j = 0; j < ndims; j++)
                    nelmts *= asize[j];
            } // if(ndims != 0)

            size_t ty_size = H5Tget_size(atype_id);
            if (0 == ty_size) {
                H5Sclose(aspace_id);
                throw InternalErr(__FILE__, __LINE__, "Cannot obtain the type size in the H5Aiterate2 call back function.");
            }

            size_t total_bytes = nelmts * ty_size;
            string total_vstring ="";
            if(H5Tis_variable_str(atype_id) > 0) {

                // Variable length string attribute values only store pointers of the actual string value.
                vector<char> temp_buf;
                temp_buf.resize(total_bytes);

                if (H5Aread(attr_id, atype_id, &temp_buf[0]) < 0){
                    H5Sclose(aspace_id);
                    throw InternalErr(__FILE__,__LINE__,"Cannot read the attribute in the H5Aiterate2 call back function");
                }

                char *temp_bp = NULL;
                temp_bp = &temp_buf[0];
                char* onestring = NULL;

                for (unsigned int temp_i = 0; temp_i <nelmts; temp_i++) {

                    // This line will assure that we get the real variable length string value.
                    onestring =*(char **)temp_bp;

                    if(onestring!= NULL) 
                        total_vstring +=string(onestring);

                    // going to the next value.
                    temp_bp +=ty_size;
                }

                if (&temp_buf[0] != NULL) {
                    // Reclaim any VL memory if necessary.
                    if (H5Dvlen_reclaim(atype_id,aspace_id,H5P_DEFAULT,&temp_buf[0]) < 0) {
                        H5Sclose(aspace_id);
                        throw InternalErr(__FILE__,__LINE__,"Cannot reclaim VL memory in the H5Aiterate2 call back function.");
                    }
                }

            }
            else {// Fixed-size string, need to retrieve the string value.

                // string attribute values 
                vector<char> temp_buf;
                temp_buf.resize(total_bytes);
                if (H5Aread(attr_id, atype_id, &temp_buf[0]) < 0){
                    H5Sclose(aspace_id);
                    throw InternalErr(__FILE__,__LINE__,"Cannot read the attribute in the H5Aiterate2 call back function");
                }
                string temp_buf_string(temp_buf.begin(),temp_buf.end());
                total_vstring = temp_buf_string.substr(0,total_bytes);

                // Note: we need to remove the string pad or term to find DIMENSION_SCALE.
                if(str_pad != H5T_STR_ERROR) 
                    total_vstring = total_vstring.substr(0,total_vstring.size()-1);
            }
           
            // Close attribute data space ID.
            if(aspace_id != -1)
                H5Sclose(aspace_id);
            if(total_vstring == "DIMENSION_SCALE"){
                (*count)++;
            }
        }
    }
    catch(...) {
        if(atype_id != -1)
            H5Tclose(atype_id);
        if(attr_id != -1)
            H5Aclose(attr_id);
        throw;
    }

    // Close IDs.
    if(atype_id != -1)
        H5Tclose(atype_id);
    if(attr_id != -1)
        H5Aclose(attr_id);

    //find both class=DIMENSION_SCALE and REFERENCE_LIST, just return 1.
    if((*count)==2) 
        return 1;


    return 0;
}

void obtain_dimnames(hid_t dset,int ndims, DS_t *dt_inst_ptr) {

    htri_t has_dimension_list = -1;
    
    string dimlist_name = "DIMENSION_LIST";
    has_dimension_list = H5Aexists(dset,dimlist_name.c_str());
    if(has_dimension_list > 0 && ndims > 0) {

        hobj_ref_t rbuf;
        vector<hvl_t> vlbuf;
        vlbuf.resize(ndims);

        hid_t attr_id = -1;
        hid_t atype_id = -1;
        hid_t amemtype_id = -1;
        hid_t aspace_id = -1;
        hid_t ref_dset = -1;

        try {        
            attr_id = H5Aopen(dset,dimlist_name.c_str(),H5P_DEFAULT);
            if (attr_id <0 ) {
                string msg = "Cannot open the attribute " + dimlist_name + " of HDF5 dataset "+ string(dt_inst_ptr->name);
                throw InternalErr(__FILE__, __LINE__, msg);
            }

            atype_id = H5Aget_type(attr_id);
            if (atype_id <0) {
                string msg = "Cannot get the datatype of the attribute " + dimlist_name + " of HDF5 dataset "+ string(dt_inst_ptr->name);
                throw InternalErr(__FILE__, __LINE__, msg);
            }

            amemtype_id = H5Tget_native_type(atype_id, H5T_DIR_ASCEND);
            if (amemtype_id < 0) {
                string msg = "Cannot get the memory datatype of the attribute " + dimlist_name + " of HDF5 dataset "+ string(dt_inst_ptr->name);
                throw InternalErr(__FILE__, __LINE__, msg);
 
            }


            if (H5Aread(attr_id,amemtype_id,&vlbuf[0]) <0)  {
                string msg = "Cannot obtain the referenced object for the variable  " + string(dt_inst_ptr->name);
                throw InternalErr(__FILE__, __LINE__, msg);
            }

            vector<char> objname;

            // The dimension names of variables will be the HDF5 dataset names dereferenced from the DIMENSION_LIST attribute.
            for (int i = 0; i < ndims; i++) {

                rbuf =((hobj_ref_t*)vlbuf[i].p)[0];
                if ((ref_dset = H5Rdereference(attr_id, H5R_OBJECT, &rbuf)) < 0) {
                    string msg = "Cannot dereference from the DIMENSION_LIST attribute  for the variable " + string(dt_inst_ptr->name);
                    throw InternalErr(__FILE__, __LINE__, msg);
                }

                ssize_t objnamelen = -1;
                if ((objnamelen= H5Iget_name(ref_dset,NULL,0))<=0) 
                    throw;
                    //throw2("Cannot obtain the dataset name dereferenced from the DIMENSION_LIST attribute  for the variable ",var->name);
                objname.resize(objnamelen+1);
                if ((objnamelen= H5Iget_name(ref_dset,&objname[0],objnamelen+1))<=0) 
                    throw;
                    //throw2("Cannot obtain the dataset name dereferenced from the DIMENSION_LIST attribute  for the variable ",var->name);

                string objname_str = string(objname.begin(),objname.end());
                // Must trim the string delimter.
                string trim_objname = objname_str.substr(0,objnamelen);
// cerr<<"objname is "<<objname_str <<endl;
                //dt_inst_ptr->dimnames.push_back(trim_objname);
                dt_inst_ptr->dimnames.push_back(trim_objname.substr(trim_objname.find_last_of("/")+1));

                H5Dclose(ref_dset);
                ref_dset = -1;
                objname.clear();
            }// for (vector<Dimension *>::iterator ird = var->dims.begin()
            if(vlbuf.size()!= 0) {

                if ((aspace_id = H5Aget_space(attr_id)) < 0)
                    throw;
                    //throw2("Cannot get hdf5 dataspace id for the attribute ",dimlistattr->name);

                if (H5Dvlen_reclaim(amemtype_id,aspace_id,H5P_DEFAULT,(void*)&vlbuf[0])<0) 
                    throw;
                    //throw2("Cannot successfully clean up the variable length memory for the variable ",var->name);

                H5Sclose(aspace_id);
           
            }

        H5Tclose(atype_id);
        H5Tclose(amemtype_id);
        H5Aclose(attr_id);
        //H5Dclose(dset);
    
    }

    catch(...) {

        if(atype_id != -1)
            H5Tclose(atype_id);

        if(amemtype_id != -1)
            H5Tclose(amemtype_id);

        if(aspace_id != -1)
            H5Sclose(aspace_id);

        if(attr_id != -1)
            H5Aclose(attr_id);

        //if(dset_id != -1)
         //   H5Dclose(dset_id);

        //throw1("Error in method GMFile::Add_UseDimscale_Var_Dim_Names_Mea_SeaWiFS_Ozone"); 
        throw;
    }
 
    }
    return ;
}
