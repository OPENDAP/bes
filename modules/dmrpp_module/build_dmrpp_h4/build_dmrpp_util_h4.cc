// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the Hyrax data server.

// Copyright (c) 2022 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
// Copyright (c) The HDF Group
// Author: Kent Yang <myang6@hdfgroup.org>
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

#include "h4config.h"
#include "hdf.h"  // HDF4 header file
#include "mfhdf.h"  // Include the HDF4 header file
#include "HdfEosDef.h"

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
#if 0
#define FAIL_ERROR(x) do { cerr << "ERROR: " << x << " " << __FILE__ << ":" << __LINE__ << endl; exit(1); } while(false)
#endif

#define ERROR(x) do { cerr << "ERROR: " << x << " " << __FILE__ << ":" << __LINE__ << endl; } while(false)

/*
 * Hold mapping information for SDS objects.
 */
using SD_mapping_info_t = struct {
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

// This function is adapted from H4mapper implemented by the HDF group. 
// h4mapper can be found from https://docs.hdfgroup.org/archive/support/projects/h4map/h4map_writer.html
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

void close_hdf4_file_ids(int32 sd_id, int32 file_id) {
    Vend(file_id);
    Hclose(file_id);
    SDend(sd_id);
}

// Try to combine the adjacent linked blocks to one block. KY 2024-03-12
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
        //                    push back the i+1's unchanged offset;
        //                    save the i+1's length to the temp_length variable.
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
    intn  ret_value = SUCCEED;

#if 0
    /* Free any info before resetting. */
    SDfree_mapping_info(map_info);
#endif
    /* Reset map_info. */
    /* HDmemset(map_info, 0, sizeof(SD_mapping_info_t)); */

    /* Save SDS id since HDmemset reset it. map_info->id will be reused. */
    /* map_info->id = sdsid; */

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
        default:
            break;

    }
    return ret_value;
}
bool SD_set_fill_value(int32 sdsid, int32 datatype, BaseType *btp) {

    string fill_value = get_sds_fill_value_str(sdsid,datatype);
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

    // Check if SDS has no data. But, what should we do??? jhrg 12/7/23
    int is_empty = 0;
    status = SDcheckempty(sdsid, &is_empty);
    if (status == FAIL) {
        ERROR("SDcheckempty() failed.");
        SDendaccess(sdsid);
        return false;
    }

    if (is_empty) {
        // FIXME Maybe this is the case where the variable is just fill values? jhrg 12/7/23
        // No, this is mostly an SDS with unlimited dimension and the dimension size is 0.
        // Now, we can just ignore since it doesn't contain any data. KY 02/12/24
        VERBOSE(cerr << "SDS is empty." << endl);
        ERROR("This SDS is empty; we haven't handled this case yet.");
        SDendaccess(sdsid);
        return false;
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
                ERROR("read_chunk() failed.");
                SDendaccess(sdsid);
                return false;
            }
            
            auto pia = write_chunk_position_in_array(rank, chunk_dimension_sizes.data(), strides.data());

            // We cannot find a chunked case when the number of blocks is greater than 1. We will issue a failure
            // when we encounter such a case for the time being. KY 02/19/2024.

            if (map_info.nblocks >1) {
#if 0
                cout << "number of blocks in a chunk is: " << map_info.nblocks << endl;
#endif
                ERROR("Number of blocks in this chunk is greater than 1.");
                SDendaccess(sdsid);
                return false;
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
            SDfree_mapping_info(&map_info);
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

#if 0
    for (int i = 0; i<offset.size(); i++) {
       cout<<"offset["<<i<<"]= "<<offset[i] <<endl;
       cout<<"length["<<i<<"]= "<<length[i] <<endl;
    }
#endif

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

#if 0
for (int i = 0; i <merged_lengths.size(); i++) {
cout <<"merged_lengths["<<i<<"]= "<<merged_lengths[i]<<endl;
cout <<"merged_offsets["<<i<<"]= "<<merged_offsets[i]<<endl;

}
#endif
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

#if 0
        int32 total_num_fields = VFnfields(vdata_id);
        if (total_num_fields == 1) {
            int32 offset = 0;
            int32 length = 0; 
            VSgetdatainfo(vdata_id,0,1,&offset,&length);           
cout <<"offset is: "<<offset <<endl;
cout <<"length is: "<<length <<endl;
            
        }
#endif

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

        for (unsigned i = 0; i < merged_number_blocks; i++)
            dc->add_chunk(endian_name, merged_lengths[i], merged_offsets[i],true,i);

    }

    VSdetach(vdata_id);
    return true;

}

#if 0
bool obtain_compress_encode_data(string &encoded_str, const Bytef*source_data,size_t source_data_size, string &err_msg) {

    uLong ssize = (uLong)source_data_size;
    uLongf csize = (uLongf)ssize*2;
    vector<Bytef> compressed_src;
    compressed_src.resize(source_data_size*2);

    int retval = compress(compressed_src.data(), &csize, source_data, ssize);
    if (retval != 0) {
        err_msg = "Fail to compress the data";
        return false;
    }

    encoded_str = base64::Base64::encode(compressed_src.data(),(int)csize);

    return true;

}
#endif


bool add_missing_eos_latlon(const string &filename,BaseType *btp, const D4Attribute *eos_ll_attr, string &err_msg) {

    VERBOSE(cerr<<"Coming to add_missing_eos_latlon"<<endl);
    
    string eos_ll_attr_value = eos_ll_attr->value(0);

    VERBOSE(cerr<<"eos_ll_attr_value: "<<eos_ll_attr_value <<endl);

    size_t space_pos = eos_ll_attr_value.find(' ');
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

    auto dc = dynamic_cast<DmrppCommon *>(btp);
    if (!dc) {
        GDdetach(gridid);
        GDclose(gridfd);
        err_msg = "Expected to find a DmrppCommon instance but did not in add_missing_eos_latlon";
        return false;
    }
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
 
#if 0
    size_t orig_buf_size = tmp_data.size()*sizeof(double);
    uLong ssize = (uLong)orig_buf_size;
    uLongf csize = (uLongf)ssize*2;
    vector<Bytef> compressed_src;
    compressed_src.resize(orig_buf_size*2);

    int retval = compress(compressed_src.data(), &csize, (const Bytef*)tmp_data.data(), ssize);
    if (retval != 0) {
        cout<<"Fail to compress the data"<<endl;
        exit(1);
    }

    string encoded = base64::Base64::encode(compressed_src.data(),(int)csize);

#endif

    GDdetach(gridid);
    GDclose(gridfd);
    
    return true;

}

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

    auto dc = dynamic_cast<DmrppCommon *>(btp);
    if (!dc) {
        err_msg = "Expected to find a DmrppCommon instance but did not in add_missing_sp_latlon";
        return false;
    }
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
 * @param file
 * @param btp
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

    // Look for the full name path for this variable
    // If one was not given via an attribute, use BaseType::FQN() which
    // relies on the variable's position in the DAP dataset hierarchy.
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
                throw BESInternalError("Cannot retrieve SDS information correctly for  " + btp->name() ,
                                   __FILE__, __LINE__);
            }
        }
    
        else {
            if (false == ingest_vdata_info_to_chunk(file_id, obj_ref,btp)) {
                close_hdf4_file_ids(sd_id,file_id);
                throw BESInternalError("Cannot retrieve vdata information correctly for  " + btp->name() ,
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
                close_hdf4_file_ids(sd_id,file_id);
                string error_msg = "Expected to find an attribute that stores either HDF4 SDS reference or HDF4 Vdata reference or eos lat/lon or special HDF4 lat/lon for ";
                throw BESInternalError(error_msg + btp->name() + " but did not.",__FILE__,__LINE__);
            }
            
        }        
    }
 

    return true;
}

bool get_chunks_for_a_variable(const string& filename,int32 sd_id, int32 file_id, BaseType *btp, bool disable_missing_data) {

    switch (btp->type()) {
        case dods_structure_c: {
            //TODO: this needs to be re-written since the data of the whole structure is retrieved. KY-2024-02-26
            // Handle this later. KY 2024-03-07
            // Comment about the above "TODO", the current HDF4 to DMR direct mapping will never go here. So
            // we may not need to handle this at all. KY 2024-03-12
#if 0
            auto sp = dynamic_cast<Structure *>(btp);
            //for_each(sp->var_begin(), sp->var_end(), [file](BaseType *btp) { get_chunks_for_a_variable(file, btp); });
#endif
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
void get_chunks_for_all_variables(const string& filename, int32 sd_id, int32 file_id, D4Group *group, bool disable_missing_data) {

    // variables in the group
    for(auto btp : group->variables()) {
        if (btp->type() != dods_group_c) {
            // if this is not a group, it is a variable
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

 
    // iterate over all the variables in the DMR
    get_chunks_for_all_variables(h4_file_name, sd_id,file_id, dmrpp->root(),disable_missing_data);

    close_hdf4_file_ids(sd_id,file_id);
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
    int status = Hishdf(file_fqn.c_str());

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

#if 0
    if (add_production_metadata) {
        // I updated this function call to reflect the changes I made to the build_dmrpp_util.cc
        // I see that it is not currently in service but it's clear that something like this
        // will be needed to establish history/provenance of the dmr++ file. - ndp 07/26/24
        inject_build_dmrpp_metadata(argc, argv, bes_conf_file_used_to_create_dmr, &dmrpp);
    }
#endif

    XMLWriter writer;
    dmrpp.print_dmrpp(writer, dmrpp_href_value);
    cout << writer.get_doc();

}


} // namespace build_dmrpp_util
