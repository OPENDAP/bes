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

#ifndef _dmrpp_array_h
#define _dmrpp_array_h 1

#include <string>
#include <utility>
#include <vector>
#include <thread>
#include <memory>
#include <queue>
#include <future>
#include <list>

#include <Array.h>

#include "DmrppCommon.h"
#include "SuperChunk.h"

// The 'read_serial()' method is more closely related to the original code
// used to read data when the DMR++ handler was initially developed for NASA.
// I modified that code for a while when we built the prototype version of
// the handler, but then morphed that into a version that would support parallel
// access. Defining this symbol will include the old code in the handler,
// although the DmrppArray::read() method will still have to be hacked to
// use it. jhrg 5/10/18
#undef USE_READ_SERIAL

namespace libdap {
class XMLWriter;
}

namespace dmrpp {

    class SuperChunk;
/**
 * @brief Extend libdap::Array so that a handler can read data using a DMR++ file.
 *
 * @note A key feature of HDF5 is that is can 'chunk' data, breaking up an
 * array into a number of smaller pieces, each of which can be compressed.
 * This code will read both array data that are chunked (and possibly compressed)
 * as well as data that are not (essentially the entire array is written in a
 * single 'chunk'). Because the two cases are different and susceptible to
 * different kinds of optimizations, we have implemented two different read()
 * methods, one for the 'no chunks' case and one for arrays 'with chunks.'
 */
class DmrppArray : public libdap::Array, public dmrpp::DmrppCommon {

private:
    void _duplicate(const DmrppArray &ts);

    bool is_projected();

    DmrppArray::dimension get_dimension(unsigned int dim_num);

    void insert_constrained_contiguous(Dim_iter dim_iter, unsigned long *target_index,
                                       std::vector<unsigned long long> &subset_addr,
                                       const std::vector<unsigned long long> &array_shape, char *data);

    void read_contiguous();

#ifdef USE_READ_SERIAL
    virtual void insert_chunk_serial(unsigned int dim, std::vector<unsigned int> *target_element_address,
        std::vector<unsigned int> *chunk_source_address, Chunk *chunk);
    virtual void read_chunks_serial();
#endif

    friend class DmrppArrayTest;
    // Called from read_chunks_unconstrained() and also using pthreads
    friend void
    process_one_chunk_unconstrained(std::shared_ptr<Chunk> chunk, const vector<unsigned long long> &chunk_shape,
            DmrppArray *array, const vector<unsigned long long> &array_shape);

    // Called from read_chunks()
    friend void
    process_one_chunk(std::shared_ptr<Chunk> chunk, DmrppArray *array, const vector<unsigned long long> &constrained_array_shape);



    virtual void insert_chunk_unconstrained(std::shared_ptr<Chunk> chunk, unsigned int dim,
                                    unsigned long long array_offset, const std::vector<unsigned long long> &array_shape,
                                    unsigned long long chunk_offset, const std::vector<unsigned long long> &chunk_shape,
                                    const std::vector<unsigned long long> &chunk_origin);

    void read_chunks();
    void read_chunks_unconstrained();

    unsigned long long get_chunk_start(const dimension &thisDim, unsigned int chunk_origin_for_dim);

    std::shared_ptr<Chunk> find_needed_chunks(unsigned int dim, std::vector<unsigned long long> *target_element_address, std::shared_ptr<Chunk> chunk);

    virtual void insert_chunk(
            unsigned int dim,
            std::vector<unsigned long long> *target_element_address,
            std::vector<unsigned long long> *chunk_element_address,
            std::shared_ptr<Chunk> chunk,
            const vector<unsigned long long> &constrained_array_shape);



    public:
    DmrppArray(const std::string &n, libdap::BaseType *v);

    DmrppArray(const std::string &n, const std::string &d, libdap::BaseType *v);

    DmrppArray(const DmrppArray &rhs);

    virtual ~DmrppArray() {
    }

    DmrppArray &operator=(const DmrppArray &rhs);

    virtual libdap::BaseType *ptr_duplicate();

    virtual bool read();

    virtual unsigned long long get_size(bool constrained = false);

    virtual std::vector<unsigned long long> get_shape(bool constrained);

    virtual void print_dap4(libdap::XMLWriter &writer, bool constrained = false);

    virtual void dump(ostream &strm) const;
};

/**
 * Args for threads that process SuperChunks, constrained or not.
 */
struct one_super_chunk_args {
    std::thread::id parent_thread_id;
    std::shared_ptr<SuperChunk> super_chunk;
    DmrppArray *array;

    one_super_chunk_args(std::shared_ptr<SuperChunk> sc, DmrppArray *a)
            : parent_thread_id(std::this_thread::get_id()), super_chunk(std::move(sc)), array(a) {}
};


/**
 * Chunk data insert args for use with pthreads. Used for reading contiguous data
 * in parallel.
 */
struct one_child_chunk_args {
    int *fds;               // pipe back to parent
    unsigned char tid;      // thread id as a byte
    std::shared_ptr<Chunk> child_chunk;     // this chunk reads data; temporary allocation
    std::shared_ptr<Chunk> master_chunk;    // this chunk gets the data; shared memory, managed by DmrppArray

    one_child_chunk_args(int *pipe, unsigned char id, std::shared_ptr<Chunk> c_c, std::shared_ptr<Chunk> m_c)
            : fds(pipe), tid(id), child_chunk(c_c), master_chunk(m_c) {}

    ~one_child_chunk_args() { }
};


/**
 * Chunk data insert args for use with pthreads. Used for reading contiguous data
 * in parallel.
 */
struct one_child_chunk_args_new {
    std::shared_ptr<Chunk> child_chunk;     // this chunk reads data; temporary allocation
    std::shared_ptr<Chunk> the_one_chunk;    // this chunk gets the data; shared memory, managed by DmrppArray

    one_child_chunk_args_new(std::shared_ptr<Chunk> c_c, std::shared_ptr<Chunk> m_c) : child_chunk(c_c), the_one_chunk(m_c) {}

    ~one_child_chunk_args_new() { }
};


bool get_next_future(list<std::future<bool>> &futures, atomic_uint &thread_counter, unsigned long timeout, string debug_prefix);

} // namespace dmrpp

#endif // _dmrpp_array_h

