// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the Hyrax data server.

// Copyright (c) 2022 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

#include <iostream>
#include <sstream>
#include <memory>
#include <iterator>
#include <unordered_set>
#include <iomanip>      // std::put_time()
#include <ctime>      // std::gmtime_r()

#include <H5Ppublic.h>
#include <H5Dpublic.h>
#include <H5Epublic.h>
#include <H5Zpublic.h>  // Constants for compression filters
#include <H5Spublic.h>
#include <H5Tpublic.h>

#include "h5common.h"   // This is in the hdf5 handler

#include <libdap/Str.h>
#include <libdap/util.h>
#include <libdap/D4Attributes.h>
#include <libdap/Array.h>

#include <Base64.h>

#include <BESNotFoundError.h>
#include <BESInternalError.h>
#include <BESInternalFatalError.h>

#include <TheBESKeys.h>
#include <BESContextManager.h>

#include "DMRpp.h"
#include "DmrppTypeFactory.h"
#include "DmrppD4Group.h"
#include "DmrppArray.h"
#include "DmrppStructure.h"
#include "D4ParserSax2.h"

#include "UnsupportedTypeException.h"

using namespace std;
using namespace libdap;
using namespace dmrpp;

namespace build_dmrpp_util {

bool verbose = false;   // Optionally set by build_dmrpp's main().

#define VERBOSE(x) do { if (verbose) (x); } while(false)
#define prolog std::string("# build_dmrpp::").append(__func__).append("() - ")

#define INVOCATION_CONTEXT "invocation"

// FYI: Filter IDs
// H5Z_FILTER_ERROR         (-1) no filter
// H5Z_FILTER_NONE          0   reserved indefinitely
// H5Z_FILTER_DEFLATE       1   deflation like gzip
// H5Z_FILTER_SHUFFLE       2   shuffle the data
// H5Z_FILTER_FLETCHER32    3   fletcher32 checksum of EDC
// H5Z_FILTER_SZIP          4   szip compression
// H5Z_FILTER_NBIT          5   nbit compression
// H5Z_FILTER_SCALEOFFSET   6   scale+offset compression
// H5Z_FILTER_RESERVED      256 filter ids below this value are reserved for library use
// FYI: Filter IDs
// H5Z_FILTER_ERROR         (-1) no filter
// H5Z_FILTER_NONE          0   reserved indefinitely
// H5Z_FILTER_DEFLATE       1   deflation like gzip
// H5Z_FILTER_SHUFFLE       2   shuffle the data
// H5Z_FILTER_FLETCHER32    3   fletcher32 checksum of EDC
// H5Z_FILTER_SZIP          4   szip compression
// H5Z_FILTER_NBIT          5   nbit compression
// H5Z_FILTER_SCALEOFFSET   6   scale+offset compression
// H5Z_FILTER_RESERVED      256 filter ids below this value are reserved for library use

/**
 * Converts the H5Z_filter_t to a readable string.
 * @param filter_type an H5Z_filter_t representing the filter_type
 * @return
 */
string h5_filter_name(H5Z_filter_t filter_type) {
    string name;
    switch(filter_type) {
        case H5Z_FILTER_NONE:
            name = "H5Z_FILTER_NONE";
            break;
        case H5Z_FILTER_DEFLATE:
            name = "H5Z_FILTER_DEFLATE";
            break;
        case H5Z_FILTER_SHUFFLE:
            name = "H5Z_FILTER_SHUFFLE";
            break;
        case H5Z_FILTER_FLETCHER32:
            name = "H5Z_FILTER_FLETCHER32";
            break;
        case H5Z_FILTER_SZIP:
            name = "H5Z_FILTER_SZIP";
            break;
        case H5Z_FILTER_NBIT:
            name = "H5Z_FILTER_NBIT";
            break;
        case H5Z_FILTER_SCALEOFFSET:
            name = "H5Z_FILTER_SCALEOFFSET";
            break;
        default:
        {
            ostringstream oss("ERROR! Unknown HDF5 FILTER (H5Z_filter_t) type: ", std::ios::ate);
            oss << filter_type;
            throw BESInternalError(oss.str(),__FILE__,__LINE__);
        }
    }
    return name;
}

/**
 * Safely creates an hdf5 plist for the dataset.
 * @param dataset
 * @return A new plist built by H5Dget_create_plist()
 */
hid_t create_h5plist(hid_t dataset){
    hid_t plist_id;
    // Get creation properties list
    plist_id = H5Dget_create_plist(dataset);
    if ( plist_id < 0 )
        throw BESInternalError("Unable to open HDF5 dataset id.", __FILE__, __LINE__);
    return plist_id;
}

/**
 * Safely converts the BaseType pointer btp to a DmrppCommon pointer.
 * @param btp
 * @return
 */
DmrppCommon *toDC(BaseType *btp){
    auto *dc = dynamic_cast<DmrppCommon *>(btp);
    if (!dc) {
        stringstream msg;
        msg << "ERROR: Expected a BaseType that was also a DmrppCommon instance.";
        msg << "(variable_name: "<< ((btp)?btp->name():"unknown") << ").";
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }
    return dc;
}

/**
 * Safely converts the BaseType pointer btp to a DmrppCommon pointer.
 * @param btp
 * @return
 */
DmrppArray *toDA(BaseType *btp){
    auto *da = dynamic_cast<DmrppArray *>(btp);
    if (!da) {
        stringstream msg;
        msg << "ERROR: Expected a BaseType that was also a DmrppArray instance.";
        msg << "(variable_name: "<< ((btp)?btp->name():"unknown") << ").";
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }
    return da;
}


/**
 * @brief Set compression info
 *
 * @param dataset_id The HDF5 dataset id
 * @param dc A pointer to the DmrppCommon instance for that dataset_id
 */
static void set_filter_information(hid_t dataset_id, DmrppCommon *dc, bool disable_dio) {

    hid_t plist_id = create_h5plist(dataset_id);

    try {
        int numfilt = H5Pget_nfilters(plist_id);
        VERBOSE(cerr << prolog << "Number of filters associated with dataset: " << numfilt << endl);
        string filters;
        size_t nelmts = 20;
        unsigned int cd_values[20];
        vector<unsigned int> deflate_levels;

        for (int filter = 0; filter < numfilt; filter++) {
            unsigned int flags;
            H5Z_filter_t filter_type = H5Pget_filter2(plist_id, filter, &flags, &nelmts,
                                                      cd_values, 0, nullptr, nullptr);
            VERBOSE(cerr << prolog << "Found H5 Filter Type: " << h5_filter_name(filter_type) << " (" << filter_type << ")" << endl);
            switch (filter_type) {
                case H5Z_FILTER_DEFLATE:
                    filters.append("deflate ");
                    VERBOSE(cerr << prolog << "Deflate compression level: " << cd_values[0] << endl);
                    deflate_levels.push_back(cd_values[0]);
                    break;
                case H5Z_FILTER_SHUFFLE:
                    filters.append("shuffle ");
                    break;
                case H5Z_FILTER_FLETCHER32:
                    filters.append("fletcher32 ");
                    break;
                default:
                    ostringstream oss("Unsupported HDF5 filter: ", std::ios::ate);
                    oss << filter_type;
                    oss << " (" << h5_filter_name(filter_type) << ")";
                    throw BESInternalError(oss.str(), __FILE__, __LINE__);
            }
        }
        H5Pclose(plist_id);

        //trimming trailing space from compression (aka filter) string
        filters = filters.substr(0, filters.size() - 1);
        dc->set_filter(filters);
        dc->set_deflate_levels(deflate_levels);
        if (!filters.empty())
            dc->set_disable_dio(disable_dio);
    }
    catch (...) {
        H5Pclose(plist_id);
        throw;
    }
}


/**
 *
 * @param dataset_id
 * @return
 */
short
is_hdf5_fill_value_defined(hid_t dataset_id)
{

    // Suppress errors to stderr.
    H5Eset_auto2(H5E_DEFAULT, nullptr, nullptr);

    auto plist_id = create_h5plist(dataset_id);

    // ret_value 0: UNDEFINED
    // ret_value 1: DEFAULT
    // ret_value 2: USER_DEFINED 
    short ret_value = -1;

    // How the fill value is defined?
    H5D_fill_value_t status;
    if ((H5Pfill_value_defined(plist_id, &status)) < 0) {
        H5Pclose(plist_id);
        throw BESInternalError("Unable to access HDF5 Fillvalue information.", __FILE__, __LINE__);
    }
    if (status == H5D_FILL_VALUE_DEFAULT)
        ret_value = 1;
    else if (status == H5D_FILL_VALUE_USER_DEFINED)
        ret_value = 2;
    else if (status == H5D_FILL_VALUE_UNDEFINED)
        ret_value = 0;
   
    return ret_value;

}

/**
 * @brief Get the HDF5 value as a string
 *
 * We need it as a string for the DMR++ XML. Used to add Fill Value
 * HDF5 attribute information to the chunk elements of the DMR++
 *
 * @param h5_type_id
 * @param value
 * @return The string representation of the value.
 */
string
get_value_as_string(hid_t h5_type_id, vector<char> &value)
{
    H5T_class_t class_type = H5Tget_class(h5_type_id);
    switch (class_type) {
        case H5T_INTEGER: {
            int sign;
            sign = H5Tget_sign(h5_type_id);
            switch (H5Tget_size(h5_type_id)) {
                case 1:
                    if (sign == H5T_SGN_2)
                        return to_string(*(int8_t *) (value.data()));
                    else
                        return to_string(*(uint8_t *) (value.data()));

                case 2:
                    if (sign == H5T_SGN_2)
                        return to_string(*(int16_t *) (value.data()));
                    else
                        return to_string(*(uint16_t *) (value.data()));

                case 4:
                    if (sign == H5T_SGN_2)
                        return to_string(*(int32_t *) (value.data()));
                    else
                        return to_string(*(uint32_t *) (value.data()));

                case 8:
                    if (sign == H5T_SGN_2)
                        return to_string(*(int64_t *) (value.data()));
                    else
                        return to_string(*(uint64_t *) (value.data()));

                default:
                    throw BESInternalError("Unable to extract integer fill value.", __FILE__, __LINE__);
            }
            break;
        }

        case H5T_FLOAT: {
            ostringstream oss;
            switch (H5Tget_size(h5_type_id)) {
                case 4:
                    oss << *(float *) (value.data());
                    return oss.str();

                case 8:
                    oss << *(double *) (value.data());
                    return oss.str();

                default:
                    throw BESInternalError("Unable to extract float fill value.", __FILE__, __LINE__);
            }
            break;
        }

        case H5T_STRING: {
            if (H5Tis_variable_str(h5_type_id)) {

                // Reading the value like this doesn't work for squat.
                string fv_str(value.begin(),value.end());
                // Now fv_str is garbage,.

                stringstream msg(prolog);
                msg << "UnsupportedTypeException: Your data granule contains a variable length H5T_STRING ";
                msg << "as a fillValue type. This is not yet supported by the dmr++ creation machinery. ";
                msg << "The variable/dataset type screening code should have intercepted this prior. ";
                msg  <<  "fillValue(" + to_string(fv_str.length()) +" chars): 0x";
                for(auto c : fv_str){
                    msg << std::hex << +c ;
                }
                throw UnsupportedTypeException(msg.str());
            }
            else {
                string str_fv(value.begin(),value.end());
                return str_fv;
            }
            break;
        }
        case H5T_ARRAY: {
            string msg(prolog + "UnsupportedTypeException: Your data granule contains an H5T_ARRAY as a fillValue type. "
                                "This is not yet supported by the dmr++ creation machinery."
                                "The variable/dataset type screening code should intercepted this prior.");
            throw UnsupportedTypeException(msg);
        }
        case H5T_COMPOUND: {
            // The fill value of compound datatype is obtained by calling get_compound_fv_as_string() else where. 
            // An error should generate and be thrown.
            string msg(prolog + "UnsupportedTypeException: The fill value of a compound datatype should not be obtained in this function. "
                                "get_compound_fv_as_string() is the right function to get the value.");
            throw UnsupportedTypeException(msg);
        }

        case H5T_REFERENCE:
        default: {
            throw BESInternalError("Unable to extract fill value from HDF5 file.", __FILE__, __LINE__);
        }
    }
}

string
get_compound_base_fill_value_as_string(hid_t h5_type_id, char* value_ptr)
{
    H5T_class_t class_type = H5Tget_class(h5_type_id);
    switch (class_type) {
        case H5T_INTEGER: {
            int sign;
            sign = H5Tget_sign(h5_type_id);
            switch (H5Tget_size(h5_type_id)) {
                case 1:
                    if (sign == H5T_SGN_2)
                        return to_string(*(int8_t *) value_ptr);
                    else
                        return to_string(*(uint8_t *) value_ptr);

                case 2:
                    if (sign == H5T_SGN_2)
                        return to_string(*(int16_t *) value_ptr);
                    else
                        return to_string(*(uint16_t *) value_ptr);

                case 4:
                    if (sign == H5T_SGN_2)
                        return to_string(*(int32_t *) value_ptr);
                    else
                        return to_string(*(uint32_t *) value_ptr);

                case 8:
                    if (sign == H5T_SGN_2)
                        return to_string(*(int64_t *) value_ptr);
                    else
                        return to_string(*(uint64_t *) value_ptr);

                default:
                    throw BESInternalError("Unable to extract integer fill value.", __FILE__, __LINE__);
            }
        }

        case H5T_FLOAT: {
            ostringstream oss;
            switch (H5Tget_size(h5_type_id)) {
                case 4:
                    oss << *(float *) value_ptr;
                    return oss.str();

                case 8:
                    oss << *(double *) value_ptr;
                    return oss.str();

                default:
                    throw BESInternalError("Unable to extract float fill value.", __FILE__, __LINE__);
            }
        }
        default:
        throw BESInternalError("The member of compound datatype that has user-defined datatype has to be either integer or float..", __FILE__, __LINE__);
    }

}

string obtain_compound_user_defined_fvalues(hid_t dtype_id, hid_t h5_plist_id, vector<char> &value) {

    string ret_value;
    hid_t memtype  = -1;

    if ((memtype = H5Tget_native_type(dtype_id, H5T_DIR_ASCEND))<0)  {
        H5Pclose(h5_plist_id);
        throw BESInternalError ("Fail to obtain memory datatype.", __FILE__, __LINE__);
    }

    int                 nmembs           = 0;
    if ((nmembs = H5Tget_nmembers(memtype)) < 0) {
        H5Tclose(memtype);
        H5Pclose(h5_plist_id);
        string err_msg = "Fail to obtain number of HDF5 compound datatype.";
        throw BESInternalError (err_msg, __FILE__, __LINE__);
    }

    // We only need to retrieve the values and save them as strings.
    // We will only have the one-layer int/float structure to handle.
    // We do need to know the memb type to put the special handling of a string.
    for (unsigned int u = 0; u < (unsigned)nmembs; u++) {

        hid_t  memb_id  = -1;
        H5T_class_t         memb_cls         = H5T_NO_CLASS;
        size_t              memb_offset      = 0;

        // Get member type ID
        if((memb_id = H5Tget_member_type(memtype, u)) < 0) {
            H5Tclose(memtype);
            H5Pclose(h5_plist_id);
            string err_msg =  "Fail to obtain the datatype of an HDF5 compound datatype member.";
            throw BESInternalError (err_msg, __FILE__, __LINE__);
        }
    
        // Get member type class
        if((memb_cls = H5Tget_member_class (memtype, u)) < 0) {
            H5Pclose(h5_plist_id);
            H5Tclose(memtype);
            H5Tclose(memb_id);
            string err_msg =  "Fail to obtain the datatype class of an HDF5 compound datatype member.";
            throw BESInternalError (err_msg, __FILE__, __LINE__);
        }

        // Get member offset,H5Tget_member_offset only fails
        // when H5Tget_memeber_class fails. Sinc H5Tget_member_class
        // is checked above. So no need to check the return value.
        memb_offset= H5Tget_member_offset(memtype,u);

        if (memb_cls ==  H5T_ARRAY) {

            hid_t at_base_type = H5Tget_super(memb_id);
            size_t at_base_type_size = H5Tget_size(at_base_type);
            H5T_class_t array_cls = H5Tget_class(at_base_type);

            if (array_cls != H5T_INTEGER && array_cls !=H5T_FLOAT) {
                H5Tclose(memtype);
                H5Tclose(memb_id);
                string err_msg =  "The base class of an HDF5 compound datatype member must be integer or float.";
                throw BESInternalError (err_msg, __FILE__, __LINE__);
            }

            // Need to retrieve the number of elements of the array
            int at_ndims = H5Tget_array_ndims(memb_id);
            if (at_ndims <= 0) {
                H5Pclose(h5_plist_id);
                H5Tclose(memtype);
                H5Tclose(at_base_type);
                H5Tclose(memb_id);
                string err_msg =  "Fail to obtain number of dimensions of the array datatype.";
                throw BESInternalError (err_msg, __FILE__, __LINE__);
            }
        
            vector<hsize_t>at_dims_h(at_ndims,0);
        
            // Obtain the number of elements for each dims
            if (H5Tget_array_dims(memb_id,at_dims_h.data())<0) {
                H5Pclose(h5_plist_id);
                H5Tclose(memtype);
                H5Tclose(at_base_type);
                H5Tclose(memb_id);
                string err_msg =  "Fail to obtain each imension size of the array datatype.";
                throw BESInternalError (err_msg, __FILE__, __LINE__);
            }

            vector<hsize_t>at_dims_offset(at_ndims,0);                   
            size_t total_array_nums = 1;
            for (const auto & ad:at_dims_h)
                total_array_nums *=ad;

            // We need to convert each value to a string and save that string as one value of a string.
            for (unsigned ar_index = 0; ar_index <total_array_nums; ar_index++) {
                char *value_ptr = value.data() + memb_offset + ar_index *at_base_type_size;
                string tmp_value = get_compound_base_fill_value_as_string(at_base_type,value_ptr);
                if (u == 0 && ar_index== 0) 
                    ret_value = tmp_value;
                else
                    ret_value = ret_value + ' '+ tmp_value;
            }

            H5Tclose(at_base_type);

        }
        else {// Scalar int/float

            //We need to figure out the datatype and convert each data value to a string.
            char *value_ptr = value.data() + memb_offset;
            string tmp_value = get_compound_base_fill_value_as_string(memb_id,value_ptr);
            if ( u == 0)
                ret_value = tmp_value;
            else 
                ret_value = ret_value + ' '+ tmp_value;
        }
        H5Tclose(memb_id);
    }

    H5Tclose(memtype);

    return ret_value;
}

unsigned short is_supported_compound_type(hid_t h5_type) {

    unsigned short ret_value = 1;
    bool has_string_memb_type = false;
    hid_t memtype = -1;
    if ((memtype = H5Tget_native_type(h5_type, H5T_DIR_ASCEND)) < 0) {
        throw InternalErr(__FILE__, __LINE__, "Fail to obtain memory datatype.");
    }
    
    hid_t  memb_id = -1;
    H5T_class_t memb_cls = H5T_NO_CLASS;
    int nmembs = 0;
    char *memb_name = nullptr;

    if ((nmembs = H5Tget_nmembers(memtype)) < 0) {
        throw InternalErr(__FILE__, __LINE__, "Fail to obtain number of HDF5 compound datatype.");
    }

    for (unsigned int u = 0; u < (unsigned) nmembs; u++) {

        if ((memb_id = H5Tget_member_type(memtype, u)) < 0)
            throw InternalErr(__FILE__, __LINE__,
                              "Fail to obtain the datatype of an HDF5 compound datatype member.");

        // Get member type class
        memb_cls = H5Tget_member_class(memtype, u);

        // Get member name
        memb_name = H5Tget_member_name(memtype, u);
        if (memb_name == nullptr)
            throw InternalErr(__FILE__, __LINE__, "Fail to obtain the name of an HDF5 compound datatype member.");

        if (memb_cls == H5T_COMPOUND) 
            ret_value = 0;
        else if (memb_cls == H5T_ARRAY) {

            hid_t at_base_type = H5Tget_super(memb_id);
            H5T_class_t array_cls = H5Tget_class(at_base_type);
            if (array_cls != H5T_INTEGER && array_cls != H5T_FLOAT && array_cls != H5T_STRING)
                ret_value = 0;
            else if (array_cls == H5T_STRING && has_string_memb_type == false)
                has_string_memb_type = true;
            H5Tclose(at_base_type);


        } else if (memb_cls != H5T_INTEGER && memb_cls != H5T_FLOAT) {
            if (memb_cls == H5T_STRING) { 
                if (has_string_memb_type == false)
                    has_string_memb_type = true;
            }
            else 
                ret_value = 0;
        } 

        // Close member type ID
        H5Tclose(memb_id);
        free(memb_name);
        if (ret_value == 0) 
            break;
    } // end for

    if (has_string_memb_type)
        ret_value = 2;
    return ret_value;

}


string
get_compound_fv_as_string(hid_t dtype_id, hid_t h5_plist_id, vector<char> &value)
{
    H5D_fill_value_t fill_value_status;
    if (H5Pfill_value_defined(h5_plist_id, &fill_value_status)<0) { 
        H5Pclose(h5_plist_id);
        throw BESInternalError("H5Pfill_value_defined failed.", __FILE__, __LINE__);
    }

    string ret_str;
    string H5_Default_fvalue = "0";
    if (fill_value_status == H5D_FILL_VALUE_DEFAULT)
        ret_str = H5_Default_fvalue;
    else if (fill_value_status == H5D_FILL_VALUE_USER_DEFINED) {

        // We don't support when a compound datatype has a string member and the fill value is user-defined. 
        if (is_supported_compound_type(dtype_id) == 2) {

            string msg(prolog + "UnsupportedTypeException: Your data granule contains an H5T_COMPOUND as user-defined fillValue type"
                       "and one member of H5T_COMPOUND is a string. " 
                       "This is not yet supported by the dmr++ creation machinery. ");
            string str_fv(value.begin(),value.end());
            throw UnsupportedTypeException(msg);
        }

        ret_str = obtain_compound_user_defined_fvalues(dtype_id, h5_plist_id, value);       
    }
    else if (fill_value_status == H5D_FILL_VALUE_UNDEFINED) {
        H5Pclose(h5_plist_id);
        throw BESInternalError("The fill value is undefined, the dmrpp module cannot handle this case now.", __FILE__, __LINE__);
    }

    return ret_str;
}

bool is_supported_vlen_type(hid_t dataset_id, hid_t h5_type) {

    bool ret_value = false;
    hid_t base_type = H5Tget_super(h5_type);
    hid_t dspace = H5Dget_space(dataset_id);
    if (H5S_SIMPLE == H5Sget_simple_extent_type(dspace) && 
       (H5Tget_class(base_type) == H5T_INTEGER || H5Tget_class(base_type) == H5T_FLOAT))
        ret_value = true;
    H5Tclose(base_type);
    H5Sclose(dspace);
    return ret_value;

}


/**
 * @brief Get the value of the File Value as a string
 * @param dataset_id
 * @return The string representation of the HDF5 Fill Value
 */
string get_hdf5_fill_value_str(hid_t dataset_id)
{
    // Suppress errors to stderr.
    H5Eset_auto2(H5E_DEFAULT, nullptr, nullptr);

    // Get creation properties list
    hid_t plist_id = create_h5plist(dataset_id);
    if (plist_id  < 0 )
        throw BESInternalError("Unable to open HDF5 dataset id.", __FILE__, __LINE__);

    try {
        hid_t dtype_id = H5Dget_type(dataset_id);
        if (dtype_id < 0)
            throw BESInternalError("Unable to get HDF5 dataset type id.", __FILE__, __LINE__);

        vector<char> value(H5Tget_size(dtype_id), 0);
        if (H5Pget_fill_value(plist_id, dtype_id, value.data()) < 0)
            throw BESInternalError("Unable to access HDF5 Fill Value.", __FILE__, __LINE__);

        string fvalue_str;
        // The fill value of the compound datatype needs to be handled separately.
        if (H5Tget_class(dtype_id) == H5T_COMPOUND) 
            fvalue_str = get_compound_fv_as_string(dtype_id,plist_id,value);
        else 
            fvalue_str =  get_value_as_string(dtype_id, value);

        H5Pclose(plist_id);

        return fvalue_str;
    }
    catch (...) {
        H5Pclose(plist_id);
        throw;
    }
}


/**
 * Converts the hdf5 string padding scheme str_pad to an instance of the
 * string_pad_type enumeration..
 * @param str_pad
 * @return
 */
string_pad_type convert_h5_str_pad_type(const H5T_str_t str_pad){
    string_pad_type pad_type;
    switch(str_pad){
        case H5T_STR_SPACEPAD:
            pad_type = dmrpp::space_pad;
            break;

        case H5T_STR_NULLTERM:
            pad_type = dmrpp::null_term;
            break;

        case H5T_STR_NULLPAD:
            pad_type = dmrpp::null_pad;
            break;

        default:
        {
            stringstream msg;
            msg << "ERROR: Received unrecognized value for H5T_str_t: " << str_pad << endl;
            throw BESInternalError(msg.str(),__FILE__,__LINE__);
        }
    }
    return pad_type;
}

/**
 * Determines string padding schemed used by the passed dataset and encodes it as an instance of the
 * string_pad_type enumeration..
 * If the dataset does not have a string padding scheme in place then an exception will be thrown.
 * @param dataset
 * @return
 */
string_pad_type get_pad_type(const hid_t dataset) {
    hid_t h5_type = H5Dget_type(dataset);
    if(h5_type < 0){
        stringstream msg;
        msg << "ERROR: H5Dget_type() failed. returned: " << h5_type;
        throw BESInternalError(msg.str(),__FILE__, __LINE__);
    }
    H5T_str_t str_pad = H5Tget_strpad(h5_type);
    if(str_pad < 0) {
        stringstream msg;
        msg << "ERROR: H5Tget_strpad() failed. returned: " << str_pad;
        throw BESInternalError(msg.str(),__FILE__, __LINE__);
    }
    return convert_h5_str_pad_type(str_pad);
}

/**
 * Adds the fixed length string array information to the array variable array_var. If the array_var is not an
 * array is not an array of strings, or if the string array is not a fixed length string array (i.e. it's a variable
 * length string array) the this method is a no-op, nothing is done.
 *
 * @param dataset_id The hdf5 dataset reference for this dataset
 * @param array_var A point to the corresponding DmrppArray into which the information will be added.
 */
void add_fixed_length_string_array_state(const hid_t dataset_id, DmrppArray *array_var){

    hid_t h5_type = H5Dget_type(dataset_id);
    if (H5Tis_variable_str(h5_type) > 0 ){
        cout << "# The dataset '" << array_var->name() << "' is a variable length string array, skipping..." << endl;
        return;
    }

    VERBOSE( cerr << prolog << "Processing the array dariable:  " << array_var->name() << endl);
    auto data_type = array_var->var()->type();

    if(data_type == libdap::dods_str_c){
        VERBOSE( cerr << prolog << "The array template variable has type libdap::dods_str_c" << endl);

        array_var->set_is_flsa(true);

        auto pad_type = get_pad_type(dataset_id);
        VERBOSE( cerr << prolog << "pad_type:  " << pad_type << endl);
        array_var->set_fixed_length_string_pad(pad_type);

        auto type_size = H5Tget_size(h5_type);
        VERBOSE( cerr << prolog << "type_size:  " << type_size << endl);
        array_var->set_fixed_string_length(type_size);
    }
}



/**
 * Adds special String array state to the passed BaseType variable.
 *
 * @param dataset
 * @param btp
 */
static void add_string_array_info(const hid_t dataset, BaseType *btp){

    Type dap_type = btp->type();
    if(dap_type != dods_array_c){
        // NOT AN ARRAY SKIPPING...
        VERBOSE( cerr << prolog << "Variable " << btp->name() << " is not a DAP Array. Skipping..." << endl);
        return;
    }
    auto dap_array = toDA(btp);
    if (dap_array->var()->type() != dods_str_c) {
        // NOT A STRING ARRAY SKIPPING...
        VERBOSE( cerr << prolog << "Variable " << dap_array->name() << " is an Array of " << dap_array->var()->type_name() << " not String. Skipping..." << endl);
        return;
    }

    auto h5_dataset_type = H5Dget_type(dataset);
    if(h5_dataset_type == H5I_INVALID_HID){
        throw BESInternalError("ERROR: H5Dget_type() failed for variable '" + dap_array->name() + "'",
                               __FILE__, __LINE__);
    }

    auto h5_type_class = H5Tget_class(h5_dataset_type);
    if(h5_type_class != H5T_STRING){
        VERBOSE( cerr << prolog << "H5Dataset " << dap_array->name() << " is not a String type (type: " << h5_type_class << "). Skipping..." << endl);
        return;
    }

    hid_t dspace = H5Dget_space(dataset);
    if (H5S_SCALAR == H5Sget_simple_extent_type(dspace)){
        VERBOSE( cerr << prolog << "H5Dataset " << dap_array->name() << " is a scalar type. Skipping..." << endl);
        return;
    }

    if (H5Tis_variable_str(h5_dataset_type) > 0) {
        VERBOSE( cerr << prolog << "Found variable length string array: " << dap_array->name() << endl);
        dap_array->set_is_vlsa(true);
    }
    else {
        VERBOSE( cerr << prolog << "Found fixed length string array: " << dap_array->name() << endl);
        add_fixed_length_string_array_state( dataset,  dap_array);
    }
}

/**
 * Returns a string representing the byte order for the dariable
 * @param dataset The dataset from which to acquire the byte order.
 * @return a string representing the byte order
 */
string byte_order_str(hid_t dataset){
    string byte_order_string;
    hid_t dtypeid = H5Dget_type(dataset);
    auto b_order = H5Tget_order(dtypeid);
    switch (b_order) {
        case H5T_ORDER_LE:
            byte_order_string = "LE";
            break;
        case H5T_ORDER_BE:
            byte_order_string = "BE";
            break;
        case H5T_ORDER_NONE:
            break;
        default:
            // unsupported enumerations: H5T_ORDER_[ERROR,VAX,MIXED]
            ostringstream oss("Unsupported HDF5 dataset byteOrder: ", std::ios::ate);
            oss << b_order << ".";
            throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }
    return byte_order_string;
}

void obtain_structure_offset(hid_t dataset, vector<unsigned int>& struct_offsets) {

    hid_t dtypeid = H5Dget_type(dataset);

    hid_t memb_id;
    size_t memb_offset = 0;

    int nmembs = H5Tget_nmembers(dtypeid);
    if (nmembs <0) {
        H5Tclose(dtypeid);
        throw BESInternalError("Cannot get the number of base datatypes in a compound datatype.", __FILE__, __LINE__);
    }

    for (unsigned int u = 0; u < (unsigned) nmembs; u++) {

        if ((memb_id = H5Tget_member_type(dtypeid, u)) < 0) {
            H5Tclose(dtypeid);
            throw BESInternalError("Cannot get the number of base datatypes in a compound datatype.", __FILE__, __LINE__);
        }

        // Get member offset
        memb_offset = H5Tget_member_offset(dtypeid, u);
        if (u !=0)
            struct_offsets.push_back(memb_offset);
            
        H5Tclose(memb_id);

    }

    // We need to add the size of the datatype to correctly retrieve the value of the next element.
    size_t type_size = H5Tget_size(dtypeid);
    if (type_size == 0) {
        H5Tclose(dtypeid);
        throw BESInternalError("Cannot get the correct data type size.", __FILE__, __LINE__);
    }
    struct_offsets.push_back(type_size);
    H5Tclose(dtypeid);
 
}



/**
 * Processes the hdf5 storage information for a variable whose data is stored in the H5D_CONTIGUOUS storage layout.
 * @param dataset The hdf5 dataset that is mate to the BaseType instance btp.
 * @param btp The dap BaseType variable which is to hold the information gleand from the hdf5 dataset.
 */
void process_contiguous_layout_dariable(hid_t dataset, BaseType *btp){
    VERBOSE(cerr << prolog << "  Storage: contiguous" << endl);

    haddr_t cont_addr = H5Dget_offset(dataset);
    hsize_t cont_size = H5Dget_storage_size(dataset);
    string byte_order = byte_order_str(dataset);

    VERBOSE(cerr << prolog << "     Addr: " << cont_addr << endl);
    VERBOSE(cerr << prolog << "     Size: " << cont_size << endl);
    VERBOSE(cerr << prolog << "byteOrder: " << byte_order << endl);

    if (cont_size > 0) {
        auto dc = toDC(btp);
        VERBOSE(cerr << prolog << "     Before add_chunk: " <<btp->name() << endl);
        dc->add_chunk(byte_order, cont_size, cont_addr, "");
    }
}

/**
 * Processes the hdf5 storage information for a variable whose data is stored in the H5D_CHUNKED storage layout.
 * @param dataset The hdf5 dataset that is mate to the BaseType instance btp.
 * @param btp The dap BaseType variable which is to hold the information gleand from the hdf5 dataset.
 */
void process_chunked_layout_dariable(hid_t dataset, BaseType *btp, bool disable_dio) {

    DmrppCommon *dc = toDC(btp);
    hid_t fspace_id = H5Dget_space(dataset);
    int dataset_rank = H5Sget_simple_extent_ndims(fspace_id);
    string byte_order = byte_order_str(dataset);

    hsize_t num_chunks = 0;
    herr_t status = H5Dget_num_chunks(dataset, fspace_id, &num_chunks);
    if (status < 0) {
        throw BESInternalError("Could not get the number of chunks for variable "+ btp->name(), __FILE__, __LINE__);
    }

    VERBOSE(cerr << prolog << "Storage: chunked." << endl);
    VERBOSE(cerr << prolog << "Number of chunks is: " << num_chunks << endl);

    set_filter_information(dataset, dc, disable_dio);

    // Get chunking information: rank and dimensions
    vector<hsize_t> chunk_dims(dataset_rank, 0);

    unsigned int chunk_rank = 0;
    hid_t plist_id = create_h5plist(dataset);
    try {
        chunk_rank = H5Pget_chunk(plist_id, dataset_rank, chunk_dims.data());
        H5Pclose(plist_id);
    }
    catch (...) {
        H5Pclose(plist_id);
        throw;
    }

    if (chunk_rank != dataset_rank)
        throw BESNotFoundError(
                "Found a chunk with rank different than the dataset's (aka variables') rank", __FILE__,
                __LINE__);

    dc->set_chunk_dimension_sizes(chunk_dims);
 
    for (unsigned int i = 0; i < num_chunks; ++i) {
        vector<hsize_t> chunk_coords(dataset_rank, 0);
        haddr_t addr = 0;
        hsize_t size = 0;

        unsigned filter_mask = 0;

        status = H5Dget_chunk_info(dataset, fspace_id, i, chunk_coords.data(),
                                   &filter_mask, &addr, &size);
        if (status < 0) {
            VERBOSE(cerr << "ERROR" << endl);
            throw BESInternalError("Cannot get HDF5 dataset storage info.", __FILE__, __LINE__);
        }

        VERBOSE(cerr << prolog << "chk_idk: " << i << ", addr: " << addr << ", size: " << size << endl);
        dc->add_chunk(byte_order, size, addr, filter_mask, chunk_coords);
    }
}

H5D_layout_t get_h5_storage_layout(hid_t dataset){
    H5D_layout_t layout_type;
    hid_t plist_id = create_h5plist(dataset);
    try {
        layout_type = H5Pget_layout(plist_id);
    }
    catch(...){
        H5Pclose(plist_id);
        throw;
    }
    return layout_type;
}

void process_compact_layout_scalar(hid_t dataset, BaseType *btp)
{

    // The variable is a scalar, not an array

    VERBOSE(cerr << prolog << "Processing scalar dariable. Storage: compact" << endl);

    hid_t dtypeid = H5Dget_type(dataset);
    VERBOSE(cerr << prolog << "   H5Dget_type(): " << dtypeid << endl);

    auto type_size = H5Tget_size(dtypeid);
    VERBOSE(cerr << prolog << "   H5Tget_size(): " << type_size << " (The size of the datatype in bytes)" << endl);

    size_t compact_storage_size = H5Dget_storage_size(dataset);
    VERBOSE(cerr << prolog << "   H5Dget_storage_size(): " << compact_storage_size << " (The amount of storage space, in bytes, or 0.)" << endl);
    if (compact_storage_size == 0) {
        throw BESInternalError("Cannot obtain the compact storage size.", __FILE__, __LINE__);
    }

    Type dap_type = btp->type();
    unsigned long long memRequired = 0;
    if (dap_type == dods_str_c)
        // The string length is 0 if no string has been stored in the internal buffer. So we cannot use length()
        // We know the length is 1 for a scalar string.
        memRequired = type_size;
    else
        memRequired = btp->length() * type_size;

    // For variable length string, the storage size and the datatype size is not the same.
    // And we don't need to know then since it is a variable length string.
    if (H5Tis_variable_str(dtypeid) == 0) {
        if (compact_storage_size != memRequired)
            throw BESInternalError("Compact storage size does not match D4Array or scalar.", __FILE__,
                                   __LINE__);
    }

    switch (dap_type)
    {
        case dods_byte_c:
        case dods_char_c:
        case dods_int8_c:
        case dods_uint8_c:
        case dods_int16_c:
        case dods_uint16_c:
        case dods_int32_c:
        case dods_uint32_c:
        case dods_float32_c:
        case dods_float64_c:
        case dods_int64_c:
        case dods_uint64_c:
        {
            vector<uint8_t> values(memRequired, 0);
            get_data(dataset, reinterpret_cast<void *>(values.data()));
            btp->set_read_p(true);
            btp->val2buf(reinterpret_cast<void *>(values.data()));
            break;
        }

        case dods_url_c:
        case dods_str_c:
        {
            auto str = dynamic_cast<libdap::Str *>(btp);
            if (H5Tis_variable_str(dtypeid) > 0) {
                vector<string> finstrval;   // passed by reference to read_vlen_string
                // @TODO Why push an empty string into the first array position? WHY?
                finstrval.emplace_back("");
                read_vlen_string(dataset, 1, nullptr, nullptr, nullptr, finstrval);
                string vlstr = finstrval[0];
                str->set_value(vlstr);
                str->set_read_p(true);
            }
            else {
                // A single string for scalar.
                vector<uint8_t> values(memRequired, 0);
                get_data(dataset, reinterpret_cast<void *>(values.data()));
                string fstr(values.begin(), values.end());
                str->set_value(fstr);
                str->set_read_p(true);
            }
            break;
        }

        default:
            throw BESInternalError("Unsupported compact storage variable type.", __FILE__, __LINE__);
    }
}


void process_compact_flsa(hid_t dataset, BaseType *btp){

    add_string_array_info(dataset, btp);

    auto pad_type = get_pad_type(dataset);
    VERBOSE( cerr << prolog << "pad_type:  " << pad_type << endl);

    auto h5_type = H5Dget_type(dataset);
    VERBOSE( cerr << prolog << "H5Dget_type():  " << h5_type << endl);

    // Since this is a fixed length string, the H5Tget_size() returns the
    // length in characters (i.e. bytes) of the fixed length string
    auto fls_length = H5Tget_size(h5_type);
    VERBOSE( cerr << prolog << "fls_length:  " << fls_length << endl);

    auto memRequired = btp->length_ll() * fls_length;

    auto array = toDA(btp);
    auto &string_buf = array->compact_str_buffer();
    string_buf.resize(memRequired);
    get_data(dataset, reinterpret_cast<void *>(string_buf.data()));
    array->set_read_p(true);
}

void process_compact_layout_array(hid_t dataset, BaseType *btp) {

    VERBOSE(cerr << prolog << "BEGIN (" << btp->type_name() << " " << btp->name() << ")" << endl);

    hid_t dtypeid = H5Dget_type(dataset);
    VERBOSE(cerr << prolog << "   H5Dget_type(): " << dtypeid << endl);

    auto type_size = H5Tget_size(dtypeid);
    VERBOSE(cerr << prolog << "   H5Tget_size(): " << type_size << " (The size of the datatype in bytes)" << endl);

    size_t compact_storage_size = H5Dget_storage_size(dataset);
    VERBOSE(cerr << prolog << "   H5Dget_storage_size(): " << compact_storage_size << " (The amount of storage space, in bytes, or 0.)" << endl);
    if (compact_storage_size == 0) {
        throw BESInternalError("Cannot obtain the compact storage size.", __FILE__, __LINE__);
    }

    Type dap_type = btp->type();
    unsigned long long memRequired = 0;
    if (dap_type == dods_str_c)
        // The string length is 0 if no string has been stored in the internal buffer. So we cannot use length()
        // We know the length is 1 for a scalar string.
        memRequired = type_size;
    else
        memRequired = btp->length() * type_size;

    // For variable length string, the storage size and the datatype size is not the same.
    // And we don't need to know then since it is a variable length string.
    if (H5Tis_variable_str(dtypeid) == 0) {
        if (compact_storage_size != memRequired)
            throw BESInternalError("Compact storage size does not match D4Array or scalar.", __FILE__,
                                   __LINE__);
    }

    auto array = toDA(btp);
    switch (array->var()->type()) {
        case dods_byte_c:
        case dods_char_c:
        case dods_int8_c:
        case dods_uint8_c:
        case dods_int16_c:
        case dods_uint16_c:
        case dods_int32_c:
        case dods_uint32_c:
        case dods_float32_c:
        case dods_float64_c:
        case dods_int64_c:
        case dods_uint64_c:
        {
            vector<uint8_t> values(memRequired, 0);
            get_data(dataset, reinterpret_cast<void *>(values.data()));
            array->set_read_p(true);
            array->val2buf(reinterpret_cast<void *>(values.data()));
            break;

        }
        case dods_url_c:
        case dods_str_c:
        {
            if (H5Tis_variable_str(dtypeid) > 0) {
                // Variable length string case.
                vector<string> finstrval;   // passed by reference to read_vlen_string
                // @TODO Why push an empty string into the first array position? WHY?
                finstrval.emplace_back("");
                read_vlen_string(dataset, 1, nullptr, nullptr, nullptr, finstrval);
                array->set_value(finstrval, (int) finstrval.size());
                array->set_read_p(true);
            }
            else {
                // Fixed length string case.
                process_compact_flsa(dataset, btp);
            }
            break;
        }

        default:
            throw BESInternalError("Unsupported compact storage variable type.", __FILE__, __LINE__);
    }
}


/**
 * Processes a variable whose data is stored in the H5D_COMPACT storage layout.
 * @param dataset The hdf5 dataset that is mate to the BaseType instance btp.
 * @param btp The dap BaseType variable which is to hold the information gleand from the hdf5 dataset.
 */
void process_compact_layout_dariable(hid_t dataset, BaseType *btp){

    VERBOSE(cerr << prolog << "Processing Compact Storage Layout Dariable" << endl);
    // -         -        -       -      -     -    -   -  - -:- -  -   -    -     -      -       -        -         -
    // This next block is all the QC stuff "sanitize your inputs"
    //
    auto dc = toDC(btp); // throws if no match

    Type dap_type = btp->type();
    if ( dap_type == dods_structure_c
         || dap_type == dods_sequence_c
         || dap_type == dods_grid_c) {
        stringstream msg;
        msg << "The variable " << btp->FQN() << " is an instance of " << btp->type_name() << ", and utilizes ";
        msg << "the hdf5 compact storage layout (H5D_COMPACT). ";
        msg << "Only arrays of string and numeric data types are supported for the compact storage layout.";
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }

    auto layout_type = get_h5_storage_layout(dataset);
    if (layout_type != H5D_COMPACT)
        throw BESInternalError(string("ERROR: The dataset is not stored with compact layout."), __FILE__, __LINE__);

    // -         -        -       -      -     -    -   -  - -:- -  -   -    -     -      -       -        -         -
    // Now the QC stuff is finished, so we go to work on the compact layout variable.
    //

    dc->set_compact(true);

    if (dap_type == dods_array_c) {
        process_compact_layout_array(dataset, btp);
    }
    else {
        process_compact_layout_scalar(dataset, btp);
    }
}


/**
 * Adds hdf5 FillValue information (if any)to the passed variable btp
 * @param dataset
 * @param btp
 */
void set_fill_value(hid_t dataset, BaseType *btp){
    short fill_value_defined = is_hdf5_fill_value_defined(dataset);
    if (fill_value_defined >0) {
        string fill_value = get_hdf5_fill_value_str(dataset);
        auto dc = toDC(btp);
        dc->set_uses_fill_value(fill_value_defined);
        dc->set_fill_value_string(fill_value);
    }
}


bool obtain_structure_string_value(hid_t memtype, size_t ty_size, hssize_t num_elms, vector<char>& encoded_struct_value,const vector<char>& struct_value, string & err_msg) {

    bool ret_value = true;
    size_t values_offset = 0;

    // Loop through all the elements in this compound datatype variable.
    for (int64_t element = 0; element < num_elms; ++element) { 

        int                 nmembs           = 0;
        size_t              struct_elem_offset = ty_size*element;

        if ((nmembs = H5Tget_nmembers(memtype)) < 0) {
            err_msg = "Fail to obtain number of HDF5 compound datatype.";
            ret_value = false;
            break;
        }

        // We only need to retrieve the values and re-assemble them.
        // We will only have the one-layer string-contained structure to handle.
        // We do need to know the memb type to put the special handling of a string.
        for (unsigned int u = 0; u < (unsigned)nmembs; u++) {

            hid_t  memb_id  = -1;
            H5T_class_t         memb_cls         = H5T_NO_CLASS;
            size_t              memb_offset      = 0;

            // Get member type ID
            if((memb_id = H5Tget_member_type(memtype, u)) < 0) {
                err_msg =  "Fail to obtain the datatype of an HDF5 compound datatype member.";
                ret_value = false;
                break;
            }
        
            // Get member type class
            if((memb_cls = H5Tget_member_class (memtype, u)) < 0) {
                H5Tclose(memb_id);
                err_msg =  "Fail to obtain the datatype class of an HDF5 compound datatype member.";
                ret_value = false;
                break;
            }

            size_t memb_size = H5Tget_size(memb_id);
        
            // Get member offset,H5Tget_member_offset only fails
            // when H5Tget_memeber_class fails. Sinc H5Tget_member_class
            // is checked above. So no need to check the return value.
            memb_offset= H5Tget_member_offset(memtype,u);

            // Here we have the offset from the original structure variable.
            values_offset = struct_elem_offset + memb_offset; 
            if (memb_cls ==  H5T_ARRAY) {

                hid_t at_base_type = H5Tget_super(memb_id);
                size_t at_base_type_size = H5Tget_size(at_base_type);
                H5T_class_t array_cls = H5Tget_class(at_base_type);

                // Need to retrieve the number of elements of the array
                // and encode each string with base64 and then separate them 
                // with ";".
                // memb_id, obtain the number of dimensions
                int at_ndims = H5Tget_array_ndims(memb_id);
                if (at_ndims <= 0) {
                    H5Tclose(at_base_type);
                    H5Tclose(memb_id);
                    err_msg =  "Fail to obtain number of dimensions of the array datatype.";
                    ret_value = false;
                    break;
                }
            
                vector<hsize_t>at_dims_h(at_ndims,0);
            
                // Obtain the number of elements for each dims
                if (H5Tget_array_dims(memb_id,at_dims_h.data())<0) {
                    H5Tclose(at_base_type);
                    H5Tclose(memb_id);
                    err_msg =  "Fail to obtain each imension size of the array datatype.";
                    ret_value = false;
                    break;
                }

                vector<hsize_t>at_dims_offset(at_ndims,0);                   
                size_t total_array_nums = 1;
                for (const auto & ad:at_dims_h)
                    total_array_nums *=ad;

                if (array_cls == H5T_STRING) {

                    vector<string> str_val;
                    str_val.resize(total_array_nums);
                    
                    if (H5Tis_variable_str(at_base_type) >0){
                        auto src = (void*)(struct_value.data()+values_offset);
                        auto temp_bp =(char*)src;
                        for (int64_t i = 0;i <total_array_nums; i++){
                            string tempstrval;
                            get_vlen_str_data(temp_bp,tempstrval);
                            str_val[i] = tempstrval;
                            temp_bp += at_base_type_size;
                        }
                    }
                    else {
                        auto src = (void*)(struct_value.data()+values_offset);
                        vector<char> fix_str_val;
                        fix_str_val.resize(total_array_nums*at_base_type_size);
                        memcpy((void*)fix_str_val.data(),src,total_array_nums*at_base_type_size);
                        string total_in_one_string(fix_str_val.begin(),fix_str_val.end());
                        for (int64_t i = 0; i<total_array_nums;i++)
                            str_val[i] = total_in_one_string.substr(i*at_base_type_size,at_base_type_size);
                    }
                    vector<string> encoded_str_val;
                    encoded_str_val.resize(str_val.size());

                    // "Matthew John;James Peter" becomes "base64(Matthew);base64(John;James);base64(Peter);"
                    for (int i = 0; i < str_val.size(); i++) {
                          string temp_str = str_val[i];
                          vector<u_int8_t>temp_val(temp_str.begin(),temp_str.end());
                          encoded_str_val[i] =  base64::Base64::encode(temp_val.data(), temp_str.size()) + ";";
                   
                    }
                    // TODO: use memcpy or other more efficient method later. We expect the size is not big.
                    for (const auto &es_val:encoded_str_val) {
                        string temp_str = es_val;
                        for(const auto &ts:temp_str) 
                            encoded_struct_value.push_back(ts);
                    }

                }
                else { // integer or float array, just obtain the whole value.
                    vector<char> int_float_array;
                    int_float_array.resize(total_array_nums*at_base_type_size);
                    memcpy((void*)int_float_array.data(),struct_value.data()+values_offset,total_array_nums*at_base_type_size);
                    for (const auto &int_float:int_float_array) 
                        encoded_struct_value.push_back(int_float);
                }
                H5Tclose(at_base_type);

            }
            else if (memb_cls == H5T_STRING) {// Scalar string

                string encoded_str;

                if (H5Tis_variable_str(memb_id) >0){
                    auto src = (void*)(struct_value.data()+values_offset);
                    auto temp_bp =(char*)src;
                    string tempstrval;
                    get_vlen_str_data(temp_bp,tempstrval);
                    vector<u_int8_t>temp_val(tempstrval.begin(),tempstrval.end());
                    encoded_str =  base64::Base64::encode(temp_val.data(), tempstrval.size()) + ";";

                }
                else {
                    auto src = (void*)(struct_value.data()+values_offset);
                    vector<char> fix_str_val;
                    fix_str_val.resize(memb_size);
                    memcpy((void*)fix_str_val.data(),src,memb_size);
                    string fix_str_value(fix_str_val.begin(),fix_str_val.end());
                    vector<u_int8_t>temp_val(fix_str_value.begin(),fix_str_value.end());
                    encoded_str =  base64::Base64::encode(temp_val.data(), fix_str_value.size()) + ";";
                }
                for (const auto &es:encoded_str)
                        encoded_struct_value.push_back(es);
 
            }
            else {// Scalar int/float
                vector<char> int_float;
                int_float.resize(memb_size);
                memcpy((void*)int_float.data(),struct_value.data()+values_offset,memb_size);
                    int_float.resize(memb_size);
                    memcpy((void*)int_float.data(),struct_value.data()+values_offset,memb_size);
                    for (const auto &int_f:int_float) 
                        encoded_struct_value.push_back(int_f);
            }

        } // end "for(unsigned u = 0)"

        if (ret_value == false) 
            break;
    } // end "for (int element=0"

    return ret_value;

}

void process_string_in_structure(hid_t dataset, hid_t type_id, BaseType *btp) {

    hid_t memtype  = -1;
    size_t ty_size = -1;

    bool is_scalar = false;

    if ((memtype = H5Tget_native_type(type_id, H5T_DIR_ASCEND))<0) 
        throw InternalErr (__FILE__, __LINE__, "Fail to obtain memory datatype.");

    ty_size = H5Tget_size(memtype);

    hid_t dspace = -1;
    if ((dspace = H5Dget_space(dataset))<0) {
        H5Tclose(memtype);
        throw InternalErr (__FILE__, __LINE__, "Cannot obtain data space.");
    }

    hssize_t num_elms = H5Sget_simple_extent_npoints(dspace);
    if (num_elms < 0) {
        H5Tclose(memtype);
        H5Sclose(dspace);
        throw InternalErr (__FILE__, __LINE__, "Cannot obtain the number of elements of the data space.");
    }

    vector<char> struct_value;
    struct_value.resize(num_elms*ty_size);
    if (H5Dread(dataset,memtype, H5S_ALL,H5S_ALL,H5P_DEFAULT,(void*)struct_value.data())<0) {
        H5Tclose(memtype);
        H5Sclose(dspace);
        throw InternalErr (__FILE__, __LINE__, "Cannot read the dataset.");
    }

    if (H5S_SCALAR == H5Sget_simple_extent_type(dspace))
        is_scalar = true;

    H5Sclose(dspace);

    bool ret_value = false;
    string err_msg;
    if (is_scalar) {
        auto ds = dynamic_cast<DmrppStructure *>(btp);
        vector<char> & ds_buffer = ds->get_structure_str_buffer();
        ret_value = obtain_structure_string_value(memtype,ty_size,num_elms,ds_buffer,struct_value,err_msg);
        ds->set_special_structure_flag(true);
        ds->set_read_p(true);
    }
    else {
        auto da = dynamic_cast<DmrppArray *>(btp);
        vector<char> &da_buffer = da->get_structure_array_str_buffer();
        ret_value = obtain_structure_string_value(memtype,ty_size,num_elms,da_buffer,struct_value,err_msg);
        da->set_special_structure_flag(true);
        da->set_read_p(true);
    }

    H5Tclose(memtype);
    if (ret_value == false) 
        throw InternalErr (__FILE__, __LINE__, err_msg);

}

//Note: the error handling part may be improved in the future.
bool handle_vlen_float_int_internal(hid_t dset_id, BaseType *btp) {

    hid_t vlen_type = H5Dget_type(dset_id);
    hid_t vlen_basetype = H5Tget_super(vlen_type);
    if (H5Tget_class(vlen_basetype) != H5T_INTEGER && H5Tget_class(vlen_basetype) != H5T_FLOAT) {
        H5Dclose(dset_id);
        throw InternalErr(__FILE__, __LINE__,"Only support float or intger variable-length datatype.");
    }

    hid_t vlen_base_memtype = H5Tget_native_type(vlen_basetype, H5T_DIR_ASCEND);
    hid_t vlen_memtype = H5Tvlen_create(vlen_base_memtype);

    // Will not support the scalar type. 
    hid_t vlen_space = H5Dget_space(dset_id);
    if (H5Sget_simple_extent_type(vlen_space) != H5S_SIMPLE) {
        H5Dclose(dset_id);
        throw InternalErr(__FILE__, __LINE__,"Only support array of float or intger variable-length datatype.");
    }

    hssize_t vlen_number_elements = H5Sget_simple_extent_npoints(vlen_space);
    vector<hvl_t> vlen_data(vlen_number_elements);
    if (H5Dread(dset_id, vlen_memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, vlen_data.data()) <0) {
        H5Dclose(dset_id);
        throw InternalErr(__FILE__, __LINE__,"Cannot read variable-length datatype data.");
    }

    auto da = dynamic_cast<DmrppArray *>(btp);
    if (!da) {
        string err_msg = "Expected to find a DmrppArray instance but did not in handle_vlen_float_int_internal().";
        throw BESInternalError(err_msg, __FILE__, __LINE__);
    }
    switch (da->var()->type()) {
        case dods_byte_c:
        case dods_uint8_c:
        case dods_char_c:
        case dods_int8_c:
        case dods_int16_c:
        case dods_uint16_c:
        case dods_int32_c:
        case dods_uint32_c:
        case dods_int64_c:
        case dods_uint64_c:
        case dods_float32_c:
        case dods_float64_c: {
            // Retrieve the last dimension size.
            libdap::Array::Dim_iter last_dim_iter = da->dim_end()-1;
            int64_t last_dim_size = da->dimension_size(last_dim_iter);
            size_t bytes_per_element = da->var()->width_ll();
            size_t total_data_buf_size = da->get_size(false)*bytes_per_element;
            vector<char> data_buf(total_data_buf_size,0);
            char *temp_data_buf_ptr = data_buf.data();

            for (ssize_t i = 0; i < vlen_number_elements; i++) {

                size_t vlen_element_size = vlen_data[i].len * bytes_per_element;

                // Copy the vlen data to the data buffer.
                memcpy(temp_data_buf_ptr,vlen_data[i].p,vlen_element_size);

                // Move the data buffer pointer to the next element.
                // In this regular array, the rest data will be filled with zero.
                temp_data_buf_ptr += last_dim_size*bytes_per_element;
                
            }
            da->val2buf(data_buf.data());
            da->set_missing_data(true);
            da->set_read_p(true);
 
            break;
        }
        default:
            throw InternalErr(__FILE__, __LINE__, "Vector::val2buf: bad type");
    }

    H5Dvlen_reclaim(vlen_memtype, vlen_space, H5P_DEFAULT, (void*)(vlen_data.data()));
    H5Sclose(vlen_space);
    H5Tclose(vlen_base_memtype);
    H5Tclose(vlen_basetype);
    H5Tclose(vlen_type);
    H5Tclose(vlen_memtype);

    return true;
    
}

bool handle_vlen_float_int(hid_t dataset, BaseType *btp) {

    bool ret_value = false;
    hid_t type_id = H5Dget_type(dataset);
    if (H5Tget_class(type_id) == H5T_VLEN) 
        ret_value = handle_vlen_float_int_internal(dataset,btp);
    H5Tclose(type_id);
    return ret_value;
}

void handle_vlen_float_int_index(hid_t file, BaseType *btp) {

    string vlen_index_name = btp->FQN();
    size_t vlen_name_pos = vlen_index_name.rfind("_vlen_index");
    if (vlen_name_pos == string::npos) {
        string err_msg = vlen_index_name + " is not a variable length index variable name.";
        H5Fclose(file);
        throw BESInternalError(err_msg, __FILE__, __LINE__);
    }

    string vlen_name = vlen_index_name.substr(0,vlen_name_pos);

    H5Eset_auto2(H5E_DEFAULT, nullptr, nullptr); 
    hid_t dset_id = H5Dopen2(file, vlen_name.c_str(), H5P_DEFAULT);
    if (dset_id < 0) 
        throw BESInternalError("HDF5 vlen dataset '" + vlen_name + "' cannot be opened.", __FILE__, __LINE__);
   
    hid_t vlen_type = H5Dget_type(dset_id);
    hid_t vlen_basetype = H5Tget_super(vlen_type);
    if (H5Tget_class(vlen_basetype) != H5T_INTEGER && H5Tget_class(vlen_basetype) != H5T_FLOAT) {
        H5Dclose(dset_id);
        throw InternalErr(__FILE__, __LINE__,"Only support float or intger variable-length datatype.");
    }

    hid_t vlen_base_memtype = H5Tget_native_type(vlen_basetype, H5T_DIR_ASCEND);
    hid_t vlen_memtype = H5Tvlen_create(vlen_base_memtype);

    // Will not support the scalar type. 
    hid_t vlen_space = H5Dget_space(dset_id);
    if (H5Sget_simple_extent_type(vlen_space) != H5S_SIMPLE) {
        H5Dclose(dset_id);
        throw InternalErr(__FILE__, __LINE__,"Only support array of float or intger variable-length datatype.");
    }

    hssize_t vlen_number_elements = H5Sget_simple_extent_npoints(vlen_space);
    vector<hvl_t> vlen_data(vlen_number_elements);
    if (H5Dread(dset_id, vlen_memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, vlen_data.data()) <0) {
        H5Dclose(dset_id);
        throw InternalErr(__FILE__, __LINE__,"Cannot read variable-length datatype data.");
    }

    auto da = dynamic_cast<DmrppArray *>(btp);
    if (!da) {
        H5Dclose(dset_id);
        string err_msg = "Expected to find a DmrppArray instance but did not in handle_vlen_float_int_internal().";
        throw BESInternalError(err_msg, __FILE__, __LINE__);
    }
    if (da->var()->type() != dods_int32_c) {
        H5Dclose(dset_id);
        string err_msg = "vlen_index datatype must be 32-bit integer.";
        throw BESInternalError(err_msg, __FILE__, __LINE__);
    }
    vector<int> vlen_index_data;
    for (ssize_t i = 0; i<vlen_number_elements; i++) 
        vlen_index_data.push_back(vlen_data[i].len);
    da->set_value_ll(vlen_index_data.data(),vlen_number_elements);   
    da->set_missing_data(true);
    da->set_read_p(true);

    H5Dvlen_reclaim(vlen_memtype, vlen_space, H5P_DEFAULT, (void*)(vlen_data.data()));
    H5Sclose(vlen_space);
    H5Tclose(vlen_base_memtype);
    H5Tclose(vlen_basetype);
    H5Tclose(vlen_type);
    H5Tclose(vlen_memtype);
    H5Dclose(dset_id);

}
/**
 * @brief Get chunk information for a HDF5 dataset in a file
 *
 * @param file The open HDF5 file
 * @param h5_dset_path The path name of the dataset in the open hdf5 file
 * @param dataset The open HDF5 dataset object
 * @param dc if not null, put the information in this variable (DmrppCommon)
 *
 * @exception BESError is thrown on error.
 */
static void get_variable_chunk_info(hid_t dataset, BaseType *btp, bool disable_dio) {

    if(verbose) {
        string type_name = btp->type_name();
        if (btp->type() == dods_array_c) {
            auto array = toDA(btp);
            type_name = array->var()->type_name();
        }
        cerr << prolog << "Processing dataset/variable: " << type_name << " " << btp->name() << endl;
    }

    if (true == handle_vlen_float_int(dataset,btp))
        return;

    // Added support for HDF5 Fill Value. jhrg 4/22/22
    set_fill_value(dataset, btp);

    // Here we want to check if this dataset is a compound datatype 
    hid_t type_id = H5Dget_type(dataset);
    if (type_id <0) {
        string err_msg = "Cannot obtain the HDF5 data type of the dataset: " + btp->name() ;
        throw BESInternalError(err_msg, __FILE__, __LINE__);
    }
    if (H5T_COMPOUND == H5Tget_class(type_id)) {

        unsigned short supported_compound_type = is_supported_compound_type(type_id);
        if (supported_compound_type ==2) {
            // When the compound datatype contains string, we need to process this dataset differently.
            process_string_in_structure(dataset,type_id, btp);
            H5Tclose(type_id);
            return;
        }
        else if (supported_compound_type ==1) {

            auto layout_type = get_h5_storage_layout(dataset);

            // For contiguous or chunk storage layouts, the compound member offset and size need to be saved.
            if (layout_type != H5D_COMPACT) {

                vector<unsigned int> struct_offsets;
                obtain_structure_offset(dataset,struct_offsets);
                VERBOSE(cerr << prolog << "struct_offsets[0]: " << struct_offsets[0]<< endl);
                // Add struct offset
                auto dc = toDC(btp);
                dc->set_struct_offsets(struct_offsets);
            }
        }
    }
    else 
       H5Tclose(type_id);

    auto layout_type = get_h5_storage_layout(dataset);

    switch (layout_type) {
        case H5D_CONTIGUOUS: { /* Contiguous Storage Layout */
            process_contiguous_layout_dariable(dataset, btp);
            break;
        }
        case H5D_CHUNKED: { /* Chunked Storage Layout */
            process_chunked_layout_dariable(dataset, btp, disable_dio);
            break;
        }
        case H5D_COMPACT: { /* Compact Storage Layout */
            process_compact_layout_dariable(dataset,btp);
            break;
        }
        default:
            ostringstream oss("Unsupported HDF5 dataset layout type: ", std::ios::ate);
            oss << layout_type << ".";
            throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }
}


/**
 * Builds a string that represents the variable's DDSish declaration.
 * @param btp
 * @return
 */
string get_type_decl(BaseType *btp){
    stringstream type_decl;
    if(btp->type() == libdap::dods_array_c){
        auto array = toDA(btp);
        type_decl << array->var()->type_name() << " " << btp->FQN();
        for(auto dim_itr = array->dim_begin(); dim_itr!=array->dim_end(); dim_itr++){
            auto dim = *dim_itr;
            type_decl << "[";
            if(!dim.name.empty()){
                type_decl << dim.name << "=";
            }
            type_decl << dim.size << "]";
        }
    }
    else {
        type_decl << btp->type_name() << " " << btp->FQN();
    }
    return type_decl.str();
}


bool is_unsupported_type(hid_t dataset_id, BaseType *btp, string &msg){
    VERBOSE(cerr << prolog << "BEGIN " << get_type_decl(btp) << endl);

    bool is_unsupported = false;
    hid_t h5_type_id = H5Dget_type(dataset_id);
    H5T_class_t class_type = H5Tget_class(h5_type_id);

    bool isArray = btp->type() == dods_array_c;

    switch (class_type) {
        case H5T_STRING: {
            if (H5Tis_variable_str(h5_type_id) && isArray) {
                stringstream msgs;
                msgs << "UnsupportedTypeException: Your data contains the dataset/variable: ";
                msgs << get_type_decl(btp) << " ";
                msgs << "which the underlying HDF5/NetCDF-4 file has stored as a";
                msgs << (isArray?"n array of ":" ");
                msgs << "variable length string";
                msgs << (isArray?"s (AVLS). ":". ");
                msgs << "This data architecture is not currently supported by ";
                msgs << "the dmr++ creation machinery. One solution available to you is to rewrite the granule ";
                msgs << "so that these arrays are represented as arrays of fixed length strings (AFLS). While ";
                msgs << "these may not be as 'elegant' as AVLS, the ragged ends of the AFLS compress well, so ";
                msgs << "the storage penalty is minimal.";
                msg = msgs.str();
                is_unsupported = false;
            }
            break;
        }
        case H5T_ARRAY: {
            stringstream msgs;
            msgs << "UnsupportedTypeException: Your data contains the dataset/variable: ";
            msgs << get_type_decl(btp) << " ";
            msgs << "which the underlying HDF5/NetCDF-4 file has stored as an array of H5T_ARRAY. ";
            msgs << "This is not yet supported by the dmr++ creation machinery.";
            msg = msgs.str();
            is_unsupported = true;
            break;
        }
        case H5T_COMPOUND: {
            unsigned short supported_compound_type = is_supported_compound_type(h5_type_id);
            if (supported_compound_type == 0) {
                stringstream msgs;
                msgs << "UnsupportedTypeException: Your data contains the dataset/variable: ";
                msgs << get_type_decl(btp) << " ";
                msgs << "which the underlying HDF5/NetCDF-4 file has stored as an HDF5 compound datatype and ";
                msgs << "the basetype of the compound datatype is not integer or float. ";
                msgs << "This is not yet supported by the dmr++ creation machinery.";
                msg = msgs.str();
                is_unsupported = true;
            }

            break;
        }
        case H5T_VLEN: {
            bool supported_vlen_type = is_supported_vlen_type(dataset_id,h5_type_id);
            if (supported_vlen_type == false) {
                stringstream msgs;
                msgs << "UnsupportedTypeException: Your data contains the dataset/variable: ";
                msgs << get_type_decl(btp) << " ";
                msgs << "which the underlying HDF5/NetCDF-4 file has stored as an HDF5 vlen datatype and ";
                msgs << "the basetype of the vlen datatype is not integer or float. ";
                msgs << "This is not yet supported by the dmr++ creation machinery.";
                msg = msgs.str();
                is_unsupported = true;
            }

            break;

        }

        default:
            break;
    }
    VERBOSE(cerr << prolog << "END  is_unsupported: " << (is_unsupported?"true":"false") << endl);
    return is_unsupported;
}


/**
 * @brief Identifies, reads, and then stores a vlss in a DAP dmr++ variable using the compact representation
 *
 * @param dataset The HDF5 dataset id.
 * @param btp The BaseType pointer to the sister DAP class
 * @return Returns true if the variable was processed, false otherwise.
 */
bool process_variable_length_string_scalar(const hid_t dataset, BaseType *btp){

    // btp->type() == dods_str_c means a scalar string, if it was an
    // array of strings then btp->type() == dods_array_c would be true
    if(btp->type() != dods_str_c) {
        return false;
    }

    auto h5_type_id = H5Dget_type(dataset);
    if(H5Tis_variable_str(h5_type_id) <= 0) {
        return false; // Not a variable length string, so, again, not our problem.
    }

    VERBOSE(cerr << prolog << "Processing VLSS: " << btp->FQN() << "\n");

    vector<string> vls_values;   // passed by reference to read_vlen_string
    vls_values.emplace_back(""); // initialize array for it's trip to Cville

    // Read the scalar string.
    read_vlen_string(dataset, 1, nullptr, nullptr, nullptr, vls_values);
    string vlss = vls_values[0];
    VERBOSE(cerr << prolog << " read_vlen_string(): " << vlss << endl);

    // Convert variable to a compact representation
    // so that its value can be stored in the dmr++
    auto dc = toDC(btp);
    dc->set_compact(true);

    // And then set the value.
    auto str = dynamic_cast<libdap::Str *>(btp);
    str->set_value(vlss);
    str->set_read_p(true);

    return true;


}

/**
 * @brief Identifies, reads, and then stores a VLSS in a DAP dmr++ variable using the compact representation
 *
 * @param dataset The HDF5 dataset id.
 * @param btp The BaseType pointer to the sister DAP class
 * @return Returns true if the variable was processed, false otherwise.
 */
bool process_variable_length_string_array(const hid_t dataset, BaseType *btp){

    if(btp->type() != dods_array_c) {
        return false; // Not an array, not our problem...
    }
    auto dap_array = toDA(btp);
    if(!dap_array){
        throw BESInternalError("Malformed DAP object " + btp->FQN() +
        " Identifies as dods_array_c but cast to DmrppArray fails!", __FILE__, __LINE__);
    }

    if(dap_array->prototype()->type() != dods_str_c){
        return false; // Not a string, not our problem...
    }

    hid_t h5_type_id = H5Dget_type(dataset);
    if(H5Tis_variable_str(h5_type_id) <= 0) {
        return false;  // Not a variable length string, so, again, not our problem.
    }
    VERBOSE(cerr << prolog << "h5_type_id: " << h5_type_id << "\n");

    dap_array->set_is_vlsa(true);
    VERBOSE(cerr << prolog << "Processing VLSA: " << dap_array->FQN() << "\n");

    auto dspace = H5Dget_space(dataset);

    int ndims = H5Sget_simple_extent_ndims(dspace);
    VERBOSE(cerr << prolog << "ndims: " << ndims << "\n");

    vector<hsize_t>count(ndims,0);
    if(H5Sget_simple_extent_dims(dspace, count.data(), nullptr) < 0){
        H5Sclose(dspace);
        H5Tclose(h5_type_id);
        H5Dclose(dataset);
        throw BESInternalError("Failed to get hdf5 count for variable: " + btp->FQN(), __FILE__, __LINE__);
    }

    stringstream msg;
    msg << "count[]: ";
    for(int i=0; i<ndims; i++) {
        if(i) msg << ",";
        msg << count[i];
    }
    VERBOSE(cerr << prolog << msg.str() << "\n");

    vector<hsize_t> offset(ndims,0);
    for(int i=0; i<ndims; i++)
        offset.emplace_back(0);


    // The following line causes an issue on a 1-element VLSA. The num_elements becomes 0.
    // See https://bugs.earthdata.nasa.gov/browse/HYRAX-1538
#if 0
    //uint64_t num_elements = dap_array->get_size(false);
#endif 
    hssize_t num_elements = H5Sget_simple_extent_npoints(dspace);
    if (num_elements < 0) {
        H5Sclose(dspace);
        H5Tclose(h5_type_id);
        H5Dclose(dataset);
        throw BESInternalError("Failed to obtain the number of elements for the variable : " + btp->FQN(), __FILE__, __LINE__);
    }

    VERBOSE(cerr << prolog << "num_elements: " << num_elements << "\n");

    vector<string> vls_values(num_elements,"");
    read_vlen_string(dataset,
                     num_elements,
                     offset.data(),
                     nullptr,
                     count.data(),
                     vls_values);

#ifndef NDEBUG
    VERBOSE(cerr << prolog << " vls_values.size(): " << vls_values.size() << "\n");
    uint64_t indx = 0;
    for (const auto &sval: vls_values) {
        VERBOSE(cerr << prolog << " vls_values[" << to_string(indx++) << "]: '" << sval << "'\n");
    }
#endif

    dap_array->set_value(vls_values,(int) vls_values.size());
    dap_array->set_read_p(true);

    return true;
}

bool check_enable_cf_fake_cv(BaseType *btp, const string& FQN) {

    bool ret_value = false;
    if (FQN.find_last_of('/')==0) {
        if (btp->type() == dods_array_c) {
            auto da = dynamic_cast<DmrppArray *>(btp);
            if (!da) {
                string err_msg = "Expected to find a DmrppArray instance but did not in check_enable_cf_fake_cv().";
                throw BESInternalError(err_msg, __FILE__, __LINE__);
            }
            // Must be 1-D floating data.
            if (btp->var()->type() == dods_float32_c && da->dimensions() == 1) {
                // Must not have attributes and the dimension name is the same as the variable name(FQN).
                const D4Attributes *d4_attrs = btp->attributes();
                if (d4_attrs) { 
                    if (d4_attrs->empty()) {
                        // Now we can check dimension name.
                        Array::Dim_iter da_dim = da->dim_begin();
                        if (da->dimension_name(da_dim) == btp->name())
                            ret_value = true;
                    }
                }
            }
        }
    }
    return ret_value;
}

/**e
 * @brief This helper function for get_chunks_for_all_variables() opens the HDF5 dataset and returns the dataset_id
 * @param file The HDF5 file to examine
 * @param btp The associated DAP variable
 * @param nc4_non_coord_candidate
 * @return The open dataset_id
 */
hid_t get_h5_dataset_id(hid_t file, BaseType *btp, const unordered_set<string> &nc4_non_coord_candidate) {
    D4Attributes *d4_attrs = btp->attributes();
    if (!d4_attrs)
        throw BESInternalError("Expected to find an attribute table for " + btp->name() + " but did not.",
                               __FILE__, __LINE__);

    // Look for the full name path for this variable
    // If one was not given via an attribute, use BaseType::FQN() which
    // relies on the variable's position in the DAP dataset hierarchy.
    const D4Attribute *attr = d4_attrs->get("fullnamepath");
        // I believe the logic is more clear in this way:
        // If fullnamepath exists and the H5Dopen2 fails to open, it should throw an error.
        // If fullnamepath doesn't exist, we should ignore the error as the reason described below:
        // (However, we should suppress the HDF5 dataset open error message.)  KY 2019-12-02
        // It's not an error if a DAP variable in a DMR from the hdf5 handler
        // doesn't exist in the file _if_ there's no 'fullnamepath' because
        // that variable was synthesized (likely for CF compliance)
    hid_t dataset = -1;
    if (attr) {
        string FQN;
        if (attr->num_values() == 1)
            FQN = attr->value(0);
        else
            FQN = btp->FQN();

        VERBOSE(cerr << prolog << "Working on: " << FQN << endl);
        // Here we have a case to handle the netCDF-4 file coming from the fileout netCDF-4 module.
        // The fullnamepath is kept to remember the original HDF5 file, but for the netCDF-4 file generated
        // from the fileout netCDF-4 module, this is no longer the case. The CF option makes everything flattened.
        // So if the H5Dopen2 fails with the name obtained from the fullnamepath attribute, 
        // we should directly open the variable with the name.

        H5Eset_auto2(H5E_DEFAULT, nullptr, nullptr);
        dataset = H5Dopen2(file, FQN.c_str(), H5P_DEFAULT);
        if (dataset < 0) {
            // We have one more try with the variable name before throwing an error.
            dataset = H5Dopen2(file,btp->name().c_str(),H5P_DEFAULT);           
            if (dataset <0) 
                throw BESInternalError("HDF5 dataset '" + FQN + "' cannot be opened.", __FILE__, __LINE__);
        }
    }
    else {
        // The current design seems to still prefer to open the dataset when the fullnamepath doesn't exist
        // So go ahead to open the dataset. Continue even if the dataset cannot be open. KY 2019-12-02
        //
        // A comment from an older version of the code:
        // It's not an error if a DAP variable in a DMR from the hdf5 handler
        // doesn't exist in the file _if_ there's no 'fullnamepath' because
        // that variable was synthesized (likely for CF compliance)
        H5Eset_auto2(H5E_DEFAULT, nullptr, nullptr);
        string FQN = btp->FQN();
        if (nc4_non_coord_candidate.find(btp->name()) != nc4_non_coord_candidate.end()) {
            string real_name_candidate = "_nc4_non_coord_" + btp->name();
            size_t fqn_last_fslash_pos = btp->FQN().find_last_of('/');
            string real_path_candidate = btp->FQN().substr(0, fqn_last_fslash_pos + 1) + real_name_candidate;
            dataset = H5Dopen2(file, real_path_candidate.c_str(), H5P_DEFAULT);
        }

        // Here we need to handle a special case for a dmr file generated by the EnableCF option in the HDF5 handler.
        // The netCDF-4's pure dimension is mapped to a fake coordinate by the EnableCF option and is ignored by the default option.
        // However, netCDF-4 still stores this dimension as an HDF5 variable with 0 values. The EnableCF option replaces
        // those 0 values with 0,1,2... as the fake coordinate. So here we need to ignore this kind of variables and let the 
        // code after this call to handle it as the EnableCF required.
            
        VERBOSE(cerr << prolog << "Working on: " << FQN << endl);
        if (dataset < 0) {
            dataset = H5Dopen2(file, FQN.c_str(), H5P_DEFAULT);
            if (dataset < 0) {
                VERBOSE(cerr << prolog << "WARNING: HDF5 dataset '" << FQN << "' cannot be opened." << endl);
            }
            else if(check_enable_cf_fake_cv(btp, FQN) == true)
                dataset = -1;
        }
    }
    return dataset;
}

/**
 * netCDF-4 variable can share the same name as a pure dimension name.
 * When this case happens, the netCDF-4 will add "_nc4_non_coord_" to the variable name to
 * distinguish the variable name from the correponding dimension name when storing in the HDF5.
 * To emulate netCDF-4 model, the HDF5 handler will remove the "_nc4_non_coord_" from the variable in dmr.
 * When obtaining this variable's information, we need to use the real HDF5 variable name.
 * KY 2022-09-11
 * When the above case occurs, a dimension name of this group must share with a variable name.
 * So we will find these variables and put them into an unodered set.
 *
 * @param group
 * @param nc4_non_coord_candidate
 */
void mk_nc4_non_coord_candidates(D4Group *group, unordered_set<string> &nc4_non_coord_candidate){

    // First obtain  dimension names.
    unordered_set<string> dimname_list;
    D4Dimensions *grp_dims = group->dims();

    if (grp_dims) {
        for (auto di = grp_dims->dim_begin(), de = grp_dims->dim_end(); di != de; ++di)
            dimname_list.insert((*di)->name());
    }

    if (!dimname_list.empty()) {
        // Then find the nc4_non_coord candidate variables,
        for (auto btp = group->var_begin(), ve = group->var_end(); btp != ve; ++btp) {
            if (dimname_list.find((*btp)->name())!=dimname_list.end())
                nc4_non_coord_candidate.insert((*btp)->name());
        }
    }

}


/**
 * @brief Iterate over all the variables in a DMR and get their chunk info
 *
 * @param file The open HDF5 file; passed through to get_variable_chunk_info
 * @param group Read variables from this DAP4 Group. Call with the root Group
 * to process all the variables in the DMR
 */
void get_chunks_for_all_variables(hid_t file, D4Group *group, bool disable_dio) {

    unordered_set<string> nc4_non_coord_candidate;
    mk_nc4_non_coord_candidates(group,nc4_non_coord_candidate);

    // variables in the group

    for(auto btp : group->variables()) {
        VERBOSE(cerr << prolog << "-------------------------------------------------------" << endl);
        VERBOSE(cerr << prolog);
        VERBOSE(btp->print_decl(cerr,"",false,false,false) );
        VERBOSE(cerr << endl);

        auto dataset  = get_h5_dataset_id(file, btp, nc4_non_coord_candidate);
        if(dataset > 0) {
            // If we have a valid dataset then we have a variable with data. I think.
            // If it's not valid we skip it, I think because the associated BaseType,
            // btp, may be a semantic object, like a dimension,  with no data
            // associated with it.
            try {
                string msg;
                if (is_unsupported_type(dataset, btp, msg)) {
                    throw UnsupportedTypeException(msg);
                }

                if (!process_variable_length_string_scalar(dataset, btp) && !process_variable_length_string_array(dataset,btp)) {

                    VERBOSE(cerr << prolog << "Building chunks for: " << get_type_decl(btp) << endl);
                    get_variable_chunk_info(dataset, btp, disable_dio);

                    VERBOSE(cerr << prolog << "Annotating String Arrays as needed for: " << get_type_decl(btp) << endl);
                    add_string_array_info(dataset, btp);
                }
                H5Dclose(dataset);
            }
            catch (...) {
                H5Dclose(dataset);
                throw;
            }
        }
        else {
            VERBOSE(cerr << prolog << "Unable to open " << btp->FQN()
            << " with the hdf5 api. Skipping chunk production. "
            << "Need to check if we need to embed the data to the dmrpp file." << endl);

            // Currently we only check if this is the artificial coordinate added by the HDF5 handler.
            D4Attributes *d4_attrs = btp->attributes();
            if (d4_attrs==nullptr)
                return;
            
            if (d4_attrs->empty() == false) {

                D4Attribute *attr = d4_attrs->find("units");
                if (attr) {
                    string attr_value = attr->value(0);
                    if (attr_value == "level") {
                        auto dc = dynamic_cast<DmrppCommon *>(btp);
                        if (!dc) {
                            string err_msg = "Expected to find a DmrppCommon instance but did not in get_chunks_for_all_variables().";
                            throw BESInternalError(err_msg, __FILE__, __LINE__);
                        }
                        auto da = dynamic_cast<DmrppArray *>(btp);
                        if (!da) {
                            string err_msg = "Expected to find a DmrppArray instance but did not in get_chunks_for_all_variables().";
                            throw BESInternalError(err_msg, __FILE__, __LINE__);
                        }
                    
                        if (da->dimensions() ==1 && btp->var()->type() == dods_int32_c){
                
                            vector<int> level_value;
                            level_value.resize((size_t)(da->length()));
                            for (int32_t i = 0; i <da->length(); i++) 
                                level_value[i] = i;
                
                            da->set_value(level_value.data(),da->length());
                            da->set_missing_data(true);
                            da->set_read_p(true);
                        }
                    }
                }
                attr = d4_attrs->find("orig_datatype");
                if (attr) {
                    string attr_value = attr->value(0);
                    if (attr_value == "VLEN_INDEX") {
                        // This is a vlen index variable. We need to find the corresponding vlen variable.
                         handle_vlen_float_int_index(file,btp);
                    }
                }
            }
            else {// The coordinate of the netCDF-4 pure dimension added by HDF5 handler's CF option doesn't have any attribute.
                // Check if this variable is 1-D floating-point array.
                if (btp->type() == dods_array_c) {

                   auto da = dynamic_cast<DmrppArray *>(btp);
                   if (!da) {
                       string err_msg = "Expected to find a DmrppArray instance but did not in get_chunks_for_all_variables().";
                       throw BESInternalError(err_msg, __FILE__, __LINE__);
                   }
               
                   if (da->dimensions() ==1 && btp->var()->type() == dods_float32_c){
           
                       da->set_missing_data(true);
            
                       vector<float> level_value;
                       level_value.resize((size_t)(da->length()));
                       for (int32_t i = 0; i <da->length(); i++) 
                           level_value[i] = i;
           
                       da->set_value(level_value.data(),da->length());
                       da->set_missing_data(true);
                       da->set_read_p(true);
                   }
                   
                }
            }
        }
    }

    // all groups in the group
    for(auto g:group->groups()) {
        get_chunks_for_all_variables(file, g,disable_dio);
    }

}

/**
 * @brief Add chunk information about to a DMRpp object
 * @param h5_file_name Read information from this file
 * @param dmrpp Dump the chunk information here
 */
void add_chunk_information(const string &h5_file_name, DMRpp *dmrpp, bool disable_dio)
{
    // Open the hdf5 file
    hid_t file = H5Fopen(h5_file_name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file < 0) {
        stringstream msg;
        msg << "Error: HDF5 file '" << h5_file_name << "' cannot be opened." << endl;
        throw BESNotFoundError(msg.str(), __FILE__, __LINE__);
    }

    // iterate over all the variables in the DMR
    try {
        get_chunks_for_all_variables(file, dmrpp->root(), disable_dio);
        H5Fclose(file);
    }
    catch (...) {
        H5Fclose(file);
        throw;
    }
}


/**
 * @brief Performs a quality control check on the user supplied data file.
 *
 * The supplied file is going to be used by build_dmrpp as the source of variable/dataset chunk information.
 * At the time of this writing only netcdf-4 and hdf5 file encodings are supported (Note that netcdf-4 is a subset of
 * hdf5 and all netcdf-4 files are de facto hdf5 files.)
 *
 * To that end this function will:
 * * Test that the file exists and can be read from.
 * * The first few bytes of the file will be checked to ensure that it is an hdf5 file.
 * * If it's not an hdf5 file the head bytes will checked to see if the file is a netcdf-3 file, as that is common
 *   mistake.
 *
 * @param file_name
 */
void qc_input_file(const string &file_fqn)
{
    //Use an ifstream file to run a check on the provided file's signature
    // to see if it is an HDF5 file. - kln 5/18/23

    if (file_fqn.empty()) {
        stringstream msg;
        msg << "HDF5 input file name must be provided (-f <input>) and be a fully qualified path name." << endl;
        throw BESInternalFatalError(msg.str(), __FILE__, __LINE__);
    }

    std::ifstream file(file_fqn, ios::binary);
    auto errnum = errno;
    if (!file)  // This is same as if(file.fail()){...}
    {
        stringstream msg;
        msg << "Encountered a Read/writing error when attempting to open the file: " << file_fqn << endl;
        msg << "*  strerror(errno): " << strerror(errnum) << endl;
        msg << "*          failbit: " << (((file.rdstate() & std::ifstream::failbit) != 0) ? "true" : "false") << endl;
        msg << "*           badbit: " << (((file.rdstate() & std::ifstream::badbit) != 0) ? "true" : "false") << endl;
        msg << "Things to check:" << endl;
        msg << "* Does the file exist at expected location?" << endl;
        msg << "* Does your user have permission to read the file?" << endl;
        throw BESInternalFatalError(msg.str(), __FILE__, __LINE__);
    }

    //NetCDF3 signatures:
    const char netcdf3Signature[] = {'C', 'D', 'F'};

    //Read the first 8 bytes (file signature) from the file
    string signature;
    signature.resize(8);
    file.read(&signature[0], signature.size());

    htri_t temp_is_hdf5 = H5Fis_hdf5(file_fqn.c_str());
    
    bool isHDF5 = (temp_is_hdf5>0)?true:false;
    //First check if file is NOT an HDF5 file, then, if it is not, check if it is netcdf3
    if (!isHDF5) {
        //Reset the file stream to read from the beginning
        file.clear();
        file.seekg(0);

        char newSignature[3];
        file.read(&signature[0], signature.size());

        bool isNetCDF3 = memcmp(newSignature, netcdf3Signature, sizeof(netcdf3Signature)) == 0;
        if (isNetCDF3) {
            stringstream msg;
            msg << "The file submitted, " << file_fqn << ", ";
            msg << "is a NetCDF-3 classic file and is not compatible with dmr++ production at this time." << endl;
            throw BESInternalFatalError(msg.str(), __FILE__, __LINE__);
        }
        else {
            stringstream msg;
            msg << "The provided file: " << file_fqn << " - ";
            msg << "is neither an HDF5 or a NetCDF-4 file, currently only HDF5 and NetCDF-4 files ";
            msg << "are supported for dmr++ production" << endl;
            throw BESInternalFatalError(msg.str(), __FILE__, __LINE__);
        }
    }
}


/**
 * @brief Recreate the command invocation given argv and argc.
 *
 * @param argc
 * @param argv
 * @return The command
 */
static string recreate_cmdln_from_args(int argc, char *argv[])
{
    stringstream ss;
    for(int i=0; i<argc; i++) {
        if (i > 0)
            ss << " ";
        ss << argv[i];
    }
    return ss.str();
}

/**
 * @brief Returns an ISO-8601 date time string for the time at which this function is called.
 * Tip-o-the-hat to Morris Day and The Time...
 * @return An ISO-8601 date time string
 */
std::string what_time_is_it(){
    // Get current time as a time_point
    auto now = std::chrono::system_clock::now();

    // Convert to system time (time_t)
    auto time_t_now = std::chrono::system_clock::to_time_t(now);

    // Convert to tm structure (GMT time)
    struct tm tbuf{};
    const std::tm* gmt_time = gmtime_r(&time_t_now, &tbuf);

    // Format the time using a stringstream
    std::stringstream ss;
    ss << std::put_time(gmt_time, "%Y-%m-%dT%H:%M:%SZ");

    return ss.str();
}

/**
 * @brief This worker method provides a SSOT for how the build_dmrpp metadata (creation time, version, and configuration information) are added to the DMR++
 *
 * @param dmrpp The DMR++ to annotate
 * @param bes_conf_doc The BES configuration document used to produce the source DMR.
 * @param invocation The invocation of the build_dmrpp program, or the request URL if the running server was used to
 * create the DMR file that is being annotated into a DMR++.
 */
void inject_build_dmrpp_metadata_worker( DMRpp *dmrpp, const string &bes_conf_doc, const string &invocation)
{
    dmrpp->set_version(CVER);

    // Build the version attributes for the DMR++
    auto version = new D4Attribute("build_dmrpp_metadata", StringToD4AttributeType("container"));

    auto creation_date = new D4Attribute("created", StringToD4AttributeType("string"));
    creation_date->add_value(what_time_is_it());
    version->attributes()->add_attribute_nocopy(creation_date);

    auto build_dmrpp_version = new D4Attribute("build_dmrpp", StringToD4AttributeType("string"));
    build_dmrpp_version->add_value(CVER);
    version->attributes()->add_attribute_nocopy(build_dmrpp_version);

    auto bes_version = new D4Attribute("bes", StringToD4AttributeType("string"));
    bes_version->add_value(CVER);
    version->attributes()->add_attribute_nocopy(bes_version);

    stringstream ldv;
    ldv << libdap_name() << "-" << libdap_version();
    auto libdap4_version =  new D4Attribute("libdap", StringToD4AttributeType("string"));
    libdap4_version->add_value(ldv.str());
    version->attributes()->add_attribute_nocopy(libdap4_version);

    if(!bes_conf_doc.empty()) {
        stringstream ss(bes_conf_doc);
        string line;
        string new_bes_conf;

        // Iterate through each line and remove the bes module library path
        while (getline(ss, line)) {
            // Check if the line contains "BES.module."
            if (line.find("BES.module.") == string::npos) {
                // If the line doesn't contain "BES.module.", add it to the result
                new_bes_conf += line + "\n";
            }
        }

        // Add the BES configuration used to create the base DMR
        auto config = new D4Attribute("configuration", StringToD4AttributeType("string"));
        config->add_value(new_bes_conf);
        version->attributes()->add_attribute_nocopy(config);
    }

    if(!invocation.empty()) {
        // How was build_dmrpp invoked?
        auto invoke = new D4Attribute("invocation", StringToD4AttributeType("string"));
        invoke->add_value(invocation);
        version->attributes()->add_attribute_nocopy(invoke);
    }
    // Inject version and configuration attributes into DMR here.
    dmrpp->root()->attributes()->add_attribute_nocopy(version);
}


/**
 * @brief Injects software version, runtime configuration, and program invocation into DMRpp as attributes.
 *
 * This method assumes that it is being called outside of a running besd and thus requires a the configuration
 * used to create the DMR be supplied, along with the program parameters as invoked.
 *
 * @param argc The number of program arguments in the invocation.
 * @param argv The program arguments for the invocation.
 * @param bes_conf_file_used_to_create_dmr  The bes.conf configuration file used to create the DMR which is being
 * annotated as a DMR++
 * @param dmrpp The DMR++ instance to anontate.
 * @note The DMRpp instance will free all memory allocated by this method.
*/
 void inject_build_dmrpp_metadata(int argc, char **argv, const string &bes_conf_file_used_to_create_dmr, DMRpp *dmrpp)
{
    string bes_configuration;
    string invocation;
    if(!bes_conf_file_used_to_create_dmr.empty()) {
        // Add the BES configuration used to create the base DMR
        TheBESKeys::ConfigFile = bes_conf_file_used_to_create_dmr;
        bes_configuration = TheBESKeys::TheKeys()->get_as_config();
    }

    invocation = recreate_cmdln_from_args(argc, argv);

    inject_build_dmrpp_metadata_worker(dmrpp, bes_configuration, invocation);

}

/**
 * @brief Injects the DMR++ provenance information: software version, runtime configuration, into the DMR++ as attributes.
 *
 * This method assumes that it is being called from inside running besd. To obtain the configuration state of the BES
 * it interrogates TheBESKeys. The invocation string consists of the request URL which is recovered from the BES Context
 * key "invocation". This value would typically be set in the BES command transmitted by the OLFS
 *
 * @param dmrpp The DMRpp instance to annotate.
 * @note The DMRpp instance will free all memory allocated by this method.
*/
void inject_build_dmrpp_metadata(DMRpp *dmrpp)
{
    bool found;

    string bes_configuration;
    string invocation;
    if(!TheBESKeys::ConfigFile.empty()) {
        // Add the BES configuration used to create the base DMR
        bes_configuration = TheBESKeys::TheKeys()->get_as_config();
    }

    // How was build_dmrpp invoked?
    invocation = BESContextManager::TheManager()->get_context(INVOCATION_CONTEXT, found);

    // Do the work now...
    inject_build_dmrpp_metadata_worker(dmrpp, bes_configuration, invocation);
}


/**
 * @brief Builds a DMR++ from an existing DMR file in conjunction with source granule file.
 *
 * @param granule_url The value to use for the XML attribute dap4:Dataset/@dmrpp:href This may be a template string,
 * or it may be the actual URL location of the source granule file.
 * @param dmr_filename The name of the file from which to read the DMR.
 * @param h5_file_fqn The granule filename.
 * @param add_production_metadata If true the production metadata (software version, configuration, and invocation) will
 * be added to the DMR++.
 * @param argc The number of arguments supplied to build_dmrpp
 * @param argv The arguments for build_dmrpp.
 */
void build_dmrpp_from_dmr_file(const string &dmrpp_href_value, const string &dmr_filename, const string &h5_file_fqn,
        bool add_production_metadata, const string &bes_conf_file_used_to_create_dmr, bool disable_dio, int argc, char *argv[])
{
    // Get dmr:
    DMRpp dmrpp;
    DmrppTypeFactory dtf;
    dmrpp.set_factory(&dtf);

    ifstream in(dmr_filename.c_str());
    D4ParserSax2 parser;
    parser.intern(in, &dmrpp, false);

    add_chunk_information(h5_file_fqn, &dmrpp, disable_dio);

    if (add_production_metadata) {
        inject_build_dmrpp_metadata(argc, argv, bes_conf_file_used_to_create_dmr, &dmrpp);
    }

    XMLWriter writer;
    dmrpp.print_dmrpp(writer, dmrpp_href_value);
    cout << writer.get_doc();
}


} // namespace build_dmrpp_util
