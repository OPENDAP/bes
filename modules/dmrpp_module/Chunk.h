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
#include <vector>

#include <curl/curl.h>

namespace dmrpp {

/**
 * This class is used to encapsulate the state and behavior needed for reading
 * chunked data associated with a DAP variable. In particular it is based on the
 * semantics of an hdf4:byteStream object, which is used to represent a chunk of
 * data in a (potentially complex) HDF4/HDF5 file.
 */
class Chunk {
private:
    std::string d_data_url;
    unsigned long long d_size;
    unsigned long long d_offset;

    bool d_is_read;
    std::vector<unsigned int> d_chunk_position_in_array;

    // These are used only during the libcurl callback;
    // they are not duplicated by the copy ctor or assignment
    // operator.
    unsigned long long d_bytes_read;
    char *d_read_buffer;
    unsigned long long d_read_buffer_size;

#if 1
    CURL *d_curl_handle;
    char d_curl_error_buf[CURL_ERROR_SIZE];
#endif

    bool d_is_in_multi_queue;

    void add_tracking_query_param(std::string& data_access_url);

    friend class ChunkTest;

protected:

    void _duplicate(const Chunk &bs)
    {
        // See above
        d_bytes_read = 0;
        d_read_buffer = 0;
        d_read_buffer_size = 0;
        d_is_read = false;
        d_curl_handle = 0;
        d_is_in_multi_queue = false;

        // These vars are easy to duplicate.
        d_size = bs.d_size;
        d_offset = bs.d_offset;
        d_data_url = bs.d_data_url;
        d_chunk_position_in_array = bs.d_chunk_position_in_array;
    }

public:

    Chunk() :
        d_data_url(""), d_size(0), d_offset(0), d_is_read(false), d_bytes_read(0), d_read_buffer(0),
        d_read_buffer_size(0), d_curl_handle(0), d_is_in_multi_queue(false)
    {
    }

    Chunk(std::string data_url, unsigned long long size, unsigned long long offset, std::string position_in_array = "") :
        d_data_url(data_url), d_size(size), d_offset(offset), d_is_read(false), d_bytes_read(0), d_read_buffer(0),
        d_read_buffer_size(0), d_curl_handle(0), d_is_in_multi_queue(false)
    {
#if 0
        ingest_position_in_array(position_in_array);
#else
        set_position_in_array(position_in_array);
#endif
    }

    Chunk(const Chunk &h4bs)
    {
        _duplicate(h4bs);
    }

    virtual ~Chunk()
    {
        delete[] d_read_buffer;
    }

    /// I think this is broken. vector<Chunk> assignment fails
    /// in the read_atomic() method but 'assignment' using a reference
    /// works. This bug shows up in DmrppCommnon::read_atomic().
    /// jhrg 4/10/18
    Chunk &operator=(const Chunk &rhs)
    {
        if (this == &rhs)
        return *this;

        _duplicate(rhs);

        return *this;
    }

    virtual CURL *get_curl_handle() const
    {
        return d_curl_handle;
    }

    virtual void cleanup_curl_handle()
    {
        if (d_curl_handle != 0) curl_easy_cleanup(d_curl_handle);
        d_curl_handle = 0;
    }

    /**
     * @brief Get the size of this byteStream's data block on disk
     */
    virtual unsigned long long get_size() const
    {
        return d_size;
    }
    /**
     * @brief Get the offset to this byteStream's data block
     */
    virtual unsigned long long get_offset() const
    {
        return d_offset;
    }

    /**
     * @brief Get the data url string for this byteStream's data block
     */
    virtual std::string get_data_url() const
    {
        return d_data_url;
    }
    /**
     * @brief Get the data url string for this byteStream's data block
     */
    virtual void set_data_url(const std::string &data_url)
    {
        d_data_url = data_url;
    }

    /**
     * @brief Get the size of the data block associated with this byteStream.
     */
    virtual unsigned long long get_bytes_read() const
    {
        return d_bytes_read;
    }

    /**
     * @brief Set the size of this byteStream's data block
     * @param size Size of the data in bytes
     */
    virtual void set_bytes_read(unsigned long long bytes_read)
    {
        d_bytes_read = bytes_read;
    }

    /**
     * @brief Allocates the intenal read buffer to be d_size bytes
     *
     * The memory of the read buffer is managed internally by this method.
     * Calling this method will release any previously allocated read buffer
     * memory and then allocate a new memory block. The bytes_read counter is
     * reset to zero.
     */
    virtual void set_rbuf_to_size()
    {
        delete[] d_read_buffer;
        d_read_buffer_size = 0;

        d_read_buffer = new char[d_size];
        d_read_buffer_size = d_size;
        set_bytes_read(0);
    }

    /**
     * Returns a pointer to the memory buffer for this byteStream. The
     * return value is NULL if no memory has been allocated.
     */
    virtual char *get_rbuf()
    {
        return d_read_buffer;
    }

    /**
     * @brief Set the read buffer
     *
     * Transfer control of the buffer to this object. The buffer must have been
     * allocated using 'new char[size]'. This object will delete any previously
     * allocated buffer and take control of the one passes in with this method.
     * The size and number of bytes read are set to the value of 'size.'
     *
     * @param buf The new buffer to be used by this instance.
     * @param size The size of the new buffer.
     */
    virtual void set_rbuf(char *buf, unsigned int size)
    {
        delete[] d_read_buffer;
        d_read_buffer_size = 0;

        d_read_buffer = buf;
        d_read_buffer_size = size;

        // FIXME Setting d_byes_read to 'size' may break code that tests to see that the
        // correct number of bytes were actually read. I think this is patched, but we
        // should think about what this field really means - the number of bytes read or
        // the size of the current read_buffer? For now this works given a mod in
        // Chunk::read(...). jhrg 1/18/17
        set_bytes_read(size);
    }

    /**
     * Returns the size, in bytes, of the read buffer for this byteStream.
     */
    virtual unsigned long long get_rbuf_size() const
    {
        return d_read_buffer_size;
    }

    virtual std::vector<unsigned int> get_position_in_array() const
    {
        return d_chunk_position_in_array;
    }

    /**
     * @brief An alternate impl of ingest_position_in_array
     *
     * @note It would be much easier to parse this string if it didn't have the
     * unneeded [, ], and comma (,) characters. The integers could be separated
     * by whitespace. Then an istringstream and a while loop would be all that's
     * needed.
     *
     * @param pia The dimension string.
     */
    virtual void set_position_in_array(const std::string &pia);

    /**
     * @brief default version of read() for types that are not chunked
     *
     * This function does the simple read operation for variables that are
     * not chunked. We treat 'not-chunked' as 'stored in a single chunk.'
     * An assumption is that data that are 'not-chunked' are also not compressed
     * in any way.
     */
    virtual void read() {
        read(false, false, 0, 0);   // default values for no compression
    }

    virtual void read(bool deflate, bool shuffle, unsigned int chunk_size, unsigned int elem_size);

    void inflate_chunk(bool deflate, bool shuffle, unsigned int chunk_size, unsigned int elem_width);

    virtual void add_to_multi_read_queue(CURLM *multi_handle);

    virtual bool is_started() const { return d_is_in_multi_queue; };
    virtual bool is_read() const { return d_is_read;  }

    virtual void set_is_read(bool state) { d_is_read = state; }

    virtual std::string get_curl_range_arg_string();

    virtual void dump(std::ostream & strm) const;

    virtual std::string to_string() const;
};

} // namespace dmrpp

#endif // _Chunk_h
