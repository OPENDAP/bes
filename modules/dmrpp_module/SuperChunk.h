// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2020 OPeNDAP, Inc.
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

#ifndef HYRAX_GIT_SUPERCHUNK_H
#define HYRAX_GIT_SUPERCHUNK_H


#include <vector>
#include <memory>
#include <thread>
#include <queue>
#include <sstream>


#include "Chunk.h"


namespace dmrpp {

// Forward Declaration;
class DmrppArray;

/**
 * @brief A SuperChunk is a collection of contiguous Chunk objects along with optimized methods for data retrieval and inflation.
 *
 */
class SuperChunk {
private:
    std::string d_id;
    DmrppArray *d_parent_array;
    std::shared_ptr<http::url> d_data_url;
    std::vector<std::shared_ptr<Chunk>> d_chunks;
    unsigned long long d_offset;
    unsigned long long d_size;
    bool d_is_read;
    char *d_read_buffer;

    bool is_contiguous(std::shared_ptr<Chunk> candidate_chunk);
    void map_chunks_to_buffer();
    void read_aggregate_bytes();

public:

    explicit SuperChunk(const std::string sc_id, DmrppArray *parent=nullptr):
    d_id(sc_id), d_parent_array(parent), d_data_url(nullptr), d_offset(0), d_size(0), d_is_read(false), d_read_buffer(nullptr){}

    virtual ~SuperChunk(){
        delete[] d_read_buffer;
    }

    virtual std::string id(){ return d_id; }

    virtual bool add_chunk(std::shared_ptr<Chunk> candidate_chunk);

    std::shared_ptr<http::url> get_data_url(){ return d_data_url; }
    virtual unsigned long long get_size(){ return d_size; }
    virtual unsigned long long get_offset(){ return d_offset; }

    virtual void read(){
        retrieve_data();
        process_child_chunks();
    }
    virtual void read_unconstrained(){
        retrieve_data();
        process_child_chunks_unconstrained();
    }

    virtual void retrieve_data();
    virtual void process_child_chunks();
    virtual void process_child_chunks_unconstrained();


    virtual bool empty(){ return d_chunks.empty(); }

    std::vector<std::shared_ptr<Chunk>> get_chunks(){ return d_chunks; }

    std::string to_string(bool verbose) const;
    virtual void dump(std::ostream & strm) const;
};

/**
 * @brief Single argument structure for a thread that will process a single Chunk for a constrained array.
 * Utilized as an argument to std::async()
 */
struct one_chunk_args {
    std::thread::id parent_thread_id;
    std::string parent_super_chunk_id;
    std::shared_ptr<Chunk> chunk;
    DmrppArray *array;
    const vector<unsigned long long> &array_shape;

    one_chunk_args(const string sc_id, std::shared_ptr<Chunk> c, DmrppArray *a, const vector<unsigned long long> &a_s)
            : parent_thread_id(std::this_thread::get_id()), parent_super_chunk_id(sc_id), chunk(std::move(c)), array(a), array_shape(a_s) {}
};

/**
 * @brief Single argument structure for a thread that will process a single Chunk for an unconstrained array.
 * Utilized as an argument to std::async()
 * The \arg chunk_shape is part of an optimization for the unconstrained array case.
 */
struct one_chunk_unconstrained_args {
    std::thread::id parent_thread_id;
    std::string parent_super_chunk_id;
    std::shared_ptr<Chunk> chunk;
    DmrppArray *array;
    const vector<unsigned long long> &array_shape;
    const vector<unsigned long long> &chunk_shape;

    one_chunk_unconstrained_args(const string sc_id, std::shared_ptr<Chunk> c, DmrppArray *a, const vector<unsigned long long> &a_s,
                                 const vector<unsigned long long> &c_s)
            : parent_thread_id(std::this_thread::get_id()), parent_super_chunk_id(sc_id), chunk(std::move(c)),
            array(a), array_shape(a_s), chunk_shape(c_s) {}
};

void process_chunks_concurrent(
        const string &super_chunk_id,
        std::queue<shared_ptr<Chunk>> &chunks,
        DmrppArray *array,
        const std::vector<unsigned long long> &shape );

void process_chunks_unconstrained_concurrent(
        const string &super_chunk_id,
        std::queue<std::shared_ptr<Chunk>> &chunks,
        const std::vector<unsigned long long> &chunk_shape,
        DmrppArray *array,
        const std::vector<unsigned long long> &array_shape);



}// namespace dmrpp


#endif //HYRAX_GIT_SUPERCHUNK_H
