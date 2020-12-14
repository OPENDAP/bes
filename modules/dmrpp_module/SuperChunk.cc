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

bool SuperChunk::add_chunk(const std::shared_ptr<Chunk> &chunk) {
    bool chunk_was_added = false;
    if(d_chunks.empty()){
        d_chunks.push_back(chunk);
        d_offset = chunk->get_offset();
        d_size = chunk->get_size();
        d_data_url = chunk->get_data_url();
        chunk_was_added =  true;
    }
    else if(is_contiguous(chunk) && chunk->get_data_url() == d_data_url){
        this->d_chunks.push_back(chunk);
        d_size += chunk->get_size();
        chunk_was_added =  true;
    }
    return chunk_was_added;
}


/**
 * @brief Returns true if the implemented rule for contiguousity
 * determines that the chunk is contiguous with this SuperChunk
 * and false otherwise.
 * @param chunk The Chunk to evaluate for contiguousness with this SuperChunk.
 * @return True if chunk isdeemed contiguous, false otherwise.
 */
bool SuperChunk::is_contiguous(const std::shared_ptr<Chunk> &chunk) {
    return (d_offset + d_size) == chunk->get_offset();
}


void SuperChunk::map_chunks_to_buffer(char * r_buff)
{
    unsigned long long bindex = 0;
    for(const auto &chunk : d_chunks){
        chunk->set_rbuf(r_buff+bindex, chunk->get_size());
        bindex += chunk->get_size();
    }
}

unsigned long long  SuperChunk::read_contiguous(char *r_buff)
{
    if (d_is_read) {
        BESDEBUG(MODULE, prolog << "Already been read! Returning." << endl);
        return d_size;
    }


    // If we make SuperChunk a child of Chunk then this goes...
    dmrpp_easy_handle *handle = DmrppRequestHandler::curl_handle_pool->get_easy_handle(this);
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
    if (d_size != get_bytes_read()) {
        ostringstream oss;
        oss << "Wrong number of bytes read for chunk; read: " << get_bytes_read() << ", expected: " << get_size();
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }

    d_is_read = true;
    return 0;
}

void SuperChunk::read() {

    // Allocate memory for SuperChunk receive buffer.
    char read_buff[d_size];

    // Massage the chunks so that their read/receive/intern data buffer
    // points to the correct section of the memory allocated into read_buff.
    // "Slice it up!"
    map_chunks_to_buffer(read_buff);

    // Read the bytes from the target URL. (pthreads, maybe depends on size...)
    // Use one (or possibly more) thread(s) depending on d_size
    // and utilize our friend cURL to stuff the bytes into read_buff
    unsigned long long bytes_read = read_contiguous(read_buff);
    if(bytes_read != d_size)
        throw BESInternalError(prolog + "Failed to read super chunk."+to_string(false),__FILE__,__LINE__);


    // Process the raw bytes from the chunk and into the target array
    // memory space.
    //
    //   for(chunk : chunks){ // more pthreads.
    //      Have each chunk process data from its section of the
    //      read buffer into the variables data space.
    //   }
    for(auto chunk : d_chunks){
        chunk->set_is_read(true);
        //chunk->raw_to_var();
    }
    // release memory as needed.

}

string SuperChunk::to_string(bool verbose) const {
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

    void SuperChunk::dump(ostream & strm) const {
        strm << to_string(false) ;
    }

} // namespace dmrpp