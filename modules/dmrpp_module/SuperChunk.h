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


#include "Chunk.h"


namespace dmrpp {

class DmrppArray;

class SuperChunk {
private:
    std::string d_data_url;
    std::vector<std::shared_ptr<Chunk>> d_chunks;
    unsigned long long d_offset;
    unsigned long long d_size;
    bool d_is_read;
    char *d_read_buffer;

    bool is_contiguous(std::shared_ptr<Chunk> candidate_chunk);
    void map_chunks_to_buffer();
    void read_aggregate_bytes();

public:
    explicit SuperChunk():
        d_data_url(""), d_offset(0), d_size(0), d_is_read(false), d_read_buffer(nullptr){}

    virtual ~SuperChunk(){
        delete[] d_read_buffer;
    }

    virtual bool add_chunk(std::shared_ptr<Chunk> candidate_chunk);

    virtual std::string get_data_url(){ return d_data_url; }
    virtual unsigned long long get_size(){ return d_size; }
    virtual unsigned long long get_offset(){ return d_offset; }

    virtual void read();
    virtual bool empty(){ return d_chunks.empty(); }


    std::vector<std::shared_ptr<Chunk>> get_chunks(){ return d_chunks; }

    virtual void chunks_to_array_values(DmrppArray *target_array);
    virtual void chunks_to_array_values_unconstrained(DmrppArray *target_array);

    std::string to_string(bool verbose) const;
    virtual void dump(std::ostream & strm) const;
};

}// namespace dmrpp


#endif //HYRAX_GIT_SUPERCHUNK_H
