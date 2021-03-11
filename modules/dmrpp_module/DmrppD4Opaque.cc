
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2016 OPeNDAP, Inc.
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

#include <cstring>

#include <string>
#include <queue>

#include <BESLog.h>
#include <BESInternalError.h>
#include <BESDebug.h>

#include "CurlHandlePool.h"
#include "DmrppRequestHandler.h"
#include "DmrppD4Opaque.h"

using namespace libdap;
using namespace std;

namespace dmrpp {

void
DmrppD4Opaque::_duplicate(const DmrppD4Opaque &)
{
}

DmrppD4Opaque::DmrppD4Opaque(const string &n) : D4Opaque(n), DmrppCommon()
{
}

DmrppD4Opaque::DmrppD4Opaque(const string &n, const string &d) : D4Opaque(n, d), DmrppCommon()
{
}

BaseType *
DmrppD4Opaque::ptr_duplicate()
{
    return new DmrppD4Opaque(*this);
}

DmrppD4Opaque::DmrppD4Opaque(const DmrppD4Opaque &rhs) : D4Opaque(rhs), DmrppCommon(rhs)
{
    _duplicate(rhs);
}

DmrppD4Opaque &
DmrppD4Opaque::operator=(const DmrppD4Opaque &rhs)
{
    if (this == &rhs)
    return *this;

    dynamic_cast<D4Opaque &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);
    DmrppCommon::m_duplicate_common(rhs);

    return *this;
}

void DmrppD4Opaque::insert_chunk(shared_ptr<Chunk>  chunk)
{
    // The size, in elements, of each of the chunk's dimensions.
    const vector<unsigned long long> &chunk_shape = get_chunk_dimension_sizes();
    if (chunk_shape.size() != 1) throw BESInternalError("Opaque variables' chunks can only have one dimension.", __FILE__, __LINE__);

    // The chunk's origin point a.k.a. its "position in array".
    const vector<unsigned long long> &chunk_origin = chunk->get_position_in_array();

    char *source_buffer = chunk->get_rbuf();
    unsigned char *target_buffer = get_buf();

    memcpy(target_buffer + chunk_origin[0], source_buffer, chunk_shape[0]);
}

#if 0
// This is the old code based on DmrppArray. jhrg 9/15/20
void DmrppD4Opaque::read_chunks_parallel()
{
    vector<Chunk> &chunk_refs = get_chunk_vec();
    if (chunk_refs.size() == 0) throw BESInternalError(string("Expected one or more chunks for variable ") + name(), __FILE__, __LINE__);

    // This is not needed here - Opaque is never constrained - but using it
    // means we can reuse the DmrppArray::read_chunks_parallel() method's logic.
    // TODO Replace with a more efficient version. jhrg 5/3/18
    queue<Chunk*> chunks_to_read;

    // Look at all the chunks
    for (vector<Chunk>::iterator c = chunk_refs.begin(), e = chunk_refs.end(); c != e; ++c) {
        chunks_to_read.push(&*c);
    }

#if !HAVE_CURL_MULTI_API
    if (DmrppRequestHandler::d_use_transfer_threads)
        LOG("The DMR++ handler is configured to use parallel transfers, but the libcurl Multi API is not present, defaulting to serial transfers");
#endif

    if (DmrppRequestHandler::d_use_transfer_threads && have_curl_multi_api) {
        // This is the parallel version of the code. It reads a set of chunks in parallel
        // using the multi curl API, then inserts them, then reads the next set, ... jhrg 5/1/18
        unsigned int max_handles = DmrppRequestHandler::curl_handle_pool->get_max_handles();
        dmrpp_multi_handle *mhandle = DmrppRequestHandler::curl_handle_pool->get_multi_handle();

       // Look only at the chunks we need, found above. jhrg 4/30/18
       while (chunks_to_read.size() > 0) {
            queue<Chunk*> chunks_to_insert;
            for (unsigned int i = 0; i < max_handles && chunks_to_read.size() > 0; ++i) {
                Chunk *chunk = chunks_to_read.front();
                chunks_to_read.pop();

                chunk->set_rbuf_to_size();
                dmrpp_easy_handle *handle = DmrppRequestHandler::curl_handle_pool->get_easy_handle(chunk);
                if (!handle) throw BESInternalError("No more libcurl handles.", __FILE__, __LINE__);

                mhandle->add_easy_handle(handle);

                chunks_to_insert.push(chunk);
            }

            mhandle->read_data(); // read and decompress chunks, then remove the easy_handles

            while (chunks_to_insert.size() > 0) {
                Chunk *chunk = chunks_to_insert.front();
                chunks_to_insert.pop();

                chunk->inflate_chunk(is_deflate_compression(), is_shuffle_compression(), get_chunk_size_in_elements(), 1 /*elem width*/);

                insert_chunk(chunk);
            }
        }
    }
    else {
        // This version is the 'serial' version of the code. It reads a chunk, inserts it,
        // reads the next one, and so on.
        while (chunks_to_read.size() > 0) {
            Chunk *chunk = chunks_to_read.front();
            chunks_to_read.pop();

            chunk->read_chunk();

            chunk->inflate_chunk(is_deflate_compression(), is_shuffle_compression(), get_chunk_size_in_elements(), 1 /*elem width*/);

            insert_chunk(chunk);
        }
    }

    set_read_p(true);
}
#endif

void DmrppD4Opaque::read_chunks()
{
    for (auto chunk : get_chunks()) {
        chunk->read_chunk();
        chunk->inflate_chunk(is_deflate_compression(), is_shuffle_compression(), get_chunk_size_in_elements(), 1 /*elem width*/);
        insert_chunk(chunk);
    }

    set_read_p(true);
}

/**
 * @brief Read opaque data
 *
 * Read opaque data using a DMR++ metadata file. This is different from the
 * DmrppArray::read() method because Opaque data cannot be subset, and it is
 * thus much simpler.
 *
 * @return Always returns true
 */
bool
DmrppD4Opaque::read()
{
    if (read_p()) return true;

    // if there are no chunks, use read a single contiguous block of data
    // and store it in the object. Note that DmrppCommon uses a single Chunk
    // instance to hold 'contiguous' data.
    if (get_chunk_dimension_sizes().empty()) {
        // read_atomic() returns a pointer to the Chunk data. When the Chunk
        // instance is freed, this memory goes away.
        char *data = read_atomic(name());
        val2buf(data);      // yes, it's not type-safe
    }
    else {
        // Handle the more complex case where the data is chunked.
        read_chunks();
    }

    return true;
}

void DmrppD4Opaque::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "DmrppD4Opaque::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    DmrppCommon::dump(strm);
    D4Opaque::dump(strm);
    strm << BESIndent::LMarg << "value:    " << "----" << /*d_buf <<*/ endl;
    BESIndent::UnIndent();
}

} // namespace dmrpp

