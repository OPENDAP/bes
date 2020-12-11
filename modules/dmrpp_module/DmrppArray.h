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
#include <vector>

#include <Array.h>

#include "DmrppCommon.h"

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
class DmrppArray : public libdap::Array, public DmrppCommon {

private:
    void _duplicate(const DmrppArray &ts);

    bool is_projected();

    DmrppArray::dimension get_dimension(unsigned int dim_num);

    void insert_constrained_contiguous(Dim_iter dim_iter, unsigned long *target_index,
                                       std::vector<unsigned int> &subset_addr,
                                       const std::vector<unsigned int> &array_shape, char *data);

    void read_contiguous();

#ifdef USE_READ_SERIAL
    virtual void insert_chunk_serial(unsigned int dim, std::vector<unsigned int> *target_element_address,
        std::vector<unsigned int> *chunk_source_address, Chunk *chunk);
    virtual void read_chunks_serial();
#endif

    // Called from read_chunks_unconstrained() and also using pthreads
    friend void
    process_one_chunk_unconstrained(std::shared_ptr<Chunk> chunk, DmrppArray *array, const vector<unsigned int> &array_shape,
                                    const vector<unsigned int> &chunk_shape);

    virtual void insert_chunk_unconstrained(std::shared_ptr<Chunk> chunk, unsigned int dim,
                                    unsigned long long array_offset, const std::vector<unsigned int> &array_shape,
                                    unsigned long long chunk_offset, const std::vector<unsigned int> &chunk_shape,
                                    const std::vector<unsigned int> &chunk_origin);

    void read_chunks_unconstrained();

    unsigned long long get_chunk_start(const dimension &thisDim, unsigned int chunk_origin_for_dim);

    std::shared_ptr<Chunk> find_needed_chunks(unsigned int dim, std::vector<unsigned int> *target_element_address, std::shared_ptr<Chunk> chunk);

    virtual void insert_chunk(
            unsigned int dim,
            std::vector<unsigned int> *target_element_address,
            std::vector<unsigned int> *chunk_element_address,
            std::shared_ptr<Chunk> chunk,
            const vector<unsigned int> &constrained_array_shape);

    // Called from read_chunks()
    friend void process_one_chunk(std::shared_ptr<Chunk> chunk, DmrppArray *array, const vector<unsigned int> &constrained_array_shape);

    void read_chunks();

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

    virtual std::vector<unsigned int> get_shape(bool constrained);

    virtual void print_dap4(libdap::XMLWriter &writer, bool constrained = false);

    virtual void dump(ostream &strm) const;
};

/**
 * Read one chunk in a thread. Uses pthreads.
 *
 * @param arg_list A pointer to a one_chunk_args instance
 * @return NULL on success; a pointer to string on failure. The string holds
 * an error message.
 */
void *one_chunk_thread(void *arg_list);

/**
 * Args for threads that process chunks for constrianed arrays.
 */
struct one_chunk_args {
    int *fds;               // pipe back to parent
    unsigned char tid;      // thread id as a byte
    std::shared_ptr<Chunk> chunk;
    DmrppArray *array;
    const vector<unsigned int> &array_shape;

    one_chunk_args(int *pipe, unsigned char id, std::shared_ptr<Chunk> c, DmrppArray *a, const vector<unsigned int> &a_s)
            : fds(pipe), tid(id), chunk(c), array(a), array_shape(a_s) {}
};

/**
 * Args passed to threads that work with unconstrained array data.
 * The \arg chunk_shape is part of an optimization for the unconstrained
 * array case.
 */
struct one_chunk_unconstrained_args {
    int *fds;               // pipe back to parent
    unsigned char tid;      // thread id as a byte
    std::shared_ptr<Chunk> chunk;
    DmrppArray *array;
    const vector<unsigned int> &array_shape;
    const vector<unsigned int> &chunk_shape;

    one_chunk_unconstrained_args(int *pipe, unsigned char id, std::shared_ptr<Chunk> c, DmrppArray *a, const vector<unsigned int> &a_s,
                                 const vector<unsigned int> &c_s)
            : fds(pipe), tid(id), chunk(c), array(a), array_shape(a_s), chunk_shape(c_s) {}
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

    // FIXME Use smart pointers here. jhrg 9/16/20
    // Done! ndp
    ~one_child_chunk_args() { }
};

} // namespace dmrpp

#endif // _dmrpp_array_h

