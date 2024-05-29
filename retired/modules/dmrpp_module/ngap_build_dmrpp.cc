// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the Hyrax data server.

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

// system includes
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <iterator>
#include <algorithm>
#include <cstdio>

#include <cstdlib>
#include <unistd.h>
#include <GetOpt.h>

// libdap4 includes
#include <libdap/D4Attributes.h>
#include <libdap/BaseType.h>
#include <libdap/D4ParserSax2.h>

// bes includes
#include "TheBESKeys.h"
#include "StandAloneApp.h"
#include "BESUtil.h"
#include "BESDebug.h"
#include "BESFileContainer.h"
#include "BESDMRResponse.h"
#include "BESDMRResponseHandler.h"
#include "BESStopWatch.h"

#include "BESError.h"
#include "BESNotFoundError.h"
#include "BESInternalError.h"
#include "BESDataHandlerInterface.h"
#include "BESSyntaxUserError.h"
#include "BESRequestHandler.h"


// hdf5 includes
//#include <H5Dpkg.h>
#include <H5Ppublic.h>
#include <H5Dpublic.h>
#include <H5Epublic.h>
#include <H5Zpublic.h>  // Constants for compression filters
#include <H5Spublic.h>
#include <../hdf5_handler/HDF5RequestHandler.h>

// module includes
#include "DMRpp.h"
#include "DmrppTypeFactory.h"
#include "DmrppD4Group.h"
#include "DmrppMetadataStore.h"
#include "BESDapNames.h"
#include "bes_default_conf.h"

using namespace std;
using namespace libdap;
using namespace dmrpp;

static bool verbose = false;
static bool very_verbose = false;
#define VERBOSE(x) do { if (verbose) x; } while(false)
#define VERY_VERBOSE(x) do { if (very_verbose) x; } while(false)

// #define DEBUG_KEY "metadata_store,dmrpp_store,dmrpp"
#define DEBUG_KEY "metadata_store,dmrpp_store,dmrpp,bes"
#define ROOT_DIRECTORY "BES.Catalog.catalog.RootDirectory"

//#define H5D_FRIEND		// Workaround, needed to use H5D_chunk_rec_t
#define H5S_MAX_RANK    32
#define H5O_LAYOUT_NDIMS	(H5S_MAX_RANK+1)

#define prolog std::string("ngap_build_dmrpp::").append(__func__).append("() - ")
#define MODULE "dmrpp"

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
    hsize_t     scaled[H5O_LAYOUT_NDIMS];    /* Logical offset to start */
    uint32_t    nbytes;                      /* Size of stored data */
    uint32_t    filter_mask;                 /* Excluded filters */
    haddr_t     chunk_addr;                  /* Address of chunk in file */
} H5D_chunk_rec_t;



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
        VERY_VERBOSE(cerr << "Number of filters associated with dataset: " << numfilt << endl);

        for (int filter = 0; filter < numfilt; filter++) {
            size_t nelmts = 0;
            unsigned int flags, filter_info;
            H5Z_filter_t filter_type = H5Pget_filter2(plist_id, filter, &flags, &nelmts, NULL, 0, NULL, &filter_info);
            VERY_VERBOSE(cerr << "Filter Type: ");

            switch (filter_type) {
            case H5Z_FILTER_DEFLATE:
                VERY_VERBOSE(cerr << "H5Z_FILTER_DEFLATE" << endl);
                dc->set_deflate(true);
                break;
            case H5Z_FILTER_SHUFFLE:
                VERY_VERBOSE(cerr << "H5Z_FILTER_SHUFFLE" << endl);
                dc->set_shuffle(true);
                break;
            default: {
                ostringstream oss("Unsupported HDF5 filter: ", std::ios::ate);
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
    std::string byteOrder = "";
    H5T_order_t byte_order = H5T_ORDER_ERROR;

    try {
        hid_t dcpl = H5Dget_create_plist(dataset);
        uint8_t layout_type = H5Pget_layout(dcpl);

        hid_t fspace_id = H5Dget_space(dataset);
        hid_t ftype_id = H5Dget_type(dataset);

        byte_order = H5Tget_order(ftype_id);
        switch (byte_order) {
            case H5T_ORDER_LE: byteOrder = "LE"; break;
            case H5T_ORDER_BE: byteOrder = "BE"; break;
            case H5T_ORDER_NONE: break;
            default:
                ostringstream oss("Unsupported HDF5 dataset byteOrder: ", std::ios::ate);
                oss << byte_order << ".";
                BESInternalError(oss.str(), __FILE__, __LINE__);
                break; // unsupported enumerations: H5T_ORDER_[ERROR,VAX,MIXED,NONE]
        }

        unsigned int dataset_rank = H5Sget_simple_extent_ndims(fspace_id);

        /* layout_type:  1 contiguous 2 chunk 3 compact */
        switch (layout_type) {

        case H5D_CONTIGUOUS: { /* Contiguous storage */
            haddr_t cont_addr = 0;
            hsize_t cont_size = 0;
            VERBOSE(cerr << "Storage: contiguous" << endl);


            cont_addr = H5Dget_offset(dataset);
            /* if statement never less than zero due to cont_addr being unsigned int. SBL 1.29.20
            if (cont_addr < 0) {
            		throw BESInternalError("Cannot obtain the offset.", __FILE__, __LINE__);
            }*/
            cont_size = H5Dget_storage_size(dataset);
            /* if statement never less than zero due to cont_size being unsigned int. SBL 1.29.20
            if (cont_size < 0) {
            		throw BESInternalError("Cannot obtain the storage size.", __FILE__, __LINE__);
            }*/
            VERBOSE(cerr << "    Addr: " << cont_addr << endl);
            VERBOSE(cerr << "    Size: " << cont_size << endl);
            VERBOSE(cerr << "byteOrder: " << byteOrder << endl);

            if (cont_size > 0) {
                if (dc) dc->add_chunk("", byteOrder, cont_size, cont_addr, "" /*pos in array*/);
            }
            break;
        }

        case H5D_CHUNKED: { /*chunking storage */
            hsize_t num_chunks = 0;
            herr_t status = H5Dget_num_chunks(dataset, fspace_id, &num_chunks);
            if (status < 0) {
                throw BESInternalError("Could not get the number of chunks",
                __FILE__, __LINE__);
            }

            VERY_VERBOSE(cerr << "storage: chunked." << endl);
            VERY_VERBOSE(cerr << "Number of chunks is " << num_chunks << endl);

            if (dc)
            	set_filter_information(dataset, dc);

            // Get chunking information: rank and dimensions
            vector<size_t> chunk_dims(dataset_rank);
            unsigned int chunk_rank = H5Pget_chunk(dcpl, dataset_rank, (hsize_t*) chunk_dims.data());
            if (chunk_rank != dataset_rank)
                throw BESNotFoundError("Found a chunk with rank different than the dataset's (aka variables's) rank", __FILE__, __LINE__);

            if (dc) dc->set_chunk_dimension_sizes(chunk_dims);

            for (unsigned int i = 0; i < num_chunks; ++i) {

                vector<hsize_t> temp_coords(dataset_rank);
                vector<unsigned int> chunk_coords(dataset_rank); //FIXME - see below

                haddr_t addr = 0;
                hsize_t size = 0;

                //H5_DLL herr_t H5Dget_chunk_info(hid_t dset_id, hid_t fspace_id, hsize_t chk_idx, hsize_t *coord, unsigned *filter_mask, haddr_t *addr, hsize_t *size);
                status = H5Dget_chunk_info(dataset, fspace_id, i, temp_coords.data(), NULL, &addr, &size);
                if (status < 0) {
                    VERBOSE(cerr << "ERROR" << endl);
                    throw BESInternalError("Cannot get HDF5 dataset storage info.", __FILE__, __LINE__);
                }

                VERY_VERBOSE(cerr << "chk_idk: " << i  << ", addr: " << addr << ", size: " << size << endl);

                //The coords need to be of type 'unsigned int' when passed into add_chunk()
                // This loop simply copies the values from the temp_coords to chunk_coords - kln 5/1/19
                for (unsigned int j = 0; j < chunk_coords.size(); ++j) {
                    chunk_coords[j] = temp_coords[j];
                }

                // FIXME Modify add_chunk so that it takes a vector<unsigned long long> or <unsigned long>
                // (depending on the machine/OS/compiler). Limiting the offset to 32-bits won't work
                // for large files. jhrg 5/21/19
                if (dc) dc->add_chunk("", byteOrder, size, addr, chunk_coords);
            }

            break;
        }

        case H5D_COMPACT: { /* Compact storage */
            //else if (layout_type == 3) {
            VERBOSE(cerr << "Storage: compact" << endl);
            size_t comp_size = 0;
            //if (H5Dget_dataset_compact_storage_info(dataset, &comp_size) < 0) {
            //    throw BESInternalError("Cannot obtain the compact storage info.", __FILE__, __LINE__);
            //}
            comp_size = H5Dget_storage_size(dataset);
			/* if statement never less than zero due to comp_size being unsigned int. SBL 1.29.20
            if (comp_size < 0) {
				throw BESInternalError("Cannot obtain the compact storage size.", __FILE__, __LINE__);
			}*/
            VERBOSE(cerr << "   Size: " << comp_size << endl);

            break;
        }

        default: {
            ostringstream oss("Unsupported HDF5 dataset layout type: ", std::ios::ate);
            oss << layout_type << ".";
            BESInternalError(oss.str(), __FILE__, __LINE__);
        }
        } // end switch
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
        // relies on the variable's position in the DAP dataset hierarchy.
        D4Attribute *attr = d4_attrs->get("fullnamepath");
        string FQN;
        // I believe the logic is more clear in this way: 
        // If fullnamepath exists and the H5Dopen2 fails to open, it should throw an error.
        // If fullnamepath doesn't exist, we should ignore the error as the reason described below:
        // (However, we should suppress the HDF5 dataset open error message.)  KY 2019-12-02
        // It's not an error if a DAP variable in a DMR from the hdf5 handler
        // doesn't exist in the file _if_ there's no 'fullnamepath' because
        // that variable was synthesized (likely for CF compliance)
        hid_t dataset = -1;
        if(attr) {
            if(attr->num_values() == 1) 
                FQN = attr->value(0);
            else
                FQN = (*v)->FQN();
            BESDEBUG("dmrpp","Working on: " <<FQN<<endl);
            dataset = H5Dopen2(file, FQN.c_str(), H5P_DEFAULT);
            if(dataset <0) 
                 throw BESInternalError("HDF5 dataset '" + FQN + "' cannot be opened.", __FILE__, __LINE__);

        }
        else {
            // The current design seems to still prefer to open the dataset when the fullnamepath doesn't exist
            // So go ahead to open the dataset. Continue even if the dataset cannot be open. KY 2019-12-02
            H5Eset_auto2(H5E_DEFAULT,NULL,NULL);
            FQN = (*v)->FQN();
            BESDEBUG("dmrpp","Working on: " <<FQN<<endl);
            dataset = H5Dopen2(file, FQN.c_str(), H5P_DEFAULT);
            if(dataset <0)
                continue;
        }

 
#if 0
        if (attr && attr->num_values() == 1)
            FQN = attr->value(0);
        else
            FQN = (*v)->FQN();

        VERBOSE(cerr << "Working on: " << FQN << endl);
        hid_t dataset = H5Dopen2(file, FQN.c_str(), H5P_DEFAULT);
        // It's not an error if a DAP variable in a DMR from the hdf5 handler
        // doesn't exist in the file _if_ there's no 'fullnamepath' because
        // that variable was synthesized (likely for CF compliance)
        if (dataset < 0 && attr == 0) {
            cerr<<"Unable to open dataset name "<<FQN <<endl;
            continue;
        }
        else if (dataset < 0)
            throw BESInternalError("HDF5 dataset '" + FQN + "' cannot be opened.", __FILE__, __LINE__);
#endif
        get_variable_chunk_info(dataset, dynamic_cast<DmrppCommon*>(*v));
     }

    // all groups in the group
    D4Group::groupsIter g = group->grp_begin();
    D4Group::groupsIter ge = group->grp_end();
    while (g != ge)
        get_chunks_for_all_variables(file, *g++);
}


/**
 * @brief Creates a temporary bes configuration file
 *
 * @param bes_conf_filename User supplied bes conf filename (is any)
 * @param data_root The data root for this bes invocation
 * @param pid Process id
 * @return The name of the bes conf file to utilize.
 */
string mktemp_bes_conf(const string &bes_conf_filename, const string &data_root, const pid_t &pid){
    stringstream tmp_conf_filename;

    // If the file name is valid then good enough. (empty string is not valid)
    std::ifstream istrm(bes_conf_filename);
    if (istrm.is_open()) {
        return bes_conf_filename;
    }
    if(!bes_conf_filename.empty()){
        cerr << "WARNING: Failed to access the supplied bes configuration file: " << bes_conf_filename << endl;
        cerr << "WARNING: Continuing with default configuration file." << endl;
    }
    
    // Otherwise use the default config and modify it for this invocation.
    string root_dir_key = "@hdf5_root_directory@";
    int index = 0;
    if(very_verbose) cerr << "start: current index: " << index << endl;
    while ((index = BES_CONF_DOC.find(root_dir_key)) != -1){
        if(very_verbose)  cerr << "current index: " << index << endl;
        BES_CONF_DOC.erase(index, root_dir_key.size());
        BES_CONF_DOC.insert(index, data_root);
    }
    tmp_conf_filename << "/tmp/nbd_" << pid << "_bes.conf";

    std::ofstream ofs(tmp_conf_filename.str(), std::ofstream::out);
    if(!ofs.is_open()){
        string msg = "Failed to open temporary file: " + tmp_conf_filename.str();
        BESDEBUG(MODULE, prolog << msg << endl);
        throw BESInternalError(msg,__FILE__,__LINE__);
    }
    ofs << BES_CONF_DOC ;

    if (very_verbose) {
        cerr << "bes_conf: " << endl << BES_CONF_DOC << endl;
    }
    return tmp_conf_filename.str();
}


/**
 * @brief Creates a temporary dmr bes command file
 *
 * @param input_data_file User supplied data file
 * @param pid Process id
 * @return The name of the bes cmd file to utilize
 */
string mktemp_get_dmr_bes_cmd(const string &input_data_file, const pid_t &pid) {

    ////////////////////////////////////////////////////
    // Get dmr bes command template
    //
    std::string get_dmr_bes_cmd =
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
            "<bes:request xmlns:bes=\"http://xml.opendap.org/ns/bes/1.0#\" reqID=\"get_dmrpp.sh\">\n"
            "    <bes:setContext name=\"dap_explicit_containers\">no</bes:setContext>\n"
            "    <bes:setContext name=\"errors\">xml</bes:setContext>\n"
            "    <bes:setContext name=\"max_response_size\">0</bes:setContext>\n"
            ""
            "    <bes:setContainer name=\"c\">DATAFILE_NAME</bes:setContainer>\n"
            ""
            "    <bes:define name=\"d\" space=\"default\">\n"
            "        <bes:container name=\"c\"/>\n"
            "    </bes:define>\n"
            ""
            "    <bes:get type=\"dmr\" definition=\"d\"/>\n"
            "</bes:request>\n"
    ;


    stringstream bes_cmd_filename;
    string key = "DATAFILE_NAME";
    int index = 0;
    if( (index=get_dmr_bes_cmd.find(key)) != -1){
        get_dmr_bes_cmd.erase(index, key.size());
        get_dmr_bes_cmd.insert(index, input_data_file);
    }

    bes_cmd_filename << "/tmp/nbd_" << pid << "_bes.cmd";
    std::ofstream ofs(bes_cmd_filename.str(), std::ofstream::out);
    if(!ofs.is_open()){
        string msg = "Failed to open temporary file: " + bes_cmd_filename.str();
        BESDEBUG(MODULE, prolog << msg << endl);
        throw BESInternalError(msg,__FILE__,__LINE__);
    }
    ofs << get_dmr_bes_cmd ;

    if(very_verbose){
        cerr << "bes_cmd: " << endl << get_dmr_bes_cmd << endl;
    }
    if(very_verbose){
        cerr << "bes_cmd_filename: " << bes_cmd_filename.str() << endl;
    }
    return bes_cmd_filename.str();

}


/**
 * @brief Builds a dmr using the besstandalone via the StandAloneApp
 * Build DMR using besstandalone (aka StandAloneApp)
 *
 * @param bes_conf_filename The name of the bes configuration file to use
 * @param bes_cmd_filename The filename chat contains the bes command(s) to execute
 * @param output_filename The output filename. If the string is empty, stdout will be used.
 */
void build_dmr_with_StandAloneApp(
        const string &bes_conf_filename,
        const string &bes_cmd_filename,
        const string &output_filename) {

    if(verbose){ cerr << "                          BEGIN: build_dmr_with_StandAloneApp()" << endl; }

    BESStopWatch sw;
    sw.start("build_dmrpp::build_dmr_with_StandAloneApp()");

    // TheBESKeys::ConfigFile = bes_conf_filename;

    string name("ngap_build_dmrpp");
    string conf_param("-c");
    string cmd_param("-i");
    string output_file_param("-f");

    std::vector<std::string> arguments;
    arguments.push_back(name);
    arguments.push_back(conf_param);
    arguments.push_back(bes_conf_filename);
    arguments.push_back(cmd_param);
    arguments.push_back(bes_cmd_filename);
    arguments.push_back(output_file_param);
    arguments.push_back(output_filename);

    std::vector<char*> argv;
    for(unsigned i=0; i<arguments.size() ; i++){
        const string &arg = arguments[i];
        argv.push_back((char*)arg.data());
    }
    argv.push_back(0);

    if(very_verbose) {
        for (unsigned i = 0; i < argv.size()-1; i++) {
            cerr << "                        argv[" << i << "]: " << argv[i] << endl;
        }
    }
    cerr << "        Command line equivalent: " << "besstandalone ";
    for (unsigned i = 1; i < argv.size()-1; i++) { cerr << argv[i] << " "; }
    cerr << endl;

    StandAloneApp app;
    app.main(argv.size()-1, argv.data());
    if (verbose) {
        cerr << "        besstandalone output to: " << output_filename << endl;
    }
    if(verbose){ cerr << "                            END: build_dmr_with_StandAloneApp()" << endl; }
}


/**
 * @brief Returns a DMR instance built by the HDF5RequestHandler from the input data file.
 *
 * @param bes_conf_filename The name of the bes configuration file to use
 * @param input_data_file
 * @param url If url is not empty then make the vaolue of the dmrpp:href attribute in the
 * Sataset element of the generated DMR.
 * @return The DMR instance built from input_data_file.
 */
DMR *build_hdf5_dmr(const string &bes_conf_filename, const string &input_data_file, const string &url){

    if(verbose){ cerr << "                          BEGIN: build_hdf5_dmr()" << endl; }
    DMR *h5_dmr = 0;
#if 0

    BESStopWatch sw;
    sw.start("build_dmrpp::build_hdf5_dmr()");

    TheBESKeys::ConfigFile = bes_conf_filename;

    HDF5RequestHandler *h5rh = new HDF5RequestHandler("h5_handler");
    BESDataHandlerInterface dhi;
    BESFileContainer *bfc = new BESFileContainer("target_file", input_data_file, "h5" );
    dhi.container = bfc;

    h5_dmr = new DMR();
    h5_dmr->set_dap_version("4.0");
    if (url.empty()) h5_dmr->set_request_xml_base(url.c_str());
    BESDMRResponse *response_object = new BESDMRResponse(h5_dmr);

    BESDMRResponseHandler dmrh("lulu the dmr response handler.");
    dmrh.set_response_object(response_object);
    dhi.response_handler = &dmrh;

    dhi.container->set_dap4_constraint("");
    dhi.container->set_dap4_function("");

    h5rh->hdf5_build_dmr(dhi);

   // delete bfc;
   // delete dmrh;
   // delete response_object;
   // delete h5rh;
#else
                 cerr << "     dmr++ prod via/hd5_handler: DISABLED. SKIPPING" << endl;
#endif

    if(verbose){ cerr << "                            END: build_hdf5_dmr()" << endl; }
    return h5_dmr;
}


/**
 * @brief Generates a dmrpp file using an input stream DMR
 *
 * @param input_data_file The name of the hdf5/netcdf-4 file to ingest
 * @param dmr_istrm The input stream containing the dmr of the same file
 * @param url_name The url that points to the file (http, https, file, etc)
 * @return Returns 0 on success non-zero otherwise.
 */
int generate_dmrpp_from_stream_dmr(const string &input_data_file, istream &dmr_istrm, const string &url_name, ostream  *dmrpp_ostrm){


    int status=0;
    hid_t file = 0;
    try {
        // Turn off automatic hdf5 error printing.
        // See: https://support.hdfgroup.org/HDF5/doc1.8/RM/RM_H5E.html#Error-SetAuto2
        //if (!verbose) H5Eset_auto2(H5E_DEFAULT, NULL, NULL);

        // For a given HDF5, get info for all the HDF5 datasets in a DMR or for a
        // given HDF5 dataset
        // Get dmr:
        unique_ptr<DMRpp> dmrpp(new DMRpp);
        DmrppTypeFactory dtf;
        dmrpp->set_factory(&dtf);

        D4ParserSax2 parser;
        parser.intern(dmr_istrm, dmrpp.get(), false);

        // Open the hdf5 file
        file = H5Fopen(input_data_file.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
        if (file < 0) {
            cerr << prolog  << "Error: HDF5 file '" + input_data_file + "' cannot be opened." << endl;
            return 1;
        }

        // iterate over all the variables in the DMR
        get_chunks_for_all_variables(file, dmrpp->root());

        XMLWriter writer;
        dmrpp->print_dmrpp(writer, url_name);

        *dmrpp_ostrm << writer.get_doc();
    }
    catch (BESError &e) {
            cerr << prolog  << "BESError: " << e.get_message() << endl;
        status = 1;
    }
    catch (std::exception &e) {
        cerr << prolog  << "std::exception: " << e.what() << endl;
        status = 1;
    }
    catch (...) {
        cerr << prolog  << "Unknown error." << endl;
        status = 1;
    }

    H5Fclose(file);

    return status;

}

/**
 * @brief Generates a dmrpp file using an input stream DMR
 *
 * @param input_data_file The name of the hdf5/netcdf-4 file to ingest
 * @param dmr_istrm The input stream containing the dmr of the same file
 * @param url_name The url that points to the file (http, https, file, etc)
 * @return Returns 0 on success non-zero otherwise.
 */
int generate_dmrpp_from_mds_dmr(const string &input_data_file, const string &url_name, ostream  *dmrpp_ostrm){

    int status=0;
    hid_t file = 0;
    try {
        // Turn off automatic hdf5 error printing.
        // See: https://support.hdfgroup.org/HDF5/doc1.8/RM/RM_H5E.html#Error-SetAuto2
        //if (!verbose) H5Eset_auto2(H5E_DEFAULT, NULL, NULL);

        bool found;
        string bes_data_root;
        try {
            TheBESKeys::TheKeys()->get_value(ROOT_DIRECTORY, bes_data_root, found);
            if (!found) {
                cerr << prolog << "Error: Could not find the BES root directory key." << endl;
                return 1;
            }
        }
        catch (BESError &e) {
            cerr << prolog  << "BESError: " << e.get_message() << endl;
            return 1;
        }

        // Use the values from the bes.conf file... jhrg 5/21/18
        bes::DmrppMetadataStore *mds = bes::DmrppMetadataStore::get_instance();
        if (!mds) {
            cerr << prolog  << "The Metadata Store (MDS) must be configured for this command to work." << endl;
            return 1;
        }

        // Use the full path to open the file, but use the 'name' (which is the
        // path relative to the BES Data Root) with the MDS.
        // Changed this to utilze assmeblePath() because simply concatenating the strings
        // is fragile. - ndp 6/6/18
        string h5_file_path = BESUtil::assemblePath(bes_data_root,input_data_file);

        //bes::DmrppMetadataStore::MDSReadLock lock = mds->is_dmr_available(input_data_file /*h5_file_path*/);
        bes::DmrppMetadataStore::MDSReadLock lock = mds->is_dmr_available(h5_file_path, input_data_file, "h5");
        if (lock()) {
            // parse the DMR into a DMRpp (that uses the DmrppTypes)
            unique_ptr<DMRpp> dmrpp(dynamic_cast<DMRpp*>(mds->get_dmr_object(input_data_file /*h5_file_path*/)));
            if (!dmrpp.get()) {
                cerr << prolog  << "Expected a DMR++ object from the DmrppMetadataStore." << endl;
                return 1;
            }

            // Open the hdf5 file
            file = H5Fopen(h5_file_path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
            if (file < 0) {
                cerr << prolog  << "Error: HDF5 file '" + h5_file_path + "' cannot be opened." << endl;
                return 1;
            }

            get_chunks_for_all_variables(file, dmrpp->root());

            dmrpp->set_href(url_name);

            mds->add_dmrpp_response(dmrpp.get(), input_data_file /*h5_file_path*/);

            XMLWriter writer;
            dmrpp->set_print_chunks(true);
            dmrpp->print_dap4(writer);

            *dmrpp_ostrm << writer.get_doc();
        }
        else {
            cerr  << prolog << "Error: Could not get a lock on the DMR for '" + h5_file_path + "'." << endl;
            return 1;
        }

    }
    catch (BESError &e) {
        cerr << prolog  << "BESError: " << e.get_message() << endl;
        status = 1;
    }
    catch (std::exception &e) {
        cerr << prolog  << "std::exception: " << e.what() << endl;
        status = 1;
    }
    catch (...) {
        cerr << prolog  << "Unknown error." << endl;
        status = 1;
    }

    H5Fclose(file);

    return status;

}


/**
 * @brief Generates a dmrpp file
 *
 * @param input_data_file The name of the data file to use
 * @param dmr_filename The string of the dmr file
 * @param url_name The path to the file
 * @param output_filename The filename of the output file
 * @return
 */
int generate_dmrpp(const string &input_data_file, const string &dmr_filename, const string &url_name, const string &output_filename) {

    int status = 0;
    if (input_data_file.empty()) {
        cerr << endl << "ERROR. Input HDF5 file name must be provided . Run again with -h to see usage document.." << endl;
        return 1;
    }

    //  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
    // Set up output stream, default output goes to stdout.
    std::ostream *dmrpp_ostrm = &cout;
    string debug_msg="              Writing output to: stdout";
    // If the output_filename is not valid, if the stream will not open. Empty is not valid.
    std::ofstream ofs(output_filename, std::ofstream::out);
    if(ofs.is_open()){
        // We'll be writing the output to the file.
        dmrpp_ostrm = &ofs;
        debug_msg = "           Writing output to:  " + output_filename;
    }
    if(verbose){ cerr << debug_msg << endl;}

    //  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
    // Set up dmr input stream.
    // If no valid dmr input file is provided the code tries to find a dmr in the mds.
    std::ifstream dmr_istream(dmr_filename, std::ifstream::in);

    // If the dmr_filename is not valid, the stream will not open. Empty is not valid.
    if(dmr_istream.is_open()){
        // If it's open we've got a valid filename.
        if(verbose){ cerr << "                 Using dmr file: " << dmr_filename << endl;}
        status = generate_dmrpp_from_stream_dmr(input_data_file, dmr_istream, url_name, dmrpp_ostrm);
    }
    else {
        // Didn't open the file, try the mds.
        if(verbose){ cerr << "           No DMR file provided. Checking MDS for dmr++: " << output_filename << endl;}
        status =  generate_dmrpp_from_mds_dmr(input_data_file, url_name, dmrpp_ostrm);

    }
    return status;
}



static const std::string usage =
        "\n"
        "Usage: ngap_build_dmrpp [options] -i <input_hdf5_file>\n"
        "\n"
        " Create the DMR++ for the <input_hdf5_file>\n"
        "\n"
        " Options:\n"
        "  -h: Show help\n"
        "  -v: Verbose: Print the DMR too\n"
        "  -V: Very Verbose: print the DMR, the command and the configuration\n"
        "     file used to build the DMR\n"
        "  -b: Turn on bes debug with these switches,.\n"
        "\n"
        "  -c: The path to the bes configuration file to use. Optional\n"
        "  -d: Use the DMR in this file, don't make one.\n"
        "  -i: The input hdf5 data filename relative to data root. (See -t)\n"
        "  -o: The output data file name. (default: stdout).\n"
        "  -u: The binary object URL Url (http(s):// or file://) where the input \n"
        "      hdf5 data file can be located. (This is injected into dmr++ file).\n"
        // "  -r: Output only the DMR, don't build DMR++\n"
        "  -t: Data root directory for the BES. (Defaults to the CWD)\n"
        "  -X: Run the alternate DMR production using hdf5_handler components.\n"
        "\n"
        "Please take note: \n"
        " * By default the BES Data Root directory is set to the CWD.\n"
        " * This utility will add entries into the bes log file specified\n"
        "   in the BES configuration file. If no BES configuration is \n"
        "   provided then the output will be written to CWD/bes.log\n"
        " * The DMR++ is built using a DMR created from the <input_hdf5_file> \n"
        "   unless a DMR file is supplied by the user using the -d switch.\n"
        " * The pathanme to the hdf5 file must be relative from the BES data root\n"
        "   directory, which by default it's the CWD of the shell from which this\n"
        "   command was run; absolute paths will not work.\n"
        "\n"
        "";
/**
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char*argv[]) {
    string h5_file_name = "";
    string h5_dset_path = "";
    string dmr_name = "";
    string url_name = "";
    string data_root = ".";
    string bes_conf_filename = "";
    bool just_dmr = false;
    int status = 0;
    string input_data_file = "";
    string output_data_file = "";
    bool run_alternate = false;
    string dmr_filename = "";

    GetOpt getopt(argc, argv, "d:t:c:i:u:o:bhvVX");
    int option_char;
    while ((option_char = getopt()) != -1) {
        switch (option_char) {
            case 'v':
                verbose = true;
                break;
            case 'V':
                verbose = true;
                very_verbose = true;
                break;
            case 'b':
                BESDebug::SetUp(string("cerr,").append(DEBUG_KEY));
                break;
            case 'd':
                dmr_filename = getopt.optarg;
                break;
            case 'i':
                input_data_file = getopt.optarg;
                break;
            case 'o':
                output_data_file = getopt.optarg;
                break;
            case 'u':
                url_name = getopt.optarg;
                break;
            case 'c':
                bes_conf_filename = getopt.optarg;
                break;
            //case 'r':
            //    just_dmr = true;
            //    break;
            case 't':
                data_root = getopt.optarg;
                break;
            case 'X':
                run_alternate = true;
                break;
            case 'h':
                cerr << usage << endl;
                exit(1);
            default:
                break;
        }
    }

    if (input_data_file.empty()) {
        cerr << endl << "ERROR - An input HDF5 filename must be provided." << endl << endl;
        cerr << usage << endl;
        exit(1);
    }
    if (verbose) cerr << "          Using input_data_file: " << input_data_file << endl;

    pid_t pid = getpid();

    bes_conf_filename = mktemp_bes_conf(bes_conf_filename, data_root, pid);
    if (verbose) { cerr << "              bes_conf_filename: " << bes_conf_filename << endl; }

    if (!run_alternate) {

        if(dmr_filename.empty()){
            string bes_cmd_filename = mktemp_get_dmr_bes_cmd(input_data_file, pid);
            if (verbose) { cerr << "               bes_cmd_filename: " << bes_cmd_filename << endl; }

            stringstream dmrfn;
            dmrfn << "/tmp/nbd_" << pid << ".dmr";
            dmr_filename = dmrfn.str();
            if (verbose) { cerr << "                   dmr_filename: " << dmr_filename << endl; }
            build_dmr_with_StandAloneApp(bes_conf_filename, bes_cmd_filename, dmr_filename);
            if(just_dmr)
                return 0;
        }

        status = generate_dmrpp(input_data_file, dmr_filename, url_name, output_data_file);
    }
    else { // Build DMR using direct calls into the BES stack

        XMLWriter xmlWriter("  ");

        // Build a dmr by making calls to the hdf5_module code.
        DMR *h5_dmr = build_hdf5_dmr(bes_conf_filename, input_data_file, url_name);
        if(!h5_dmr)
            return 1;

        // Write that dmr as an XML doc
        h5_dmr->print_dap4(xmlWriter);
        delete h5_dmr;

        // Turn the XML doc into an input stream using  istringstream
        istringstream dmr_istrm(xmlWriter.get_doc());
        if (very_verbose) { cerr << endl << xmlWriter.get_doc() << endl; }

        //  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
        // Set up output stream, default output goes to stdout.
        std::ostream *dmrpp_ostrm = &cout;
        string debug_msg="              Writing output to: stdout";
        // If the output_filename is not valid, if the stream will not open. Empty is not valid.
        std::ofstream ofs(output_data_file, std::ofstream::out);
        if(ofs.is_open()){
            // We'll be writing the output to the file.
            dmrpp_ostrm = &ofs;
            debug_msg = "           Writing output to:  " + output_data_file;
        }
        if(verbose){ cerr << debug_msg << endl;}

        // Pass that stream to generate_dmrpp.
        status = generate_dmrpp_from_stream_dmr(input_data_file, dmr_istrm, url_name, dmrpp_ostrm);
    }
    return status;
}
