// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the Hyrax data server.

// Copyright (c) The HDF Group
// Copyright (c) 2022 OPeNDAP, Inc.
// Author: Kent Yang <myang6@hdfgroup.org>
// Author: James Gallagher <jgallagher@opendap.org>

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
#include <memory>
#include <iterator>
#include <unordered_set>
#include <iomanip>      // std::put_time()
#include <ctime>      // std::gmtime_r()

#include "config.h"
#include "hdf.h"  // HDF4 header file
#include "mfhdf.h"  // HDF4 header file
#include "HdfEosDef.h" // HDF-EOS2 header file

#include <libdap/Str.h>
#include <libdap/Structure.h>
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
#include "DmrppByte.h"
#include "D4ParserSax2.h"

#define ERROR(x) do { cerr << "Internal Error: " << x << " at " << __FILE__ << ":" << __LINE__ << endl; } while(false)

/*
 * Hold number of data blocks, offset/length information for SDS objects.
 */
using SD_mapping_info_t = struct {
    int32 nblocks;              /* number of data blocks in dataset */
    int32 *offsets;             /* offsets of data blocks */
    int32 *lengths;             /* lengths (in bytes) of data blocks */
};

using namespace std;
using namespace libdap;
using namespace dmrpp;

namespace build_dmrpp_util_h4 {

bool verbose = false;   // Optionally set by build_dmrpp's main().

#define VERBOSE(x) do { if (verbose) (x); } while(false)
#define prolog std::string("build_dmrpp_h4::").append(__func__).append("() - ")

constexpr auto INVOCATION_CONTEXT = "invocation";

int SDfree_mapping_info(SD_mapping_info_t  *map_info)
{
    intn  ret_value = SUCCEED;

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

void close_hdf4_file_ids(int32 sd_id, int32 file_id) {
    Vend(file_id);
    Hclose(file_id);
    SDend(sd_id);
}

// Try to combine the adjacent linked blocks to one block. KY 2024-03-12
size_t combine_linked_blocks(const SD_mapping_info_t &map_info, vector<int> & merged_lengths, vector<int> &merged_offsets) {

    int num_eles = map_info.nblocks;

    // The first element offset should always be fixed.
    merged_offsets.push_back(map_info.offsets[0]);

    int temp_length = map_info.lengths[0];

    for (int i = 0; i <(num_eles-1); i++) {

        // If not contiguous, push back the i's new length;
        //                    push back the i+1's unchanged offset;
        //                    save the i+1's length to the temp_length variable.
        if (map_info.offsets[i+1] !=(map_info.offsets[i] + map_info.lengths[i])) {
            merged_lengths.push_back(temp_length);
            merged_offsets.push_back(map_info.offsets[i+1]);
            temp_length = map_info.lengths[i+1];
        }
        else { // contiguous, just update the temp_length variable.
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
 * @brief Read chunk information from an HDF4 dataset
 * @param sdsid
 * @param map_info
 * @param origin The parameter origin must be NULL when the data is not stored in chunking layout.
 * When the data are chunked, SDgetdatainfo can be called on a single chunk and origin is used to
 * specify the coordinates of the chunk.
 * Note: The coordinates of a chunk is not the same as the coordinates of this
 * chunk in the whole array. For detailed description and illustration, check section 3.12.3 and FIGURE 3d of
 * the HDF4 user's guide that can be found under https://portal.hdfgroup.org/documentation/ .
 * Note: The code of this method is largely adapted from the HDF4 mapper software implemented by the HDF group.
 *       h4mapper can be found from https://docs.hdfgroup.org/archive/support/projects/h4map/h4map_writer.html.
 * @return
 */

int read_chunk(int sdsid, SD_mapping_info_t *map_info, int *origin)
{
    intn  ret_value = SUCCEED;


    // First check if this chunk/data stream has any block of data.
    intn info_count = SDgetdatainfo(sdsid, origin, 0, 0, nullptr, nullptr);
    if (info_count == FAIL) {
        ERROR("SDgetedatainfo() failed in read_chunk().\n");
        return FAIL;
    }

    // If we find it has data, retrieve the offsets and length information for this chunk or data stream.
    if (info_count > 0) {
        map_info->nblocks = (int32) info_count;
        map_info->offsets = (int32 *)HDmalloc(sizeof(int32)*map_info->nblocks);
        if (map_info->offsets == nullptr) {
            ERROR("HDmalloc() failed: Out of Memory");
            return FAIL;
        }
        map_info->lengths = (int32 *)HDmalloc(sizeof(int32)*map_info->nblocks);
        if (map_info->lengths == nullptr) {
            HDfree(map_info->offsets);
            ERROR("HDmalloc() failed: Out of Memory");
            return FAIL;
        }

        ret_value = SDgetdatainfo(sdsid, origin, 0, info_count, map_info->offsets, map_info->lengths);
        return ret_value;
    }

    return ret_value;

} /* read_chunk */


string get_sds_fill_value_str(int32 sdsid, int32 datatype, bool &dmrpp_h4_error, string &err_msg) {

    intn emptySDS = 0;
    string ret_value;
    
    if (SDcheckempty(sdsid,&emptySDS) == FAIL) {
        SDendaccess(sdsid);
        dmrpp_h4_error = true;
        err_msg = "SDcheckempty fails at "+string(__FILE__) +":" + to_string(__LINE__);
        return ret_value;
    }

    switch (datatype) {

        case DFNT_UINT8:
        case DFNT_UCHAR: {
            uint8_t fvalue;
            if (SDgetfillvalue(sdsid, &fvalue) == SUCCEED)
                ret_value = to_string(fvalue);
            // If the SDS is empty, we return the default netCDF fill value since SDS is following netCDF.
            else if(emptySDS == 1)
                ret_value = to_string(255);
        }
            break;
        case DFNT_INT8:
        case DFNT_CHAR: {
            int8_t fvalue;
            if (SDgetfillvalue(sdsid, &fvalue) == SUCCEED)
                ret_value = to_string(fvalue);
            else if(emptySDS == 1)
                ret_value = to_string(FILL_BYTE);
        }
            break;

        case DFNT_INT16: {
            int16_t fvalue;
            if (SDgetfillvalue(sdsid, &fvalue) == SUCCEED)
                ret_value = to_string(fvalue);
            else if(emptySDS == 1)
                ret_value = to_string(FILL_SHORT);
        }
            break;

        case DFNT_UINT16: {
            uint16_t fvalue;
            if (SDgetfillvalue(sdsid, &fvalue) == SUCCEED)
                ret_value = to_string(fvalue);
             else if(emptySDS == 1)
                ret_value = to_string(65535);
        }
            break;

        case DFNT_INT32: {
            int32_t fvalue;
            if (SDgetfillvalue(sdsid, &fvalue) == SUCCEED)
                ret_value = to_string(fvalue);
            else if(emptySDS == 1)
                ret_value = to_string(FILL_LONG);
        }
            break;

        case DFNT_UINT32: {
            uint32_t fvalue;
            if (SDgetfillvalue(sdsid, &fvalue) == SUCCEED)
                ret_value = to_string(fvalue);
            else if(emptySDS == 1)
                ret_value = to_string(4294967295);
        }
            break;

        case DFNT_FLOAT: {
            float fvalue;
            if (SDgetfillvalue(sdsid, &fvalue) == SUCCEED)
                ret_value = to_string(fvalue);
            else if(emptySDS == 1)
                ret_value = to_string(FILL_FLOAT);
        }
            break;

        case DFNT_DOUBLE: {
            double fvalue;
            if (SDgetfillvalue(sdsid, &fvalue) == SUCCEED)
                ret_value = to_string(fvalue);
            else if(emptySDS == 1)
                ret_value = to_string(FILL_DOUBLE);
 
        }
            break;
        default:
            break;

    }
    return ret_value;
}

bool SD_set_fill_value(int32 sdsid, int32 datatype, BaseType *btp) {

    bool dmrpp_h4_error = false;
    string err_msg;
    string fill_value = get_sds_fill_value_str(sdsid,datatype,dmrpp_h4_error,err_msg);
    if (dmrpp_h4_error) {
        ERROR(err_msg);
        return false;
    }

    if (!fill_value.empty()) {
         auto dc = dynamic_cast<DmrppCommon *>(btp);
         if (!dc) {
             ERROR("Expected to find a DmrppCommon instance but did not.");
             return false;
         }
        dc->set_uses_fill_value(true);
        dc->set_fill_value_string(fill_value);
    }
    return true;
}

bool  ingest_sds_info_to_chunk(int file, int32 obj_ref, BaseType *btp) {

    int32 sds_index = SDreftoindex(file, obj_ref);
    if (sds_index == FAIL) {
        ERROR("SDreftoindex() failed");
        return false;
    }

    int sdsid = SDselect(file, sds_index);
    if (sdsid == FAIL) {
        ERROR("SDselect() failed");
        return false;
    }

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
        ERROR("SDgetinfo() failed.");
        SDendaccess(sdsid);
        return false;
    }

    HDF_CHUNK_DEF cdef;
    int32 chunk_flag = -1;        /* chunking flag */

    status = SDgetchunkinfo(sdsid, &cdef, &chunk_flag);
    if (status == FAIL) {
        ERROR("SDgetchunkinfo() failed.");
        SDendaccess(sdsid);
        return false;
    }

    switch (chunk_flag) {
        case HDF_NONE: /* No chunking. */
            VERBOSE(cerr << "No chunking.\n");
            break;
        case HDF_CHUNK: /* Chunking. */
        case HDF_COMP: /* Compression. */
            if (rank <= 0) {
                ERROR("Invalid rank.");
                SDendaccess(sdsid);
                return false;
            }
            VERBOSE(cerr << "HDF_CHUNK or HDF_COMP.\n");
            break;
        case HDF_NBIT:
            /* NBIT compression. */
            ERROR("NBit Compression chunking not supported.");
            SDendaccess(sdsid);
            return false;
        default:
            ERROR("Unknown chunking flag.");
            SDendaccess(sdsid);
            return false;
    }

    string endian_name;
    hdf_ntinfo_t info;          /* defined in hdf.h near line 142. */
    int result = Hgetntinfo(data_type, &info);
    if (result == FAIL) {
        ERROR("Hgetntinfo() failed.");
        SDendaccess(sdsid);
        return false;
    }
    else {
        if (strncmp(info.byte_order, "bigEndian", 9) == 0)
            endian_name = "BE";
        else if (strncmp(info.byte_order, "littleEndian", 12) == 0)
            endian_name = "LE";
        else
            endian_name = "UNKNOWN";
    }

    auto dc = dynamic_cast<DmrppCommon *>(btp);
    if (!dc){
       ERROR("Expected to find a DmrppCommon instance but did not.");
       SDendaccess(sdsid);
       return false;
    }

    // Need to check SDS compression info. Unlike HDF5, HDF4 can be compressed without using chunks.
    // So we need to cover both cases. KY 02/12/2024

    comp_coder_t        comp_coder_type = COMP_CODE_NONE;
    comp_info                    c_info;

    if (SDgetcompinfo(sdsid, &comp_coder_type, &c_info) == FAIL) {
        ERROR("SDgetcompinfo() failed.");
        SDendaccess(sdsid);
        return false;
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
            // Add the deflated-compression level. KY 02/20/24
            if (comp_coder_type == COMP_CODE_DEFLATE) {
                dc->ingest_compression_type("deflate");
                vector<unsigned int> deflate_levels;
                deflate_levels.push_back(c_info.deflate.level);
                dc->set_deflate_levels(deflate_levels);
            } else {
                SDendaccess(sdsid);
                ERROR("Encounter unsupported compression method. Currently only support deflate compression.");
                return false;
            }

        }

        dc->set_chunk_dimension_sizes(chunk_dimension_sizes);


        vector<unsigned long long> position_in_array(rank, 0);

        /* Initialize steps. */
        vector<int> steps(rank, 0);
        vector<int32_t> strides(rank, 0);
        int number_of_chunks = 1;
        for (int i = 0; i < rank; i++) {
            // The following line considers the un-even chunk case, otherwise we will miss the extra chunk(s). 
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

        // Obtain the offset/length of all chunks.
        vector<uint32> info_count(number_of_chunks);
        intn max_num_blocks = SDgetallchunkdatainfo(sdsid,number_of_chunks,rank,steps.data(),0,info_count.data(),
                                               nullptr,nullptr);
        // The max_num_blocks should at least be 1.
        if (max_num_blocks < 1) {
            ERROR("SDgetallchunkdatainfo failed.");
            SDendaccess(sdsid);
            return false;
        }

        vector<int32>offsetarray(max_num_blocks*number_of_chunks);
        vector<int32>lengtharray(max_num_blocks*number_of_chunks);

        max_num_blocks = SDgetallchunkdatainfo(sdsid,number_of_chunks,rank,steps.data(),max_num_blocks,info_count.data(),
                                               offsetarray.data(),lengtharray.data());
        if (max_num_blocks < 1) {
            ERROR("SDgetallchunkdatainfo failed.");
            SDendaccess(sdsid);
            return false;
        }
        bool LBChunk = (max_num_blocks>1);

        intn ol_counter = 0;
        for (int k = 0; k < number_of_chunks; ++k) {
            
            auto pia = write_chunk_position_in_array(rank, chunk_dimension_sizes.data(), strides.data());

            // When we find this chunk contains multiple blocks.
            if (info_count[k] >1) {

                VERBOSE(cerr << "number of blocks in a chunk is: " << info_count[k]<< endl);
                for (unsigned int i = 0; i < info_count[k]; i++) {
                    VERBOSE(cerr << "offsets[" << k << ", " << i << "]: " << offsetarray[ol_counter] << endl);
                    VERBOSE(cerr << "lengths[" << k << ", " << i << "]: " << lengtharray[ol_counter] << endl);
                    // add the block index for this chunk.
                    dc->add_chunk(endian_name, lengtharray[ol_counter], offsetarray[ol_counter], pia,true,i);
                    ol_counter++;
                }
                ol_counter +=max_num_blocks-info_count[k];
                
            }

            else if (info_count[k] == 1) {

                dc->add_chunk(endian_name, lengtharray[ol_counter], offsetarray[ol_counter], pia);
                ol_counter +=max_num_blocks;
            }
            else if (info_count[k] == 0) 
                ol_counter +=max_num_blocks;

            // Increase strides for each dimension. The fastest varying dimension is rank-1.
            int scale = 1;
            for(int i = rank-1; i >= 0 ; i--) {
                if ((k+1) % scale == 0) {
                    strides[i] = ++strides[i] % steps[i];
                }
                scale = scale * steps[i];
            }
        }
        if (LBChunk) 
            dc->set_multi_linked_blocks_chunk(LBChunk);
    }

    // Leave the following code for the time being in case there are issues in the optimization.
#if 0

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
            // Add the deflated-compression level. KY 02/20/24
            if (comp_coder_type == COMP_CODE_DEFLATE) {
                dc->ingest_compression_type("deflate");
                vector<unsigned int> deflate_levels;
                deflate_levels.push_back(c_info.deflate.level);
                dc->set_deflate_levels(deflate_levels);
            } else {
                SDendaccess(sdsid);
                ERROR("Encounter unsupported compression method. Currently only support deflate compression.");
                return false;
            }

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

        bool LBChunk = false;

        for (int k = 0; k < number_of_chunks; ++k) {
            auto info_count = read_chunk(sdsid, &map_info, strides.data());
            if (info_count == FAIL) {
                ERROR("read_chunk() failed.");
                SDendaccess(sdsid);
                return false;
            }
            
            auto pia = write_chunk_position_in_array(rank, chunk_dimension_sizes.data(), strides.data());

            // When we find this chunk contains multiple blocks.
            if (map_info.nblocks >1) {

                if (!LBChunk) 
                    LBChunk = true;
                VERBOSE(cerr << "number of blocks in a chunk is: " << map_info.nblocks << endl);
                for (unsigned int i = 0; i < map_info.nblocks; i++) {
                    VERBOSE(cerr << "offsets[" << k << ", " << i << "]: " << map_info.offsets[i] << endl);
                    VERBOSE(cerr << "lengths[" << k << ", " << i << "]: " << map_info.lengths[i] << endl);
                    // add the block index for this chunk.
                    dc->add_chunk(endian_name, map_info.lengths[i], map_info.offsets[i], pia,true,i);

                }
            }

            else if (map_info.nblocks == 1) {
                    dc->add_chunk(endian_name, map_info.lengths[0], map_info.offsets[0], pia);
            }

            // Increase strides for each dimension. The fastest varying dimension is rank-1.
            int scale = 1;
            for(int i = rank-1; i >= 0 ; i--) {
                if ((k+1) % scale == 0) {
                    strides[i] = ++strides[i] % steps[i];
                }
                scale = scale * steps[i];
            }
            SDfree_mapping_info(&map_info);
        }
        if (LBChunk) 
            dc->set_multi_linked_blocks_chunk(LBChunk);
    }
#endif
    else if (chunk_flag == HDF_NONE) {
        SD_mapping_info_t map_info;
        map_info.nblocks = 0;
        map_info.offsets = nullptr;
        map_info.lengths = nullptr;

         // Also need to consider the case when the variable is compressed but not chunked.
        if (comp_coder_type != COMP_CODE_NONE) {
            if (comp_coder_type == COMP_CODE_DEFLATE) {
                dc->ingest_compression_type("deflate");
                vector<unsigned int> deflate_levels;
                deflate_levels.push_back(c_info.deflate.level);
                dc->set_deflate_levels(deflate_levels);
            }
            else {
                ERROR("Encounter unsupported compression method. Currently only support deflate compression.");
                SDendaccess(sdsid);
                return false;
            }
        }

        vector<int> origin(rank, 0);
        auto info_count = read_chunk(sdsid, &map_info, origin.data());
        if (info_count == FAIL) {
            ERROR("read_chunk() failed in the middle of ingest_sds_info_to_chunk().");
            SDendaccess(sdsid);
            return false;
        }
        
        vector<unsigned long long> position_in_array(rank, 0);
        if (map_info.nblocks ==1) {
              dc->add_chunk(endian_name, map_info.lengths[0], map_info.offsets[0], position_in_array);
        }
        else if (map_info.nblocks>1) {
            // Here we will see if we can combine the number of contiguous blocks to a bigger one.
            // This is necessary since HDF4 may store small size data in large number of contiguous linked blocks.
            // KY 2024-02-22
            vector<int> merged_lengths;
            vector<int> merged_offsets;
            size_t merged_number_blocks = combine_linked_blocks(map_info, merged_lengths, merged_offsets);
            for (unsigned i = 0; i < merged_number_blocks; i++) {
                VERBOSE(cerr << "offsets[" << i << "]: " << map_info.offsets[i] << endl);
                VERBOSE(cerr << "lengths[" << i << "]: " << map_info.lengths[i] << endl);

                // This has linked blocks. Add this information to the dmrpp:chunks.
                dc->add_chunk(endian_name, merged_lengths[i], merged_offsets[i], true,i);

            }
        }
        SDfree_mapping_info(&map_info);
    }
    else {
        // TODO Handle the other cases. jhrg 12/7/23
        ERROR("Unknown chunking type.");
        SDendaccess(sdsid);
        return false;
    }

    // Add the fillvalue support
    if (false == SD_set_fill_value(sdsid,data_type, btp)) {
        ERROR("SD_set_fill_value failed");
        SDendaccess(sdsid);
        return false;
    }

    // Need to close the SDS interface. KY 2024-02-22
    SDendaccess(sdsid);
    return true;
}

size_t combine_linked_blocks_vdata( const vector<int>& lengths, const vector<int>& offsets, vector<int> & merged_lengths, vector<int> &merged_offsets) {

    size_t num_eles = lengths.size();

    // The first element offset should always be fixed.
    merged_offsets.push_back(offsets[0]);
    int temp_length = lengths[0];

    for (size_t i = 0; i <(num_eles-1); i++) {

        // If not contiguous, push back the i's new length;
        //                    push back the i+1's unchanged offset.
        //                    save the i+1's length to the temp length variable.
        if (offsets[i+1] !=(offsets[i] + lengths[i])) {
            merged_lengths.push_back(temp_length);
            merged_offsets.push_back(offsets[i+1]);
            temp_length = lengths[i+1];
        }
        else { // contiguous, just update the temp length variable.
            temp_length +=lengths[i+1];
        }

    }

    // Update the last length.
    merged_lengths.push_back(temp_length);

    return merged_lengths.size();

}


bool  ingest_vdata_info_to_chunk(int32 file_id, int32 obj_ref, BaseType *btp) {

    int32 vdata_id = VSattach(file_id, obj_ref, "r");
    if (vdata_id == FAIL){
        ERROR("VSattach() failed.");
        return false;
    }

    // Retrieve endianess
    int32 field_datatype = VFfieldtype(vdata_id,0);
    if (field_datatype == FAIL){
        ERROR("VFfieldtype() failed.");
        VSdetach(vdata_id);
        return false;
    }

    string endian_name;
    hdf_ntinfo_t info;          /* defined in hdf.h near line 142. */
    int result = Hgetntinfo(field_datatype, &info);
    if (result == FAIL) {
        ERROR("Hgetntinfo() failed in ingest_vdata_info_to_chunk.");
        VSdetach(vdata_id);
        return false;
    }
    else {
        if (strncmp(info.byte_order, "bigEndian", 9) == 0)
            endian_name = "BE";
        else if (strncmp(info.byte_order, "littleEndian", 12) == 0)
            endian_name = "LE";
        else
            endian_name = "UNKNOWN";
    }

    int32 num_linked_blocks = VSgetdatainfo(vdata_id,0,0,nullptr,nullptr);

    if (num_linked_blocks == FAIL){
        ERROR("VSgetdatainfo() failed in ingest_vdata_info_to_chunk.");
        VSdetach(vdata_id);
        return false;
    }
    if (num_linked_blocks == 1) {// most time.

        int32 offset = 0;
        int32 length = 0; 
        if (VSgetdatainfo(vdata_id,0,1,&offset,&length) == FAIL){
            ERROR("VSgetdatainfo() failed, cannot obtain offset and length info.");
            VSdetach(vdata_id);
            return false;
        }

        VERBOSE(cerr << "vdata offset: " << offset << endl);
        VERBOSE(cerr << "length: " << length << endl);
 
        auto dc = dynamic_cast<DmrppCommon *>(btp);
        if (!dc) {
            ERROR("Expected to find a DmrppCommon instance but did not in ingested_vdata_info_to_chunk");
            VSdetach(vdata_id);
            return false;
        }
        auto da = dynamic_cast<DmrppArray *>(btp);
        if (!da) {
            ERROR("Expected to find a DmrppArray instance but did not in ingested_vdata_info_to_chunk");
            VSdetach(vdata_id);
            return false;
        }

        if (da->width_ll() != (int64_t)length) {
            ERROR("The retrieved vdata size is no equal to the original size.");
            VSdetach(vdata_id);
            return false;
        }
        dc->add_chunk(endian_name, (unsigned long long)length, (unsigned long long)offset,"");

    }
    else if (num_linked_blocks >1) {

        //merging the linked blocks.
        vector<int> lengths;
        vector<int> offsets;
        lengths.resize(num_linked_blocks);
        offsets.resize(num_linked_blocks);
        if (VSgetdatainfo(vdata_id, 0, num_linked_blocks, offsets.data(), lengths.data()) == FAIL) {
            ERROR("VSgetdatainfo() failed, cannot obtain offset and length info when the num_linked_blocks is >1.");
            VSdetach(vdata_id);
            return false;
        }
        vector<int>merged_lengths;
        vector<int>merged_offsets;
        size_t merged_number_blocks = combine_linked_blocks_vdata(lengths,offsets,merged_lengths,merged_offsets);
        auto dc = dynamic_cast<DmrppCommon *>(btp);
        if (!dc) {
            ERROR("Expected to find a DmrppArray instance but did not when num_linked_blocks is >1.");
            VSdetach(vdata_id);
            return false;
        }

        VERBOSE(cerr << "merged block offsets: ");
        VERBOSE(copy(merged_offsets.begin(), merged_offsets.end(),
                     ostream_iterator<int32>(cerr, " ")));

        VERBOSE(cerr << "merged block lengths: ");
        VERBOSE(copy(merged_lengths.begin(), merged_lengths.end(),
                     ostream_iterator<int32>(cerr, " ")));
        VERBOSE(cerr<<endl);


        for (unsigned i = 0; i < merged_number_blocks; i++)
            dc->add_chunk(endian_name, merged_lengths[i], merged_offsets[i],true,i);

    }

    VSdetach(vdata_id);
    return true;

}

// Add the missing CF grid variables data. See Appendix F of CF conventions(https://cfconventions.org/)
// This function won't be called if users choose not to insert the missing data inside the dmrpp file.
bool add_missing_cf_grid(const string &filename,BaseType *btp, const D4Attribute *eos_cf_attr, string &err_msg) {

    VERBOSE(cerr<<"Coming to add_missing_cf_grid"<<endl);
    
    string eos_cf_attr_value = eos_cf_attr->value(0);

    VERBOSE(cerr<<"eos_cf_attr_value: "<<eos_cf_attr_value <<endl);

    size_t space_pos = eos_cf_attr_value.find_last_of(' ');
    if (space_pos ==string::npos) { 
        err_msg = "Attribute eos_latlon must have space inside";
        return false;
    }

    string grid_name = eos_cf_attr_value.substr(0,space_pos);
    string cf_name = eos_cf_attr_value.substr(space_pos+1);

    VERBOSE(cerr<<"grid_name: "<<grid_name <<endl);
    VERBOSE(cerr<<"cf_name: "<<cf_name<<endl);

    bool is_xdim = true;
    if (cf_name =="YDim")
        is_xdim = false;

    int32 gridfd = GDopen(const_cast < char *>(filename.c_str()), DFACC_READ);
    if (gridfd <0) {
        err_msg = "HDF-EOS: GDopen failed";
        return false;
    }
    int32 gridid = GDattach(gridfd, const_cast<char *>(grid_name.c_str()));
    if (gridid <0) {
        err_msg = "HDF-EOS: GDattach failed to attach " + grid_name;
        GDclose(gridfd);
        return false;
    }

    int32 xdim = 0;
    int32 ydim = 0;

    float64 upleft[2];
    float64 lowright[2];

    // Retrieve dimensions and X-Y coordinates of corners
    if (GDgridinfo(gridid, &xdim, &ydim, upleft,
                   lowright) == -1) {
        GDdetach(gridid);
        GDclose(gridfd);
        err_msg = "GDgridinfo failed for grid name: " + grid_name;
        return false;
    }

    auto da = dynamic_cast<DmrppArray *>(btp);
    if (!da) {
        GDdetach(gridid);
        GDclose(gridfd);
        err_msg = "Expected to find a DmrppArray instance but did not in add_missing_cf_grid";
        return false;
    }

    da->set_missing_data(true);
 
    int total_num = is_xdim?xdim:ydim;
    vector<double>val(total_num);    
    double evalue = 0;
    double svalue = 0;

    if (is_xdim) {
        svalue = upleft[0];
        evalue = lowright[0];
    }
    else {
        svalue = upleft[1];
        evalue = lowright[1];
    }

    double step_v = (evalue - svalue)/total_num;

    val[0] = svalue;
    for(int i = 1;i<total_num; i++)
        val[i] = val[i-1] + step_v;

    da->set_value(val.data(),total_num);
    da->set_read_p(true);
       

    GDdetach(gridid);
    GDclose(gridfd);
 
    return true;

}

// Add the missing eos latitude/longitude data. This function won't be called if users choose not to insert the data
// inside the dmrpp file.
bool add_missing_eos_latlon(const string &filename,BaseType *btp, const D4Attribute *eos_ll_attr, string &err_msg) {

    VERBOSE(cerr<<"Coming to add_missing_eos_latlon"<<endl);
    
    string eos_ll_attr_value = eos_ll_attr->value(0);

    VERBOSE(cerr<<"eos_ll_attr_value: "<<eos_ll_attr_value <<endl);

    size_t space_pos = eos_ll_attr_value.find_last_of(' ');
    if (space_pos ==string::npos) { 
        err_msg = "Attribute eos_latlon must have space inside";
        return false;
    }

    string grid_name = eos_ll_attr_value.substr(0,space_pos);
    string ll_name = eos_ll_attr_value.substr(space_pos+1);

    VERBOSE(cerr<<"grid_name: "<<grid_name <<endl);
    VERBOSE(cerr<<"ll_name: "<<ll_name<<endl);
    bool is_lat = true;
    if (ll_name =="lon")
        is_lat = false;

    int32 gridfd = GDopen(const_cast < char *>(filename.c_str()), DFACC_READ);
    if (gridfd <0) {
        err_msg = "HDF-EOS: GDopen failed";
        return false;
    }
    int32 gridid = GDattach(gridfd, const_cast<char *>(grid_name.c_str()));
    if (gridid <0) {
        err_msg = "HDF-EOS: GDattach failed to attach " + grid_name;
        GDclose(gridfd);
        return false;
    }

    // Declare projection code, zone, etc grid parameters. 

    int32 projcode = -1; 
    int32 zone     = -1;
    int32 sphere   = -1;
    float64 params[16];

    int32 xdim = 0;
    int32 ydim = 0;

    float64 upleft[2];
    float64 lowright[2];

    if (GDprojinfo (gridid, &projcode, &zone, &sphere, params) <0) {
        GDdetach(gridid);
        GDclose(gridfd);
        err_msg = "GDprojinfo failed for grid name: " + grid_name;
        return false;
    }

    // Retrieve dimensions and X-Y coordinates of corners
    if (GDgridinfo(gridid, &xdim, &ydim, upleft,
                   lowright) == -1) {
        GDdetach(gridid);
        GDclose(gridfd);
        err_msg = "GDgridinfo failed for grid name: " + grid_name;
        return false;
    }

 
    // Retrieve pixel registration information 
    int32 pixreg = 0; 
    intn r = GDpixreginfo (gridid, &pixreg);
    if (r != 0) {
        GDdetach(gridid);
        GDclose(gridfd);
        err_msg = "GDpixreginfo failed for grid name: " + grid_name;
        return false;
    }

    //Retrieve grid pixel origin 
    int32 origin = 0; 
    r = GDorigininfo (gridid, &origin);
    if (r != 0) {
        GDdetach(gridid);
        GDclose(gridfd);
        err_msg = "GDoriginfo failed for grid name: " + grid_name;
        return false;
    }

    vector<int32>rows;
    vector<int32>cols;
    vector<float64>lon;
    vector<float64>lat;
    rows.resize(xdim*ydim);
    cols.resize(xdim*ydim);
    lon.resize(xdim*ydim);
    lat.resize(xdim*ydim);

    int i = 0;
    int j = 0;
    int k = 0; 

    /* Fill two arguments, rows and columns */
    // rows             cols
    //   /- xdim  -/      /- xdim  -/
    //   0 0 0 ... 0      0 1 2 ... x
    //   1 1 1 ... 1      0 1 2 ... x
    //       ...              ...
    //   y y y ... y      0 1 2 ... x

    for (k = j = 0; j < ydim; ++j) {
        for (i = 0; i < xdim; ++i) {
            rows[k] = j;
            cols[k] = i;
            ++k;
        }
    }

    // The following code aims to handle large MCD Grid(GCTP_GEO projection) such as 21600*43200 lat and lon.
    // These MODIS MCD files don't follow HDF-EOS standard way to represent lat/lon (DDDMMMSSS);
    // they simply represent lat/lon as the normal representation -180.0 or -90.0.
    // For example, if the real longitude value is 180.0, HDF-EOS needs the value to be represented as 180000000 rather than 180.
    // So we need to make the representation follow the HDF-EOS2 way. 
    if (((int)(lowright[0]/1000)==0) &&((int)(upleft[0]/1000)==0)
               && ((int)(upleft[1]/1000)==0) && ((int)(lowright[1]/1000)==0)) {
        if (projcode == GCTP_GEO){ 
            for (int i =0; i<2;i++) {
                lowright[i] = lowright[i]*1000000;
                upleft[i] = upleft[i] *1000000;
            }
        }
    }

    r = GDij2ll (projcode, zone, params, sphere, xdim, ydim, upleft, lowright,
                 xdim * ydim, rows.data(), cols.data(), lon.data(), lat.data(), pixreg, origin);

    if (r != 0) {
        GDdetach(gridid);
        GDclose(gridfd);
        err_msg = "cannot calculate grid latitude and longitude for grid name: " + grid_name;
        return false;
    }

    VERBOSE(cerr<<"The first value of longtitude: "<<lon[0]<<endl);
    VERBOSE(cerr<<"The first value of latitude: "<<lat[0]<<endl);


    auto da = dynamic_cast<DmrppArray *>(btp);
    if (!da) {
        GDdetach(gridid);
        GDclose(gridfd);
        err_msg = "Expected to find a DmrppArray instance but did not in add_missing_eos_latlon";
        return false;
    }

    da->set_missing_data(true);
    
    if (is_lat) { 
        // Need to check if CEA or GEO
        if (projcode == GCTP_CEA || projcode == GCTP_GEO) {
            vector<float64>out_lat;
            out_lat.resize(ydim);
            j=0;
            for (i =0; i<xdim*ydim;i = i+xdim){
                out_lat[j] = lat[i];
                j++;
            }
            da->set_value(out_lat.data(),ydim);
            
        }
        else 
            da->set_value(lat.data(),xdim*ydim);
    }
    else {
        if (projcode == GCTP_CEA || projcode == GCTP_GEO) {
            vector<float64>out_lon;
            out_lon.resize(xdim);
            for (i =0; i<xdim;i++)
                out_lon[i] = lon[i];
            da->set_value(out_lon.data(),xdim);

        }
        else 
            da->set_value(lon.data(),xdim*ydim);

    }
    da->set_read_p(true);
 
    GDdetach(gridid);
    GDclose(gridfd);
    
    return true;

}

// Add the missing lat/lon for special HDF4 files. Currently only for TRMM level 3.
bool add_missing_sp_latlon(BaseType *btp, const D4Attribute *sp_ll_attr, string &err_msg) {

    VERBOSE(cerr<<"Coming to add_missing_sp_latlon"<<endl);
    
    string sp_ll_attr_value = sp_ll_attr->value(0);

    VERBOSE(cerr<<"sp_ll_attr_value: "<<sp_ll_attr_value <<endl);
    size_t space_pos = sp_ll_attr_value.find(' ');
    if (space_pos ==string::npos) { 
        err_msg = "Attribute sp_h4_ll must have space inside";
        return false;
    }
    string ll_name = sp_ll_attr_value.substr(0,space_pos);
    VERBOSE(cerr<<"ll_name: "<<ll_name<<endl);
    
    size_t sec_space_pos = sp_ll_attr_value.find(' ',space_pos+1);
    if (sec_space_pos ==string::npos) { 
        err_msg = "Attribute sp_h4_ll must have two spaces inside";
        return false;
    }
    string ll_start_str = sp_ll_attr_value.substr(space_pos+1,sec_space_pos-space_pos-1);
    VERBOSE(cerr<<"ll_start: "<<ll_start_str<<endl);

    string ll_res_str = sp_ll_attr_value.substr(sec_space_pos);
    VERBOSE(cerr<<"ll_res: "<<ll_res_str<<endl);

    float ll_start = stof(ll_start_str);
    float ll_res   = stof(ll_res_str);

    auto da = dynamic_cast<DmrppArray *>(btp);
    if (!da) {
        err_msg = "Expected to find a DmrppArray instance but did not in add_missing_sp_latlon";
        return false;
    }

    if (da->dimensions() !=1){
        err_msg = "The number of dimensions of the array should be 1.";
        return false;
    }

    da->set_missing_data(true);
 
    vector<float32> ll_value;
    ll_value.resize((size_t)(da->length()));
    for (int64_t i = 0; i <da->length(); i++) {
        ll_value[i] = ll_start+ll_res/2+i*ll_res;
    }

    da->set_value(ll_value.data(),da->length());
    return true;


}
/**
 * @param filename : File name
 * @param sd_id : HDF4 SD interface ID
 * @param file_id : HDF4 H interface ID
 * @param btp the DAP4 object pointer
 * @param disable_missing_data flag to disable the generation and storing of the missing data in the dmrpp file
 * @return true if the produced output that seems valid, false otherwise.
 */
bool get_chunks_for_an_array(const string& filename, int32 sd_id, int32 file_id, BaseType *btp, bool disable_missing_data) {

    VERBOSE(cerr<<"var name: "<<btp->name() <<endl);

    // Here we need to retrieve the attribute value dmr_sds_ref of btp.
    D4Attributes *d4_attrs = btp->attributes();
    if (!d4_attrs) {
        close_hdf4_file_ids(sd_id,file_id);
        throw BESInternalError("Expected to find an DAP4 attribute list for " + btp->name() + " but did not.",
                               __FILE__, __LINE__);
    }

    // We need to find the object reference number to retrieve the offset and lenght.
    // Currently we only support HDF4 SDS and Vdata. That's the HDF4 objects what NASA HDF4/HDF-EOS2 files contain.
    D4Attribute *attr = d4_attrs->find("dmr_sds_ref");
    int32 obj_ref = 0;
    bool is_sds = false;
    if (attr)
        is_sds = true;
    else 
        attr = d4_attrs->find("dmr_vdata_ref");

    if (attr) {
        obj_ref = stoi(attr->value(0));
        if (is_sds) {
            if (false == ingest_sds_info_to_chunk(sd_id, obj_ref,btp)) {
                close_hdf4_file_ids(sd_id,file_id);
                throw BESInternalError("Cannot retrieve SDS information correctly for " + btp->name() ,
                                   __FILE__, __LINE__);
            }
        }
    
        else {
            if (false == ingest_vdata_info_to_chunk(file_id, obj_ref,btp)) {
                close_hdf4_file_ids(sd_id,file_id);
                throw BESInternalError("Cannot retrieve vdata information correctly for " + btp->name() ,
                                   __FILE__, __LINE__);
            }
        }
    }
    else if (disable_missing_data == false){

        VERBOSE(cerr<<"coming to eos_latlon block"<<endl);
        // Here we will check if the eos_latlon exists. Add dmrpp::missingdata
        attr = d4_attrs->find("eos_latlon");
        if (attr) {
            string err_msg;
            bool ret_value = add_missing_eos_latlon(filename, btp, attr,err_msg);
            if (ret_value == false) {
                close_hdf4_file_ids(sd_id,file_id);
                throw BESInternalError(err_msg,__FILE__,__LINE__);
            }

        }
        else { 
            attr = d4_attrs->find("sp_h4_ll");
            if (attr) {
                string err_msg;
                bool ret_value = add_missing_sp_latlon(btp, attr,err_msg);
                if (ret_value == false) {
                    close_hdf4_file_ids(sd_id,file_id);
                    throw BESInternalError(err_msg,__FILE__,__LINE__);
                }
            }
            else {
                attr = d4_attrs->find("eos_cf_grid");
                if (attr) {
                    string err_msg;
                    bool ret_value = add_missing_cf_grid(filename, btp, attr,err_msg);
                    if (ret_value == false) {
                        close_hdf4_file_ids(sd_id,file_id);
                        throw BESInternalError(err_msg,__FILE__,__LINE__);
                    }
                
                }
                else {
                    close_hdf4_file_ids(sd_id,file_id);
                    string error_msg = "Expected to find an attribute that stores either HDF4 SDS reference or HDF4 Vdata reference or eos lat/lon or special HDF4 lat/lon or special cf grid for ";
                    throw BESInternalError(error_msg + btp->name() + " but did not.",__FILE__,__LINE__);
                }
            }
            
        }        
    }

    return true;
}

// Currently this function is only for CF grid_mapping dummy variable.
bool handle_chunks_for_none_array(BaseType *btp, bool disable_missing_data, string &err_msg) {

    bool ret_value = false;

    VERBOSE(cerr<<"For none_array: var name: "<<btp->name() <<endl);

    D4Attributes *d4_attrs = btp->attributes();
    if (d4_attrs->empty() == false && btp->type() == dods_byte_c) {

        auto attr = d4_attrs->find("eos_cf_grid_mapping");

        if (disable_missing_data == false) {

            // Here we don't bother to check the attribute value since this CF grid variable value is dummy.
            if (attr) {
    
                auto db = dynamic_cast<DmrppByte *>(btp);
                if (!db) {
                    err_msg = "Expected to find a DmrppByte instance but did not in handle_chunks_for_none_array";
                    return false;
                }

                VERBOSE(cerr<<"For none_array cf dummy grid variable: var name: "<<btp->name() <<endl);

                char buf='p';
                db->set_missing_data(true);
                db->set_value((dods_byte)buf);
                db->set_read_p(true);
    
            }
        }
        ret_value = true;
    }

    return ret_value;
 
}

// Obtain offset/length information for a variable.
bool get_chunks_for_a_variable(const string& filename,int32 sd_id, int32 file_id, BaseType *btp, bool disable_missing_data) {

    switch (btp->type()) {
        case dods_structure_c: {
            close_hdf4_file_ids(sd_id,file_id);
            throw BESInternalError("Structure scalar is not supported by DAP4 for HDF4 at this time.\n", __FILE__, __LINE__);
        }
        case dods_sequence_c:
            VERBOSE(cerr << btp->FQN() << ": Sequence is not supported by DMR++ for HDF4 at this time.\n");
            return false;
        case dods_grid_c: {
            close_hdf4_file_ids(sd_id,file_id);
            throw BESInternalError("Grids are not supported by DAP4.", __FILE__, __LINE__);
        }
        case dods_array_c:
            return get_chunks_for_an_array(filename,sd_id,file_id, btp,disable_missing_data);
        default: {
            string err_msg;
            bool ret_value = handle_chunks_for_none_array(btp,disable_missing_data,err_msg);
            if (ret_value == false) {
                if (err_msg.empty() == false) { 
                    close_hdf4_file_ids(sd_id,file_id);
                    throw BESInternalError(err_msg, __FILE__, __LINE__);
                }
                else 
                    VERBOSE(cerr << btp->FQN() << ": " << btp->type_name() << " is not supported by DMR++ for HDF4 at this time.\n");
            }
            return ret_value;
        }
    }
}

/**
 * @brief Iterate over all the variables in a DMR and get their chunk info
 *
 * @param file The open HDF4 file; passed through to get_variable_chunk_info
 * @param group Read variables from this DAP4 Group. Call with the root Group
 * @param sd_id : HDF4 SD interface ID
 * @param file_id : HDF4 H interface ID
 * @param disable_missing_data flag to disable the generation and storing of the missing data in the dmrpp file
 * to process all the variables in the DMR
 */
void get_chunks_for_all_variables(const string& filename, int32 sd_id, int32 file_id, D4Group *group, bool disable_missing_data) {

    // Variables in the group
    for(auto btp : group->variables()) {
        if (btp->type() != dods_group_c) {
            // If this is not a group, it is a variable
            // This is the part where we find out if a variable can be used with DMR++
            if (!get_chunks_for_a_variable(filename,sd_id,file_id, btp, disable_missing_data)) {
                ERROR("Could not include DMR++ metadata for variable " << btp->FQN());
            }
        }
        else {
            // child group
            auto g = dynamic_cast<D4Group*>(btp);
            if (!g)
                throw BESInternalError("Expected "  + btp->name() + " to be a D4Group but it is not.", __FILE__, __LINE__);
            get_chunks_for_all_variables(filename,sd_id,file_id, g, disable_missing_data);
        }
    }
    // all groups in the group
    for (auto g = group->grp_begin(), ge = group->grp_end(); g != ge; ++g) {
        get_chunks_for_all_variables(filename,sd_id,file_id, *g, disable_missing_data);
    }

}

/**
 * @brief Add chunk information about to a DMRpp object
 * @param h4_file_name Read information from this file
 * @param dmrpp Dump the chunk information here
*  @param disable_missing_data flag to disable the generation and storing of the missing data in the dmrpp file
 */
void add_chunk_information(const string &h4_file_name, DMRpp *dmrpp, bool disable_missing_data)
{
    // Open the hdf4 file
    int32 sd_id = SDstart(h4_file_name.c_str(), DFACC_READ);
    if (sd_id < 0) {
        stringstream msg;
        msg << "Error: HDF4 file '" << h4_file_name << "' cannot be opened." << endl;
        throw BESNotFoundError(msg.str(), __FILE__, __LINE__);
    }

    int32 file_id = Hopen(h4_file_name.c_str(),DFACC_READ,0);
    if (file_id < 0) {
        stringstream msg;
        msg << "Error: HDF4 file '" << h4_file_name << "' Hopen failed." << endl;
        throw BESNotFoundError(msg.str(), __FILE__, __LINE__);
    }
    
    if (Vstart(file_id)<0) { 
        stringstream msg;
        msg << "Error: HDF4 file '" << h4_file_name << "' Vstart failed." << endl;
        throw BESNotFoundError(msg.str(), __FILE__, __LINE__);
    }
 
    // Iterate over all the variables in the DMR
    get_chunks_for_all_variables(h4_file_name, sd_id, file_id, dmrpp->root(),disable_missing_data);

    close_hdf4_file_ids(sd_id,file_id);
}

/**
 * @brief Performs a quality control check on the user supplied data file.
 *
 * * Test that the hdf4 file exists and can be read from.
 *
 * @param file_name
 */
void qc_input_file(const string &file_fqn)
{

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
    int status = Hishdf(file_fqn.c_str());

    //Check if file is NOT an HDF4 file
    if (status != 1) {
        stringstream msg;
        msg << "The provided file: " << file_fqn << " - ";
        msg << "is not an HDF4 file" << endl;
        throw BESInternalFatalError(msg.str(), __FILE__, __LINE__);
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
    auto version_unique = make_unique<D4Attribute>("build_dmrpp_metadata", StringToD4AttributeType("container"));
    auto version = version_unique.get();

    auto creation_date_unique = make_unique<D4Attribute>("created", StringToD4AttributeType("string")); 
    auto creation_date = creation_date_unique.get();
    creation_date->add_value(what_time_is_it()); 
    version->attributes()->add_attribute_nocopy(creation_date_unique.release()); 

    auto build_dmrpp_version_unique = make_unique<D4Attribute>("build_dmrpp", StringToD4AttributeType("string"));
    auto build_dmrpp_version = build_dmrpp_version_unique.get();
    build_dmrpp_version->add_value(CVER);
    version->attributes()->add_attribute_nocopy(build_dmrpp_version_unique.release());

    auto bes_version_unique = make_unique<D4Attribute>("bes", StringToD4AttributeType("string"));
    auto bes_version = bes_version_unique.get();
    bes_version->add_value(CVER);
    version->attributes()->add_attribute_nocopy(bes_version_unique.release());

    stringstream ldv;
    ldv << libdap_name() << "-" << libdap_version();
    auto libdap4_version_unique =  make_unique<D4Attribute>("libdap", StringToD4AttributeType("string"));
    auto libdap4_version = libdap4_version_unique.get();
    libdap4_version->add_value(ldv.str());
    version->attributes()->add_attribute_nocopy(libdap4_version_unique.release());

    if(!bes_conf_doc.empty()) {
        // Add the BES configuration used to create the base DMR
        auto config_unique = make_unique<D4Attribute>("configuration", StringToD4AttributeType("string"));
        auto config = config_unique.get();
        config->add_value(bes_conf_doc);
        version->attributes()->add_attribute_nocopy(config_unique.release());
    }

    if(!invocation.empty()) {
        // How was build_dmrpp invoked?
        auto invoke_unique = make_unique<D4Attribute>("invocation", StringToD4AttributeType("string"));
        auto invoke = invoke_unique.get();
        invoke->add_value(invocation);
        version->attributes()->add_attribute_nocopy(invoke_unique.release());
    }
    // Inject version and configuration attributes into DMR here.
    dmrpp->root()->attributes()->add_attribute_nocopy(version_unique.release());
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
 * @param h4_file_fqn The granule filename.
 * @param add_production_metadata If true the production metadata (software version, configuration, and invocation) will
 * be added to the DMR++.
 * @param argc The number of arguments supplied to build_dmrpp
 * @param argv The arguments for build_dmrpp.
 */
void build_dmrpp_from_dmr_file(const string &dmrpp_href_value, const string &dmr_filename, const string &h4_file_fqn,
        bool add_production_metadata, bool disable_missing_data, const string &bes_conf_file_used_to_create_dmr, int argc, char *argv[])
{
    // Get dmr:
    DMRpp dmrpp;
    DmrppTypeFactory dtf;
    dmrpp.set_factory(&dtf);

    ifstream in(dmr_filename.c_str());
    D4ParserSax2 parser;
    parser.intern(in, &dmrpp, false);

    add_chunk_information(h4_file_fqn, &dmrpp,disable_missing_data);
        
    if (add_production_metadata) {
        inject_version_and_configuration(argc,argv,bes_conf_file_used_to_create_dmr,&dmrpp);
    }

    XMLWriter writer;
    dmrpp.print_dmrpp(writer, dmrpp_href_value);
    cout << writer.get_doc();

}


} // namespace build_dmrpp_util
