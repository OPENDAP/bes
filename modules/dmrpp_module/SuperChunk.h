//
// Created by ndp on 12/4/20.
//

#ifndef HYRAX_GIT_SUPERCHUNK_H
#define HYRAX_GIT_SUPERCHUNK_H


#include <vector>
#include <memory>


#include "Chunk.h"


namespace dmrpp {

class DmrppCommon;

class SuperChunk {
private:
    std::string d_data_url;
    std::vector<const std::shared_ptr<Chunk>> d_chunks;
    unsigned long long d_offset;
    unsigned long long d_size;
    bool d_is_read;

    bool is_contiguous(const std::shared_ptr<Chunk> &chunk);
    void map_chunks_to_buffer(char *r_buff);
    unsigned long long  read_contiguous(char *r_buff);

public:
    explicit SuperChunk(): d_data_url(""), d_offset(0), d_size(0), d_is_read(false){}
    ~SuperChunk() = default;;
    virtual bool add_chunk(const std::shared_ptr<Chunk> &chunk);

    virtual void set_offset(unsigned long long offset){
        d_offset = offset;
    }
    virtual unsigned long long get_offset(){ return d_offset; };

    virtual void set_size(unsigned long long size){ d_size = size; }
    virtual unsigned long long get_size(){ return d_size; }

    std::string get_curl_range_arg_string();

    virtual void set_data_url(const std::string &url){
        d_data_url = url;
    }
    virtual std::string get_data_url(){  return d_data_url; }

    virtual void read();
    virtual bool empty(){ return d_chunks.empty(); };

    std::string to_string(bool verbose) const;
    virtual void dump(std::ostream & strm) const;
};

}// namespace dmrpp


#endif //HYRAX_GIT_SUPERCHUNK_H
