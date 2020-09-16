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
#include <vector>
#include <queue>
#include <iterator>

#include <cstring>
#include <cassert>
#include <cerrno>

#include <pthread.h>
#include <cmath>

#include <unistd.h>

#include <D4Enum.h>
#include <D4EnumDefs.h>
#include <D4Attributes.h>
#include <D4Maps.h>
#include <D4Group.h>

#include "BESLog.h"
#include "BESInternalError.h"
#include "BESDebug.h"

#include "CurlHandlePool.h"
#include "Chunk.h"
#include "DmrppArray.h"
#include "DmrppRequestHandler.h"
#include "Base64.h"

// Used with BESDEBUG
static const string dmrpp_3 = "dmrpp:3";
static const string dmrpp_4 = "dmrpp:4";

using namespace libdap;
using namespace std;

#define MB (1024*1024)

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
    DmrppCommon::m_duplicate_common(rhs);

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
 * @note When getting the whole AIRS file, the profiler shows that the code spends
 * about 1s here. There is a better performing replacement for this function. See
 * multiplier(const vector<unsigned int> &shape, unsigned int k) below in
 * read_chunk_unconstrained() and elsewhere in this file.
 *
 * @param address_in_target N-tuple zero-based index of an element in N-space
 * @param target_shape N-tuple of the array's dimension sizes.
 * @return The offset into the vector used to store the values.
 */
static unsigned long long get_index(const vector<unsigned int> &address_in_target, const vector<unsigned int> &target_shape)
{
    assert(address_in_target.size() == target_shape.size());    // ranks must be equal

    vector<unsigned int>::const_reverse_iterator shape_index = target_shape.rbegin();
    vector<unsigned int>::const_reverse_iterator index = address_in_target.rbegin(), index_end = address_in_target.rend();

    unsigned long long multiplier = *shape_index++;
    unsigned long long offset = *index++;

    while (index != index_end) {
        assert(*index < *shape_index); // index < shape for each dim

        offset += multiplier * *index++;
        multiplier *= *shape_index++;
    }

    return offset;
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
    Dim_iter dim = dim_begin(), edim = dim_end();
    vector<unsigned int> shape;

    // For a 3d array, this method took 14ms without reserve(), 5ms with
    // (when called many times).
    shape.reserve(edim - dim);

    for (; dim != edim; dim++) {
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

/**
 * @brief This recursive private method collects values from the rbuf and copies
 * them into buf. It supports stop, stride, and start and while correct is not
 * efficient.
 */
void DmrppArray::insert_constrained_contiguous(Dim_iter dimIter, unsigned long *target_index, vector<unsigned int> &subsetAddress,
    const vector<unsigned int> &array_shape, char /*Chunk*/*src_buf)
{
    BESDEBUG("dmrpp", "DmrppArray::"<< __func__ << "() - subsetAddress.size(): " << subsetAddress.size() << endl);

    unsigned int bytesPerElt = prototype()->width();

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
        // TODO Replace this loop with a call to std::memcpy()
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
                insert_constrained_contiguous(dimIter, target_index, subsetAddress, array_shape, src_buf);
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
 * @brief Thread to insert data from one chunk
 *
 * @param arg_list
 */
void *one_chunk_unconstrained_thread(void *arg_list)
{
    one_chunk_unconstrained_args *args = reinterpret_cast<one_chunk_unconstrained_args*>(arg_list);

    try {
        process_one_chunk_unconstrained(args->chunk, args->array, args->array_shape, args->chunk_shape);
    }
    catch (BESError &error) {
        write(args->fds[1], &args->tid, sizeof(args->tid));
        delete args;
        pthread_exit(new string(error.get_verbose_message()));
    }

    // tid is a char and thus us written atomically. Writing this tells the parent
    // thread the child is complete and it should call pthread_join(tid, ...)
    write(args->fds[1], &args->tid, sizeof(args->tid));

    delete args;
    pthread_exit(NULL);
}

/**
 * @brief Manage parallel transfer for contiguous data
 *
 * Read data for one of the 'child chunks' made to read data for a variable
 * with contiguous storage in parallel.
 *
 * @param arg_list
 */
void *one_child_chunk_thread(void *arg_list)
{
    one_child_chunk_args *args = reinterpret_cast<one_child_chunk_args*>(arg_list);

    try {
        args->child_chunk->read_chunk();

        assert(args->master_chunk->get_rbuf());
        assert(args->child_chunk->get_rbuf());
        assert(args->child_chunk->get_bytes_read() == args->child_chunk->get_size());

        // master offset \/
        // master chunk:  mmmmmmmmmmmmmmmm
        // child chunks:  1111222233334444 (there are four child chunks)
        // child offsets: ^   ^   ^   ^
        // For this example, child_1_offset - master_offset == 0 (that's always true)
        // child_2_offset - master_offset == 4; child_2_offset - master_offset == 8
        // and child_3_offset - master_offset == 12.
        // Those are the starting locations with in the data buffer of the master chunk
        // where that child chunk should be written.
        // Note: all of the offset values start at the begining of the file.

        unsigned int offset_within_master_chunk = args->child_chunk->get_offset() - args->master_chunk->get_offset();

        memcpy(args->master_chunk->get_rbuf() + offset_within_master_chunk, args->child_chunk->get_rbuf(), args->child_chunk->get_bytes_read());
    }
    catch (BESError &error) {
        write(args->fds[1], &args->tid, sizeof(args->tid));

        delete args;
        pthread_exit(new string(error.get_verbose_message()));
    }

    // tid is a char and thus us written atomically. Writing this tells the parent
    // thread the child is complete and it should call pthread_join(tid, ...)
    write(args->fds[1], &args->tid, sizeof(args->tid));

    delete args;
    pthread_exit(NULL);
}

/**
 * @brief Read an array that is stored using one 'chunk.'
 *
 * @todo This code should be tested to make sure that an access that requires
 * authentication which then fails is properly handled. All the threads will
 * need to be stopped. Also, an auth that succeeds _may_ need to be restarted.
 *
 * @return Always returns true, matching the libdap::Array::read() behavior.
 */
void DmrppArray::read_contiguous()
{
    // These first four lines reproduce DmrppCommon::read_atomic(). The call
    // to Chunk::inflate_chunk() handles 'contiguous' data that are compressed.
    // And since we need the chunk, I copied the read_atomix code here.

    vector<Chunk> &chunk_refs = get_chunk_vec();

    if (chunk_refs.size() != 1) throw BESInternalError(string("Expected only a single chunk for variable ") + name(), __FILE__, __LINE__);

    // This is the original chunk for this 'contiguous' variable.
    Chunk &master_chunk = chunk_refs[0];

    unsigned long long master_chunk_size = master_chunk.get_size();

    // If we want to read the chunk in parallel. Only read in parallel above some
    // threshold. jhrg 9/21/19
    // Only use parallel read if the chunk is over 2MB, otherwise it is easier to just read it as is kln 9/23/19
    if (DmrppRequestHandler::d_use_parallel_transfers && master_chunk_size > DmrppRequestHandler::d_min_size) {

        // Allocated memory for the 'master chunk' so the threads can transfer data
        // from the child chunks to it.
        master_chunk.set_rbuf_to_size();

		// The number of child chunks are determined based on the size of the data.
        // If the size of the master chunk is 3MB then 3 chunks will be made. We will round down
        //  when necessary and handle the remainder later on (3.3MB = 3 chunks, 4.2MB = 4 chunks, etc.) kln 9/23/19
        unsigned int num_chunks = floor(master_chunk_size/MB);
        if ( num_chunks >= DmrppRequestHandler::d_max_parallel_transfers)
        	num_chunks = DmrppRequestHandler::d_max_parallel_transfers;

		// This pipe is used by the child threads to indicate completion
		int fds[2];
		int status = pipe(fds);
		if (status < 0)
			throw BESInternalError(string("Could not open a pipe for thread communication: ").append(strerror(errno)), __FILE__, __LINE__);

		// Use the original chunk's size and offset to evenly split it into smaller chunks
		unsigned long long chunk_size = master_chunk_size / num_chunks;
		unsigned long long chunk_offset = master_chunk.get_offset();

		// If the size of the master chunk is not evenly divisible by num_chunks, capture
		// the remainder here and increase the size of the last chunk by this number of bytes.
		unsigned int chunk_remainder = master_chunk.get_size() % num_chunks;

		string chunk_url = master_chunk.get_data_url();

		// Setup a queue to break up the original master_chunk and keep track of the pieces
		queue<Chunk *> chunks_to_read;

		for (unsigned int i = 0; i < num_chunks-1; i++) {
			chunks_to_read.push(new Chunk(chunk_url, chunk_size, (chunk_size * i) + chunk_offset));
		}
		// See above for details about chunk_remainder. jhrg 9/21/19
		chunks_to_read.push(new Chunk(chunk_url, chunk_size + chunk_remainder, (chunk_size * (num_chunks-1)) + chunk_offset));

		// Start the max number of processing pipelines
		pthread_t threads[DmrppRequestHandler::d_max_parallel_transfers];
		memset(&threads[0], 0, sizeof(pthread_t) * DmrppRequestHandler::d_max_parallel_transfers);

		try {
			unsigned int num_threads = 0;

			// start initial set of threads
			for (unsigned int i = 0; i < (unsigned int) DmrppRequestHandler::d_max_parallel_transfers && chunks_to_read.size() > 0; ++i) {
				Chunk *current_chunk = chunks_to_read.front();
				chunks_to_read.pop();

				// thread number is 'i'
				one_child_chunk_args *args = new one_child_chunk_args(fds, i, current_chunk, &master_chunk);
				int status = pthread_create(&threads[i], NULL, dmrpp::one_child_chunk_thread, (void*) args);

				if (status == 0) {
					++num_threads;
					BESDEBUG(dmrpp_3, "started thread: " << i << endl);
				}
				else {
					ostringstream oss("Could not start process_one_chunk_unconstrained thread for master_chunk ", std::ios::ate);
					oss << i << ": " << strerror(status);
					BESDEBUG(dmrpp_3, oss.str());
					throw BESInternalError(oss.str(), __FILE__, __LINE__);
				}
			}

			// Now join the child threads, creating replacement threads if needed
			while (num_threads > 0) {
				unsigned char tid;   // bytes can be written atomically
				// Block here until a child thread writes to the pipe, then read the byte
				int bytes = ::read(fds[0], &tid, sizeof(tid));
				if (bytes != sizeof(tid))
					throw BESInternalError(string("Could not read the thread id: ").append(strerror(errno)), __FILE__, __LINE__);

				if (!(tid < DmrppRequestHandler::d_max_parallel_transfers)) {
					ostringstream oss("Invalid thread id read after thread exit: ", std::ios::ate);
					oss << tid;
					throw BESInternalError(oss.str(), __FILE__, __LINE__);
				}

				string *error;
				int status = pthread_join(threads[tid], (void**)&error);
				--num_threads;
				BESDEBUG(dmrpp_3, "joined thread: " << (unsigned int)tid << ", there are: " << num_threads << endl);

				if (status != 0) {
					ostringstream oss("Could not join process_one_chunk_unconstrained thread for master_chunk ", std::ios::ate);
					oss << tid << ": " << strerror(status);
					throw BESInternalError(oss.str(), __FILE__, __LINE__);
				}
				else if (error != 0) {
					BESInternalError e(*error, __FILE__, __LINE__);
					delete error;
					throw e;
				}
				else if (chunks_to_read.size() > 0) {
					Chunk *current_chunk = chunks_to_read.front();
					chunks_to_read.pop();

					// thread number is 'tid,' the number of the thread that just completed
					one_child_chunk_args *args = new one_child_chunk_args(fds, tid, current_chunk, &master_chunk);
					int status = pthread_create(&threads[tid], NULL, dmrpp::one_child_chunk_thread, (void*) args);

					if (status != 0) {
						ostringstream oss;
						oss << "Could not start process_one_chunk_unconstrained thread for master_chunk " << tid << ": " << strerror(status);
						throw BESInternalError(oss.str(), __FILE__, __LINE__);
					}
					++num_threads;
					BESDEBUG(dmrpp_3, "started thread: " << (unsigned int)tid << ", there are: " << threads << endl);
				}
			}

			// Once done with the threads, close the communication pipe.
			close(fds[0]);
			close(fds[1]);
		}
		catch (...) {
			// cancel all the threads, otherwise we'll have threads out there using up resources
			// defined in DmrppCommon.cc
			join_threads(threads, DmrppRequestHandler::d_max_parallel_transfers);
			// close the pipe used to communicate with the child threads
			close(fds[0]);
			close(fds[1]);
			// re-throw the exception
			throw;
		}
    }
    else {
        // Else read the master_chunk as is
		master_chunk.read_chunk();
    }

    master_chunk.inflate_chunk(is_deflate_compression(), is_shuffle_compression(), get_chunk_size_in_elements(), var()->width());

    // 'master_chunk' now holds the data. Transfer it to the Array.

    if (!is_projected()) {  // if there is no projection constraint
        val2buf(master_chunk.get_rbuf());      // yes, it's not type-safe
    }
    else {                  // apply the constraint
        vector<unsigned int> array_shape = get_shape(false);

        // Reserve space in this array for the constrained size of the data request
        reserve_value_capacity(get_size(true));
        unsigned long target_index = 0;
        vector<unsigned int> subset;

        insert_constrained_contiguous(dim_begin(), &target_index, subset, array_shape, master_chunk.get_rbuf());
    }

    set_read_p(true);
}

/**
 * @brief What is the first element to use from a chunk
 *
 * For a chunk that fits in the array at \arg chunk_origin, what is the first element
 * of that chunk that will be transferred to the array? It may be that the first element
 * is actually not part of the chunk (given the array, its constraint, and the
 * \arg chunk_origin), and that indicates this chunk will not be used at all.
 *
 * @param thisDim Look at this dimension of the chunk and array
 * @param chunk_origin The chunk's position in the array for this given dimension
 * @return The first _element_ of the chunk to transfer.
 */
unsigned long long DmrppArray::get_chunk_start(const dimension &thisDim, unsigned int chunk_origin)
{
    // What's the first element that we are going to access for this dimension of the chunk?
    unsigned long long first_element_offset = 0; // start with 0
    if ((unsigned) (thisDim.start) < chunk_origin) {
        // If the start is behind this chunk, then it's special.
        if (thisDim.stride != 1) {
            // And if the stride isn't 1, we have to figure our where to begin in this chunk.
            first_element_offset = (chunk_origin - thisDim.start) % thisDim.stride;
            // If it's zero great!
            if (first_element_offset != 0) {
                // otherwise we adjustment to get correct first element.
                first_element_offset = thisDim.stride - first_element_offset;
            }
        }
    }
    else {
        first_element_offset = thisDim.start - chunk_origin;
    }

    return first_element_offset;
}

#ifdef USE_READ_SERIAL
/**
 * Insert data from \arg chunk into the array given the current constraint
 *
 * Recursive calls build up the two vectors \arg target_element_address and
 * \arg chunk_element_address. These vectors start out with \arg dim elements,
 * the \arg chunk_element_address holds 0, 0, ..., 0 and the \arg target_element_address
 * holds the index of the first value of this chunk in the target array
 *
 * @note This method will be called several time for any given chunk, so the
 * chunk_read() and chunk_inflate() methods 'protect' the chunk against being read
 * or decompressed more than once. For reading this is not a fatal error (but a waste
 * of time), but it is a fatal error decompression. The code in read_chunk_parallel()
 * does not have this problem (but it uses the same read and inflate code and thus
 * I've left in the tracking booleans.
 *
 * @param dim
 * @param target_element_address
 * @param chunk_element_address
 * @param chunk
 * @return
 */
void DmrppArray::insert_chunk_serial(unsigned int dim, vector<unsigned int> *target_element_address, vector<unsigned int> *chunk_element_address,
    Chunk *chunk)
{
    BESDEBUG("dmrpp", __func__ << " dim: "<< dim << " BEGIN "<< endl);

    // The size, in elements, of each of the chunk's dimensions.
    const vector<unsigned int> &chunk_shape = get_chunk_dimension_sizes();

    // The chunk's origin point a.k.a. its "position in array".
    const vector<unsigned int> &chunk_origin = chunk->get_position_in_array();

    dimension thisDim = this->get_dimension(dim);

    // Do we even want this chunk?
    if ((unsigned) thisDim.start > (chunk_origin[dim] + chunk_shape[dim]) || (unsigned) thisDim.stop < chunk_origin[dim]) {
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
        // Read and Process chunk
        chunk->read_chunk();

        chunk->inflate_chunk(is_deflate_compression(), is_shuffle_compression(), get_chunk_size_in_elements(), var()->width());

        char *source_buffer = chunk->get_rbuf();
        char *target_buffer = get_buf();
        unsigned int elem_width = prototype()->width();

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

void DmrppArray::read_chunks_serial()
{
    BESDEBUG("dmrpp", __func__ << " for variable '" << name() << "' - BEGIN" << endl);

    vector<Chunk> &chunk_refs = get_chunk_vec();
    if (chunk_refs.size() == 0) throw BESInternalError(string("Expected one or more chunks for variable ") + name(), __FILE__, __LINE__);

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
}
#endif

/**
 * @brief Look at all the chunks and mark those that should be read.
 *
 * This method is used by read_chunks_parallel() to determine which
 * of the chunks that make up this array should be read, decompressed
 * and inserted into the array. The assumption is that the array is
 * subset in some way, so not all of the chunks need to be read.
 *
 * This method works in elements, not bytes.
 *
 * This method calls itself, completing to computation when \arg dim
 * has gone from 0 to the rank (rank-1, actually) of the array. As it does this, the
 * vector `target_element_address` is built up for the given chunk.
 * When \arg dim is the array's rank, `target_element_address` will
 * have a value for all but the rightmost dimension.
 *
 * @param dim Starting with 0, compute values for this dimension of the array
 * @param target_element_address Initially empty, this becomes the location
 * in the array where data should be written.
 * @param chunk This is the chunk.
 */
Chunk *
DmrppArray::find_needed_chunks(unsigned int dim, vector<unsigned int> *target_element_address, Chunk *chunk)
{
    BESDEBUG(dmrpp_3, __func__ << " BEGIN, dim: " << dim << endl);

    // The size, in elements, of each of the chunk's dimensions.
    const vector<unsigned int> &chunk_shape = get_chunk_dimension_sizes();

    // The chunk's origin point a.k.a. its "position in array".
    const vector<unsigned int> &chunk_origin = chunk->get_position_in_array();

    dimension thisDim = this->get_dimension(dim);

    // Do we even want this chunk?
    if ((unsigned) thisDim.start > (chunk_origin[dim] + chunk_shape[dim]) || (unsigned) thisDim.stop < chunk_origin[dim]) {
        return 0; // No. No, we do not. Skip this chunk.
    }

    // What's the first element that we are going to access for this dimension of the chunk?
    unsigned long long chunk_start = get_chunk_start(thisDim, chunk_origin[dim]);

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
        return chunk;
    }
    else {
        // Not the last dimension, so we continue to proceed down the Recursion Branch.
        for (unsigned int chunk_index = chunk_start; chunk_index <= chunk_end; chunk_index += thisDim.stride) {
            (*target_element_address)[dim] = (chunk_index + chunk_origin[dim] - thisDim.start) / thisDim.stride;

            // Re-entry here:
            Chunk *needed = find_needed_chunks(dim + 1, target_element_address, chunk);
            if (needed) return needed;
        }
    }

    return 0;
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
void DmrppArray::insert_chunk(unsigned int dim, vector<unsigned int> *target_element_address, vector<unsigned int> *chunk_element_address,
    Chunk *chunk, const vector<unsigned int> &constrained_array_shape)
{
    // The size, in elements, of each of the chunk's dimensions.
    const vector<unsigned int> &chunk_shape = get_chunk_dimension_sizes();

    // The chunk's origin point a.k.a. its "position in array".
    const vector<unsigned int> &chunk_origin = chunk->get_position_in_array();

    dimension thisDim = this->get_dimension(dim);

    // What's the first element that we are going to access for this dimension of the chunk?
    unsigned long long chunk_start = get_chunk_start(thisDim, chunk_origin[dim]);

    // Now we figure out the correct last element, based on the subset expression
    unsigned long long end_element = chunk_origin[dim] + chunk_shape[dim] - 1;
    if ((unsigned) thisDim.stop < end_element) {
        end_element = thisDim.stop;
    }

    unsigned long long chunk_end = end_element - chunk_origin[dim];

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
            (*target_element_address)[dim] = (start_element - thisDim.start); // / thisDim.stride;
            // Compute where we are going to read it from
            (*chunk_element_address)[dim] = chunk_start;

            // See below re get_index()
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

                // These calls to get_index() can be removed as with the insert...unconstrained() code.
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
            insert_chunk(dim + 1, target_element_address, chunk_element_address, chunk, constrained_array_shape);
        }
    }
}

/**
 * @brief Read chunked data
 *
 * @todo This code could be made faster if it moved handling the multiple curl
 * easy_handles here (either using pthreads or the multi api or multi sockets
 * api). This would enable reading and inserting chunks in parallel. Right now
 * we just read in parallel, then insert, then read, ...
 *
 * Read chunked data, using either parallel or serial data transfers, depending on
 * the DMR++ handler configuration parameters.
 */
void DmrppArray::read_chunks()
{
    vector<Chunk> &chunk_refs = get_chunk_vec();
    if (chunk_refs.size() == 0) throw BESInternalError(string("Expected one or more chunks for variable ") + name(), __FILE__, __LINE__);

    // Find all the chunks to read. I used a queue to preserve the chunk order, which
    // made using a debugger easier. However, order does not matter, AFAIK.
    queue<Chunk *> chunks_to_read;

    // Look at all the chunks
    for (vector<Chunk>::iterator c = chunk_refs.begin(), e = chunk_refs.end(); c != e; ++c) {
        Chunk &chunk = *c;

        vector<unsigned int> target_element_address = chunk.get_position_in_array();
        Chunk *needed = find_needed_chunks(0 /* dimension */, &target_element_address, &chunk);
        if (needed) chunks_to_read.push(needed);
    }

    reserve_value_capacity(get_size(true));
    vector<unsigned int> constrained_array_shape = get_shape(true);

    BESDEBUG(dmrpp_3, "d_use_parallel_transfers: " << DmrppRequestHandler::d_use_parallel_transfers << endl);
    BESDEBUG(dmrpp_3, "d_max_parallel_transfers: " << DmrppRequestHandler::d_max_parallel_transfers << endl);

    // TODO Move this to the handler startup? This is a CentOS6 issue, it goes away with C7, Ubuntu, ...
#if !HAVE_CURL_MULTI_API
    if (DmrppRequestHandler::d_use_parallel_transfers)
        LOG("The DMR++ handler is configured to use parallel transfers, but the libcurl Multi API is not present, defaulting to serial transfers");
#endif

    if (DmrppRequestHandler::d_use_parallel_transfers && have_curl_multi_api) {
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
                if (!handle) {
                    throw BESInternalError("No more libcurl handles.", __FILE__, __LINE__);
                }

                BESDEBUG(dmrpp_3, "Queuing: " << chunk->to_string() << endl);
                mhandle->add_easy_handle(handle);

                chunks_to_insert.push(chunk);
            }

            mhandle->read_data(); // read, then remove the easy_handles

            while (chunks_to_insert.size() > 0) {
                Chunk *chunk = chunks_to_insert.front();
                chunks_to_insert.pop();

                chunk->inflate_chunk(is_deflate_compression(), is_shuffle_compression(), get_chunk_size_in_elements(), var()->width());

                vector<unsigned int> target_element_address = chunk->get_position_in_array();
                vector<unsigned int> chunk_source_address(dimensions(), 0);

                BESDEBUG(dmrpp_3, "Inserting: " << chunk->to_string() << endl);
                insert_chunk(0 /* dimension */, &target_element_address, &chunk_source_address, chunk, constrained_array_shape);
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

            chunk->inflate_chunk(is_deflate_compression(), is_shuffle_compression(), get_chunk_size_in_elements(), var()->width());

            vector<unsigned int> target_element_address = chunk->get_position_in_array();
            vector<unsigned int> chunk_source_address(dimensions(), 0);

            BESDEBUG(dmrpp_3, "Inserting: " << chunk->to_string() << endl);
            insert_chunk(0 /* dimension */, &target_element_address, &chunk_source_address, chunk, constrained_array_shape);
        }
    }

    set_read_p(true);
}

/**
 * For dimension \arg k, compute the multiplier needed for the row-major array
 * offset formula. The formula is:
 *
 * Given an Array A has dimension sizes N0, N1, N2, ..., Nd-1
 *
 * for k = 0 to d-1 sum ( for l = k+1 to d-1 product ( Nl ) nk )
 *                        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *                                    multiplier
 *
 * @param shape The sizes of the dimensions of the array
 * @param k The dimension in question
 */
static unsigned long multiplier(const vector<unsigned int> &shape, unsigned int k)
{
    assert(shape.size() > 1);
    assert(shape.size() > k + 1);

    vector<unsigned int>::const_iterator i = shape.begin(), e = shape.end();
    advance(i, k + 1);
    unsigned long multiplier = *i++;
    while (i != e) {
        multiplier *= *i++;
    }

    return multiplier;
}

/**
 * @brief Insert a chunk into an unconstrained Array
 *
 * This code is called recursively, until the \arg dim is rank-1 for this
 * Array.
 *
 * @note dimensions 0..k are d0, d1, d2, ..., dk and dk, the rightmost
 * dimension, varies the fastest (row-major order). To compute the offset
 * for coordinate c0, c1, c2, ..., ck. e.g., c0(d1 * d2) + c1(d2) + c2 when
 * k == 2
 *
 * @param chunk The chunk that holds data to insert
 * @param dim The current dimension
 * @param array_offset Insert values at this point in the Array
 * @param array_shape The size of the Array's dimensions
 * @param chunk_offset Insert data from this point in the chunk
 * @param chunk_shape The size of the chunk's dimensions
 * @param chunk_origin Where this chunk fits into the Array
 */
void DmrppArray::insert_chunk_unconstrained(Chunk *chunk, unsigned int dim, unsigned long long array_offset, const vector<unsigned int> &array_shape,
    unsigned long long chunk_offset, const vector<unsigned int> &chunk_shape, const vector<unsigned int> &chunk_origin)
{
    // Now we figure out the correct last element. It's possible that a
    // chunk 'extends beyond' the Array bounds. Here 'end_element' is the
    // last element of the destination array
    dimension thisDim = this->get_dimension(dim);
    unsigned long long end_element = chunk_origin[dim] + chunk_shape[dim] - 1;
    if ((unsigned) thisDim.stop < end_element) {
        end_element = thisDim.stop;
    }

    unsigned long long chunk_end = end_element - chunk_origin[dim];

    unsigned int last_dim = chunk_shape.size() - 1;
    if (dim == last_dim) {
        unsigned int elem_width = prototype()->width();

        array_offset += chunk_origin[dim];

        // Compute how much we are going to copy
        unsigned long long chunk_bytes = (end_element - chunk_origin[dim] + 1) * elem_width;
        char *source_buffer = chunk->get_rbuf();
        char *target_buffer = get_buf();
        memcpy(target_buffer + (array_offset * elem_width), source_buffer + (chunk_offset * elem_width), chunk_bytes);
    }
    else {
        unsigned long mc = multiplier(chunk_shape, dim);
        unsigned long ma = multiplier(array_shape, dim);

        // Not the last dimension, so we continue to proceed down the Recursion Branch.
        for (unsigned int chunk_index = 0 /*chunk_start*/; chunk_index <= chunk_end; ++chunk_index) {
            unsigned long long next_chunk_offset = chunk_offset + (mc * chunk_index);
            unsigned long long next_array_offset = array_offset + (ma * (chunk_index + chunk_origin[dim]));

            // Re-entry here:
            insert_chunk_unconstrained(chunk, dim + 1, next_array_offset, array_shape, next_chunk_offset, chunk_shape, chunk_origin);
        }
    }
}

/**
 * @brief Friend function, insert data from one chunk in this this array
 *
 * @param chunk
 * @param array
 * @param array_shape
 * @param chunk_shape
 */
void process_one_chunk_unconstrained(Chunk *chunk, DmrppArray *array, const vector<unsigned int> &array_shape,
    const vector<unsigned int> &chunk_shape)
{
    chunk->read_chunk();

    if (array->is_deflate_compression() || array->is_shuffle_compression())
        chunk->inflate_chunk(array->is_deflate_compression(), array->is_shuffle_compression(), array->get_chunk_size_in_elements(),
            array->var()->width());

    array->insert_chunk_unconstrained(chunk, 0, 0, array_shape, 0, chunk_shape, chunk->get_position_in_array());
}

void DmrppArray::read_chunks_unconstrained()
{
    vector<Chunk> &chunk_refs = get_chunk_vec();
    if (chunk_refs.size() == 0) throw BESInternalError(string("Expected one or more chunks for variable ") + name(), __FILE__, __LINE__);

    reserve_value_capacity(get_size());

    // The size in element of each of the array's dimensions
    const vector<unsigned int> array_shape = get_shape(true);
    // The size, in elements, of each of the chunk's dimensions
    const vector<unsigned int> chunk_shape = get_chunk_dimension_sizes();

    BESDEBUG(dmrpp_3, __func__ << endl);

    BESDEBUG(dmrpp_3, "d_use_parallel_transfers: " << DmrppRequestHandler::d_use_parallel_transfers << endl);
    BESDEBUG(dmrpp_3, "d_max_parallel_transfers: " << DmrppRequestHandler::d_max_parallel_transfers << endl);

    if (DmrppRequestHandler::d_use_parallel_transfers) {
        queue<Chunk *> chunks_to_read;

        // Queue all of the chunks
        for (vector<Chunk>::iterator c = chunk_refs.begin(), e = chunk_refs.end(); c != e; ++c)
            chunks_to_read.push(&(*c));

        // This pipe is used by the child threads to indicate completion
        int fds[2];
        int status = pipe(fds);
        if (status < 0)
            throw BESInternalError(string("Could not open a pipe for thread communication: ").append(strerror(errno)), __FILE__, __LINE__);

        // Start the max number of processing pipelines
        pthread_t threads[DmrppRequestHandler::d_max_parallel_transfers];
        memset(&threads[0], 0, sizeof(pthread_t) * DmrppRequestHandler::d_max_parallel_transfers);

#if 0
        // set the thread[] elements to null - this serves as a sentinel value
        for (unsigned int i = 0; i < (unsigned int)DmrppRequestHandler::d_max_parallel_transfers; ++i) {
            memset(&threads[i], 0, sizeof(pthread_t));
        }
#endif


        try {
            unsigned int num_threads = 0;
            for (unsigned int i = 0; i < (unsigned int) DmrppRequestHandler::d_max_parallel_transfers && chunks_to_read.size() > 0; ++i) {
                Chunk *chunk = chunks_to_read.front();
                chunks_to_read.pop();

                // thread number is 'i'
                one_chunk_unconstrained_args *args = new one_chunk_unconstrained_args(fds, i, chunk, this, array_shape, chunk_shape);
                int status = pthread_create(&threads[i], NULL, dmrpp::one_chunk_unconstrained_thread, (void*) args);
                if (status == 0) {
                    ++num_threads;
                    BESDEBUG(dmrpp_3, "started thread: " << i << endl);
                }
                else {
                    ostringstream oss("Could not start process_one_chunk_unconstrained thread for chunk ", std::ios::ate);
                    oss << i << ": " << strerror(status);
                    BESDEBUG(dmrpp_3, oss.str());
                    throw BESInternalError(oss.str(), __FILE__, __LINE__);
                }
            }

            // Now join the child threads, creating replacement threads if needed
            while (num_threads > 0) {
                unsigned char tid;   // bytes can be written atomically
                // Block here until a child thread writes to the pipe, then read the byte
                int bytes = ::read(fds[0], &tid, sizeof(tid));
                if (bytes != sizeof(tid))
                    throw BESInternalError(string("Could not read the thread id: ").append(strerror(errno)), __FILE__, __LINE__);

                if (!(tid < DmrppRequestHandler::d_max_parallel_transfers)) {
                    ostringstream oss("Invalid thread id read after thread exit: ", std::ios::ate);
                    oss << tid;
                    throw BESInternalError(oss.str(), __FILE__, __LINE__);
                }

                string *error;
                int status = pthread_join(threads[tid], (void**)&error);
                --num_threads;
                BESDEBUG(dmrpp_3, "joined thread: " << (unsigned int)tid << ", there are: " << num_threads << endl);

                if (status != 0) {
                    ostringstream oss("Could not join process_one_chunk_unconstrained thread for chunk ", std::ios::ate);
                    oss << tid << ": " << strerror(status);
                    throw BESInternalError(oss.str(), __FILE__, __LINE__);
                }
                else if (error != 0) {
                    BESInternalError e(*error, __FILE__, __LINE__);
                    delete error;
                    throw e;
                }
                else if (chunks_to_read.size() > 0) {
                    Chunk *chunk = chunks_to_read.front();
                    chunks_to_read.pop();

                    // thread number is 'tid,' the number of the thread that just completed
                    one_chunk_unconstrained_args *args = new one_chunk_unconstrained_args(fds, tid, chunk, this, array_shape, chunk_shape);
                    int status = pthread_create(&threads[tid], NULL, dmrpp::one_chunk_unconstrained_thread, (void*) args);
                    if (status != 0) {
                        ostringstream oss;
                        oss << "Could not start process_one_chunk_unconstrained thread for chunk " << tid << ": " << strerror(status);
                        throw BESInternalError(oss.str(), __FILE__, __LINE__);
                    }
                    ++num_threads;
                    BESDEBUG(dmrpp_3, "started thread: " << (unsigned int)tid << ", there are: " << threads << endl);
                }
            }

            // Once done with the threads, close the communication pipe.
            close(fds[0]);
            close(fds[1]);
        }
        catch (...) {
            // cancel all the threads, otherwise we'll have threads out there using up resources
            // defined in DmrppCommon.cc
            join_threads(threads, DmrppRequestHandler::d_max_parallel_transfers);
            // close the pipe used to communicate with the child threads
            close(fds[0]);
            close(fds[1]);
            // re-throw the exception
            throw;
        }
    }
    else {  // Serial transfers
        for (vector<Chunk>::iterator c = chunk_refs.begin(), e = chunk_refs.end(); c != e; ++c) {
            Chunk &chunk = *c;
            process_one_chunk_unconstrained(&chunk, this, array_shape, chunk_shape);
        }
    }

    set_read_p(true);
}

/**
 * @brief Read data for the array
 *
 * This reads data for a variable and loads it into memory. The software is
 * specialized for reading data using HTTP for either arrays stored in one
 * contiguous piece of memory or in a series of chunks.
 *
 * @return Always returns true
 * @exception BESError Thrown when the data cannot be read, for a number of
 * reasons, including various network I/O issues.
 */
bool DmrppArray::read()
{
    if (read_p()) return true;

    // Single chunk and 'contiguous' are the same for this code.

    if (get_immutable_chunks().size() == 1 || get_chunk_dimension_sizes().empty()) {
        BESDEBUG(dmrpp_4, "Calling read_contiguous() for " << name() << endl);
        read_contiguous();    // Throws on various errors
    }
    else {  // Handle the more complex case where the data is chunked.
        if (!is_projected()) {
            BESDEBUG(dmrpp_4, "Calling read_chunks_unconstrained() for " << name() << endl);
            read_chunks_unconstrained();
        }
        else {
            BESDEBUG(dmrpp_4, "Calling read_chunks() for " << name() << endl);
            read_chunks();
        }
    }

    return true;
}

/**
 * Classes used with the STL for_each() algorithm; stolen from libdap::Array.
 */
///@{
class PrintD4ArrayDimXMLWriter: public unary_function<Array::dimension&, void> {
    XMLWriter &xml;
    // Was this variable constrained using local/direct slicing? i.e., is d_local_constraint set?
    // If so, don't use shared dimensions; instead emit Dim elements that are anonymous.
    bool d_constrained;
public:

    PrintD4ArrayDimXMLWriter(XMLWriter &xml, bool c) :
        xml(xml), d_constrained(c)
    {
    }

    void operator()(Array::dimension &d)
    {
        // This duplicates code in D4Dimensions (where D4Dimension::print_dap4() is defined
        // because of the need to print the constrained size of a dimension. I think that
        // the constraint information has to be kept here and not in the dimension (since they
        // are shared dims). Could hack print_dap4() to take the constrained size, however.
        if (xmlTextWriterStartElement(xml.get_writer(), (const xmlChar*) "Dim") < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not write Dim element");

        string name = (d.dim) ? d.dim->fully_qualified_name() : d.name;
        // If there is a name, there must be a Dimension (named dimension) in scope
        // so write its name but not its size.
        if (!d_constrained && !name.empty()) {
            if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "name", (const xmlChar*) name.c_str()) < 0)
                throw InternalErr(__FILE__, __LINE__, "Could not write attribute for name");
        }
        else if (d.use_sdim_for_slice) {
            assert(!name.empty());
            if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "name", (const xmlChar*) name.c_str()) < 0)
                throw InternalErr(__FILE__, __LINE__, "Could not write attribute for name");
        }
        else {
            ostringstream size;
            size << (d_constrained ? d.c_size : d.size);
            if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "size", (const xmlChar*) size.str().c_str()) < 0)
                throw InternalErr(__FILE__, __LINE__, "Could not write attribute for name");
        }

        if (xmlTextWriterEndElement(xml.get_writer()) < 0) throw InternalErr(__FILE__, __LINE__, "Could not end Dim element");
    }
};

class PrintD4ConstructorVarXMLWriter: public unary_function<BaseType*, void> {
    XMLWriter &xml;
    bool d_constrained;
public:
    PrintD4ConstructorVarXMLWriter(XMLWriter &xml, bool c) :
        xml(xml), d_constrained(c)
    {
    }

    void operator()(BaseType *btp)
    {
        btp->print_dap4(xml, d_constrained);
    }
};

class PrintD4MapXMLWriter: public unary_function<D4Map*, void> {
    XMLWriter &xml;

public:
    PrintD4MapXMLWriter(XMLWriter &xml) :
        xml(xml)
    {
    }

    void operator()(D4Map *m)
    {
        m->print_dap4(xml);
    }
};
///@}

/**
 * @brief Shadow libdap::Array::print_dap4() - optionally prints DMR++ chunk information
 *
 * This version of libdap::BaseType::print_dap4() will print information about
 * HDF5 chunks when the value of the static class filed dmrpp::DmrppCommon::d_print_chunks
 * is true. The method DMRpp::print_dmrpp() will set the _d_pprint_chunks_ field to
 * true causing this method to include the _chunks_ elements in its output. When
 * the field's value is false, this method prints the same output as libdap::Array.
 *
 * @note There are, no doubt, better ways to do this than using what is essentially a
 * global flag; one way is to  synchronize access to a DMR C++ object and a DOM
 * tree for the same DMR document. The chunk information can be read from the DMR and
 * inserted into the DOM tree, which then printed. If the
 * approach I took here becomes an issue (i.e., if we have to fix problems in libdap and
 * here because of code duplication), we should probably recode this and the related
 * methods to use the 'DOM tree approach.'
 *
 * @param xml Write the XML to this instance of XMLWriter
 * @param constrained True if the response should be constrained. False by default
 *
 * @see DmrppCommon::print_dmrpp()
 * @see DMRpp::print_dmrpp()
 */
void DmrppArray::print_dap4(XMLWriter &xml, bool constrained /*false*/)
{
    if (constrained && !send_p()) return;

    if (xmlTextWriterStartElement(xml.get_writer(), (const xmlChar*) var()->type_name().c_str()) < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not write " + type_name() + " element");

    if (!name().empty())
        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "name", (const xmlChar*) name().c_str()) < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not write attribute for name");

    // Hack job... Copied from D4Enum::print_xml_writer. jhrg 11/12/13
    if (var()->type() == dods_enum_c) {
        D4Enum *e = static_cast<D4Enum*>(var());
        string path = e->enumeration()->name();
        if (e->enumeration()->parent()) {
            // print the FQN for the enum def; D4Group::FQN() includes the trailing '/'
            path = static_cast<D4Group*>(e->enumeration()->parent()->parent())->FQN() + path;
        }
        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "enum", (const xmlChar*) path.c_str()) < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not write attribute for enum");
    }

    if (prototype()->is_constructor_type()) {
        Constructor &c = static_cast<Constructor&>(*prototype());
        for_each(c.var_begin(), c.var_end(), PrintD4ConstructorVarXMLWriter(xml, constrained));
        // bind2nd(mem_fun_ref(&BaseType::print_dap4), xml));
    }

    // Drop the local_constraint which is per-array and use a per-dimension on instead
    for_each(dim_begin(), dim_end(), PrintD4ArrayDimXMLWriter(xml, constrained));

    attributes()->print_dap4(xml);

    for_each(maps()->map_begin(), maps()->map_end(), PrintD4MapXMLWriter(xml));

    // Only print the chunks info if there. This is the code added to libdap::Array::print_dap4().
    // jhrg 5/10/18
    if (DmrppCommon::d_print_chunks && get_immutable_chunks().size() > 0) print_chunks_element(xml, DmrppCommon::d_ns_prefix);

    if (DmrppCommon::d_print_chunks && is_compact_layout() && read_p()) {
        u_int8_t *values;
        u_int8_t width = buf2val(reinterpret_cast<void**>(&values));
        std::string encoded = base64::Base64::encode(values,width);
        print_compact_element( xml, DmrppCommon::d_ns_prefix, encoded);
    }

    if (xmlTextWriterEndElement(xml.get_writer()) < 0) throw InternalErr(__FILE__, __LINE__, "Could not end " + type_name() + " element");
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
