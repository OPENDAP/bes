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

#include <iostream>
#include <sstream>
#include <memory>
#include <iterator>

#include <cstdlib>

#include <H5Ppublic.h>
#include <H5Dpublic.h>
#include <H5Epublic.h>
#include <H5Zpublic.h>  // Constants for compression filters
#include <H5Spublic.h>
#include <H5Tpublic.h>

#include "h5common.h"   // This is in the hdf5 handler

#include <libdap/Array.h>
#include <libdap/util.h>
#include <libdap/D4Attributes.h>

#include <BESDebug.h>
#include <BESNotFoundError.h>
#include <BESInternalError.h>

#include "DMRpp.h"
#include "DmrppTypeFactory.h"
#include "DmrppD4Group.h"

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

string h5_filter_name(int type) {
    string name;
    switch(type) {
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
            ostringstream oss("ERROR! Unknown HDF5 FILTER! type: ", std::ios::ate);
            oss << type;
            name = oss.str();
            break;
        }
    }
    return name;
}

/**
 * @brief Set compression info
 *
 * @param dataset_id The HDF5 dataset id
 * @param dc A pointer to the DmrppCommon instance for that dataset_id
 */
static void set_filter_information(hid_t dataset_id, DmrppCommon *dc) {
    hid_t plist_id = H5Dget_create_plist(dataset_id);

    try {
        int numfilt = H5Pget_nfilters(plist_id);
        VERBOSE(cerr << "Number of filters associated with dataset: " << numfilt << endl);
        string filters;

        for (int filter = 0; filter < numfilt; filter++) {
            size_t nelmts = 0;
            unsigned int flags, filter_info;
            H5Z_filter_t filter_type = H5Pget_filter2(plist_id, filter, &flags, &nelmts, nullptr, 0, nullptr, &filter_info);
            VERBOSE(cerr << "Found H5 Filter Type: " << h5_filter_name(filter_type) << " (" << filter_type << ")" << endl);
            switch (filter_type) {
                case H5Z_FILTER_DEFLATE:
                    // For HYRAX-733. Sometimes deflate shows up twice in the filters for a variable.
                    // The DMR++ ignores the second time deflate is listed, but other users of the DMR++
                    // might not be savvy and try to decompress the chunk/buffer/variable two times,
                    // which will fail.
                    // Here, we only add 'deflate' if it is not already in the filters info. jhrg 5/24/22
                    if (filters.find("deflate") == string::npos)
                        filters.append("deflate ");
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
        //trimming trailing space from compression (aka filter) string
        filters = filters.substr(0, filters.length() - 1);
        dc->set_filter(filters);
    }
    catch (...) {
        H5Pclose(plist_id);
        throw;
    }

    H5Pclose(plist_id);
}

bool
is_hdf5_fill_value_defined(hid_t dataset_id)
{
    hid_t plist_id;

    // Suppress errors to stderr.
    H5Eset_auto2(H5E_DEFAULT, nullptr, nullptr);

    // Get creation properties list
    if ( (plist_id = H5Dget_create_plist(dataset_id)) < 0 )
        throw BESInternalError("Unable to open HDF5 dataset id.", __FILE__, __LINE__);

    // How the fill value is defined?
    H5D_fill_value_t status;
    if ( (H5Pfill_value_defined(plist_id, &status)) < 0 ) {
        H5Pclose(plist_id);
        throw BESInternalError("Unable to access HDF5 Fillvalue information.", __FILE__, __LINE__);
    }

    H5Pclose(plist_id);

    return status != H5D_FILL_VALUE_UNDEFINED;
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
    int sign;
    switch (class_type) {
        case H5T_INTEGER:
            sign = H5Tget_sign(h5_type_id);
            switch (H5Tget_size(h5_type_id)) {
                case 1:
                    if (sign == H5T_SGN_2)
                        return to_string(*(int8_t *)(value.data()));
                    else
                        return to_string(*(uint8_t *)(value.data()));
                    
                case 2:
                    if (sign == H5T_SGN_2)
                        return to_string(*(int16_t *)(value.data()));
                    else
                        return to_string(*(uint16_t *)(value.data()));
                    
                case 4:
                    if (sign == H5T_SGN_2)
                        return to_string(*(int32_t *)(value.data()));
                    else
                        return to_string(*(uint32_t *)(value.data()));
                    
                case 8:
                    if (sign == H5T_SGN_2)
                        return to_string(*(int64_t *)(value.data()));
                    else
                        return to_string(*(uint64_t *)(value.data()));
                    
                default:
                    throw BESInternalError("Unable extract integer fill value.", __FILE__, __LINE__);
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
        }

            // TODO jhrg 4/22/22
        case H5T_STRING:
            return "unsupported-string";
        case H5T_ARRAY:
            return "unsupported-array";
        case H5T_COMPOUND:
            return "unsupported-compound";

        case H5T_REFERENCE:
        default:
            throw BESInternalError("Unable extract fill value.", __FILE__, __LINE__);
    }
}

/**
 * @brief Get the value of the File Value as a string
 * @param dataset_id
 * @return The string representation of the HDF5 Fill Value
 */
string get_hdf5_fill_value(hid_t dataset_id)
{
    // Suppress errors to stderr.
    H5Eset_auto2(H5E_DEFAULT, nullptr, nullptr);

    // Get creation properties list
    hid_t plist_id = H5Dget_create_plist(dataset_id);
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
 * @brief Get chunk information for a HDF5 dataset in a file
 *
 * @param file The open HDF5 file
 * @param h5_dset_path The path name of the dataset in the open hdf5 file
 * @param dataset The open HDF5 dataset object
 * @param dc if not null, put the information in this variable (DmrppCommon)
 *
 * @exception BESError is thrown on error.
 */
static void get_variable_chunk_info(hid_t dataset, DmrppCommon *dc) {
    std::string byteOrder;
    H5T_order_t byte_order;

    // Added support for HDF5 Fill Value. jhrg 4/22/22
    bool fill_value_defined = is_hdf5_fill_value_defined(dataset);
    if (fill_value_defined) {
        string fill_value = get_hdf5_fill_value(dataset);
        dc->set_uses_fill_value(fill_value_defined);
        dc->set_fill_value_string(fill_value);
    }

    try {
        hid_t dcpl = H5Dget_create_plist(dataset);
        uint8_t layout_type = H5Pget_layout(dcpl);

        hid_t fspace_id = H5Dget_space(dataset);
        hid_t dtypeid = H5Dget_type(dataset);

        byte_order = H5Tget_order(dtypeid);
        switch (byte_order) {
            case H5T_ORDER_LE:
                byteOrder = "LE";
                break;
            case H5T_ORDER_BE:
                byteOrder = "BE";
                break;
            case H5T_ORDER_NONE:
                break;
            default:
                // unsupported enumerations: H5T_ORDER_[ERROR,VAX,MIXED]
                ostringstream oss("Unsupported HDF5 dataset byteOrder: ", std::ios::ate);
                oss << byte_order << ".";
                throw BESInternalError(oss.str(), __FILE__, __LINE__);
        }

        int dataset_rank = H5Sget_simple_extent_ndims(fspace_id);

        size_t dsize = H5Tget_size(dtypeid);

        /* layout_type:  1 contiguous 2 chunk 3 compact */
        switch (layout_type) {

            case H5D_CONTIGUOUS: { /* Contiguous storage */
                VERBOSE(cerr << "Storage:   contiguous" << endl);

                haddr_t cont_addr = H5Dget_offset(dataset);
                hsize_t cont_size = H5Dget_storage_size(dataset);

                VERBOSE(cerr << "     Addr: " << cont_addr << endl);
                VERBOSE(cerr << "     Size: " << cont_size << endl);
                VERBOSE(cerr << "byteOrder: " << byteOrder << endl);

                if (cont_size > 0 && dc) {
                    dc->add_chunk(byteOrder, cont_size, cont_addr, "" /*pos in array*/);
                }
                break;
            }
            case H5D_CHUNKED: { /*chunking storage */
                hsize_t num_chunks = 0;
                herr_t status = H5Dget_num_chunks(dataset, fspace_id, &num_chunks);
                if (status < 0) {
                    throw BESInternalError("Could not get the number of chunks", __FILE__, __LINE__);
                }

                VERBOSE(cerr << "Storage:   chunked." << endl);
                VERBOSE(cerr << "Number of chunks is: " << num_chunks << endl);

                if (dc)
                    set_filter_information(dataset, dc);

                // Get chunking information: rank and dimensions
                vector<hsize_t> chunk_dims(dataset_rank);
                unsigned int chunk_rank = H5Pget_chunk(dcpl, dataset_rank, &chunk_dims[0]);
                if (chunk_rank != dataset_rank)
                    throw BESNotFoundError(
                            "Found a chunk with rank different than the dataset's (aka variables') rank", __FILE__,
                            __LINE__);

                if (dc) dc->set_chunk_dimension_sizes(chunk_dims);

                for (unsigned int i = 0; i < num_chunks; ++i) {
                    vector<hsize_t> chunk_coords(dataset_rank);
                    haddr_t addr = 0;
                    hsize_t size = 0;

                    status = H5Dget_chunk_info(dataset, fspace_id, i, &chunk_coords[0],
                                               nullptr, &addr, &size);
                    if (status < 0) {
                        VERBOSE(cerr << "ERROR" << endl);
                        throw BESInternalError("Cannot get HDF5 dataset storage info.", __FILE__, __LINE__);
                    }

                    VERBOSE(cerr << "chk_idk: " << i << ", addr: " << addr << ", size: " << size << endl);
                    if (dc) dc->add_chunk(byteOrder, size, addr, chunk_coords);
                }

                break;
            }

            case H5D_COMPACT: { /* Compact storage */
                VERBOSE(cerr << "Storage: compact" << endl);

                size_t comp_size = H5Dget_storage_size(dataset);
                VERBOSE(cerr << "   Size: " << comp_size << endl);

                if (comp_size == 0) {
                    throw BESInternalError("Cannot obtain the compact storage size.", __FILE__, __LINE__);
                }

                vector<uint8_t> values;

                auto btp = dynamic_cast<Array *>(dc);
                if (btp != nullptr) {
                    dc->set_compact(true);
                    size_t memRequired = btp->length() * dsize;

                    if (comp_size != memRequired) {
                        throw BESInternalError("Compact storage size does not match D4Array.", __FILE__, __LINE__);
                    }

                    switch (btp->var()->type()) {
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
                        case dods_uint64_c: {
                            values.resize(memRequired);
                            get_data(dataset, reinterpret_cast<void *>(&values[0]));
                            btp->set_read_p(true);
                            btp->val2buf(reinterpret_cast<void *>(&values[0]));
                            break;

                        }

                        case dods_str_c: {
                            if (H5Tis_variable_str(dtypeid) > 0) {
                                vector<string> finstrval = {""};   // passed by reference to read_vlen_string
                                read_vlen_string(dataset, 1, nullptr, nullptr, nullptr, finstrval);
                                btp->set_value(finstrval, (int)finstrval.size());
                                btp->set_read_p(true);
                            }
                            else {
                                // For this case, the Array is really a single string - check for that
                                // with the following assert - but is an Array because the string data
                                // is stored as an array of chars (hello, FORTRAN). Read the chars, make
                                // a string and load that into a vector<string> (which will be a vector
                                // of length one). Set that as the value of the Array. Really, this
                                // value could be stored as a scalar, but that's complicated and client
                                // software might be expecting an array, so better to handle it this way.
                                // jhrg 9/17/20
                                assert(btp->length() == 1);
                                values.resize(memRequired);
                                get_data(dataset, reinterpret_cast<void *>(&values[0]));
                                string str(values.begin(), values.end());
                                vector<string> strings = {str};
                                btp->set_value(strings, (int)strings.size());
                                btp->set_read_p(true);
                            }
                            break;
                        }

                        default:
                            throw BESInternalError("Unsupported compact storage variable type.", __FILE__, __LINE__);
                    }

                }
                else {
                    throw BESInternalError("Compact storage variable is not a D4Array.",
                                           __FILE__, __LINE__);
                }
                break;
            }

            default:
                ostringstream oss("Unsupported HDF5 dataset layout type: ", std::ios::ate);
                oss << layout_type << ".";
                throw BESInternalError(oss.str(), __FILE__, __LINE__);
        }
    }
    catch (...) {
        H5Dclose(dataset);
        throw;
    }

    H5Dclose(dataset);
}

/**
 * @brief Iterate over all the variables in a DMR and get their chunk info
 *
 * @param file The open HDF5 file; passed through to get_variable_chunk_info
 * @param group Read variables from this DAP4 Group. Call with the root Group
 * to process all the variables in the DMR
 */
void get_chunks_for_all_variables(hid_t file, D4Group *group) {
    // variables in the group
    for (auto v = group->var_begin(), ve = group->var_end(); v != ve; ++v) {
        // if this variable has a 'fullnamepath' attribute, use that and not the
        // FQN value.
        D4Attributes *d4_attrs = (*v)->attributes();
        if (!d4_attrs)
            throw BESInternalError("Expected to find an attribute table for " + (*v)->name() + " but did not.",
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
        hid_t dataset;
        if (attr) {
            string FQN;
            if (attr->num_values() == 1)
                FQN = attr->value(0);
            else
                FQN = (*v)->FQN();
            BESDEBUG("dmrpp", "Working on: " << FQN << endl);
            dataset = H5Dopen2(file, FQN.c_str(), H5P_DEFAULT);
            if (dataset < 0)
                throw BESInternalError("HDF5 dataset '" + FQN + "' cannot be opened.", __FILE__, __LINE__);

        } else {
            // The current design seems to still prefer to open the dataset when the fullnamepath doesn't exist
            // So go ahead to open the dataset. Continue even if the dataset cannot be open. KY 2019-12-02
            //
            // A comment from an older version of the code:
            // It's not an error if a DAP variable in a DMR from the hdf5 handler
            // doesn't exist in the file _if_ there's no 'fullnamepath' because
            // that variable was synthesized (likely for CF compliance)
            H5Eset_auto2(H5E_DEFAULT, nullptr, nullptr);
            string FQN = (*v)->FQN();
            BESDEBUG("dmrpp", "Working on: " << FQN << endl);
            dataset = H5Dopen2(file, FQN.c_str(), H5P_DEFAULT);
            if (dataset < 0)
                continue;
        }

        get_variable_chunk_info(dataset, dynamic_cast<DmrppCommon *>(*v));
    }

    // all groups in the group
    for (auto g = group->grp_begin(), ge = group->grp_end(); g != ge; ++g)
        get_chunks_for_all_variables(file, *g);
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
        throw BESNotFoundError(string("Error: HDF5 file '").append(h5_file_name).append("' cannot be opened."), __FILE__, __LINE__);
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

} // namespace build_dmrpp_util