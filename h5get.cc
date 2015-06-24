// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c) 2007-2013 The HDF Group, Inc. and OPeNDAP, Inc.
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
// You can contact The HDF Group, Inc. at 1901 South First Street,
// Suite C-2, Champaign, IL 61820  

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
#include <BESDebug.h>

using namespace libdap;

///////////////////////////////////////////////////////////////////////////////
/// \fn get_attr_info(hid_t dset, int index, DSattr_t *attr_inst_ptr,
///                  int *ignoreptr)
///  will get attribute information.
///
/// This function will get attribute information: datatype, dataspace(dimension
/// sizes) and number of dimensions and put it into a data struct.
///
/// \param[in]  dset  dataset id
/// \param[in]  index  index of attribute
/// \param[out] attr_inst_ptr an attribute instance pointer
/// \param[out] ignoreptr  a flag to record whether it can be ignored.
/// \return pointer to attribute structure
/// \throw InternalError 
///////////////////////////////////////////////////////////////////////////////
hid_t get_attr_info(hid_t dset, int index, DSattr_t * attr_inst_ptr,
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

    // The following datatype will not be supported for mapping to DAS.
    // Note: H5T_COMPOUND and H5T_ARRAY can be mapped to DAP2 DAS(variable)
    // but not map to DAS due to really rarely used and unimportance.
    // 1-D variable length of string can also be mapped to both DAS and DDS.
    // The variable length string class is H5T_STRING rather than H5T_VLEN,
    // So safe here. 
    // We also ignore the mapping of integer 64 bit since DAP2 doesn't
    // support 64-bit integer. In theory, DAP2 doesn't support long double
    // (128-bit or 92-bit floating point type), since this rarely happens
    // in DAP application, we simply don't consider here.
    if ((ty_class == H5T_TIME) || (ty_class == H5T_BITFIELD)
        || (ty_class == H5T_OPAQUE) || (ty_class == H5T_ENUM)
        || (ty_class == H5T_REFERENCE) ||(ty_class == H5T_COMPOUND)
        || (ty_class == H5T_VLEN) || (ty_class == H5T_ARRAY) 
        || ((ty_class == H5T_INTEGER) && (H5Tget_size(ty_id)== 8))) {//64-bit int
        
        *ignore_attr_ptr = true;
        H5Tclose(ty_id);
        return attrid;
    }
        
    hid_t aspace_id;
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
/// \fn get_dap_type(hid_t type)
/// returns the string representation of HDF5 type.
///
/// This function will get the text representation(string) of the corresponding
/// DODS datatype. DODS-HDF5 subclass method will use this function.
///
/// \return string
/// \param type datatype id
///////////////////////////////////////////////////////////////////////////////
string get_dap_type(hid_t type)
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
            if (sign == H5T_SGN_NONE)      
                return BYTE;    
            else
                return INT16;
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
void get_dataset(hid_t pid, const string &dname, DS_t * dt_inst_ptr)
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

    for (int j = 0; j < ndims; j++) {
        (*dt_inst_ptr).size[j] = size[j];
    }

    H5Tclose(dtype);
    H5Sclose(dspace);
    H5Dclose(dset);
 
    BESDEBUG("h5", "<get_dataset() dimension=" << ndims << " elements=" <<
        nelmts << endl);
}


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
