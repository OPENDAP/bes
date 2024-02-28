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

#include "h4config.h"
#include "hdf.h"  // HDF4 header file
#include "mfhdf.h"  // Include the HDF4 header file

#include <libdap/Str.h>
#include <libdap/Structure.h>
#include <libdap/util.h>
#include <libdap/D4Attributes.h>

#include <BESNotFoundError.h>
#include <BESInternalError.h>
#include <BESInternalFatalError.h>

#include <TheBESKeys.h>

#include "DMRpp.h"
#include "DmrppTypeFactory.h"
#include "DmrppD4Group.h"
#include "DmrppArray.h"
#include "D4ParserSax2.h"

#define COMP_INFO 512 /*!< Max buffer size for compression information.  */

#define FAIL_ERROR(x) do { cerr << "ERROR: " << x << " " << __FILE__ << ":" << __LINE__ << endl; exit(1); } while(false)
#define ERROR(x) do { cerr << "ERROR: " << x << " " << __FILE__ << ":" << __LINE__ << endl; } while(false)

/*
 * Hold mapping information for SDS objects.
 */
using SD_mapping_info_t = struct {
    char comp_info[COMP_INFO];  /*!< compression information */
    int32 nblocks;              /*!< number of data blocks in dataset */
    int32 *offsets;             /*!< offsets of data blocks */
    int32 *lengths;             /*!< lengths (in bytes) of data blocks */
    int32 id;                   /*!< SDS id  */
    int32 data_type;            /*!< data type */
    intn is_empty;              /*!< flag for checking empty  */
    VOIDP fill_value;           /*!< fill value  */
};

using namespace std;
using namespace libdap;
using namespace dmrpp;

namespace build_dmrpp_util_h4 {

bool verbose = false;   // Optionally set by build_dmrpp's main().

#define VERBOSE(x) do { if (verbose) (x); } while(false)
#define prolog std::string("build_dmrpp_h4::").append(__func__).append("() - ")

#if 0
// will be used later maybe? jhrg 12/7/23
constexpr auto INVOCATION_CONTEXT = "invocation";
#endif

// I believe this is copied from H4mapper. Should give credit.
int SDfree_mapping_info(SD_mapping_info_t  *map_info)
{
    intn  ret_value = SUCCEED;

    /* nothing to free */
    if (map_info == nullptr)
        return SUCCEED;

    map_info->nblocks = 0;

    if (map_info->offsets != nullptr) {
        HDfree(map_info->offsets);
        map_info->offsets = nullptr;
    }

    if (map_info->lengths) {
        HDfree(map_info->lengths);
        map_info->lengths = nullptr;
    }

    return ret_value;
}

size_t combine_linked_blocks(const SD_mapping_info_t &map_info, vector<int> & merged_lengths, vector<int> &merged_offsets) {

    int num_eles = map_info.nblocks;

#if 0
    for (int i = 0; i<offset.size(); i++) {
       cout<<"offset["<<i<<"]= "<<offset[i] <<endl;
       cout<<"length["<<i<<"]= "<<length[i] <<endl;
    }
#endif

    // The first element offset should always be fixed.
    merged_offsets.push_back(map_info.offsets[0]);
    int temp_length = map_info.lengths[0];

    for (int i = 0; i <(num_eles-1); i++) {

        // If not contiguous, push back the i's new length;
        //                    push back the i+1's unchanged offset.
        //                    save the i+1's length to the temp length variable.
        if (map_info.offsets[i+1] !=(map_info.offsets[i] + map_info.lengths[i])) {
            merged_lengths.push_back(temp_length);
            merged_offsets.push_back(map_info.offsets[i+1]);
            temp_length = map_info.lengths[i+1];
        }
        else { // contiguous, just update the temp length variable.
            temp_length +=map_info.lengths[i+1];
        }

    }

    // Update the last length.
    merged_lengths.push_back(temp_length);

    return merged_lengths.size();

}
/**
 * @brief Write chunk position in array.
 * @param rank Array rank
 * @param lengths
 * @param strides Size of each chunk dimension
 * @return
 */
vector<unsigned long long>
write_chunk_position_in_array(int rank, const unsigned long long *lengths, const int32_t *strides)
{
    vector<unsigned long long> chunk_pos;

    for(int i = 0; i < rank; i++){
        unsigned long long index = lengths[i] * (unsigned long long)strides[i];
        chunk_pos.push_back(index);
    }

    return chunk_pos;
}

/**
 * @brief Read chunk information from a HDF4 dataset
 * @param sdsid
 * @param map_info
 * @param origin The parameter origin must be NULL when the data is not stored in chunking layout.
 * When the data are chunked, SDgetdatainfo can be called on a single chunk and origin is used to
 * specify the coordinates of the chunk.
 * Note: The coordinates of a chunk is not the same as the coordinates of this
 * chunk in the whole array. For detailed description and illustration, check section 3.12.3 and FIGURE 3d of
 * the HDF4 user's guide that can be found under https://portal.hdfgroup.org/documentation/ .
 * Note: The code of this method is largely adapted from the HDF4 mapper software implemented by the HDF group.
 * @return
 */

int read_chunk(int sdsid, SD_mapping_info_t *map_info, int *origin)
{
    intn  info_count = 0;
    intn  ret_value = SUCCEED;

    /* Free any info before resetting. */
    SDfree_mapping_info(map_info);
    /* Reset map_info. */
    /* HDmemset(map_info, 0, sizeof(SD_mapping_info_t)); */

    /* Save SDS id since HDmemset reset it. map_info->id will be reused. */
    /* map_info->id = sdsid; */

    // First check if this chunk/data stream has any block of data.
    info_count = SDgetdatainfo(sdsid, origin, 0, 0, nullptr, nullptr);
    if (info_count == FAIL) {
        FAIL_ERROR("SDgetedatainfo() failed in read_chunk().\n");
    }

    // If we find it has data, retrieve the offsets and length information for this chunk or data stream.
    if (info_count > 0) {
        map_info->nblocks = (int32) info_count;
        map_info->offsets = (int32 *)HDmalloc(sizeof(int32)*map_info->nblocks);
        if (map_info->offsets == nullptr) {
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }
        map_info->lengths = (int32 *)HDmalloc(sizeof(int32)*map_info->nblocks);
        if (map_info->lengths == nullptr) {
            FAIL_ERROR("HDmalloc() failed: Out of Memory");
        }

        ret_value = SDgetdatainfo(sdsid, origin, 0, info_count, map_info->offsets, map_info->lengths);
        return ret_value;
    }

    return ret_value;

} /* read_chunk */

string get_sds_fill_value_str(int32 sdsid, int32 datatype) {

    string ret_value;
    switch (datatype) {

        case DFNT_UINT8:
        case DFNT_UCHAR: {
            uint8_t fvalue;
            if (SDgetfillvalue(sdsid, &fvalue) == SUCCEED)
                ret_value = to_string(fvalue);
        }
            break;
        case DFNT_INT8:
        case DFNT_CHAR: {
            int8_t fvalue;
            if (SDgetfillvalue(sdsid, &fvalue) == SUCCEED)
                ret_value = to_string(fvalue);
        }
            break;

        case DFNT_INT16: {
            int16_t fvalue;
            if (SDgetfillvalue(sdsid, &fvalue) == SUCCEED)
                ret_value = to_string(fvalue);
        }
            break;

        case DFNT_UINT16: {
            uint16_t fvalue;
            if (SDgetfillvalue(sdsid, &fvalue) == SUCCEED)
                ret_value = to_string(fvalue);
        }
            break;

        case DFNT_INT32: {
            int32_t fvalue;
            if (SDgetfillvalue(sdsid, &fvalue) == SUCCEED)
                ret_value = to_string(fvalue);
        }
            break;

        case DFNT_UINT32: {
            uint32_t fvalue;
            if (SDgetfillvalue(sdsid, &fvalue) == SUCCEED)
                ret_value = to_string(fvalue);
        }
            break;

        case DFNT_FLOAT: {
            float fvalue;
            if (SDgetfillvalue(sdsid, &fvalue) == SUCCEED)
                ret_value = to_string(fvalue);
        }
            break;

        case DFNT_DOUBLE: {
            double fvalue;
            if (SDgetfillvalue(sdsid, &fvalue) == SUCCEED)
                ret_value = to_string(fvalue);

        }
            break;


    }
    return ret_value;
}
void SD_set_fill_value(int32 sdsid, int32 datatype, BaseType *btp) {

    string fill_value = get_sds_fill_value_str(sdsid,datatype);
    if (fill_value.empty()==false) {
         auto dc = dynamic_cast<DmrppCommon *>(btp);
         if (!dc)
            throw BESInternalError("Expected to find a DmrppCommon instance for " + btp->name() + " but did not.",
         __FILE__, __LINE__);
        dc->set_uses_fill_value(true);
        dc->set_fill_value_string(fill_value);
    }
}
bool  ingest_sds_info_to_chunk(int file, int32 obj_ref, BaseType *btp) {

    int32 sds_index = SDreftoindex(file, obj_ref);
    int sdsid = SDselect(file, sds_index);

    VERBOSE(cerr << "Name: " << btp->name() << endl);
    VERBOSE(cerr << "DMR FQN: " << btp->FQN() << endl);
    VERBOSE(cerr << "sdsid: " << sdsid << endl);

    char obj_name[H4_MAX_NC_NAME]; /* name of the object */
    int32 rank = -1;                 /* number of dimensions */
    int32 dimsizes[H4_MAX_VAR_DIMS]; /* dimension sizes */
    int32 data_type = -1;            /* data type */
    int32 num_attrs = -1;            /* number of global attributes */

    int status = SDgetinfo(sdsid, obj_name, &rank, dimsizes, &data_type, &num_attrs);
    if (status == FAIL) {
        FAIL_ERROR("SDgetinfo() failed.");
    }

    HDF_CHUNK_DEF cdef;
    int32 chunk_flag = -1;        /* chunking flag */

    status = SDgetchunkinfo(sdsid, &cdef, &chunk_flag);
    if (status == FAIL) {
        FAIL_ERROR("SDgetchunkinfo() failed.");
    }

    switch (chunk_flag) {
        case HDF_NONE: /* No chunking. */
            VERBOSE(cerr << "No chunking.\n");
            break;
        case HDF_CHUNK: /* Chunking. */
        case HDF_COMP: /* Compression. */
            if (rank <= 0) {
                ERROR("Invalid rank.");
                return false;
            }
            VERBOSE(cerr << "HDF_CHUNK or HDF_COMP.\n");
            break;
        case HDF_NBIT:
            /* NBIT compression. */
            ERROR("NBit Compression chunking not supported.");
            return false;
        default:
            ERROR("Unknown chunking flag.");
            return false;
    }

    string endian_name;
    hdf_ntinfo_t info;          /* defined in hdf.h near line 142. */
    int result = Hgetntinfo(data_type, &info);
    if (result == FAIL) {
        FAIL_ERROR("Hgetntinfo() failed.");
    }
    else {
        if (strncmp(info.byte_order, "bigEndian", 9) == 0)
            endian_name = "BE";
        else if (strncmp(info.byte_order, "littleEndian", 12) == 0)
            endian_name = "LE";
        else
            endian_name = "UNKNOWN";
    }

    // Check if SDS has no data. But, what should we do??? jhrg 12/7/23
    int is_empty = 0;
    status = SDcheckempty(sdsid, &is_empty);
    if (status == FAIL) {
        FAIL_ERROR("SDcheckempty() failed.");
    }

    if (is_empty) {
        // FIXME Maybe this is the case where the variable is just fill values? jhrg 12/7/23
        // No, this is mostly an SDS with unlimited dimension and the dimension size is 0.
        // Now, we can just ignore since it doesn't contain any data. KY 02/12/24
        VERBOSE(cerr << "SDS is empty." << endl);
        return false;
    }

    auto dc = dynamic_cast<DmrppCommon *>(btp);
    if (!dc)
        throw BESInternalError("Expected to find a DmrppCommon instance for " + btp->name() + " but did not.", __FILE__, __LINE__);

    // Need to check SDS compression info. Unlike HDF5, HDF4 can be compressed without using chunks.
    // So we need to cover both cases. KY 02/12/2024

    comp_coder_t        comp_coder_type = COMP_CODE_NONE;
    comp_info                    c_info;

    if (SDgetcompinfo(sdsid, &comp_coder_type, &c_info) == FAIL) {
             FAIL_ERROR("SDgetcompinfo() failed.");
    }

    // Also need to consider the case when the variable is compressed but not chunked.
    if (chunk_flag == HDF_CHUNK || chunk_flag == HDF_COMP ) {
        vector<unsigned long long> chunk_dimension_sizes(rank, 0);
        for (int i = 0; i < rank; i++) {
            if (chunk_flag == HDF_CHUNK) {
                chunk_dimension_sizes[i] = cdef.chunk_lengths[i];
            }
            else {  // chunk_flag is HDF_COMP
                chunk_dimension_sizes[i] = cdef.comp.chunk_lengths[i];
            }
        }
        if (chunk_flag == HDF_COMP) {
            // Add the deflate level. KY 02/20/24
            if (comp_coder_type == COMP_CODE_DEFLATE) {
                dc->ingest_compression_type("deflate");
                vector<unsigned int> deflate_levels;
                deflate_levels.push_back(c_info.deflate.level);
                dc->set_deflate_levels(deflate_levels);
            } else
                FAIL_ERROR("Encounter unsupported compression method. Currently only support deflate compression.");
        }
        dc->set_chunk_dimension_sizes(chunk_dimension_sizes);

        SD_mapping_info_t map_info;
        map_info.is_empty = 0;
        map_info.nblocks = 0;
        map_info.offsets = nullptr;
        map_info.lengths = nullptr;
        map_info.data_type = data_type;

        vector<unsigned long long> position_in_array(rank, 0);

        /* Initialize steps. */
        vector<int> steps(rank, 0);
        vector<int32_t> strides(rank, 0);
        int number_of_chunks = 1;
        for(int i = 0; i < rank; i++) {

            // This doesn't consider the un-even chunk case. It will miss the extra chunk(s). So correct it. KY 2/20/24
#if 0
            steps[i] = (dimsizes[i] / chunk_dimension_sizes[i]) ;
#endif
            steps[i] =  1 + ((dimsizes[i] - 1) / chunk_dimension_sizes[i]);
            number_of_chunks = number_of_chunks * steps[i];
            strides[i] = 0;
        }

        VERBOSE(cerr << "number_of_chunks: " << number_of_chunks << endl);
        VERBOSE(cerr << "rank: " << rank << endl);
        VERBOSE(cerr << "chunk dimension sizes: ");
        VERBOSE(copy(chunk_dimension_sizes.begin(), chunk_dimension_sizes.end(),
                     ostream_iterator<int32>(cerr, " ")));
        VERBOSE(cerr<<endl);


        for (int k = 0; k < number_of_chunks; ++k) {
            auto info_count = read_chunk(sdsid, &map_info, strides.data());
            if (info_count == FAIL) {
                FAIL_ERROR("SDgetedatainfo() failed in read_chunk().");
            }

            auto pia = write_chunk_position_in_array(rank, chunk_dimension_sizes.data(), strides.data());

            // Critical TODO:
            // Very possible that the offset and length obtained for a block cannot be decompressed directly
            // like the HDF5 chunks. Significant work needs to be done in the dmrpp module. New information needs
            // to be added in the dmrpp file. Pending on the ticket HYRAX-1335. KY 02/13/2024.
            // We cannot find a chunked case when the number of blocks is greater than 1. We will issue a failure
            // when we encounter such a case for the time being. KY 02/19/2024.

            if (map_info.nblocks >1) {
#if 0
               cout << "number of blocks in a chunk is: " << map_info.nblocks << endl;
#endif
               FAIL_ERROR("Number of blocks in this chunk is greater than 1.");
            }

            for (int i = 0; i < map_info.nblocks; i++) {
                VERBOSE(cerr << "offsets[" << k << ", " << i << "]: " << map_info.offsets[i] << endl);
                VERBOSE(cerr << "lengths[" << k << ", " << i << "]: " << map_info.lengths[i] << endl);
                dc->add_chunk(endian_name, map_info.lengths[i], map_info.offsets[i], pia);
            }

            // Increase strides for each dimension. The fastest varying dimension is rank-1.
            int scale = 1;
            for(int i = rank-1; i >= 0 ; i--) {
                if ((k+1) % scale == 0) {
                    strides[i] = ++strides[i] % steps[i];
                }
                scale = scale * steps[i];
            }
        }
    }
    else if (chunk_flag == HDF_NONE) {
        SD_mapping_info_t map_info;
        map_info.is_empty = 0;
        map_info.nblocks = 0;
        map_info.offsets = nullptr;
        map_info.lengths = nullptr;
        map_info.data_type = data_type;

         // Also need to consider the case when the variable is compressed but not chunked.
        if (comp_coder_type != COMP_CODE_NONE) {
            if (comp_coder_type == COMP_CODE_DEFLATE) {
                dc->ingest_compression_type("deflate");
                vector<unsigned int> deflate_levels;
                deflate_levels.push_back(c_info.deflate.level);
                dc->set_deflate_levels(deflate_levels);
            }
            else
                FAIL_ERROR("Encounter unsupported compression method. Currently only support deflate compression.");
        }

        vector<int> origin(rank, 0);
        auto info_count = read_chunk(sdsid, &map_info, origin.data());
        if (info_count == FAIL) {
            FAIL_ERROR("SDgetedatainfo() failed in read_chunk().");
        }
        vector<unsigned long long> position_in_array(rank, 0);
        if (map_info.nblocks ==1) {
              dc->add_chunk(endian_name, map_info.lengths[0], map_info.offsets[0], position_in_array);
        }
        else {
            // Here we will see if we can combine the number of contiguous blocks to a bigger one.
            // This is necessary since HDF4 may store small size data in large number of contiguous linked blocks.
            // KY 2024-02-22
            vector<int> merged_lengths;
            vector<int> merged_offsets;
            size_t merged_number_blocks = combine_linked_blocks(map_info, merged_lengths, merged_offsets);
            for (unsigned i = 0; i < merged_number_blocks; i++) {
                VERBOSE(cerr << "offsets[" << i << "]: " << map_info.offsets[i] << endl);
                VERBOSE(cerr << "lengths[" << i << "]: " << map_info.lengths[i] << endl);

                // Critical TODO:
                //  Here is a bug. the position cannot be 0 for the second block if this is treated as a chunk.
                // Also very possible that the offset and length obtained for a block cannot be decompressed directly
                // like the HDF5 chunks. Significant work needs to be done in the dmrpp module. New information needs
                // to be added in the dmrpp file. Pending on the ticket HYRAX-1335. KY 02/13/2024.
                dc->add_chunk(endian_name, merged_lengths[i], merged_offsets[i], position_in_array);
            }
        }
    }
    else {
        // TODO Handle the other cases. jhrg 12/7/23
        FAIL_ERROR("Unknown chunking type.");
    }

    // Add the fillvalue support
    SD_set_fill_value(sdsid,data_type, btp);

    // Need to close the SDS interface. KY 2024-02-22
    SDendaccess(sdsid);
    return true;
}
/**
 * @note see write_array_chunks_byte_stream() in h4mapwriter for the original version of this code.
 * @param file
 * @param btp
 * @return true if the produced output that seems valid, false otherwise.
 */
bool get_chunks_for_an_array(int file, BaseType *btp) {

    // Here we need to retrieve the attribute value dmr_sds_ref of btp.
    D4Attributes *d4_attrs = btp->attributes();
    if (!d4_attrs)
        throw BESInternalError("Expected to find an DAP4 attribute list for " + btp->name() + " but did not.",
                               __FILE__, __LINE__);

    // Look for the full name path for this variable
    // If one was not given via an attribute, use BaseType::FQN() which
    // relies on the variable's position in the DAP dataset hierarchy.
    D4Attribute *attr = d4_attrs->find("dmr_sds_ref");
    int32 obj_ref = 0;
    bool is_sds = false;
    if (attr)
        is_sds = true;
    else {
        attr = d4_attrs->find("dmr_vdata_ref");
        if (!attr)
            throw BESInternalError("Expected to find an attribute that stores either HDF4 SDS reference or HDF4 Vdata reference for " + btp->name() + " but did not.",
                               __FILE__, __LINE__);
    }
    obj_ref = stoi(attr->value(0));
    if (is_sds) {
        if (false == ingest_sds_info_to_chunk(file, obj_ref,btp))
            throw BESInternalError("Cannot retrieve SDS information correctly for  " + btp->name() ,
                               __FILE__, __LINE__);
    }
    // TODO: Later we will add the support of retrieving data from vdata. KY 02/13/24
#if 0
        string name = btp->name();
        const char *sdsName = name.c_str();
        // TODO For a more complete version of this, use references and tags, not names.
        //  Also, the use of FQNs will not, in general, work for HDF4 files, given the
        //  unusual way that HDF4 files are organized. jhrg 12/5/23
        int sds_index = SDnametoindex(file, sdsName);
#endif

    return true;
}

bool get_chunks_for_a_variable(int file, BaseType *btp) {

    switch (btp->type()) {
        case dods_structure_c: {
            auto sp = dynamic_cast<Structure *>(btp);
            //TODO: this needs to be re-written since the data of the whole structure is retrieved. KY-2024-02-26
            for_each(sp->var_begin(), sp->var_end(), [file](BaseType *btp) { get_chunks_for_a_variable(file, btp); });
            return true;
        }
        case dods_sequence_c:
            VERBOSE(cerr << btp->FQN() << ": Sequence is not supported by DMR++ for HDF4 at this time.\n");
            return false;
        case dods_grid_c:
            throw BESInternalError("Grids are not supported by DAP4.", __FILE__, __LINE__);
        case dods_array_c:
            return get_chunks_for_an_array(file, btp);
        default:
            VERBOSE(cerr << btp->FQN() << ": " << btp->type_name() << " is not supported by DMR++ for HDF4 at this time.\n");
            return false;
    }
}

/**
 * @brief Iterate over all the variables in a DMR and get their chunk info
 *
 * @param file The open HDF4 file; passed through to get_variable_chunk_info
 * @param group Read variables from this DAP4 Group. Call with the root Group
 * to process all the variables in the DMR
 */
void get_chunks_for_all_variables(int file, D4Group *group) {
    // variables in the group
    for(auto btp : group->variables()) {
        if (btp->type() != dods_group_c) {
            // if this is not a group, it is a variable
            // This is the part where we find out if a variable can be used with DMR++
            if (!get_chunks_for_a_variable(file, btp)) {
                ERROR("Could not include DMR++ metadata for variable " << btp->FQN());
            }
        }
        else {
            // child group
            auto g = dynamic_cast<D4Group*>(btp);
            if (!g)
                throw BESInternalError("Expected "  + btp->name() + " to be a D4Group but it is not.", __FILE__, __LINE__);
            get_chunks_for_all_variables(file, g);
        }
    }
    // all groups in the group
    for (auto g = group->grp_begin(), ge = group->grp_end(); g != ge; ++g) {
        get_chunks_for_all_variables(file, *g);
    }
}

/**
 * @brief Add chunk information about to a DMRpp object
 * @param h4_file_name Read information from this file
 * @param dmrpp Dump the chunk information here
 */
void add_chunk_information(const string &h4_file_name, DMRpp *dmrpp)
{
    // Open the hdf4 file
    int h4file = SDstart(h4_file_name.c_str(), DFACC_READ);
    if (h4file < 0) {
        stringstream msg;
        msg << "Error: HDF4 file '" << h4_file_name << "' cannot be opened." << endl;
        throw BESNotFoundError(msg.str(), __FILE__, __LINE__);
    }

    // iterate over all the variables in the DMR
    try {
        get_chunks_for_all_variables(h4file, dmrpp->root());
        // need to use SDend instead of Hclose. KY 2024-02-22
        SDend(h4file);
    }
    catch (...) {
        SDend(h4file);
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
    // to see if it is an HDF4 file. - kln 11/20/23

    if (file_fqn.empty()) {
        stringstream msg;
        msg << "HDF4 input file name must be provided (-f <input>) and be a fully qualified path name." << endl;
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

    //HDF4 signature:
    int file_id = Hopen(file_fqn.c_str(), DFACC_READ, 0);
    int status = Hishdf(file_fqn.c_str());
    Hclose(file_id);

    //Check if file is NOT an HDF4 file
    if (status != 1) {
        stringstream msg;
        msg << "The provided file: " << file_fqn << " - ";
        msg << "is not an HDF4 file" << endl;
        throw BESInternalFatalError(msg.str(), __FILE__, __LINE__);
    }
}


#if 0
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
#endif

/**
 * @brief Builds a DMR++ from an existing DMR file in conjunction with source granule file.
 *
 * @param granule_url The value to use for the XML attribute dap4:Dataset/@dmrpp:href This may be a template string,
 * or it may be the actual URL location of the source granule file.
 * @param dmr_filename The name of the file from which to read the DMR.
 * @param h4_file_fqn The granule filename.
 * @param add_production_metadata If true the production metadata (software version, configuration, and invocation) will
 * be added to the DMR++.
 * @param argc The number of arguments supplied to build_dmrpp
 * @param argv The arguments for build_dmrpp.
 */
void build_dmrpp_from_dmr_file(const string &dmrpp_href_value, const string &dmr_filename, const string &h4_file_fqn,
        bool add_production_metadata, const string &bes_conf_file_used_to_create_dmr, int argc, char *argv[])
{
    // Get dmr:
    DMRpp dmrpp;
    DmrppTypeFactory dtf;
    dmrpp.set_factory(&dtf);

    ifstream in(dmr_filename.c_str());
    D4ParserSax2 parser;
    parser.intern(in, &dmrpp, false);

    add_chunk_information(h4_file_fqn, &dmrpp);

#if 0
    if (add_production_metadata) {
        inject_version_and_configuration(argc, argv, bes_conf_file_used_to_create_dmr, &dmrpp);
    }
#endif

    XMLWriter writer;
    dmrpp.print_dmrpp(writer, dmrpp_href_value);
    cout << writer.get_doc();

}


} // namespace build_dmrpp_util
