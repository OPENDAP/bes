// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2016 OPeNDAP, Inc.
// Author: Nathan David Potter <ndp@opendap.org>, James Gallagher
// <jgallagher@opendap.org>
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

#include <cstdlib>

#include <H5Ppublic.h>
#include <H5Dpublic.h>
#include <H5Epublic.h>

#include <DMRpp.h>
#include <BaseType.h>
#include <D4ParserSax2.h>
#include <GetOpt.h>

#include <BESUtil.h>
#include <BESError.h>
#include <BESInternalError.h>

#include "DmrppTypeFactory.h"
#include "DmrppD4Group.h"

using namespace std;
using namespace libdap;
using namespace dmrpp;

static bool verbose = false;
#define VERBOSE(x) do { if (verbose) x; } while(false)

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

        // TODO Wrap the resources like dcpl_id in try/catch blocks so that the
        // calls to H5Pclose(dcpl_id) for each error can be removed. jhrg 5/7/18
        H5D_fill_value_t fvalue_status;
        if (H5Pfill_value_defined(dcpl_id, &fvalue_status) < 0) {
            H5Pclose(dcpl_id);
            throw BESInternalError("Cannot obtain the fill value status.", __FILE__, __LINE__);
        }
        if (fvalue_status == H5D_FILL_VALUE_UNDEFINED) {
            // TODO Replace with switch(), here and elsewhere. jhrg 5/7/18
            if (layout_type == 1)
                cerr << " The storage size is 0 and the storge type is contiguous." << endl;
            else if (layout_type == 2)
                cerr << " The storage size is 0 and the storage type is chunking." << endl;
            else if (layout_type == 3)
                cerr << " The storage size is 0 and the storage type is compact." << endl;

            cerr << " The Fillvalue is undefined ." << endl;
        }
        else {
            if (layout_type == 1)
                cerr << " The storage size is 0 and the storage type is contiguous." << endl;
            else if (layout_type == 2)
                cerr << " The storage size is 0 and the storage type is chunking." << endl;
            else if (layout_type == 3)
                cerr << " The storage size is 0 and the storage type is compact." << endl;

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
                cerr << "The size of the datatype is greater than 8 bytes, Use HDF5 API H5Pget_fill_value() to retrieve the fill value of this dataset." << endl;
        }
        H5Pclose(dcpl_id);
    }
    else {
        if (layout_type == 1)
            cerr << " The storage size is 0 and the storage type is contiguous." << endl;
        else if (layout_type == 2)
            cerr << " The storage size is 0 and the storage type is chunking." << endl;
        else if (layout_type == 3)
            cerr << " The storage size is 0 and the storage type is compact." << endl;

        cerr << "The datatype is neither float nor integer,use HDF5 API H5Pget_fill_value() to retrieve the fill value of this dataset." << endl;
    }
}

/**
 * @brief Get chunk information for a HDF5 dataset in a file
 *
 * @todo Needs to get information about compression used by the dataset
 *
 * @param file The open HDF5 file
 * @param h5_dset_path The path name of the dataset in the open hdf5 file
 * @param dc if not null, put the information in this variable (DmrppCommon)
 *
 * @exception BESError is thrown on error.
 */
static void get_variable_chunk_info(hid_t file, const string &h5_dset_path, DmrppCommon *dc)
{
    hid_t dataset = H5Dopen2(file, h5_dset_path.c_str(), H5P_DEFAULT);
    if (dataset < 0)
        throw BESError("HDF5 dataset '" + h5_dset_path + "' cannot be opened.", BES_NOT_FOUND_ERROR, __FILE__, __LINE__);

    try {
        uint8_t layout_type = 0;
        uint8_t storage_status = 0;
        hsize_t num_chunk = 0;

        herr_t status = H5Dget_dataset_storage_info(dataset, &layout_type, &num_chunk, &storage_status);
        if (status < 0) {
            throw BESInternalError("Cannot get HDF5 dataset storage info.", __FILE__, __LINE__);
        }

        // TODO Replace this with a 'not found' error? It seems that chunk informatio
        // is found only when storage_status != 0. jhrg 5/7/18
        if (storage_status == 0) {
            print_dataset_type_info(dataset, layout_type);
        }
        else {
            /* layout_type:  1 contiguous 2 chunk 3 compact */
            if (layout_type == 1) {/* Contiguous storage */
                haddr_t cont_addr = 0;
                hsize_t cont_size = 0;
                VERBOSE(cerr << "Storage: contiguous" << endl);
                if (H5Dget_dataset_contiguous_storage_info(dataset, &cont_addr, &cont_size) < 0) {
                    throw BESInternalError("Cannot obtain the contiguous storage info.", __FILE__, __LINE__);
                }
                VERBOSE(cerr << "    Addr: " << cont_addr << endl);
                VERBOSE(cerr << "    Size: " << cont_size << endl);

                if (dc)
                    dc->add_chunk("", cont_size, cont_addr, "" /*pos in array*/);
            }
            else if (layout_type == 2) {/*chunking storage */
                VERBOSE(cerr << "storage: chunked." << endl);
                VERBOSE(cerr << "Number of chunks is " << num_chunk << endl);

                /* Allocate the memory for the struct to obtain the chunk storage information */
                //H5D_chunk_storage_info_t *chunk_st_ptr = (H5D_chunk_storage_info_t*) calloc(num_chunk, sizeof(H5D_chunk_storage_info_t));
                // Replaced with C++ vector<> jhrg 5/7/18
                vector<H5D_chunk_storage_info_t> chunk_st_ptr(num_chunk);

                unsigned int num_chunk_dims = 0;
                if (H5Dget_dataset_chunk_storage_info(dataset, &chunk_st_ptr[0], &num_chunk_dims) < 0) {
                    throw BESInternalError("Cannot get HDF5 chunk storage info. successfully.", __FILE__, __LINE__);
                }

                VERBOSE(cerr << "    Number of dimensions in a chunk is " << num_chunk_dims - 1 << endl);

                for (hsize_t i = 0; i < num_chunk; i++) {
                    VERBOSE(cerr << "    Chunk index:  " << i << endl);
                    VERBOSE(cerr << "    Number of bytes: " << chunk_st_ptr[i].nbytes << endl);
                    VERBOSE(cerr << "    Physical offset: " << chunk_st_ptr[i].chunk_addr << endl);
                    VERBOSE(cerr << "    Logical offset: ");

                    vector<unsigned int> chunk_pos_in_array;
                     for (unsigned int j = 0; j < num_chunk_dims - 1; j++)
                         chunk_pos_in_array.push_back(chunk_st_ptr[i].chunk_offset[j]);

                    VERBOSE(copy(chunk_pos_in_array.begin(), chunk_pos_in_array.end(), ostream_iterator<unsigned int>(cerr, " ")));
                    VERBOSE(cerr << endl);

                    if (dc)
                        dc->add_chunk("", chunk_st_ptr[i].nbytes, chunk_st_ptr[i].chunk_addr, chunk_pos_in_array);
                }
            }
            else if (layout_type == 3) {/* Compact storage */
                VERBOSE(cerr << "Storage: compact" << endl);
                size_t comp_size = 0;
                if (H5Dget_dataset_compact_storage_info(dataset, &comp_size) < 0) {
                    throw BESInternalError("Cannot obtain the compact storage info.", __FILE__, __LINE__);
                }
                VERBOSE(cerr << "   Size: " << comp_size << endl);
            }
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
static void get_chunks_for_all_variables(hid_t file, D4Group *group)
{
    // variables in the group
    Constructor::Vars_iter v = group->var_begin();
    Constructor::Vars_iter ve = group->var_end();
    while (v != ve) {
        cerr << (*v)->FQN() << endl;
        get_variable_chunk_info(file, (*v)->FQN(), dynamic_cast<DmrppCommon*>(*v));
        ++v;
    }

    // all groups in the group
    D4Group::groupsIter g = group->grp_begin();
    D4Group::groupsIter ge = group->grp_end();
    while (g != ge)
        get_chunks_for_all_variables(file, *g);
}

int main(int argc, char*argv[])
{
    string h5_file_name = "";
    string h5_dset_path = "";
    string dmr_name = "";

    GetOpt getopt(argc, argv, "f:o:d:r:hv");
    int option_char;
    while ((option_char = getopt()) != -1) {
        switch (option_char) {
        case 'v':
            verbose = true; // verbose hdf5 errors
            break;
        case 'd':
            h5_dset_path = getopt.optarg;
            break;
        case 'f':
            h5_file_name = getopt.optarg;
            break;
        case 'r':
            dmr_name = getopt.optarg;
            break;
        case 'o':
            // FIXME
            break;
        case 'h':
            cerr << "build_dmrpp [-v] -f <input> [-r <dmr> | -d <dset anme>] -o <output> | build_dmrpp -h" << endl;
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

        // Open the hdf5 file
        file = H5Fopen(h5_file_name.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
        if (file < 0) {
            cerr << "Error: HDF5 file '" + h5_file_name + "' cannot be opened." << endl;
            return 1;
        }

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

            // iterate over all the variables in the DMR
            get_chunks_for_all_variables(file, dmrpp->root());

            XMLWriter writer;
            dmrpp->print_dmrpp(writer, false /*constrained*/, true /*print_chunks*/);
            cout << writer.get_doc();
        }
        else if (!h5_dset_path.empty()) {
            get_variable_chunk_info(file, h5_dset_path, 0);
        }
        else {
            cerr << "Error: One of -d <hdf5 dataset name> or -r <DAP4 DMR name> must be given." << endl;
            return 1;
        }
    }
    catch (BESError &e) {
        cerr << "Error: " << e.get_message() << endl;
    }
    catch (std::exception &e) {
        cerr << "Error: " << e.what() << endl;
    }
    catch (...) {
        cerr << "Unknown error." << endl;
    }

    H5Fclose(file);

    return 0;
}
