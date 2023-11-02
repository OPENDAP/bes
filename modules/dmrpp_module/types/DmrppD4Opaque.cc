
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
#include "Chunk.h"

using namespace libdap;
using namespace std;

namespace dmrpp {

DmrppD4Opaque &
DmrppD4Opaque::operator=(const DmrppD4Opaque &rhs)
{
    if (this == &rhs)
    return *this;

    D4Opaque::operator=(rhs);
    DmrppCommon::operator=(rhs);

    return *this;
}

void DmrppD4Opaque::insert_chunk(shared_ptr<Chunk> chunk)
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

void DmrppD4Opaque::read_chunks()
{
    for (auto chunk : get_immutable_chunks()) {
        chunk->read_chunk();
        if (!is_filters_empty()){
            chunk->filter_chunk(get_filters(), get_chunk_size_in_elements(), 1 /*elem width*/);
        }

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
    if (!get_chunks_loaded())
        load_chunks(this);

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

void
DmrppD4Opaque::set_send_p(bool state)
{
    if (!get_attributes_loaded())
        load_attributes(this);

    D4Opaque::set_send_p(state);
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

