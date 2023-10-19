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

#include <BESNotFoundError.h>
#include <BESInternalError.h>
#include <BESInternalFatalError.h>

#include <TheBESKeys.h>
#include <BESContextManager.h>

#include "DMRpp.h"
#include "DmrppTypeFactory.h"
#include "DmrppD4Group.h"
#include "DmrppArray.h"
#include "D4ParserSax2.h"

#include "UnsupportedTypeException.h"

#if 0
#define H5S_MAX_RANK    32
#define H5O_LAYOUT_NDIMS    (H5S_MAX_RANK+1)

/*
 * "Generic" chunk record.  Each chunk is keyed by the minimum logical
 * N-dimensional coordinates and the datatype size of the chunk.
 * The fastest-varying dimension is assumed to reference individual bytes of
 * the array, so a 100-element 1-D array of 4-byte integers would really be a
 * 2-D array with the slow varying dimension of size 100 and the fast varying
 * dimension of size 4 (the storage dimensionality has very little to do with
 * the real dimensionality).
 *
 * The chunk's file address, filter mask and size on disk are not key values.
 */
typedef struct H5D_chunk_rec_t {
    hsize_t scaled[H5O_LAYOUT_NDIMS];    /* Logical offset to start */
    uint32_t nbytes;                      /* Size of stored data */
    uint32_t filter_mask;                 /* Excluded filters */
    haddr_t chunk_addr;                  /* Address of chunk in file */
} H5D_chunk_rec_t;
#endif

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
static void set_filter_information(hid_t dataset_id, DmrppCommon *dc) {

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
bool
is_hdf5_fill_value_defined(hid_t dataset_id)
{

    // Suppress errors to stderr.
    H5Eset_auto2(H5E_DEFAULT, nullptr, nullptr);

    auto plist_id = create_h5plist(dataset_id);

    try {
        // How the fill value is defined?
        H5D_fill_value_t status;
        if ((H5Pfill_value_defined(plist_id, &status)) < 0) {
            H5Pclose(plist_id);
            throw BESInternalError("Unable to access HDF5 Fillvalue information.", __FILE__, __LINE__);
        }
        H5Pclose(plist_id);
        return status != H5D_FILL_VALUE_UNDEFINED;
    }
    catch (...) {
        H5Pclose(plist_id);
        throw;
    }

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
                    throw BESInternalError("Unable extract integer fill value.", __FILE__, __LINE__);
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
                    throw BESInternalError("Unable extract float fill value.", __FILE__, __LINE__);
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
                //return fv_str;
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
            string str_fv(value.begin(),value.end());
            throw UnsupportedTypeException(msg);
        }
        case H5T_COMPOUND: {
            string msg(prolog + "UnsupportedTypeException: Your data granule contains an H5T_COMPOUND as fillValue type. "
                       "This is not yet supported by the dmr++ creation machinery. ");
            string str_fv(value.begin(),value.end());
            throw UnsupportedTypeException(msg);
        }

        case H5T_REFERENCE:
        default: {
            throw BESInternalError("Unable extract fill value from HDF5 file.", __FILE__, __LINE__);
        }
    }
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

        vector<char> value(H5Tget_size(dtype_id));
        if (H5Pget_fill_value(plist_id, dtype_id, value.data()) < 0)
            throw BESInternalError("Unable to access HDF5 Fill Value.", __FILE__, __LINE__);

        H5Pclose(plist_id);
        return get_value_as_string(dtype_id, value);
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
 * @brief @TODO THIS IS A DUMMY FUNCTION AND NOT AN ACTUAL IMPLEMENTATION
 * @param dataset
 * @param da
 */
static void add_vlen_str_array_info(hid_t dataset, DmrppArray *da){
    string ons_str="0:26,26:35,35:873,873:5000";
    da->set_ons_string(ons_str);
    da->set_is_vlsa(true);
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
        add_vlen_str_array_info( dataset, dap_array);
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
        dc->add_chunk(byte_order, cont_size, cont_addr, "");
    }

}


/**
 * Processes the hdf5 storage information for a variable whose data is stored in the H5D_CHUNKED storage layout.
 * @param dataset The hdf5 dataset that is mate to the BaseType instance btp.
 * @param btp The dap BaseType variable which is to hold the information gleand from the hdf5 dataset.
 */
void process_chunked_layout_dariable(hid_t dataset, BaseType *btp) {

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

    set_filter_information(dataset, dc);

    // Get chunking information: rank and dimensions
    vector<hsize_t> chunk_dims(dataset_rank);

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
        vector<hsize_t> chunk_coords(dataset_rank);
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
    vector<uint8_t> values;

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
            values.resize(memRequired);
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
                values.resize(memRequired);
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
    vector<uint8_t> values;

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
            values.resize(memRequired);
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
    bool fill_value_defined = is_hdf5_fill_value_defined(dataset);
    if (fill_value_defined) {
        string fill_value = get_hdf5_fill_value_str(dataset);
        auto dc = toDC(btp);
        dc->set_uses_fill_value(fill_value_defined);
        dc->set_fill_value_string(fill_value);
    }
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
static void get_variable_chunk_info(hid_t dataset, BaseType *btp) {

    if(verbose) {
        string type_name = btp->type_name();
        if (btp->type() == dods_array_c) {
            auto array = toDA(btp);
            type_name = array->var()->type_name();
        }
        cerr << prolog << "Processing dataset/variable: " << type_name << " " << btp->name() << endl;
    }
    // Added support for HDF5 Fill Value. jhrg 4/22/22
    set_fill_value(dataset, btp);

    hid_t plist_id = create_h5plist(dataset);
    uint8_t layout_type = 0;
    try {
        layout_type = H5Pget_layout(plist_id);
        H5Pclose(plist_id);
    }
    catch (...) {
        H5Pclose(plist_id);
        throw;
    }

    switch (layout_type) {
        case H5D_CONTIGUOUS: { /* Contiguous Storage Layout */
            process_contiguous_layout_dariable(dataset, btp);
            break;
        }
        case H5D_CHUNKED: { /* Chunked Storage Layout */
            process_chunked_layout_dariable(dataset, btp);
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


/**
 * @brief Examines the hdf5 dataset, dataset_id, and returns true if the dataset is encoded as an unsupported type.
 * @param dataset_id The dataset to examine
 * @param btp The associated libdap::BaseType variable for this hdf5 dataset.
 * @param msg If an unsupported type is encountered a message about it will be returned in the msg return value
 * parameter.
 * @return True if the type of dataset_id is unsupported, false otherwise.
 */
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
                is_unsupported = true;
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

        default:
            break;
    }
    return is_unsupported;
}

/**
 *
 * @param dataset The HDF5 dataset id.
 * @param btp The BaseType pointer to the sister DAP class
 * @return Returns true if the variable was processed, false otherwise.
 */
bool process_variable_length_string_scalar(const hid_t dataset, BaseType *btp){
    // Do the vlss case as a compact variable.
    // btp->type() == dods_str_c means a scalar string, if it was an array of strings it would be dods_array_c
    if(btp->type() == dods_str_c && H5Tis_variable_str(H5Dget_type(dataset)) > 0) {
        vector<string> finstrval;   // passed by reference to read_vlen_string
        finstrval.emplace_back(""); // initialize array for it's trip to Cville

        // Read the scalar string.
        read_vlen_string(dataset, 1, nullptr, nullptr, nullptr, finstrval);
        string vlstr = finstrval[0];
        VERBOSE(cerr << prolog << " read_vlen_string(): " << vlstr << endl);

        // Convert variable to a compact representation
        // so that its value can be stored in the dmr++
        auto dc = toDC(btp);
        dc->set_compact(true);

        // And then set the value.
        auto str = dynamic_cast<libdap::Str *>(btp);
        str->set_value(vlstr);
        str->set_read_p(true);

        return true;
    }
    return false;
}
/**
 * @brief Iterate over all the variables in a DMR and get their chunk info
 *
 * @param file The open HDF5 file; passed through to get_variable_chunk_info
 * @param group Read variables from this DAP4 Group. Call with the root Group
 * to process all the variables in the DMR
 */
void get_chunks_for_all_variables(hid_t file, D4Group *group) {

    // netCDF-4 variable can share the same name as a pure dimension name. 
    // When this case happens, the netCDF-4 will add "_nc4_non_coord_" to the variable name to
    // distinguish the variable name from the correponding dimension name when storing in the HDF5. 
    // To emulate netCDF-4 model, the HDF5 handler will remove the "_nc4_non_coord_" from the variable in dmr.
    // When obtaining this variable's information, we need to use the real HDF5 variable name.
    // KY 2022-09-11
    // When the above case occurs, a dimension name of this group must share with a variable name.
    // So we will find these variables and put them into an unodered set. 

    unordered_set<string> nc4_non_coord_candidate;

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


    // variables in the group

    for(auto btp : group->variables()) {
        VERBOSE(cerr << prolog << "-------------------------------------------------------" << endl);

        // if this variable has a 'fullnamepath' attribute, use that and not the
        // FQN value.
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
            dataset = H5Dopen2(file, FQN.c_str(), H5P_DEFAULT);
            if (dataset < 0) {
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
                size_t fqn_last_fslash_pos = btp->FQN().find_last_of("/");
                string real_path_candidate = btp->FQN().substr(0,fqn_last_fslash_pos+1)+real_name_candidate;
                dataset = H5Dopen2(file, real_path_candidate.c_str(), H5P_DEFAULT);
            }
            
            VERBOSE(cerr << prolog << "Working on: " << FQN << endl);
            if (dataset < 0)  {
                dataset = H5Dopen2(file, FQN.c_str(), H5P_DEFAULT);
                if (dataset < 0) {
                   VERBOSE(cerr << prolog << "WARNING: HDF5 dataset '" << FQN << "' cannot be opened." << endl);
                    // throw BESInternalError("HDF5 dataset '" + FQN + "' cannot be opened.", __FILE__, __LINE__);
                    continue;
                }
            }
        }

        try {
            string msg;
            if(is_unsupported_type(dataset, btp, msg)){

                // @TODO What should really happen in this case?
                //   - Throw an exception?
                //   - Elide the variable from the dmr++?
                //   - Demote dataset/var to Attribute?
                //   - Mark the variable as "unsupported" so that it's metadata are transmitted but
                //     it's data cannot be read.
                throw UnsupportedTypeException(msg);
            }

            if(!process_variable_length_string_scalar(dataset,btp)) {

                VERBOSE(cerr << prolog << "Building chunks for: " << get_type_decl(btp) << endl);
                get_variable_chunk_info(dataset, btp);

                VERBOSE(cerr << prolog << "Annotating String Arrays as needed for: " << get_type_decl(btp) << endl);
                add_string_array_info(dataset, btp);
            }
            H5Dclose(dataset);
        }
        catch (UnsupportedTypeException &uste){
            // TODO - If we are going to elide a variable because it is an unsupported type, I think
            //  that this would be the place to do it.
            cerr << prolog << "Caught UnsupportedTypeException for variable " << btp->FQN() << " message: " << uste.what() << endl;
            throw;
        }
        catch (...) {
            H5Dclose(dataset);
            throw;
        }
    }

    // all groups in the group
    // for(auto g:group->groups())
    //     get_chunks_for_all_variables(file, g);

    for (auto g = group->grp_begin(), ge = group->grp_end(); g != ge; ++g) {
        get_chunks_for_all_variables(file, *g);
    }
}

/**
 * @brief Add chunk information about to a DMRpp object
 * @param h5_file_name Read information from this file
 * @param dmrpp Dump the chunk information here
 */
void add_chunk_information(const string &h5_file_name, DMRpp *dmrpp)
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
        get_chunks_for_all_variables(file, dmrpp->root());
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
 * hdf5 and all netcdf-4 files are defacto hdf5 files.)
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

    //HDF5 and NetCDF3 signatures:
    const char hdf5Signature[] = {'\211', 'H', 'D', 'F', '\r', '\n', '\032', '\n'};
    const char netcdf3Signature[] = {'C', 'D', 'F'};

    //Read the first 8 bytes (file signature) from the file
    string signature;
    signature.resize(8);
    file.read(&signature[0], signature.size());

    //First check if file is NOT an HDF5 file, then, if it is not, check if it is netcdf3
    bool isHDF5 = memcmp(&signature[0], hdf5Signature, sizeof(hdf5Signature)) == 0;
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
 * @brief This worker method provides a SSOT for how the version and configuration information are added to the DMR++
 *
 * @param dmrpp The DMR++ to annotate
 * @param bes_conf_doc The BES configuration document used to produce the source DMR.
 * @param invocation The invocation of the build_dmrpp program, or the request URL if the running server was used to
 * create the DMR file that is being annotated into a DMR++.
 */
void inject_version_and_configuration_worker( DMRpp *dmrpp, const string &bes_conf_doc, const string &invocation)
{
    dmrpp->set_version(CVER);

    // Build the version attributes for the DMR++
    auto version = new D4Attribute("build_dmrpp_metadata", StringToD4AttributeType("container"));

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
        // Add the BES configuration used to create the base DMR
        auto config = new D4Attribute("configuration", StringToD4AttributeType("string"));
        config->add_value(bes_conf_doc);
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
 void inject_version_and_configuration(int argc, char **argv, const string &bes_conf_file_used_to_create_dmr, DMRpp *dmrpp)
{
    string bes_configuration;
    string invocation;
    if(!bes_conf_file_used_to_create_dmr.empty()) {
        // Add the BES configuration used to create the base DMR
        TheBESKeys::ConfigFile = bes_conf_file_used_to_create_dmr;
        bes_configuration = TheBESKeys::TheKeys()->get_as_config();
    }

    invocation = recreate_cmdln_from_args(argc, argv);

    inject_version_and_configuration_worker(dmrpp, bes_configuration, invocation);

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
void inject_version_and_configuration(DMRpp *dmrpp)
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
    inject_version_and_configuration_worker(dmrpp, bes_configuration, invocation);
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
        bool add_production_metadata, const string &bes_conf_file_used_to_create_dmr, int argc, char *argv[])
{
    // Get dmr:
    DMRpp dmrpp;
    DmrppTypeFactory dtf;
    dmrpp.set_factory(&dtf);

    ifstream in(dmr_filename.c_str());
    D4ParserSax2 parser;
    parser.intern(in, &dmrpp, false);

    add_chunk_information(h5_file_fqn, &dmrpp);

    if (add_production_metadata) {
        inject_version_and_configuration(argc, argv, bes_conf_file_used_to_create_dmr, &dmrpp);
    }

    XMLWriter writer;
    dmrpp.print_dmrpp(writer, dmrpp_href_value);
    cout << writer.get_doc();

}


} // namespace build_dmrpp_util
