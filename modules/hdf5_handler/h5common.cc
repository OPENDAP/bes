// data server.

// Copyright (c) 2007-2023 The HDF Group, Inc. and OPeNDAP, Inc.
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
// You can contact The HDF Group, Inc. at 410 E University Ave,
// Suite 200, Champaign, IL 61820  

///////////////////////////////////////////////////////////////////////////////
/// \file h5common.cc
/// 
/// Common helper functions to access HDF5 data for both the CF and the default options.
///
///
///////////////////////////////////////////////////////////////////////////////

#include "h5common.h"

#include <cstring>
#include <libdap/InternalErr.h>
#include <BESInternalError.h>
#include <BESDebug.h>


using namespace std;
using namespace libdap;

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
        string msg = "Failed to get the datatype of the dataset.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }
    hid_t dspace = -1;
    if ((dspace = H5Dget_space(dset)) < 0) {
        H5Tclose(dtype);
        string msg = "Failed to get the data space of the dataset.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }
    //  Use HDF5 H5Tget_native_type API
    hid_t memtype = H5Tget_native_type(dtype, H5T_DIR_ASCEND);
    if (memtype < 0) {
        H5Tclose(dtype);
        H5Sclose(dspace);
        string msg = "Failed to get memory type.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    if (H5Dread(dset, memtype, dspace, dspace, H5P_DEFAULT, buf)
            < 0) {
        H5Tclose(dtype);
        H5Tclose(memtype);
        H5Sclose(dspace);
        string msg = "Failed to read data. ";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    if (H5Tclose(dtype) < 0){
        H5Tclose(memtype);
        H5Sclose(dspace);
        string msg = "Unable to release the dtype.";
	throw BESInternalError(msg,__FILE__,__LINE__);
    }

    if (H5Tclose(memtype) < 0){
        H5Sclose(dspace);
        string msg = "Unable to release the memtype.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    if(H5Sclose(dspace)<0) {
        string msg = "Unable to release the data space.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

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
void get_strdata(int64_t strindex, char *allbuf, char *buf, int elesize)
{
    char *tempvalue = allbuf;   // The beginning of entire buffer.

    BESDEBUG("h5", ">get_strdata(): "
        << " strindex=" << strindex << " allbuf=" << allbuf << endl);

    // Tokenize the convbuf. 
    // The following line causes the degradation of the performance.
    // The code seems a leftover of a debugging process.
#if 0
    for (int i = 0; i < strindex; i++) {
        tempvalue = tempvalue + elesize;
    }
#endif
    tempvalue = tempvalue +strindex *elesize;

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
get_slabdata(hid_t dset, const int64_t *offset, const int64_t *step, const int64_t *count, const int num_dim,
             void *buf)
{
    BESDEBUG("h5", ">get_slabdata() " << endl);

    hid_t dtype = H5Dget_type(dset);
    if (dtype < 0) {
        string msg = "Could not get data type.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }
    // Using H5T_get_native_type API
    hid_t memtype = H5Tget_native_type(dtype, H5T_DIR_ASCEND);
    if (memtype < 0) {
        H5Tclose(dtype);
        string msg = "Could not get memory type.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    hid_t dspace = H5Dget_space(dset);
    if (dspace < 0) {
        H5Tclose(dtype);
        H5Tclose(memtype);
        string msg = "Could not get data space";
        throw BESInternalError(msg,__FILE__,__LINE__);
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
                           (const hsize_t *)dyn_offset.data(), dyn_step.data(),
                            dyn_count.data(), nullptr) < 0) {
        H5Tclose(dtype);
        H5Tclose(memtype);
        H5Sclose(dspace);
        string msg = "Could not select hyperslab";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    hid_t memspace = H5Screate_simple(num_dim, dyn_count.data(), nullptr);
    if (memspace < 0) {
        H5Tclose(dtype);
        H5Tclose(memtype);
        H5Sclose(dspace);
        string msg = "Could not create memory space.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    if (H5Dread(dset, memtype, memspace, dspace, H5P_DEFAULT, buf) < 0) {
        H5Tclose(dtype);
        H5Tclose(memtype);
        H5Sclose(dspace);
        H5Sclose(memspace);
        string msg = "Could not get data.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    if (H5Sclose(dspace) < 0){
        H5Tclose(dtype);
        H5Tclose(memtype);
        H5Sclose(memspace);
        string msg = "Unable to close the dspace.";
	throw BESInternalError(msg,__FILE__,__LINE__);
    }
    if (H5Sclose(memspace) < 0){
        H5Tclose(dtype);
        H5Tclose(memtype);
        string msg = "Unable to close the memspace.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }
    if (H5Tclose(dtype) < 0){
        H5Tclose(memtype);
        string msg = "Unable to close the dtype.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    if (H5Tclose(memtype) < 0){
        string msg = "Unable to close the memory datatype.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    BESDEBUG("h5", "<get_slabdata() " << endl);
    return 0;
}

bool read_vlen_string(hid_t dsetid, const int64_t nelms, const hsize_t *hoffset, const hsize_t *hstep,
                      const hsize_t *hcount, vector<string> &finstrval)
{

    hid_t dspace = -1;
    hid_t mspace = -1;
    hid_t dtypeid = -1;
    hid_t memtype = -1;
    bool is_scalar = false;

    if ((dspace = H5Dget_space(dsetid))<0) {
        string msg = "Cannot obtain HDF5 variable length string data space.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    if(H5S_SCALAR == H5Sget_simple_extent_type(dspace))
        is_scalar = true;

    if (!is_scalar) {
        if (H5Sselect_hyperslab(dspace, H5S_SELECT_SET,
                               hoffset, hstep,
                               hcount, nullptr) < 0) {
            H5Sclose(dspace);
            string msg = "Cannot generate the hyperslab of the HDF5 variable length string dataset.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        int d_num_dim = H5Sget_simple_extent_ndims(dspace);
        if(d_num_dim < 0) {
            H5Sclose(dspace);
            string msg = "Cannot obtain the number of dimensions of the data space.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        mspace = H5Screate_simple(d_num_dim, hcount,nullptr);
        if (mspace < 0) {
            H5Sclose(dspace);
            string msg = "Cannot create the memory space.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }
    }

    if ((dtypeid = H5Dget_type(dsetid)) < 0) {
        if (!is_scalar)
            H5Sclose(mspace);
        H5Sclose(dspace);
        string msg = "Cannot obtain the datatype.";
        throw BESInternalError(msg,__FILE__,__LINE__);

    }

    if ((memtype = H5Tget_native_type(dtypeid, H5T_DIR_ASCEND))<0) {
        if (!is_scalar)
            H5Sclose(mspace);
        H5Tclose(dtypeid);
        H5Sclose(dspace);
        string msg = "Fail to obtain memory datatype.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    size_t ty_size = H5Tget_size(memtype);

    vector <char> strval;
    strval.resize(nelms*ty_size);
    hid_t read_ret = -1;
    if (is_scalar)
        read_ret = H5Dread(dsetid,
                           memtype,
                           H5S_ALL,
                           H5S_ALL,
                           H5P_DEFAULT,
                           (void*)strval.data());
    else 
        read_ret = H5Dread(dsetid,
                           memtype,
                           mspace,
                           dspace,
                           H5P_DEFAULT,
                           (void*)strval.data());

    if (read_ret < 0) {
        if (!is_scalar)
            H5Sclose(mspace);
        H5Tclose(memtype);
        H5Tclose(dtypeid);
        H5Sclose(dspace);
        string msg = "Fail to read the HDF5 variable length string dataset.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    read_vlen_string_value(nelms, strval, finstrval, ty_size);
    claim_vlen_string_memory(memtype, dspace, dtypeid, mspace, strval, is_scalar);
    if (false == is_scalar) 
        H5Sclose(mspace);
    H5Tclose(memtype);
    H5Tclose(dtypeid);
    H5Sclose(dspace);
 
    return true;

}

void read_vlen_string_value(const int64_t nelms, vector<char> &strval, vector<string>&finstrval, size_t ty_size ) {
    // For scalar, nelms is 1.
    const char *temp_bp = strval.data();
    for (int i =0;i<nelms;i++) {
        string finalstr_val;
        get_vlen_str_data(temp_bp, finalstr_val);
        finstrval[i] = finalstr_val;
        temp_bp +=ty_size;
    }

}

void claim_vlen_string_memory(hid_t memtype, hid_t dspace, hid_t dtypeid, hid_t mspace, vector<char> &strval,
                              bool is_scalar) {

    if (false == strval.empty()) {
        herr_t ret_vlen_claim;
        if (true == is_scalar)
            ret_vlen_claim = H5Dvlen_reclaim(memtype,dspace,H5P_DEFAULT,(void*)strval.data());
        else
            ret_vlen_claim = H5Dvlen_reclaim(memtype,mspace,H5P_DEFAULT,(void*)strval.data());
        if (ret_vlen_claim < 0){
            if (false == is_scalar)
                H5Sclose(mspace);
            H5Tclose(memtype);
            H5Tclose(dtypeid);
            H5Sclose(dspace);
            string msg = "Cannot reclaim the memory buffer of the HDF5 variable length string.";
            throw BESInternalError(msg,__FILE__,__LINE__);

        }
    }
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

void get_vlen_str_data(const char*temp_bp,string &finalstr_val) {

    const char *onestring = *(const char**)temp_bp;
    if(onestring!=nullptr )
        finalstr_val =string(onestring);
    else // We will add a nullptr is onestring is nullptr.
        finalstr_val="";

}
