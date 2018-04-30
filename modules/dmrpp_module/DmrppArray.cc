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
#include <sstream>
#include <iomanip>
#include <set>
#include <stack>

#include <cstring>
#include <cassert>

#include <unistd.h>

#include "BESInternalError.h"
#include "BESDebug.h"

#include "DmrppArray.h"
#include "DmrppUtil.h"

static const string dmrpp_3 = "dmrpp:3";

using namespace libdap;
using namespace std;

namespace dmrpp {

void DmrppArray::_duplicate(const DmrppArray &)
{
}

DmrppArray::DmrppArray(const string &n, BaseType *v) :
                Array(n, v, true /*is dap4*/), DmrppCommon()
{
}

DmrppArray::DmrppArray(const string &n, const string &d, BaseType *v) :
                Array(n, d, v, true), DmrppCommon()
{
}

BaseType *
DmrppArray::ptr_duplicate()
{
    return new DmrppArray(*this);
}

DmrppArray::DmrppArray(const DmrppArray &rhs) :
                Array(rhs), DmrppCommon(rhs)
{
    _duplicate(rhs);
}

DmrppArray &
DmrppArray::operator=(const DmrppArray &rhs)
{
    if (this == &rhs) return *this;

    dynamic_cast<Array &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);
    DmrppCommon::_duplicate(rhs);

    return *this;
}

/**
 * @brief Is this Array subset?
 * @return True if the array has a projection expression, false otherwise
 */
bool DmrppArray::is_projected()
{
    for (Dim_iter p = dim_begin(), e = dim_end(); p != e; ++p)
        if (dimension_size(p, true) != dimension_size(p, false)) return true;

    return false;
}

/**
 * @brief Compute the index of the address_in_target for an an array of target_shape.
 *
 * Internally we store N-dimensional arrays using a one dimensional array (i.e., a
 * vector). Compute the offset in that vector that corresponds to a location in
 * the N-dimensional array. For example, for three dimensional array of size
 * 2 x 3 x 4, the data values are stored in a 24 element vector and the item at
 * location 1,1,1 (zero-based indexing) would be at offset 1*1 + 1*4 + 1 * 4*3 == 15.
 *
 * @param address_in_target N-tuple zero-based index of an element in N-space
 * @param target_shape N-tuple of the array's dimension sizes.
 * @return The offset into the vector used to store the values.
 */
unsigned long long get_index(const vector<unsigned int> &address_in_target, const vector<unsigned int> &target_shape)
{
    assert(address_in_target.size() == target_shape.size());    // ranks must be equal

    vector<unsigned int>::const_reverse_iterator shape_index = target_shape.rbegin();
    vector<unsigned int>::const_reverse_iterator index = address_in_target.rbegin(), index_end = address_in_target.rend();

    unsigned long long multiplier = *shape_index++;
    unsigned long long offset = *index++;

    while (index != index_end) {
        assert(*index < *shape_index); // index < shape for each dim

        offset += multiplier * *index++;
        multiplier *= *shape_index++;   // TODO Remove the unneeded multiply. jhrg 3/24/17
    }

    return offset;
}

/**
 * @brief This recursive private method collects values from the rbuf and copies
 * them into buf. It supports stop, stride, and start and while correct is not
 * efficient.
 */
void DmrppArray::insert_constrained_no_chunk(Dim_iter dimIter, unsigned long *target_index,
    vector<unsigned int> &subsetAddress, const vector<unsigned int> &array_shape, char /*Chunk*/ *src_buf)
{
    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() - subsetAddress.size(): " << subsetAddress.size() << endl);

    unsigned int bytesPerElt = prototype()->width();
    // char *sourceBuf = src_buf; // ->get_rbuf();
    char *dest_buf = get_buf();

    unsigned int start = this->dimension_start(dimIter, true);
    unsigned int stop = this->dimension_stop(dimIter, true);
    unsigned int stride = this->dimension_stride(dimIter, true);

    dimIter++;

    // The end case for the recursion is dimIter == dim_end(); stride == 1 is an optimization
    // See the else clause for the general case.
    if (dimIter == dim_end() && stride == 1) {
        // For the start and stop indexes of the subset, get the matching indexes in the whole array.
        subsetAddress.push_back(start);
        unsigned long start_index = get_index(subsetAddress, array_shape);
        subsetAddress.pop_back();

        subsetAddress.push_back(stop);
        unsigned long stop_index = get_index(subsetAddress, array_shape);
        subsetAddress.pop_back();

        // Copy data block from start_index to stop_index
        // FIXME Replace this loop with a call to std::memcpy()
        for (unsigned long sourceIndex = start_index; sourceIndex <= stop_index; sourceIndex++) {
            unsigned long target_byte = *target_index * bytesPerElt;
            unsigned long source_byte = sourceIndex * bytesPerElt;
            // Copy a single value.
            for (unsigned long i = 0; i < bytesPerElt; i++) {
                dest_buf[target_byte++] = src_buf[source_byte++];
            }
            (*target_index)++;
        }
    }
    else {
        for (unsigned int myDimIndex = start; myDimIndex <= stop; myDimIndex += stride) {

            // Is it the last dimension?
            if (dimIter != dim_end()) {
                // Nope!
                // then we recurse to the last dimension to read stuff
                subsetAddress.push_back(myDimIndex);
                insert_constrained_no_chunk(dimIter, target_index, subsetAddress, array_shape, src_buf);
                subsetAddress.pop_back();
            }
            else {
                // We are at the last (inner most) dimension.
                // So it's time to copy values.
                subsetAddress.push_back(myDimIndex);
                unsigned int sourceIndex = get_index(subsetAddress, array_shape);
                subsetAddress.pop_back();

                // Copy a single value.
                unsigned long target_byte = *target_index * bytesPerElt;
                unsigned long source_byte = sourceIndex * bytesPerElt;

                for (unsigned int i = 0; i < bytesPerElt; i++) {
                    dest_buf[target_byte++] = src_buf[source_byte++];
                }
                (*target_index)++;
            }
        }
    }
}

/**
 * @brief Read an array that is stored with using one 'chunk.'
 *
 * @return Always returns true, matching the libdap::Array::read() behavior.
 */
bool DmrppArray::read_contiguous()
{
    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() for " << name() << " BEGIN" << endl);

    char *data = read_atomic(name());

    if (!is_projected()) {  // if there is no projection constraint
        val2buf(data);      // yes, it's not type-safe
    }
    else {
        vector<unsigned int> array_shape = get_shape(false);

        // Reserve space in this array for the constrained size of the data request
        reserve_value_capacity(get_size(true));
        unsigned long target_index = 0;
        vector<unsigned int> subset;

        insert_constrained_no_chunk(dim_begin(), &target_index, subset, array_shape, data);
    }

    set_read_p(true);

    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() for " << name() << " END"<< endl);

    return true;
}

/**
 * @brief Return the total number of elements in this Array
 * @param constrained If true, use the constrained size of the array,
 * otherwise use the full size.
 * @return The number of elements in this Array
 */
unsigned long long DmrppArray::get_size(bool constrained)
{
    // number of array elements in the constrained array
    unsigned long long size = 1;
    for (Dim_iter dim = dim_begin(), end = dim_end(); dim != end; dim++) {
        size *= dimension_size(dim, constrained);
    }
    return size;
}

/**
 * @brief Get the array shape
 *
 * @param constrained If true, return the shape of the constrained array.
 * @return A vector<int> that describes the shape of the array.
 */
vector<unsigned int> DmrppArray::get_shape(bool constrained)
{
    vector<unsigned int> shape;
    for (Dim_iter dim = dim_begin(); dim != dim_end(); dim++) {
        shape.push_back(dimension_size(dim, constrained));
    }

    return shape;
}

/**
 * @brief Return the ith dimension object for this Array
 * @param i The index of the dimension object to return
 * @return The dimension object
 */
DmrppArray::dimension DmrppArray::get_dimension(unsigned int i)
{
    assert(i <= (dim_end() - dim_begin()));
    return *(dim_begin() + i);
}

bool DmrppArray::read_chunks_serial()
{
    BESDEBUG("dmrpp", __func__ << " for variable '" << name() << "' - BEGIN" << endl);

    vector<Chunk> &chunk_refs = get_chunk_vec();
    if (chunk_refs.size() == 0)
        throw BESInternalError(string("Expected one or more chunks for variable ") + name(), __FILE__, __LINE__);

    // Allocate target memory.
    reserve_value_capacity(get_size(true));

    /*
     * Find the chunks to be read, make curl_easy handles for them, and
     * stuff them into our curl_multi handle. This is a recursive activity
     * which utilizes the same code that copies the data from the chunk to
     * the variables.
     */
    for (unsigned long i = 0; i < chunk_refs.size(); i++) {
        Chunk &chunk = chunk_refs[i];

        vector<unsigned int> chunk_source_address(dimensions(), 0);
        vector<unsigned int> target_element_address = chunk.get_position_in_array();

        // Recursive insertion operation.
        insert_chunk_serial(0, &target_element_address, &chunk_source_address, &chunk);
    }

    set_read_p(true);

    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() for " << name() << " END"<< endl);

    return true;
}

/**
 * @brief What is the first element to use from a chunk
 *
 * For a chunk that fits in the array at \arg chunk_origin, what is the first element
 * of that chunk that will be transferred to the array? It may be that the first element
 * is actually not part of the chunk (given the array, its constraint, and the
 * \arg chunk_origin), and that indicates this chunk will not be used at all.
 *
 * @param dim Look at this dimension of the chunk and array
 * @param chunk_origin The chunk's position in the array
 * @return The first _element_ of the chunk to transfer.
 */
unsigned long long DmrppArray::get_chunk_start(unsigned int dim, const vector<unsigned int>& chunk_origin)
{
    dimension thisDim = this->get_dimension(dim);

    // What's the first element that we are going to access for this dimension of the chunk?
    unsigned long long first_element_offset = 0; // start with 0
    if ((unsigned) (thisDim.start) < chunk_origin[dim]) {
        // If the start is behind this chunk, then it's special.
        if (thisDim.stride != 1) {
            // And if the stride isn't 1, we have to figure our where to begin in this chunk.
            first_element_offset = (chunk_origin[dim] - thisDim.start) % thisDim.stride;
            // If it's zero great!
            if (first_element_offset != 0) {
                // otherwise we adjustment to get correct first element.
                first_element_offset = thisDim.stride - first_element_offset;
            }
        }
    }
    else {
        first_element_offset = thisDim.start - chunk_origin[dim];
    }

    return first_element_offset;
}

/**
 * Insert data from \arg chunk into the array given the current constraint
 *
 * Recursive calls build up the two vectors \arg target_element_address and
 * \arg chunk_element_address. These vectors start out with \arg dim elements,
 * the \arg chunk_element_address holds 0, 0, ..., 0 and the \arg target_element_address
 * holds the index of the first value of this chunk in the target array
 *
 * @param dim
 * @param target_element_address
 * @param chunk_element_address
 * @param chunk
 * @return
 */
void DmrppArray::insert_chunk_serial(unsigned int dim, vector<unsigned int> *target_element_address,
    vector<unsigned int> *chunk_element_address, Chunk *chunk)
{
    BESDEBUG("dmrpp", __func__ << " dim: "<< dim << " BEGIN "<< endl);

    // The size, in elements, of each of the chunk's dimensions.
    vector<unsigned int> chunk_shape = get_chunk_dimension_sizes();

    // The chunk's origin point a.k.a. its "position in array".
    vector<unsigned int> chunk_origin = chunk->get_position_in_array();

    dimension thisDim = this->get_dimension(dim);

    // Do we even want this chunk?
    if ((unsigned) thisDim.start > (chunk_origin[dim] + chunk_shape[dim])
        || (unsigned) thisDim.stop < chunk_origin[dim]) {
        return; // No. No, we do not. Skip this.
    }

    // What's the first element that we are going to access for this dimension of the chunk?
    unsigned int first_element_offset = get_chunk_start(dim, chunk_origin);

    // Is the next point to be sent in this chunk at all? If no, return.
    if (first_element_offset > chunk_shape[dim]) {
        return;
    }

    // Now we figure out the correct last element, based on the subset expression
    unsigned long long end_element = chunk_origin[dim] + chunk_shape[dim] - 1;
    if ((unsigned) thisDim.stop < end_element) {
        end_element = thisDim.stop;
    }

    unsigned long long chunk_start = first_element_offset; //start_element - chunk_origin[dim];
    unsigned long long chunk_end = end_element - chunk_origin[dim];
    vector<unsigned int> constrained_array_shape = get_shape(true);

    unsigned int last_dim = chunk_shape.size() - 1;
    if (dim == last_dim) {
        BESDEBUG("dmrpp", __func__ << " dim: "<< dim << " THIS IS THE INNER-MOST DIM. "<< endl);

        // Read and Process chunk
        chunk->read_serial(is_deflate_compression(), is_shuffle_compression(),
            get_chunk_size_in_elements(), var()->width());

        char *source_buffer = chunk->get_rbuf();
        char *target_buffer = get_buf();
        unsigned int elem_width = prototype()->width();

#define STRIDE_OPT 1
#if STRIDE_OPT
        if (thisDim.stride == 1) {
            // The start element in this array
            unsigned long long start_element = chunk_origin[dim] + first_element_offset;
           // Compute how much we are going to copy
            unsigned long long chunk_constrained_inner_dim_bytes = (end_element - start_element + 1) * elem_width;

            // Compute where we need to put it.
            (*target_element_address)[dim] = (start_element - thisDim.start) / thisDim.stride;
            // Compute where we are going to read it from
            (*chunk_element_address)[dim] = first_element_offset;

            unsigned int target_char_start_index = get_index(*target_element_address, constrained_array_shape) * elem_width;
            unsigned int chunk_char_start_index = get_index(*chunk_element_address, chunk_shape) * elem_width;

            memcpy(target_buffer + target_char_start_index, source_buffer + chunk_char_start_index,
                chunk_constrained_inner_dim_bytes);
        }
        else {
#endif

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
#if STRIDE_OPT
    }
#endif

    }
    else {
        // Not the last dimension, so we continue to proceed down the Recursion Branch.
        for (unsigned int chunk_index = chunk_start; chunk_index <= chunk_end; chunk_index += thisDim.stride) {
            (*target_element_address)[dim] = (chunk_index + chunk_origin[dim] - thisDim.start) / thisDim.stride;
            (*chunk_element_address)[dim] = chunk_index;

            // Re-entry here:
            insert_chunk_serial(dim + 1, target_element_address, chunk_element_address, chunk);
        }
    }
}

/**
 * @brief Look at all the chunks and mark those that should be read.
 *
 * @todo Save the target element address with the chunk for use in the
 * insert code.
 *
 * @param dim
 * @param target_element_address
 * @param chunk_element_address
 * @param chunk
 */
Chunk *
DmrppArray::find_needed_chunks(unsigned int dim, vector<unsigned int> *target_element_address, Chunk *chunk)
{
    BESDEBUG(dmrpp_3, __func__ << " BEGIN, dim: " << dim << endl);

    // The size, in elements, of each of the chunk's dimensions.
    // TODO Edit get_chunk_dimension_sizes to return a reference
    vector<unsigned int> chunk_shape = get_chunk_dimension_sizes();

    // The chunk's origin point a.k.a. its "position in array".
    // TODO Edit get_position_in_array to return a reference
    vector<unsigned int> chunk_origin = chunk->get_position_in_array();

    dimension thisDim = this->get_dimension(dim);

    // Do we even want this chunk?
    if ((unsigned) thisDim.start > (chunk_origin[dim] + chunk_shape[dim])
        || (unsigned) thisDim.stop < chunk_origin[dim]) {
        return 0; // No. No, we do not. Skip this.
    }

    // What's the first element that we are going to access for this dimension of the chunk?
    unsigned long long chunk_start = get_chunk_start(dim, chunk_origin);

    // Is the next point to be sent in this chunk at all? If no, return.
    if (chunk_start > chunk_shape[dim]) {
        return 0;
    }

    // Now we figure out the correct last element, based on the subset expression
    unsigned long long end_element = chunk_origin[dim] + chunk_shape[dim] - 1;
    if ((unsigned) thisDim.stop < end_element) {
        end_element = thisDim.stop;
    }

    unsigned long long chunk_end = end_element - chunk_origin[dim];

    unsigned int last_dim = chunk_shape.size() - 1;
    if (dim == last_dim) {
#ifndef NDEBUG
        if (BESDebug::IsSet(dmrpp_3)) {
            std::copy(target_element_address->begin(), target_element_address->end(),
                ostream_iterator<unsigned int>(*BESDebug::GetStrm(), " "));
        }
#endif
        // Potential optimization: record target_element_address in the chunk
        return chunk;
    }
    else {
        // Not the last dimension, so we continue to proceed down the Recursion Branch.
        for (unsigned int chunk_index = chunk_start; chunk_index <= chunk_end; chunk_index += thisDim.stride) {
            (*target_element_address)[dim] = (chunk_index + chunk_origin[dim] - thisDim.start) / thisDim.stride;

            // Re-entry here:
            Chunk *needed = find_needed_chunks(dim + 1, target_element_address, /*chunk_element_address,*/ chunk);
            if (needed)
                return needed;
        }
    }

    return 0;   // Should never get here
}

void DmrppArray::insert_chunk(unsigned int dim, vector<unsigned int> *target_element_address,
    vector<unsigned int> *chunk_element_address, Chunk *chunk)
{
    // The size, in elements, of each of the chunk's dimensions.
    vector<unsigned int> chunk_shape = get_chunk_dimension_sizes();

    // The chunk's origin point a.k.a. its "position in array".
    vector<unsigned int> chunk_origin = chunk->get_position_in_array();

    dimension thisDim = this->get_dimension(dim);

    // Do we even want this chunk? FIXME This should be an exception
    if ((unsigned) thisDim.start > (chunk_origin[dim] + chunk_shape[dim])
        || (unsigned) thisDim.stop < chunk_origin[dim]) {
        return; // No. No, we do not. Skip this.
    }

    // What's the first element that we are going to access for this dimension of the chunk?
    unsigned long long chunk_start = get_chunk_start(dim, chunk_origin);

    // Is the next point to be sent in this chunk at all? If no, return. FIXME Exception
    if (chunk_start > chunk_shape[dim]) {
        return;
    }

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

#define STRIDE_OPT 1
#if STRIDE_OPT
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

            memcpy(target_buffer + target_char_start_index, source_buffer + chunk_char_start_index,
                chunk_constrained_inner_dim_bytes);
        }
        else {
#endif
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
#if STRIDE_OPT
    }
#endif

    }
    else {
        // Not the last dimension, so we continue to proceed down the Recursion Branch.
        for (unsigned int chunk_index = chunk_start; chunk_index <= chunk_end; chunk_index += thisDim.stride) {
            (*target_element_address)[dim] = (chunk_index + chunk_origin[dim] - thisDim.start) / thisDim.stride;
            (*chunk_element_address)[dim] = chunk_index;

            // Re-entry here:
            insert_chunk(dim + 1, target_element_address, chunk_element_address, chunk);
        }
    }
}

bool DmrppArray::read_chunks_parallel()
{
    vector<Chunk> &chunk_refs = get_chunk_vec();
    if (chunk_refs.size() == 0)
        throw BESInternalError(string("Expected one or more chunks for variable ") + name(), __FILE__, __LINE__);

    // Find all the chunks to read
    vector<Chunk *>chunks_to_read;

    // Look at all the chunks
    for (vector<Chunk>::iterator c = chunk_refs.begin(), e = chunk_refs.end(); c != e; ++c) {
        Chunk &chunk = *c;

        vector<unsigned int> target_element_address = chunk.get_position_in_array();
        Chunk *needed = find_needed_chunks(0 /* dimension */, &target_element_address, &chunk);
        if (needed)
            chunks_to_read.push_back(needed);
    }

    reserve_value_capacity(get_size(true));

    // Look only at the chunks we need, found above.
    for (vector<Chunk *>::iterator i = chunks_to_read.begin(), e = chunks_to_read.end(); i != e; ++i) {
        Chunk *chunk = *i;

        chunk->read_serial(is_deflate_compression(), is_shuffle_compression(), get_chunk_size_in_elements(),
            var()->width());

        vector<unsigned int> target_element_address = chunk->get_position_in_array();
        vector<unsigned int> chunk_source_address(dimensions(), 0);
        insert_chunk(0 /* dimension */, &target_element_address, &chunk_source_address, chunk);
    }

    set_read_p(true);

    return true;
}

/**
 * Reads chunked array data from the relevant sources (as indicated by each
 * Chunk object) for this array.
 */
bool DmrppArray::read()
{
    if (read_p()) return true;

    if (get_chunk_dimension_sizes().empty()) {
        return read_contiguous();    // Throws on various errors
    }
    else {  // Handle the more complex case where the data is chunked.
#if 0
        return read_chunks_serial();
#endif

        return read_chunks_parallel();
    }
}

void DmrppArray::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "DmrppArray::" << __func__ << "(" << (void *) this << ")" << endl;
    BESIndent::Indent();
    DmrppCommon::dump(strm);
    Array::dump(strm);
    strm << BESIndent::LMarg << "value: " << "----" << /*d_buf <<*/endl;
    BESIndent::UnIndent();
}

} // namespace dmrpp


#if 0

// This is the old version, after much hacking, but with the special case for stride == 1
// still in place. jhrg 4/28/18

void DmrppArray::insert_chunk_serial(unsigned int dim, vector<unsigned int> *target_element_address,
    vector<unsigned int> *chunk_element_address, Chunk *chunk)
{
    BESDEBUG("dmrpp", __func__ << " dim: "<< dim << " BEGIN "<< endl);

    // The size, in elements, of each of the chunk's dimensions.
    vector<unsigned int> chunk_shape = get_chunk_dimension_sizes();

    // The chunk's origin point a.k.a. its "position in array".
    vector<unsigned int> chunk_origin = chunk->get_position_in_array();

    dimension thisDim = this->get_dimension(dim);

    // What's the first element that we are going to access for this dimension of the chunk?
    unsigned int first_element_offset = 0;// start with 0

    if ((unsigned) thisDim.start < chunk_origin[dim]) {
        // If the start is behind this chunk, then it's special.
        if (thisDim.stride != 1) {
            // And if the stride isn't 1, we have to figure our where to begin in this chunk.
            first_element_offset = (chunk_origin[dim] - thisDim.start) % thisDim.stride;
            // If it's zero great!
            if (first_element_offset != 0) {
                // otherwise we adjustment to get correct first element.
                first_element_offset = thisDim.stride - first_element_offset;
            }
        }
    }
    else {
        first_element_offset = thisDim.start - chunk_origin[dim];
    }

    // Is the next point to be sent in this chunk at all? If no, return false.
    if (first_element_offset > chunk_shape[dim]) {
        return;
    }

    // The start element in this array
    unsigned long long start_element = chunk_origin[dim] + first_element_offset;

    // Now we figure out the correct last element, based on the subset expression
    unsigned long long end_element = chunk_origin[dim] + chunk_shape[dim] - 1;
    if ((unsigned) thisDim.stop < end_element) {
        end_element = thisDim.stop;
    }

    // Do we even want this chunk?
    if ((unsigned) thisDim.start > (chunk_origin[dim] + chunk_shape[dim])
        || (unsigned) thisDim.stop < chunk_origin[dim]) {
        // No. No, we do not. Skip this.

        return;
    }

    unsigned long long chunk_start = first_element_offset; //start_element - chunk_origin[dim];
    unsigned long long chunk_end = end_element - chunk_origin[dim];
    vector<unsigned int> constrained_array_shape = get_shape(true);

    unsigned int last_dim = chunk_shape.size() - 1;
    if (dim == last_dim) {
        BESDEBUG("dmrpp", __func__ << " dim: "<< dim << " THIS IS THE INNER-MOST DIM. "<< endl);

        // Read and Process chunk
        chunk->read_serial(is_deflate_compression(), is_shuffle_compression(),
            get_chunk_size_in_elements(), var()->width());

        char *source_buffer = chunk->get_rbuf();
        char *target_buffer = get_buf();
        unsigned int elem_width = prototype()->width();

        if (thisDim.stride == 1) {
            // Compute how much we are going to copy
            unsigned long long chunk_constrained_inner_dim_bytes = (end_element - start_element + 1) * elem_width;

            // Compute where we need to put it.
            (*target_element_address)[dim] = (start_element - thisDim.start) / thisDim.stride;
            // Compute where we are going to read it from
            (*chunk_element_address)[dim] = first_element_offset;

            unsigned int target_char_start_index = get_index(*target_element_address, constrained_array_shape) * elem_width;//prototype()->width();
            unsigned int chunk_char_start_index = get_index(*chunk_element_address, chunk_shape) * elem_width;//prototype()->width();

            memcpy(target_buffer + target_char_start_index, source_buffer + chunk_char_start_index,
                chunk_constrained_inner_dim_bytes);
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
    else {
        // Not the last dimension, so we continue to proceed down the Recursion Branch.
        for (unsigned int chunk_index = chunk_start; chunk_index <= chunk_end; chunk_index += thisDim.stride) {
            (*target_element_address)[dim] = (chunk_index + chunk_origin[dim] - thisDim.start) / thisDim.stride;
            (*chunk_element_address)[dim] = chunk_index;

            // Re-entry here:
            insert_chunk_serial(dim + 1, target_element_address, chunk_element_address, chunk);
        }
    }
}
#endif


#if 0
/**
 * Reads a the chunks that make up this array's content and copies just the
 * relevant values into the array's memory buffer.
 *
 * Currently, this will collect a curl_easy handle for each chunk required
 * by the current constraint (might be all of them). The handles are placed in
 * a curl_multi handle. Once collected the curl_multi handle is "run"
 * until everything has been completely retrieved or has erred. With the chunks
 * read and in memory the code then initiates a copy of the results into the
 * array variable's internal buffer.
 */
bool DmrppArray::read_chunks_multi()
{
    BESDEBUG("dmrpp", __FUNCTION__ << " for variable '" << name() << "' - BEGIN" << endl);

    vector<Chunk> &chunk_refs = get_chunk_vec();
    if (chunk_refs.size() == 0)
    throw BESInternalError(string("Expected one or more chunks for variable ") + name(), __FILE__, __LINE__);

    // Allocate target memory.
    // FIXME - I think this needs to be the constrained size! Or no bigger than
    // the sum of the sizes of the chunks when they are uncompressed. jhrg 4/10/18
    reserve_value_capacity(length());
    vector<unsigned int> array_shape = get_shape(false);

    /* get a curl_multi handle */
    CURLM *curl_multi_handle = curl_multi_init();

    /*
     * Find the chunks to be read, make curl_easy handles for them, and
     * stuff them into our curl_multi handle. This is a recursive activity
     * which utilizes the same code that copies the data from the chunk to
     * the variables.
     */
    for (unsigned long i = 0; i < chunk_refs.size(); i++) {
        Chunk &chunk = chunk_refs[i];
        BESDEBUG("dmrpp:2", "BEGIN Processing chunk[" << i << "]: " << chunk.to_string() << endl);

        vector<unsigned int> target_element_address = chunk.get_position_in_array();
        vector<unsigned int> chunk_source_address(dimensions(), 0);

        // Recursive insertion operation.
        bool flag = insert_constrained_chunk(0, &target_element_address, &chunk_source_address, &chunk, curl_multi_handle);

        BESDEBUG("dmrpp:2",
            "DmrppArray::" << __func__ <<"(): END Processing chunk[" << i << "]  "
            "(chunk was " << (chunk.is_started()?"QUEUED":"NOT_QUEUED") <<
            " and " << (chunk.is_read()?"READ":"NOT_READ") << ") flag: "<< flag << endl);
    }

    /*
     * Now that we have all of the curl_easy handles for all the chunks of this array
     * that we need to read in our curl_multi handle
     * we dive into multi_finish() to get all of the chunks read.
     */
    multi_finish(curl_multi_handle, &chunk_refs);

    /*
     * The chunks are all read, so we jump back into the recursive code to copy the
     * correct values out of each chunk and into the array memory.
     */
    for (unsigned long i = 0; i < chunk_refs.size(); i++) {
        Chunk &h4bs = chunk_refs[i];
        BESDEBUG("dmrpp",
            "DmrppArray::" << __func__ <<"(): BEGIN Processing chunk[" << i << "]: " << h4bs.to_string() << endl);
        vector<unsigned int> target_element_address = h4bs.get_position_in_array();
        vector<unsigned int> chunk_source_address(dimensions(), 0);
        // Recursive insertion operation.
        // Note that curl_multi_handle is now null. This 'triggers,' incombination
        // with the logic in Chunk, the 'decompress and insert the data' mode
        // of insert_constrained_chunk(). jhrg 4/10/18
        bool flag = insert_constrained_chunk(0, &target_element_address, &chunk_source_address, &h4bs, 0);
        BESDEBUG("dmrpp",
            "DmrppArray::" << __func__ <<"(): END Processing chunk[" << i << "]  (chunk was " << (h4bs.is_read()?"READ":"SKIPPED") << ") flag: "<< flag << endl);
    }
    //##############################################################################

    set_read_p(true);

    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() for " << name() << " END"<< endl);
    return true;
}

/**
 * This helper method reads completely all of the curl_easy handles in the multi_handle.
 *
 * This means that we are reading some or all of the chunks and the chunk vector is
 * passed in so that the curl_easy handle held in each Chunk that was read can be
 * cleaned up once the request has been completed.
 *
 * Once this method is completed we will be ready to copy all of the data from the
 * chunks to the array memory
 */
void DmrppArray::multi_finish(CURLM *multi_handle, vector<Chunk> *chunk_refs)
{
    BESDEBUG("dmrpp", "DmrppArray::" << __func__ <<"() BEGIN" << endl);

    int still_running;
    int repeats = 0;
    long long lap_counter = 0;  // TODO Remove or ... see below
    CURLMcode mcode;

    do {
        int numfds;

        lap_counter++;        // TODO make this depend on BESDEBG if we really need it
        BESDEBUG("dmrpp", "DmrppArray::" << __func__ <<"() Calling curl_multi_perform()" << endl);
        // Read from one or more handles and get the number 'still running'.
        // This returns when there's currently no more to read
        mcode = curl_multi_perform(multi_handle, &still_running);
        BESDEBUG("dmrpp", "DmrppArray::" << __func__ <<"() Completed curl_multi_perform() mcode: " << mcode << endl);

        if (mcode == CURLM_OK) {
            /* wait for activity, timeout or "nothing" */
            BESDEBUG("dmrpp", "DmrppArray::" << __func__ <<"() Calling curl_multi_wait()" << endl);
            // Block until one or more handles have new data to be read or until a timer expires.
            // The timer is set to 1000 milliseconds. Return the number of handles ready for reading.
            mcode = curl_multi_wait(multi_handle, NULL, 0, 1000, &numfds);
            BESDEBUG("dmrpp", "DmrppArray::" << __func__ <<"() Completed curl_multi_wait() mcode: " << mcode << endl);
        }

        // TODO Can cmode be anything other than CURLM_OK?
        // TODO Move this to an else clause and maybe add a note that the error is handled below
        // TODO Actually, it would be clearer to throw here...
        if (mcode != CURLM_OK) {
            break;
        }

        // TODO I don't get the point of this loop... I see it in the docs, but I don't see why it's needed

        /* 'numfds' being zero means either a timeout or no file descriptors to
         wait for. Try timeout on first occurrence, then assume no file
         descriptors and no file descriptors to wait for means wait for 100
         milliseconds. */

        if (!numfds) {
            repeats++; /* count number of repeated zero numfds */
            if (repeats > 1) {
                /* sleep 100 milliseconds */
                usleep(100 * 1000);   // usleep takes sleep time in us (1 millionth of a second)
            }
        }
        else
        repeats = 0;

    }while (still_running);

    BESDEBUG("dmrpp",
        "DmrppArray::" << __func__ <<"() CURL-MULTI has finished! laps: " << lap_counter << "  still_running: "<< still_running << endl);

    if (mcode == CURLM_OK) {
        CURLMsg *msg; /* for picking up messages with the transfer status */
        int msgs_left; /* how many messages are left */

        /* See how the transfers went */
        while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
            int found = 0;
            string h4bs_str = "No Chunk Found For Handle!";
            /* Find out which handle this message is about */
            for (unsigned int idx = 0; idx < chunk_refs->size(); idx++) {
                Chunk *this_h4bs = &(*chunk_refs)[idx];

                CURL *curl_handle = this_h4bs->get_curl_handle();
                found = (msg->easy_handle == curl_handle);
                if (found) {
                    //this_h4bs->set_is_read(true);
                    h4bs_str = this_h4bs->to_string();
                    break;
                }
            }

            if (msg->msg == CURLMSG_DONE) {
                BESDEBUG("dmrpp",
                    "DmrppArray::" << __func__ <<"() Chunk Read Completed For Chunk: " << h4bs_str << endl);
            }
            else {
                ostringstream oss;
                oss << "DmrppArray::" << __func__ << "() Chunk Read Did Not Complete. CURLMsg.msg: " << msg->msg
                << " Chunk: " << h4bs_str;
                BESDEBUG("dmrpp", oss.str() << endl);
                throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
            }
        }

    }

    /* Free the CURL handles */
    for (unsigned int idx = 0; idx < chunk_refs->size(); idx++) {
        CURL *easy_handle = (*chunk_refs)[idx].get_curl_handle();
        curl_multi_remove_handle(multi_handle, easy_handle);
        (*chunk_refs)[idx].cleanup_curl_handle();
    }

    curl_multi_cleanup(multi_handle);

    if (mcode != CURLM_OK) {
        ostringstream oss;
        oss << "DmrppArray: CURL operation Failed!. multi_code: " << mcode << endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }

    BESDEBUG("dmrpp", "DmrppArray::" << __func__ <<"() END" << endl);
}

/**
 * @brief This recursive call inserts a (previously read) chunk's data into the
 * appropriate parts of the Array object's internal memory.
 *
 * Successive calls climb into the array to the insertion point for the current
 * chunk's innermost row. Once located, this row is copied into the array at the
 * insertion point. The next row for insertion is located by returning from the
 * insertion call to the next dimension iteration in the call recursive call
 * stack.
 *
 * This starts with dimension 0 and the chunk_row_insertion_point_address set
 * to the chunks origin point
 *
 * @param dim is the dimension on which we are working. We recurse from
 * dimension 0 to the last dimension
 * @param target_element_address - This vector is used to hold the element
 * address in the result array to where this chunk's data will be written.
 * @param chunk_source_address - This vector is used to hold the chunk
 * element address from where data will be read. The values of this are relative to
 * the chunk's origin (position in array).
 * @param chunk The Chunk containing the read data values to insert.
 * @return
 */
bool DmrppArray::insert_constrained_chunk(unsigned int dim, vector<unsigned int> *target_element_address,
    vector<unsigned int> *chunk_source_address, Chunk *chunk, CURLM *multi_handle)
{

    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - dim: "<< dim << " BEGIN "<< endl);

    // The size, in elements, of each of the chunk's dimensions.
    // TODO We assume all chunks have the same size for any given array.
    vector<unsigned int> chunk_shape = get_chunk_dimension_sizes();

    // The array index of the last dimension
    unsigned int last_dim = chunk_shape.size() - 1;

    // The chunk's origin point a.k.a. its "position in array".
    vector<unsigned int> chunk_origin = chunk->get_position_in_array();

    dimension thisDim = this->get_dimension(dim);

    // What's the first element that we are going to access for this dimension of the chunk?
    unsigned int first_element_offset = 0;// start with 0
    if ((unsigned) thisDim.start < chunk_origin[dim]) {
        // If the start is behind this chunk, then it's special.
        if (thisDim.stride != 1) {
            // And if the stride isn't 1, we have to figure our where to begin in this chunk.
            first_element_offset = (chunk_origin[dim] - thisDim.start) % thisDim.stride;
            // If it's zero great!
            if (first_element_offset != 0) {
                // otherwise we adjustment to get correct first element.
                first_element_offset = thisDim.stride - first_element_offset;
            }
        }
    }
    else {
        first_element_offset = thisDim.start - chunk_origin[dim];
    }

    // Is the next point to be sent in this chunk at all? If no, return false.
    if (first_element_offset > chunk_shape[dim]) {
        return false;
    }

    unsigned long long start_element = chunk_origin[dim] + first_element_offset;

    // Now we figure out the correct last element, based on the subset expression
    unsigned long long end_element = chunk_origin[dim] + chunk_shape[dim] - 1;
    if ((unsigned) thisDim.stop < end_element) {
        end_element = thisDim.stop;
    }

    // Do we even want this chunk?
    if ((unsigned) thisDim.start > (chunk_origin[dim] + chunk_shape[dim])
        || (unsigned) thisDim.stop < chunk_origin[dim]) {
        // No. No, we do not. Skip this.

        return false;
    }

    unsigned long long chunk_start = start_element - chunk_origin[dim];
    unsigned long long chunk_end = end_element - chunk_origin[dim];

    if (dim == last_dim) {
        BESDEBUG("dmrpp", " __func__  dim: "<< dim << " THIS IS THE INNER-MOST DIM. "<< endl);

        if (multi_handle) {
            BESDEBUG("dmrpp", "Queuing chunk for retrieval: " << chunk->to_string() << endl);

            chunk->add_to_multi_read_queue(multi_handle);
            return true;
        }
        else {
            BESDEBUG("dmrpp", "DmrppArray::"<< __func__ <<"() - Reading " << chunk->to_string() << endl);

            // Read and Process chunk

            // Trick: if multi_handle is false but d_is_in_multi_queue is true, then this chunk
            // has been read. The chunk->read() call looks for that and skips calling
            // curl_read_byte_stream() from DmrppUtil.cc. It _will_, however, decompress
            // the chunk, putting the dat in the rbuf, clearing d_is_in_multi_queue and
            // setting d_is_read.
            //
            // That is why the 'multi' read() method for this class (read_chunks()) calls
            // this method twice - once to queue the reads and once to decompress and 'insert'
            // the data from the chunks into the array's data buffer.
            // jhrg 4/10/18

            chunk->read(is_deflate_compression(), is_shuffle_compression(), get_chunk_size_in_elements(), var()->width());
            char * source_buffer = chunk->get_rbuf();

            if (thisDim.stride == 1) {
                //#############################################################################
                // ND - inner_stride == 1

                // Compute how much we are going to copy
                unsigned long long chunk_constrained_inner_dim_elements = end_element - start_element + 1;

                unsigned long long chunk_constrained_inner_dim_bytes = chunk_constrained_inner_dim_elements
                * prototype()->width();

                // Compute where we need to put it.
                (*target_element_address)[dim] = (start_element - thisDim.start) / thisDim.stride;

                unsigned int target_start_element_index = get_index(*target_element_address, get_shape(true));

                unsigned int target_char_start_index = target_start_element_index * prototype()->width();

                // Compute where we are going to read it from
                (*chunk_source_address)[dim] = first_element_offset;

                unsigned int chunk_start_element_index = get_index(*chunk_source_address, chunk_shape);

                unsigned int chunk_char_start_index = chunk_start_element_index * prototype()->width();

                char *target_buffer = get_buf();

                // Copy the bytes
                memcpy(target_buffer + target_char_start_index, source_buffer + chunk_char_start_index,
                    chunk_constrained_inner_dim_bytes);
            }
            else {
                //#############################################################################
                // inner_stride != 1
                unsigned long long chunk_start = start_element - chunk_origin[dim];

                unsigned long long chunk_end = end_element - chunk_origin[dim];

                for (unsigned int chunk_index = chunk_start; chunk_index <= chunk_end; chunk_index += thisDim.stride) {
                    // Compute where we need to put it.
                    (*target_element_address)[dim] = (chunk_index + chunk_origin[dim] - thisDim.start) / thisDim.stride;

                    unsigned int target_start_element_index = get_index(*target_element_address, get_shape(true));

                    unsigned int target_char_start_index = target_start_element_index * prototype()->width();

                    // Compute where we are going to read it from
                    (*chunk_source_address)[dim] = chunk_index;

                    unsigned int chunk_start_element_index = get_index(*chunk_source_address, chunk_shape);

                    unsigned int chunk_char_start_index = chunk_start_element_index * prototype()->width();

                    char *target_buffer = get_buf();

                    // Copy the bytes
                    memcpy(target_buffer + target_char_start_index, source_buffer + chunk_char_start_index,
                        prototype()->width());
                }
            }
        }
    }
    else {
        // Not the last dimension, so we continue to proceed down the Recursion Branch.
        for (unsigned int dim_index = chunk_start; dim_index <= chunk_end; dim_index += thisDim.stride) {
            (*target_element_address)[dim] = (chunk_origin[dim] + dim_index - thisDim.start) / thisDim.stride;
            (*chunk_source_address)[dim] = dim_index;

            // Re-entry here:
            bool flag = insert_constrained_chunk(dim + 1, target_element_address, chunk_source_address, chunk, multi_handle);
            if (flag)
            return true;
        }
    }
    return false;
}
#endif


