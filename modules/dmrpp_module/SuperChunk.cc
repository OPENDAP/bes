// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Nathan Potter<ndp@opendap.org>
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

#include <sstream>
#include <vector>
#include <string>

#include "BESInternalError.h"
#include "BESDebug.h"
#include "CurlUtils.h"

#include "DmrppRequestHandler.h"
#include "CurlHandlePool.h"
#include "DmrppCommon.h"
#include "DmrppArray.h"
#include "DmrppNames.h"
#include "Chunk.h"
#include "SuperChunk.h"

#define prolog std::string("SuperChunk::").append(__func__).append("() - ")

using std::stringstream;
using std::string;
using std::vector;

namespace dmrpp {

#if 0
string SuperChunk::get_curl_range_arg_string() {
    return curl::get_range_arg_string(d_offset, d_size);
}
#endif

/**
 * @brief Attempts to add a new Chunk to this SuperChunk.
 *
 * If the passed chunk has the same data url, and is it is contiguous with the
 * current end if the SuperChunk the Chunk is added, otherwise it is skipped.
 * @param candidate_chunk The Chunk to add.
 * @return True when the chunk is added, false otherwise.
 */
bool SuperChunk::add_chunk(const std::shared_ptr<Chunk> candidate_chunk) {
    bool chunk_was_added = false;
    if(d_chunks.empty()){
        d_chunks.push_back(candidate_chunk);
        d_offset = candidate_chunk->get_offset();
        d_size = candidate_chunk->get_size();
        d_data_url = candidate_chunk->get_data_url();
        chunk_was_added =  true;
    }
    else if(is_contiguous(candidate_chunk) &&
            candidate_chunk->get_data_url() == d_data_url ){

        this->d_chunks.push_back(candidate_chunk);
        d_size += candidate_chunk->get_size();
        chunk_was_added =  true;
    }
    return chunk_was_added;
}


/**
 * @brief Returns true if candidate_chunk is "contiguous" with the end of the SuperChunk instance.
 *
 * Returns true if the implemented rule for contiguousity determines that the candidate_chunk is
 * contiguous with this SuperChunk and false otherwise.
 *
 * Currently the rule is that the offset of the candidate_chunk must be the same as the current
 * offset + size of the SuperChunk.
 *
 * @param candidate_chunk The Chunk to evaluate for contiguousness with this SuperChunk.
 * @return True if chunk isdeemed contiguous, false otherwise.
 */
bool SuperChunk::is_contiguous(const std::shared_ptr<Chunk> candidate_chunk) {
    return (d_offset + d_size) == candidate_chunk->get_offset();
}

/**
 * @brief  Assigns each Chunk held by the SuperChunk a read buffer.
 *
 * Each Chunks read buffer is mapped to the corresponding section of the SuperChunk's
 * enclosing read buffer.
 */
void SuperChunk::map_chunks_to_buffer()
{
    unsigned long long bindex = 0;
    for(const auto &chunk : d_chunks){
        chunk->set_read_buffer(d_read_buffer + bindex, chunk->get_size(),0, false);
        bindex += chunk->get_size();
        if(bindex>d_size){
            stringstream msg;
            msg << "ERROR The computed buffer index, " << bindex << " is larger than expected size of the SuperChunk. ";
            msg << "d_size: " << d_size;
            throw BESInternalError(msg.str(), __FILE__, __LINE__);

        }
    }
}

/**
 * @brief Reads the contiguous range of bytes associated with the SuperChunk from the data URL.
 */
void SuperChunk::read_aggregate_bytes()
{
    // Since we already have a good infrastructure for reading Chunks, we just make a big-ol-Chunk to
    // use for grabbing bytes. Then, once read, we'll use the child Chunks to do the dirty work of inflating
    // and moving the results into the DmrppCommon object.
    Chunk chunk(d_data_url, "NOT_USED", d_size, d_offset);

    chunk.set_read_buffer(d_read_buffer, d_size,0,false);

    dmrpp_easy_handle *handle = DmrppRequestHandler::curl_handle_pool->get_easy_handle(&chunk);
    if (!handle)
        throw BESInternalError(prolog + "No more libcurl handles.", __FILE__, __LINE__);

    try {
        handle->read_data();  // throws if error
        DmrppRequestHandler::curl_handle_pool->release_handle(handle);
    }
    catch(...) {
        DmrppRequestHandler::curl_handle_pool->release_handle(handle);
        throw;
    }

    // If the expected byte count was not read, it's an error.
    if (d_size != chunk.get_bytes_read()) {
        ostringstream oss;
        oss << "Wrong number of bytes read for chunk; read: " << chunk.get_bytes_read() << ", expected: " << d_size;
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }

    d_is_read = true;
}


/**
 * @brief Cause the SuperChunk and all of it's subordinate Chunks to be read.
 */
void SuperChunk::read() {
    if (d_is_read) {
        BESDEBUG(MODULE, prolog << "SuperChunk (" << (void **) this << ") has already been read! Returning." << endl);
        return;
    }

    if(!d_read_buffer){
        // Allocate memory for SuperChunk receive buffer.
        // release memory in destructor.
        d_read_buffer = new char[d_size];
    }

    // Massage the chunks so that their read/receive/intern data buffer
    // points to the correct section of the d_read_buffer memory.
    // "Slice it up!"
    map_chunks_to_buffer();

    // Read the bytes from the target URL. (pthreads, maybe depends on size...)
    // Use one (or possibly more) thread(s) depending on d_size
    // and utilize our friend cURL to stuff the bytes into d_read_buffer
    read_aggregate_bytes();

    // Set each Chunk's read state to true.
    // Set each chunks byte count to the expected
    // size for the chunk - because upstream events
    // have assured this to be true.
    for(auto chunk : d_chunks){
        chunk->set_is_read(true);
        chunk->set_bytes_read(chunk->get_size());
    }

}

/**
 * @brief Reads SuperChunk, processes subordinate Chunks and writes data in to target_array.
 * @param target_array The array into which to write the data.
 */
    void SuperChunk::chunks_to_array_values(DmrppArray *target_array) {
        BESDEBUG(MODULE, prolog << "BEGIN" << endl );

        read();

        vector<unsigned int> constrained_array_shape = target_array->get_shape(true);

        for(auto &chunk :d_chunks){
            if (target_array->is_deflate_compression() || target_array->is_shuffle_compression())
                chunk->inflate_chunk(target_array->is_deflate_compression(), target_array->is_shuffle_compression(),
                                     target_array->get_chunk_size_in_elements(), target_array->var()->width());

            vector<unsigned int> target_element_address = chunk->get_position_in_array();
            vector<unsigned int> chunk_source_address(target_array->dimensions(), 0);

            target_array->insert_chunk(
                    0 /* dimension */,
                    &target_element_address,
                    &chunk_source_address,
                    chunk,
                    constrained_array_shape);
        }

        BESDEBUG(MODULE, prolog << "END" << endl );
    }

    /**
 * @brief Reads SuperChunk, processes subordinate Chunks and writes data in to target_array.
 * @param target_array The array into which to write the data.
 */
    void SuperChunk::chunks_to_array_values_unconstrained(DmrppArray *target_array) {
        BESDEBUG(MODULE, prolog << "BEGIN" << endl );

        read();

        // The size in element of each of the array's dimensions
        const vector<unsigned int> array_shape = target_array->get_shape(true);
        // The size, in elements, of each of the chunk's dimensions
        const vector<unsigned int> chunk_shape = target_array->get_chunk_dimension_sizes();


        for(auto &chunk :d_chunks){
            if (target_array->is_deflate_compression() || target_array->is_shuffle_compression())
                chunk->inflate_chunk(target_array->is_deflate_compression(), target_array->is_shuffle_compression(),
                                     target_array->get_chunk_size_in_elements(), target_array->var()->width());

            vector<unsigned int> target_element_address = chunk->get_position_in_array();
            vector<unsigned int> chunk_source_address(target_array->dimensions(), 0);

            target_array->insert_chunk_unconstrained(chunk, 0, 0, array_shape, 0, chunk_shape, chunk->get_position_in_array());
        }

        BESDEBUG(MODULE, prolog << "END" << endl );
    }

/**
 * @brief Makes a string representation of the SuperChunk.
 * @param verbose If set true then details of the subordinate Chunks will be included.
 * @return  A string representation of the SuperChunk.
 */
string SuperChunk::to_string(bool verbose=false) const {
    stringstream msg;
    msg << "[SuperChunk: " << (void **)this;
    msg << " offset: " << d_offset;
    msg << " size: " << d_size ;
    msg << " chunk_count: " << d_chunks.size();
    //msg << " parent: " << d_parent->name();
    msg << "]";
    if (verbose) {
        msg << endl;
        for (auto chunk: d_chunks) {
            msg << chunk->to_string() << endl;
        }
    }
    return msg.str();
}

/**
 * @brief Writes the to_string() output to the stream strm.
 * @param strm
 */
void SuperChunk::dump(ostream & strm) const {
    strm << to_string(false) ;
}

} // namespace dmrpp