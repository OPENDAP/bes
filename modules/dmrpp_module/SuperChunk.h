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
    bool d_chunks_mapped;
    shared_ptr<char> d_read_buffer;

    bool is_contiguous(const std::shared_ptr<Chunk> chunk);
    void map_chunks_to_buffer(shared_ptr<char> r_buff);
    void read_contiguous(shared_ptr<char> r_buff, unsigned long long r_buff_size);

public:
    explicit SuperChunk():
        d_data_url(""), d_offset(0), d_size(0), d_is_read(false), d_chunks_mapped(false), d_read_buffer(nullptr){}

    ~SuperChunk(){
        for(auto chunk:d_chunks){
            if(d_chunks_mapped)
                chunk->set_read_buffer(nullptr,0,0,false);
        }
    }
    virtual bool add_chunk(std::shared_ptr<Chunk> chunk);

#if 0
    // These setter methods are not needed as these values are set by the processing of
    // adding Chunks to the SuperChunk. In fact setter methods for this members would
    // be ill advised based on the class operations.
    virtual void set_offset(unsigned long long offset){
        d_offset = offset;
    }
    virtual void set_size(unsigned long long size){ d_size = size; }

    virtual void set_data_url(const std::string &url){
        d_data_url = url;
    }
#endif
    virtual std::string get_data_url(){  return d_data_url; }
    virtual unsigned long long get_size(){ return d_size; }
    virtual unsigned long long get_offset(){ return d_offset; };

    virtual void read();
    virtual bool empty(){ return d_chunks.empty(); };


    std::vector<std::shared_ptr<Chunk>> get_chunks(){ return d_chunks; }


    std::string to_string(bool verbose) const;
    virtual void dump(std::ostream & strm) const;
};

}// namespace dmrpp


#endif //HYRAX_GIT_SUPERCHUNK_H
