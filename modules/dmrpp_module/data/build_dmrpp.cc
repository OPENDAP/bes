// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2018 OPeNDAP, Inc.
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
#include <fstream>
#include <sstream>
#include <memory>
#include <iterator>

#include <cstdlib>

#include <H5Ppublic.h>
#include <H5Dpublic.h>
#include <H5Epublic.h>
#include <H5Zpublic.h>  // Constants for compression filters

#include <DMRpp.h>
#include <D4Attributes.h>
#include <BaseType.h>
#include <D4ParserSax2.h>
#include <GetOpt.h>

#include <TheBESKeys.h>
#include <BESUtil.h>
#include <BESDebug.h>
#include <BESError.h>
#include <BESInternalError.h>

#include "DmrppTypeFactory.h"
#include "DmrppD4Group.h"
#include "DmrppMetadataStore.h"

using namespace std;
using namespace libdap;
using namespace dmrpp;

static bool verbose = false;
#define VERBOSE(x) do { if (verbose) x; } while(false)

#define DEBUG_KEY "metadata_store,dmrpp_store,dmrpp"
#define ROOT_DIRECTORY "BES.Catalog.catalog.RootDirectory"

/**
 * @brief Print information about the data type
 *
 * @note Calling this indicates that the build_dmrpp utility could not
 * get chunk information, and is probably an error, but I'm not sure
 * that's always the case. jhrg 5/7/18
 *
 * @param dataset
 * @param layout_type
 */
static void print_dataset_type_info(hid_t dataset, uint8_t layout_type)
{
    hid_t dtype_id = H5Dget_type(dataset);
    if (dtype_id < 0) {
        throw BESInternalError("Cannot obtain the correct HDF5 datatype.", __FILE__, __LINE__);
    }

    if (H5Tget_class(dtype_id) == H5T_INTEGER || H5Tget_class(dtype_id) == H5T_FLOAT) {
        hid_t dcpl_id = H5Dget_create_plist(dataset);
        if (dcpl_id < 0) {
            throw BESInternalError("Cannot obtain the HDF5 dataset creation property list.", __FILE__, __LINE__);
        }

        try {
            // Wrap the resources like dcpl_id in try/catch blocks so that the
            // calls to H5Pclose(dcpl_id) for each error can be removed. jhrg 5/7/18
            H5D_fill_value_t fvalue_status;
            if (H5Pfill_value_defined(dcpl_id, &fvalue_status) < 0) {
                H5Pclose(dcpl_id);
                throw BESInternalError("Cannot obtain the fill value status.", __FILE__, __LINE__);
            }
            if (fvalue_status == H5D_FILL_VALUE_UNDEFINED) {
                // Replace with switch(), here and elsewhere. jhrg 5/7/18
                if (layout_type == 1)
                    cerr << " The storage size is 0 and the storage type is contiguous." << endl;
                else if (layout_type == 2)
                    cerr << " The storage size is 0 and the storage type is chunking." << endl;
                else if (layout_type == 3) cerr << " The storage size is 0 and the storage type is compact." << endl;

                cerr << " The Fillvalue is undefined ." << endl;
            }
            else {
                if (layout_type == 1)
                    cerr << " The storage size is 0 and the storage type is contiguous." << endl;
                else if (layout_type == 2)
                    cerr << " The storage size is 0 and the storage type is chunking." << endl;
                else if (layout_type == 3) cerr << " The storage size is 0 and the storage type is compact." << endl;

                char* fvalue = NULL;
                size_t fv_size = H5Tget_size(dtype_id);
                if (fv_size == 1)
                    fvalue = (char*) (malloc(1));
                else if (fv_size == 2)
                    fvalue = (char*) (malloc(2));
                else if (fv_size == 4)
                    fvalue = (char*) (malloc(4));
                else if (fv_size == 8) fvalue = (char*) (malloc(8));

                if (fv_size <= 8) {
                    if (H5Pget_fill_value(dcpl_id, dtype_id, (void*) (fvalue)) < 0) {
                        H5Pclose(dcpl_id);
                        throw BESInternalError("Cannot obtain the fill value status.", __FILE__, __LINE__);
                    }
                    if (H5Tget_class(dtype_id) == H5T_INTEGER) {
                        H5T_sign_t fv_sign = H5Tget_sign(dtype_id);
                        if (fv_size == 1) {
                            if (fv_sign == H5T_SGN_NONE) {
                                cerr << "This dataset's datatype is unsigned char " << endl;
                                cerr << "and the fillvalue is " << *fvalue << endl;
                            }
                            else {
                                cerr << "This dataset's datatype is char and the fillvalue is " << *fvalue << endl;
                            }
                        }
                        else if (fv_size == 2) {
                            if (fv_sign == H5T_SGN_NONE) {
                                cerr << "This dataset's datatype is unsigned short and the fillvalue is " << *fvalue << endl;
                            }
                            else {
                                cerr << "This dataset's datatype is short and the fillvalue is " << *fvalue << endl;
                            }
                        }
                        else if (fv_size == 4) {
                            if (fv_sign == H5T_SGN_NONE) {
                                cerr << "This dataset's datatype is unsigned int and the fillvalue is " << *fvalue << endl;
                            }
                            else {
                                cerr << "This dataset's datatype is int and the fillvalue is " << *fvalue << endl;
                            }
                        }
                        else if (fv_size == 8) {
                            if (fv_sign == H5T_SGN_NONE) {
                                cerr << "This dataset's datatype is unsigned long long and the fillvalue is " << *fvalue << endl;
                            }
                            else {
                                cerr << "This dataset's datatype is long long and the fillvalue is " << *fvalue << endl;
                            }
                        }
                    }
                    if (H5Tget_class(dtype_id) == H5T_FLOAT) {
                        if (fv_size == 4) {
                            cerr << "This dataset's datatype is float and the fillvalue is " << *fvalue << endl;
                        }
                        else if (fv_size == 8) {
                            cerr << "This dataset's datatype is double and the fillvalue is " << *fvalue << endl;
                        }
                    }

                    if (fvalue != NULL) free(fvalue);
                }
                else
                    cerr
                        << "The size of the datatype is greater than 8 bytes, Use HDF5 API H5Pget_fill_value() to retrieve the fill value of this dataset."
                        << endl;
            }
        }
        catch (...) {
            H5Pclose(dcpl_id);
            throw;
        }
        H5Pclose(dcpl_id);
    }
    else {
        if (layout_type == 1)
            cerr << " The storage size is 0 and the storage type is contiguous." << endl;
        else if (layout_type == 2)
            cerr << " The storage size is 0 and the storage type is chunking." << endl;
        else if (layout_type == 3) cerr << " The storage size is 0 and the storage type is compact." << endl;

        cerr << "The datatype is neither float nor integer,use HDF5 API H5Pget_fill_value() to retrieve the fill value of this dataset." << endl;
    }
}

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
 * @brief Set compression info
 *
 * @param dataset_id The HDF5 dataset id
 * @param dc A pointer to the DmrppCommon instance for that dataset_id
 */
static void set_filter_information(hid_t dataset_id, DmrppCommon *dc)
{
    hid_t plist_id = H5Dget_create_plist(dataset_id);

    try {
        int numfilt = H5Pget_nfilters(plist_id);
        VERBOSE(cerr << "Number of filters associated with dataset: " << numfilt << endl);

        for (int filter = 0; filter < numfilt; filter++) {
            size_t nelmts = 0;
            unsigned int flags, filter_info;
            H5Z_filter_t filter_type = H5Pget_filter2(plist_id, filter, &flags, &nelmts, NULL, 0, NULL, &filter_info);
            VERBOSE(cerr << "Filter Type: ");

            switch (filter_type) {
            case H5Z_FILTER_DEFLATE:
                VERBOSE(cerr << "H5Z_FILTER_DEFLATE" << endl);
                dc->set_deflate(true);
                break;
            case H5Z_FILTER_SHUFFLE:
                VERBOSE(cerr << "H5Z_FILTER_SHUFFLE" << endl);
                dc->set_shuffle(true);
                break;
            default: {
                ostringstream oss("Unsupported HDF5 filter: ");
                oss << filter_type;
                throw BESInternalError(oss.str(), __FILE__, __LINE__);
            }
            }
        }
    }
    catch (...) {
        H5Pclose(plist_id);
        throw;
    }

    H5Pclose(plist_id);
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
static void get_variable_chunk_info(hid_t dataset, DmrppCommon *dc)
{
    try {
        uint8_t layout_type = 0;
        uint8_t storage_status = 0;
        hsize_t num_chunk = 0;

        herr_t status = H5Dget_dataset_storage_info(dataset, &layout_type, &num_chunk, &storage_status);
        if (status < 0) {
            throw BESInternalError("Cannot get HDF5 dataset storage info.", __FILE__, __LINE__);
        }

        VERBOSE(cerr << "layout: " << (int)layout_type << ", chunks: " << num_chunk << ", storage: " << (int)storage_status << endl);

        // Replace this with a 'not found' error? It seems that chunk information
        // is found only when storage_status != 0. jhrg 5/7/18
        if (storage_status == 0) {
            print_dataset_type_info(dataset, layout_type);
        }
#if 0
        else {
#endif

            /* layout_type:  1 contiguous 2 chunk 3 compact */
            switch (layout_type) {

            case H5D_CONTIGUOUS: { /* Contiguous storage */
                haddr_t cont_addr = 0;
                hsize_t cont_size = 0;
                VERBOSE(cerr << "Storage: contiguous" << endl);
                if (H5Dget_dataset_contiguous_storage_info(dataset, &cont_addr, &cont_size) < 0) {
                    throw BESInternalError("Cannot obtain the contiguous storage info.", __FILE__, __LINE__);
                }
                VERBOSE(cerr << "    Addr: " << cont_addr << endl);
                VERBOSE(cerr << "    Size: " << cont_size << endl);

                if (dc) dc->add_chunk("", cont_size, cont_addr, "" /*pos in array*/);

                break;
            }
            case H5D_CHUNKED: { /*chunking storage */
                VERBOSE(cerr << "storage: chunked." << endl);
                VERBOSE(cerr << "Number of chunks is " << num_chunk << endl);

                if (dc) set_filter_information(dataset, dc);

                // Get the chunk dimensions
                hid_t cparms = H5Dget_create_plist(dataset);
                if (cparms < 0) throw BESInternalError("Could not open a property list for the HDF5 dataset.", __FILE__, __LINE__);

                try {
#if 0
                    // Property list pointer
                    H5P_genplist_t *plist = H5P_object_verify(cparms, H5P_DATASET_CREATE);
                    if (!plist)
                    throw BESInternalError("Could not open a property list for '" + h5_dset_path + "'.", __FILE__, __LINE__);

                    /* Get layout property */
                    H5O_layout_t layout; /* Layout property */
                    if (H5P_get(plist, H5D_CRT_LAYOUT_NAME, &layout) < 0)
                    throw BESInternalError("Could not get the layout for '" + h5_dset_path + "'.", __FILE__, __LINE__);

                    // layout.u.chunk.ndims
#endif
                    // Allocate the memory for the struct to obtain the chunk storage information. Kent Yang
                    // wrote the H5Dget_dataset_chunk_storage_info() function; the alternative is to use the
                    // layout object above. jhrg 5/10/18
                    vector<H5D_chunk_storage_info_t> chunk_st_ptr(num_chunk);
                    unsigned int num_chunk_dims = 0;
                    if (H5Dget_dataset_chunk_storage_info(dataset, &chunk_st_ptr[0], &num_chunk_dims) < 0)
                        throw BESInternalError("Cannot get HDF5 chunk storage info.", __FILE__, __LINE__);

                    num_chunk_dims -= 1; // num_chunk_dims is rank + 1. not sure why. jhrg 5/10/18
                    VERBOSE(cerr << "Number of dimensions in a chunk is " << num_chunk_dims << endl);

                    // Get chunking information: rank and dimensions
                    vector<hsize_t> chunk_dims(num_chunk_dims);
                    unsigned int rank_chunk = H5Pget_chunk(cparms, num_chunk_dims, &chunk_dims[0]);

                    if (rank_chunk != num_chunk_dims) throw BESInternalError("Unexpected chunk dimension mismatch.", __FILE__, __LINE__);

                    VERBOSE(cerr << "Chunk rank: " << rank_chunk << endl);
                    VERBOSE(cerr << "Chunk dimension sizes: ");
                    VERBOSE(copy(chunk_dims.begin(), chunk_dims.end(), ostream_iterator<hsize_t>(cerr, " ")));
                    VERBOSE(cerr << endl);

                    if (dc) dc->set_chunk_dimension_sizes(chunk_dims);

                    for (hsize_t i = 0; i < num_chunk; i++) {
                        VERBOSE(cerr << "    Chunk index:  " << i << endl);
                        VERBOSE(cerr << "    Number of bytes: " << chunk_st_ptr[i].nbytes << endl);
                        VERBOSE(cerr << "    Physical offset: " << chunk_st_ptr[i].chunk_addr << endl);
                        VERBOSE(cerr << "    Logical offset: ");

                        vector<unsigned int> chunk_pos_in_array;
                        for (unsigned int j = 0; j < num_chunk_dims; j++)
                            chunk_pos_in_array.push_back(chunk_st_ptr[i].chunk_offset[j]);

                        VERBOSE(copy(chunk_pos_in_array.begin(), chunk_pos_in_array.end(), ostream_iterator<unsigned int>(cerr, " ")));
                        VERBOSE(cerr << endl);

                        // Note that the data_url is an empty string; use the root element href attribute for this value.
                        // Each chunk has the same data_url in 'architecture #2.' jhrg 5/10/18
                        if (dc) dc->add_chunk("", chunk_st_ptr[i].nbytes, chunk_st_ptr[i].chunk_addr, chunk_pos_in_array);
                    }
                }
                catch (...) {
                    H5Pclose(cparms);
                    throw;
                }

                H5Pclose(cparms);

                break;
            }

            case H5D_COMPACT: { /* Compact storage */
                //else if (layout_type == 3) {
                VERBOSE(cerr << "Storage: compact" << endl);
                size_t comp_size = 0;
                if (H5Dget_dataset_compact_storage_info(dataset, &comp_size) < 0) {
                    throw BESInternalError("Cannot obtain the compact storage info.", __FILE__, __LINE__);
                }
                VERBOSE(cerr << "   Size: " << comp_size << endl);

                break;
            }

            default: {
                ostringstream oss("Unsupported HDF5 dataset layout type: ");
                oss << layout_type << ".";
                BESInternalError(oss.str(), __FILE__, __LINE__);
            }
            } // end switch
#if 0
    }
#endif

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
static void get_chunks_for_all_variables(hid_t file, D4Group *group)
{
    // variables in the group
    for (Constructor::Vars_iter v = group->var_begin(), ve = group->var_end(); v != ve; ++v) {
        // if this variable has a 'fullnamepath' attribute, use that and not the
        // FQN value.
        D4Attributes *d4_attrs = (*v)->attributes();
        if (!d4_attrs)
            throw BESInternalError("Expected to find an attribute table for " + (*v)->name() + " but did not.", __FILE__, __LINE__);

        // Look for the full name path for this variable
        // If one was not given via an attribute, use BaseType::FQN() which
        // relies on the varaible's position in the DAP dataset hierarchy.
        D4Attribute *attr = d4_attrs->get("fullnamepath");
        string FQN;
        if (attr && attr->num_values() == 1)
            FQN = attr->value(0);
        else
            FQN = (*v)->FQN();

        VERBOSE(cerr << "Working on: " << FQN << endl);
        hid_t dataset = H5Dopen2(file, FQN.c_str(), H5P_DEFAULT);
        // It's not an error if a DAP variable in a DMR from the hdf5 handler
        // doesn't exist in the file _if_ there's no 'fullnamepath' because
        // that variable was synthesized (likely for CF compliance)
        if (dataset < 0 && attr == 0)
            continue;
        else if (dataset < 0)
            throw BESInternalError("HDF5 dataset '" + FQN + "' cannot be opened.", __FILE__, __LINE__);

        get_variable_chunk_info(dataset, dynamic_cast<DmrppCommon*>(*v));
     }

    // all groups in the group
    D4Group::groupsIter g = group->grp_begin();
    D4Group::groupsIter ge = group->grp_end();
    while (g != ge)
        get_chunks_for_all_variables(file, *g++);
}

int main(int argc, char*argv[])
{
    string h5_file_name = "";
    string h5_dset_path = "";
    string dmr_name = "";
    string url_name = "";
    int status=0;

    GetOpt getopt(argc, argv, "c:f:r:u:dhv");
    int option_char;
    while ((option_char = getopt()) != -1) {
        switch (option_char) {
        case 'v':
            verbose = true; // verbose hdf5 errors
            break;

        case 'd':
            BESDebug::SetUp(string("cerr,").append(DEBUG_KEY));
            break;

        case 'f':
            h5_file_name = getopt.optarg;
            break;
        case 'r':
            dmr_name = getopt.optarg;
            break;
        case 'u':
            url_name = getopt.optarg;
            break;
        case 'c':
            TheBESKeys::ConfigFile = getopt.optarg;
            break;
        case 'h':
            cerr << "build_dmrpp [-v] -c <bes.conf> -f <data file>  [-u <href url>] | build_dmrpp -f <data file> -r <dmr file> build_dmrpp -h" << endl;
            exit(1);
        default:
            break;
        }
    }

    if (h5_file_name.empty()) {
        cerr << "HDF5 file name must be given (-f <input>)." << endl;
        return 1;
    }

    hid_t file = 0;
    try {
        // Turn off automatic hdf5 error printing.
        // See: https://support.hdfgroup.org/HDF5/doc1.8/RM/RM_H5E.html#Error-SetAuto2
        if (!verbose) H5Eset_auto2(H5E_DEFAULT, NULL, NULL);

        // For a given HDF5, get info for all the HDF5 datasets in a DMR or for a
        // given HDF5 dataset
        if (!dmr_name.empty()) {
            // Get dmr:
            auto_ptr<DMRpp> dmrpp(new DMRpp);
            DmrppTypeFactory dtf;
            dmrpp->set_factory(&dtf);

            ifstream in(dmr_name.c_str());
            D4ParserSax2 parser;
            parser.intern(in, dmrpp.get(), false);

            // Open the hdf5 file
            file = H5Fopen(h5_file_name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
            if (file < 0) {
                cerr << "Error: HDF5 file '" + h5_file_name + "' cannot be opened." << endl;
                return 1;
            }

            // iterate over all the variables in the DMR
            get_chunks_for_all_variables(file, dmrpp->root());

            XMLWriter writer;
            dmrpp->print_dmrpp(writer, url_name);

            cout << writer.get_doc();
        }
        else {
            bool found;
            string bes_data_root;
            try {
                TheBESKeys::TheKeys()->get_value(ROOT_DIRECTORY, bes_data_root, found);
                if (!found) {
                    cerr << "Error: Could not find the BES root directory key." << endl;
                    return 1;
                }
            }
            catch (BESError &e) {
                cerr << "Error: " << e.get_message() << endl;
                return 1;
            }

            // Use the values from the bes.conf file... jhrg 5/21/18
            bes::DmrppMetadataStore *mds = bes::DmrppMetadataStore::get_instance();
            if (!mds) {
                cerr << "The Metadata Store (MDS) must be configured for this command to work." << endl;
                return 1;
            }

            // Use the full path to open the file, but use the 'name' (which is the
            // path relative to the BES Data Root) with the MDS.
            // Changed this to utilze assmeblePath() because simply concatenating the strings
            // is fragile. - ndp 6/6/18
            string h5_file_path = BESUtil::assemblePath(bes_data_root,h5_file_name);

            bes::DmrppMetadataStore::MDSReadLock lock = mds->is_dmr_available(h5_file_name /*h5_file_path*/);
            if (lock()) {
                // parse the DMR into a DMRpp (that uses the DmrppTypes)
                auto_ptr<DMRpp> dmrpp(dynamic_cast<DMRpp*>(mds->get_dmr_object(h5_file_name /*h5_file_path*/)));
                if (!dmrpp.get()) {
                    cerr << "Expected a DMR++ object from the DmrppMetadataStore." << endl;
                    return 1;
                }

                // Open the hdf5 file
                file = H5Fopen(h5_file_path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
                if (file < 0) {
                    cerr << "Error: HDF5 file '" + h5_file_path + "' cannot be opened." << endl;
                    return 1;
                }

                get_chunks_for_all_variables(file, dmrpp->root());

                dmrpp->set_href(url_name);

                mds->add_dmrpp_response(dmrpp.get(), h5_file_name /*h5_file_path*/);

                XMLWriter writer;
                dmrpp->set_print_chunks(true);
                dmrpp->print_dap4(writer);

                cout << writer.get_doc();
            }
            else {
                cerr << "Error: Could not get a lock on the DMR for '" + h5_file_path + "'." << endl;
                return 1;
            }
        }
    }
    catch (BESError &e) {
        cerr << "Error: " << e.get_message() << endl;
        status = 1;
    }
    catch (std::exception &e) {
        cerr << "Error: " << e.what() << endl;
        status = 1;
    }
    catch (...) {
        cerr << "Unknown error." << endl;
        status = 1;
    }

    H5Fclose(file);

    return status;
}
