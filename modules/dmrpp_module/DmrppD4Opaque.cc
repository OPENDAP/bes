
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

#include <string>

#include <BESError.h>
#include <BESDebug.h>


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
    DmrppCommon::_duplicate(rhs);

    return *this;
}

/**
 * @brief Insert a chunk into this array
 *
 * This method inserts the given chunk into the array. Unlike other versions of this
 * method, it _does not_ first check to see if the chunk should be inserted.
 *
 * This method is called recursively, with successive values of \arg dim, until
 * dim is equal to the rank of the array (act. rank - 1). The \arg target_element_address
 * and \arg chunk_element_address are the addresses, in 'element space' of the
 * location in this array where
 *
 * @note Only call this method when it is know that \arg chunk should be inserted
 * into the array. The chunk be both read and decompressed.
 *
 * @param dim
 * @param target_element_address
 * @param chunk_element_address
 * @param chunk
 */
void DmrppD4Opaque::insert_chunk(unsigned int dim, vector<unsigned int> *target_element_address, vector<unsigned int> *chunk_element_address,
    Chunk *chunk)
{
    // The size, in elements, of each of the chunk's dimensions.
    const vector<unsigned int> &chunk_shape = get_chunk_dimension_sizes();

    // The chunk's origin point a.k.a. its "position in array".
    const vector<unsigned int> &chunk_origin = chunk->get_position_in_array();

    dimension thisDim = this->get_dimension(dim);

    // What's the first element that we are going to access for this dimension of the chunk?
    unsigned long long chunk_start = get_chunk_start(dim, chunk_origin);

    // Now we figure out the correct last element, based on the subset expression
    unsigned long long end_element = chunk_origin[dim] + chunk_shape[dim] - 1;
    if ((unsigned) thisDim.stop < end_element) {
        end_element = thisDim.stop;
    }

    unsigned long long chunk_end = end_element - chunk_origin[dim];
    vector<unsigned int> constrained_array_shape = get_shape(true);

    unsigned int last_dim = chunk_shape.size() - 1;
    if (dim == last_dim) {
        char *source_buffer = chunk->get_rbuf();
        char *target_buffer = get_buf();
        unsigned int elem_width = prototype()->width();

        if (thisDim.stride == 1) {
            // The start element in this array
            unsigned long long start_element = chunk_origin[dim] + chunk_start;
            // Compute how much we are going to copy
            unsigned long long chunk_constrained_inner_dim_bytes = (end_element - start_element + 1) * elem_width;

            // Compute where we need to put it.
            (*target_element_address)[dim] = (start_element - thisDim.start) / thisDim.stride;
            // Compute where we are going to read it from
            (*chunk_element_address)[dim] = chunk_start;

            unsigned int target_char_start_index = get_index(*target_element_address, constrained_array_shape) * elem_width;
            unsigned int chunk_char_start_index = get_index(*chunk_element_address, chunk_shape) * elem_width;

            memcpy(target_buffer + target_char_start_index, source_buffer + chunk_char_start_index, chunk_constrained_inner_dim_bytes);
        }
        else {
            // Stride != 1
            for (unsigned int chunk_index = chunk_start; chunk_index <= chunk_end; chunk_index += thisDim.stride) {
                // Compute where we need to put it.
                (*target_element_address)[dim] = (chunk_index + chunk_origin[dim] - thisDim.start) / thisDim.stride;

                // Compute where we are going to read it from
                (*chunk_element_address)[dim] = chunk_index;

                unsigned int target_char_start_index = get_index(*target_element_address, constrained_array_shape) * elem_width;
                unsigned int chunk_char_start_index = get_index(*chunk_element_address, chunk_shape) * elem_width;

                memcpy(target_buffer + target_char_start_index, source_buffer + chunk_char_start_index, elem_width);
            }
        }
    }
#if 0
    else {
        // Not the last dimension, so we continue to proceed down the Recursion Branch.
        for (unsigned int chunk_index = chunk_start; chunk_index <= chunk_end; chunk_index += thisDim.stride) {
            (*target_element_address)[dim] = (chunk_index + chunk_origin[dim] - thisDim.start) / thisDim.stride;
            (*chunk_element_address)[dim] = chunk_index;

            // Re-entry here:
            insert_chunk(dim + 1, target_element_address, chunk_element_address, chunk);
        }
    }
#endif

}

void DmrppD4Opaque::read_chunks()
{
    vector<Chunk> &chunk_refs = get_chunk_vec();
    if (chunk_refs.size() == 0) throw BESInternalError(string("Expected one or more chunks for variable ") + name(), __FILE__, __LINE__);

    // This is not needed here - Opaque is never constrained - but using it
    // means we can reuse the DmrppArray::read_chunks_parallel() method's logic.
    // TODO Replace with a more efficient version. jhrg 5/3/18
    queue<Chunk *> chunks_to_read;

    // Look at all the chunks
    for (vector<Chunk>::iterator c = chunk_refs.begin(), e = chunk_refs.end(); c != e; ++c) {
        Chunk &chunk = *c;

        chunks_to_read.push(c);
    }

    reserve_value_capacity(get_size(true));

    if (DmrppRequestHandler::d_use_parallel_transfers) {
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

                BESDEBUG(dmrpp_3, "Queuing: " << chunk->to_string() << endl);
                mhandle->add_easy_handle(handle);

                chunks_to_insert.push(chunk);
            }

            mhandle->read_data(); // read and decompress chunks, then remove the easy_handles

            while (chunks_to_insert.size() > 0) {
                Chunk *chunk = chunks_to_insert.front();
                chunks_to_insert.pop();

                chunk->inflate_chunk(is_deflate_compression(), is_shuffle_compression(), get_chunk_size_in_elements(), 1);

                vector<unsigned int> target_element_address = chunk->get_position_in_array();
                vector<unsigned int> chunk_source_address(dimensions(), 0);

                BESDEBUG(dmrpp_3, "Inserting: " << chunk->to_string() << endl);
                insert_chunk(0 /* dimension */, &target_element_address, &chunk_source_address, chunk);
            }
        }
    }
    else {
        // This version is the 'serial' version of the code. It reads a chunk, inserts it,
        // reads the next one, and so on.
        while (chunks_to_read.size() > 0) {
            Chunk *chunk = chunks_to_read.front();
            chunks_to_read.pop();

            BESDEBUG(dmrpp_3, "Reading: " << chunk->to_string() << endl);
            chunk->read_chunk();

            chunk->inflate_chunk(is_deflate_compression(), is_shuffle_compression(), get_chunk_size_in_elements(), 1);

            vector<unsigned int> target_element_address = chunk->get_position_in_array();
            vector<unsigned int> chunk_source_address(dimensions(), 0);

            BESDEBUG(dmrpp_3, "Inserting: " << chunk->to_string() << endl);
            insert_chunk(0 /* dimension */, &target_element_address, &chunk_source_address, chunk);
        }
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
        read_chunks_parallel();
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

