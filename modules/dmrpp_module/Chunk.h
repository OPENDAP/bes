// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2016 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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

#ifndef _Chunk_h
#define _Chunk_h 1

#include <string>
#include <utility>
#include <vector>
#include <memory>

// BES
#include "url_impl.h"

// libdap4
#include <libdap/util.h>


// This is used to track access to 'cloudydap' accesses in the S3 logs
// by adding a query string that will show up in those logs. This is
// activated by using a special BES context with the name 'cloudydap.'
//
// We disabled this (via ENABLE_TRACKING_QUERY_PARAMETER) because it
// used regexes and was a performance killer. jhrg 4/22/22
#define S3_TRACKING_CONTEXT "cloudydap"
#define ENABLE_TRACKING_QUERY_PARAMETER 0

namespace dmrpp {

union fill_value {
    int8_t int8;
    int16_t int16;
    int32_t int32;
    int64_t int64;

    uint8_t uint8;
    uint16_t uint16;
    uint32_t uint32;
    uint64_t uint64;

    float f;
    double d;
};

// Callback functions used by chunk readers
size_t chunk_header_callback(char *buffer, size_t size, size_t nitems, void *data);
size_t chunk_write_data(void *buffer, size_t size, size_t nmemb, void *data);

void process_s3_error_response(const std::shared_ptr<http::url> &data_url, const std::string &xml_message);

/**
 * This class is used to encapsulate the state and behavior needed for reading
 * chunked data associated with a DAP variable. In particular it is based on the
 * semantics of an hdf4:Chunk object, which is used to represent a chunk of
 * data in a (potentially complex) HDF4/HDF5 file.
 */
class Chunk {
private:
    std::shared_ptr<http::url> d_data_url;
    std::string d_query_marker;
    std::string d_byte_order;
    std::string d_fill_value;
    unsigned long long d_size{0};
    unsigned long long d_offset{0};
    unsigned long long direct_io_offset{0};
    unsigned int d_filter_mask{0};

    // The following two members mean this chunk is actually a linked block.
    // We don't need to remember chunk position but to remember the linked block index.
    bool linked_block=false;
    unsigned int linked_block_index = 0;

    // The following two memebers mean this chunk contains multiple linked blocks.
    // This is still a chunk but inside this chunk it contains multiple linked blocks.
    // We will record this chunk multiple times with different multi_linked_block_index_in_dmrpp_file.
    // This is for generating the dmrpp file. 
    bool multi_linked_blocks =false;
    unsigned int multi_linked_block_index_in_dmrpp_file=0;

    std::vector<std::pair<unsigned long long,unsigned long long>> mlb_offset_lengths;

    bool d_uses_fill_value{false};
    libdap::Type d_fill_value_type{libdap::dods_null_c};
    std::vector<std::pair<libdap::Type,int>> compound_udf_type_elms;

    std::vector<unsigned long long> d_chunk_position_in_array;

    // These are used only during the libcurl callback; they are not duplicated by the
    // copy ctor or assignment operator.

    /**
     *  d_read_buffer_is_mine
     *
     *  This flag controls if the currently
     *  held read buffer memory is released (via a call to 'delete[]')
     *  when an instance is destroyed or when Chunk::set_rbuf_to_size()
     *  or Chunk::set_read_buffer() are invoked. This way, memory
     *  allocated elsewhere can be assigned to a chunk and the chunk
     *  can be used to process the bytes (inflate etc) and the assigned
     *  memory is not deleted during the chunks lifecycle. This includes
     *  when when the chunk is inflating data - if the result won't fit
     *  in the current buffer and the chunk doesn't own it, then the
     *  chunk just allocates new memory for the result and installs it,
     *  dropping the old pointer (no delete[]) and taking possession of
     *  the new memory so it is correctly released as described here.
     */
    bool d_read_buffer_is_mine {true};
    unsigned long long d_bytes_read {0};
    char *d_read_buffer {nullptr};
    unsigned long long d_read_buffer_size {0};
    bool d_is_read {false};
    bool d_is_inflated {false};
    std::string d_response_content_type;

    friend class ChunkTest;
    friend class DmrppCommonTest;
    friend class MockChunk;

    unsigned int obtain_compound_udf_type_size() const;
    unsigned int get_value_size(libdap::Type);
    const char* get_value_ptr(fill_value &,libdap::Type, const std::string &,bool);
    void obtain_fv_strs(vector<string>& fv_str, const string &v) const;
    void get_compound_fvalue(const string &v, vector<char> &compound_fvalue) const;

protected:

    void _duplicate(const Chunk &bs)
    {
        d_size = bs.d_size;
        d_offset = bs.d_offset;
        direct_io_offset = bs.direct_io_offset;
        d_filter_mask = bs.d_filter_mask;
        linked_block = bs.linked_block;
        linked_block_index = bs.linked_block_index;
        multi_linked_block_index_in_dmrpp_file = bs.multi_linked_block_index_in_dmrpp_file;
        multi_linked_blocks = bs.multi_linked_blocks;
        d_data_url = bs.d_data_url;
        d_byte_order = bs.d_byte_order;
        d_fill_value = bs.d_fill_value;
        d_uses_fill_value = bs.d_uses_fill_value;
        d_query_marker = bs.d_query_marker;
        d_chunk_position_in_array = bs.d_chunk_position_in_array;
    }

public:

    /**
     * @brief Get an empty chunk
     *
     * @note This constructor does not read the Query String marker from the BES
     * context system. You must call Chunk::add_tracking_query_param() if you
     * want that information added with Chunks created using this constructor.
     *
     * @see Chunk::add_tracking_query_param()
     */
    Chunk() = default;

    /**
     * @brief Get a chunk initialized with values
     *
     * @param data_url Where to read this chunk's data
     * @param order The data storage byte_order
     * @param size The number of bytes to read
     * @param offset Read \arg size bytes starting from this offset
     * @param pia_str A string that provides the logical position of this chunk
     * in an Array. Has the syntax '[1,2,3,4]'.
     */
    Chunk(std::shared_ptr<http::url> data_url, std::string order, unsigned long long size,
          unsigned long long offset, const std::string &pia_str = "") :
            d_data_url(std::move(data_url)), d_byte_order(std::move(order)),
            d_size(size),  d_offset(offset)
    {
#if ENABLE_TRACKING_QUERY_PARAMETER
        add_tracking_query_param();
#endif
        set_position_in_array(pia_str);
    }

    /**
     * @brief Get a chunk initialized with values, the data URL will not be set.
     *
     * @param data_url Where to read this chunk's data
     * @param order The data storage byte_order
     * @param size The number of bytes to read
     * @param offset Read \arg size bytes starting from this offset
     * @param pia_str A string that provides the logical position of this chunk
     * in an Array. Has the syntax '[1,2,3,4]'.
     */
    Chunk(std::string order, unsigned long long size, unsigned long long offset,  const std::string &pia_str = "") :
            d_byte_order(std::move(order)),  d_size(size), d_offset(offset) {
#if ENABLE_TRACKING_QUERY_PARAMETER
        add_tracking_query_param();
#endif
        set_position_in_array(pia_str);
    }

    Chunk(std::shared_ptr<http::url> data_url, std::string order, unsigned long long size,
          unsigned long long offset, bool lb, unsigned int lblock_index,  const std::string &pia_str = "") :
            d_data_url(std::move(data_url)), d_byte_order(std::move(order)),
            d_size(size),  d_offset(offset), linked_block(lb),linked_block_index(lblock_index)
    {
#if ENABLE_TRACKING_QUERY_PARAMETER
        add_tracking_query_param();
#endif
        set_position_in_array(pia_str);
    }

    Chunk(std::string order, unsigned long long size, unsigned long long offset,
         bool lb, unsigned int lblock_index, const std::string &pia_str = "") :
            d_byte_order(std::move(order)),  d_size(size), d_offset(offset),
            linked_block(lb), linked_block_index(lblock_index)  {
#if ENABLE_TRACKING_QUERY_PARAMETER
        add_tracking_query_param();
#endif
        set_position_in_array(pia_str);
    }

    // For build_dmrpp that has multiple linked blocks in a chunk
    Chunk(std::shared_ptr<http::url> data_url, std::string order, unsigned long long size, unsigned long long offset,
          const std::vector<unsigned long long> &pia_vec,bool is_multi_lb, unsigned int lb_index) :
            d_data_url(std::move(data_url)), d_byte_order(std::move(order)),
            d_size(size), d_offset(offset),  multi_linked_blocks(is_multi_lb), multi_linked_block_index_in_dmrpp_file(lb_index) {
#if ENABLE_TRACKING_QUERY_PARAMETER
        add_tracking_query_param();
#endif
        set_position_in_array(pia_vec);
    }

    Chunk(std::string order, unsigned long long size, unsigned long long offset,
          const std::vector<unsigned long long> &pia_vec,bool is_multi_lb, unsigned int lb_index) :
            d_byte_order(std::move(order)),
            d_size(size), d_offset(offset),  multi_linked_blocks(is_multi_lb), multi_linked_block_index_in_dmrpp_file(lb_index) {
#if ENABLE_TRACKING_QUERY_PARAMETER
        add_tracking_query_param();
#endif
        set_position_in_array(pia_vec);
    }

    // For retrieving dmrpp that has multiple linked blocks in a chunk
    Chunk(std::shared_ptr<http::url> data_url, std::string order, 
          const std::string &pia_vec,const std::vector<std::pair<unsigned long long, unsigned long long>> &lb_ol) :
            d_data_url(std::move(data_url)), d_byte_order(std::move(order))
             {
#if ENABLE_TRACKING_QUERY_PARAMETER
        add_tracking_query_param();
#endif
        set_position_in_array(pia_vec);
        set_multi_linked_offset_length(lb_ol);
        if (lb_ol.empty()==false) 
            multi_linked_blocks = true;
    }

    Chunk(std::string order, 
          const std::string &pia_vec,const std::vector<std::pair<unsigned long long, unsigned long long>> &lb_ol) :
            d_byte_order(std::move(order))
             {
#if ENABLE_TRACKING_QUERY_PARAMETER
        add_tracking_query_param();
#endif
        set_position_in_array(pia_vec);
        set_multi_linked_offset_length(lb_ol);
    }

    /**
     * @brief Get a chunk initialized with values
     *
     * @param data_url Where to read this chunk's data
     * @param order The data storage byte order
     * @param size The number of bytes to read
     * @param offset Read \arg size bytes starting from this offset
     * @param pia_vec The logical position of this chunk in an Array; a std::vector
     * of unsigned ints.
     */
    Chunk(std::shared_ptr<http::url> data_url, std::string order, unsigned long long size, unsigned long long offset,
          const std::vector<unsigned long long> &pia_vec) :
            d_data_url(std::move(data_url)), d_byte_order(std::move(order)),
            d_size(size), d_offset(offset) {
#if ENABLE_TRACKING_QUERY_PARAMETER
        add_tracking_query_param();
#endif
        set_position_in_array(pia_vec);
    }

    Chunk(std::shared_ptr<http::url> data_url, std::string order, unsigned long long size, unsigned long long offset,
          unsigned int filter_mask, const std::vector<unsigned long long> &pia_vec) :
            d_data_url(std::move(data_url)), d_byte_order(std::move(order)),
            d_size(size), d_offset(offset), d_filter_mask(filter_mask)  {
#if ENABLE_TRACKING_QUERY_PARAMETER
        add_tracking_query_param();
#endif
        set_position_in_array(pia_vec);
    }
    /**
     * @brief Get a chunk initialized with values, the data URl will not be set.
     *
     * @param data_url Where to read this chunk's data
     * @param order The data storage byte order
     * @param size The number of bytes to read
     * @param offset Read \arg size bytes starting from this offset
     * @param pia_vec The logical position of this chunk in an Array; a std::vector
     * of unsigned ints.
     */
    Chunk(std::string order, unsigned long long size, unsigned long long offset,
          unsigned int filter_mask, const std::vector<unsigned long long> &pia_vec) :
            d_byte_order(std::move(order)), d_size(size), d_offset(offset), d_filter_mask(filter_mask) {
#if ENABLE_TRACKING_QUERY_PARAMETER
        add_tracking_query_param();
#endif
        set_position_in_array(pia_vec);
    }

    Chunk(std::string order, unsigned long long size, unsigned long long offset,
          const std::vector<unsigned long long> &pia_vec) :
            d_byte_order(std::move(order)), d_size(size), d_offset(offset) {
#if ENABLE_TRACKING_QUERY_PARAMETER
        add_tracking_query_param();
#endif
        set_position_in_array(pia_vec);
    }
    Chunk(std::string order, std::string fill_value, libdap::Type fv_type, unsigned long long chunk_size, std::vector<unsigned long long> pia) :
            d_byte_order(std::move(order)), d_fill_value(std::move(fill_value)), d_size(chunk_size),
            d_uses_fill_value(true), d_fill_value_type(fv_type), d_chunk_position_in_array(std::move(pia)) {
    }

    // Add a constructor when the chunk_offset is provided and chunk_position_in_array is not necessary. KY 2023-02-17
    Chunk(std::string order, std::string fill_value, libdap::Type fv_type, unsigned long long size,
          unsigned long long offset, const std::string &pia_str = "") :
            d_byte_order(std::move(order)),d_fill_value(std::move(fill_value)),
            d_size(size), d_offset(offset), d_uses_fill_value(true), d_fill_value_type(fv_type)
    {
#if ENABLE_TRACKING_QUERY_PARAMETER
        add_tracking_query_param();
#endif
        set_position_in_array(pia_str);
    }

    Chunk(std::string order, std::string fill_value, libdap::Type fv_type, unsigned long long chunk_size, std::vector<unsigned long long> pia, const std::vector<std::pair<libdap::Type,int>> & compound_udf_type_elms) :
            d_byte_order(std::move(order)), d_fill_value(std::move(fill_value)), d_size(chunk_size),
            d_uses_fill_value(true), d_fill_value_type(fv_type), d_chunk_position_in_array(std::move(pia)) {
        set_compound_udf_info(compound_udf_type_elms);

    }


    Chunk(const Chunk &h4bs)
    {
        _duplicate(h4bs);
    }

    virtual ~Chunk()
    {
        if(d_read_buffer_is_mine)
            delete[] d_read_buffer;
        d_read_buffer = nullptr;
    }

    /// I think this is broken. vector<Chunk> assignment fails
    /// in the read_atomic() method but 'assignment' using a reference
    /// works. This bug shows up in DmrppCommnon::read_atomic().
    /// jhrg 4/10/18
    Chunk &operator=(const Chunk &rhs)
    {
        if (this == &rhs) return *this;

        _duplicate(rhs);

        return *this;
    }

    /// @brief Get the response type of the last response
    virtual std::string get_response_content_type() { return d_response_content_type; }

    /// @brief Set the response type of the last response
    void  set_response_content_type(const std::string &ct) { d_response_content_type = ct; }

    /// @return Get the chunk byte order
    virtual std::string get_byte_order() { return d_byte_order; }

    /// @return Get the size of this Chunk's data block on disk
    virtual unsigned long long get_size() const
    {
        return d_size;
    }

    /// @return  Get the offset to this Chunk's data block
    virtual unsigned long long get_offset() const
    {
        return d_offset;
    }

    virtual void set_size(unsigned long long storage_size) 
    {
        d_size = storage_size;
    }

    virtual unsigned long long get_direct_io_offset() const
    {
        return direct_io_offset;
    }

    virtual void set_direct_io_offset(unsigned long long dio_offset) {
        direct_io_offset = dio_offset;
    }

    virtual unsigned int get_filter_mask() const
    {
        return d_filter_mask;
    }

    virtual bool get_linked_block() const
    {
        return linked_block;
    }

    virtual unsigned int get_linked_block_index() const
    {
        return linked_block_index;
    }

    virtual bool get_multi_linked_blocks() const
    {
        return multi_linked_blocks;
    }

    virtual unsigned int get_multi_linked_block_index_in_dmrpp_file() const
    {
        return multi_linked_block_index_in_dmrpp_file;
    }


    /// @return Return true if the the chunk uses 'fill value.'
    virtual bool get_uses_fill_value() const { return d_uses_fill_value; }
    virtual libdap::Type get_fill_value_type() const { return d_fill_value_type; }

    /// @return Return the fill value as a string or "" if get_fill_value() is false
    virtual std::string get_fill_value() const { return d_fill_value; }

    /// @return Get the data url for this Chunk's data block
    virtual std::shared_ptr<http::url>  get_data_url() const;

    /// @brief Set the data url for this Chunk's data block
    virtual void set_data_url(std::shared_ptr<http::url> data_url)
    {
        d_data_url = std::move(data_url);
    }

    virtual bool get_read_buffer_is_mine() { return d_read_buffer_is_mine; }
    /// @return Get the number of bytes read so far for this Chunk.
    virtual unsigned long long get_bytes_read() const
    {
        return d_bytes_read;
    }

    /**
     * @brief Set the size of this Chunk's data block
     * @param size Size of the data in bytes
     */
    virtual void set_bytes_read(unsigned long long bytes_read)
    {
        d_bytes_read = bytes_read;
    }

    /**
     * @brief Allocates the internal read buffer to be d_size bytes
     *
     * The memory of the read buffer is managed internally by this method.
     *
     * The class maintains an internal flag, d_read_buffer_is_mine, which
     * controls if the currently held read buffer memory is released
     * (via a call to 'delete[]') when an this method is invoked.
     *
     * If the CHunk owns the read buffer, then calling this method
     * will release any previously allocated read buffer memory and then
     * allocate a new memory block. The bytes_read counter is
     * reset to zero.
     */
    virtual void set_rbuf_to_size()
    {
        if(d_read_buffer_is_mine)
            delete[] d_read_buffer;
        d_read_buffer = new char[d_size];
        d_read_buffer_size = d_size;
        d_read_buffer_is_mine = true;
        set_bytes_read(0);
    }

    /// @return A pointer to the memory buffer for this Chunk.
    /// The return value is NULL if no memory has been allocated.
    virtual char *get_rbuf()
    {
        return d_read_buffer;
    }

    /**
     * @brief Set the target read buffer for this chunk.
     *
     * @param buf The new buffer to install into the Chunk.
     * @param buf_size The size of the passed buffer.
     * @param bytes_read The number of bytes that have been read into buf. In practice
     * this is the offset in buf at which new bytes should be added, (default: 0)
     * @param assume_ownership If true, then the memory pointed to by buf will be deleted (using delete[])
     * when the Chunk object's destructor is called. If false then the Chunk's destructor will not attempt to
     * free/delete the memory pointed to by buf. (default: true)
     */
     void set_read_buffer(char *buf, unsigned long long buf_size, unsigned long long bytes_read = 0,
                          bool assume_ownership = true ) {
        if(d_read_buffer_is_mine)
            delete[] d_read_buffer;
        d_read_buffer_is_mine = assume_ownership;
        d_read_buffer = buf;
        d_read_buffer_size = buf_size;

        set_bytes_read(bytes_read);
    }

    /// @return The size, in bytes, of the current read buffer for this Chunk.
    virtual unsigned long long get_rbuf_size() const
    {
        return d_read_buffer_size;
    }

    /// @return The chunk's position in the array, as a vector of ints.
    virtual const std::vector<unsigned long long> &get_position_in_array() const
    {
        return d_chunk_position_in_array;
    }

    void add_tracking_query_param();

    void set_position_in_array(const std::string &pia);
    void set_position_in_array(const std::vector<unsigned long long> &pia);

    void set_multi_linked_offset_length(const std::vector<std::pair<unsigned long long,unsigned long long>> &lb_offset_lengths){

        for (const auto &lb_ol:lb_offset_lengths) {
            
            std::pair<unsigned long long,unsigned long long> temp_pair;
            temp_pair.first = lb_ol.first;
            temp_pair.second = lb_ol.second;
            mlb_offset_lengths.push_back(temp_pair);
        }

    }
    void obtain_multi_linked_offset_length(vector<std::pair<unsigned long long, unsigned long long>> & cur_chunk_lb_offset_length) const{

         for (const auto &lb_ol:mlb_offset_lengths) {
            
            std::pair<unsigned long long,unsigned long long> temp_pair;
            temp_pair.first = lb_ol.first;
            temp_pair.second = lb_ol.second;
            cur_chunk_lb_offset_length.push_back(temp_pair);
        }

    }

    void set_compound_udf_info(const std::vector<std::pair<libdap::Type,int>> &structure_type_element){

        for (const auto &ste:structure_type_element) {
            
            std::pair<libdap::Type,int> temp_pair;
            temp_pair.first = ste.first;
            temp_pair.second = ste.second;
            compound_udf_type_elms.push_back(temp_pair);
        }

    }

    virtual void read_chunk();
    virtual void read_chunk_dio();
    virtual void load_fill_values();

    virtual void filter_chunk(const std::string &filters, unsigned long long chunk_size, unsigned long long elem_width);

    virtual bool get_is_read() { return d_is_read; }
    virtual void set_is_read(bool state) { d_is_read = state; }

    virtual std::string get_curl_range_arg_string();

    static void parse_chunk_position_in_array_string(const std::string &pia, std::vector<unsigned long long> &pia_vect);

    virtual void dump(std::ostream & strm) const;

    virtual std::string to_string() const;
};

} // namespace dmrpp

#endif // _Chunk_h
