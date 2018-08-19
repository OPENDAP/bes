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

#define USE_PTHREADS 0

namespace dmrpp {

// Callback function used by chunk readers
size_t chunk_write_data(void *buffer, size_t size, size_t nmemb, void *data);

/**
 * This class is used to encapsulate the state and behavior needed for reading
 * chunked data associated with a DAP variable. In particular it is based on the
 * semantics of an hdf4:Chunk object, which is used to represent a chunk of
 * data in a (potentially complex) HDF4/HDF5 file.
 */
class Chunk {
private:
    std::string d_data_url;
    std::string d_query_marker;
    unsigned long long d_size;
    unsigned long long d_offset;

    std::vector<unsigned int> d_chunk_position_in_array;

    // These are used only during the libcurl callback;
    // they are not duplicated by the copy ctor or assignment
    // operator.
    unsigned long long d_bytes_read;
    char *d_read_buffer;
    unsigned long long d_read_buffer_size;
    bool d_is_read;
    bool d_is_inflated;

    static const std::string tracking_context;

    friend class ChunkTest;
    friend class DmrppCommonTest;

protected:

    void _duplicate(const Chunk &bs)
    {
        // See above
        d_bytes_read = 0;
        d_read_buffer = 0;
        d_read_buffer_size = 0;
        d_is_read = false;
        d_is_inflated = false;

        d_size = bs.d_size;
        d_offset = bs.d_offset;
        d_data_url = bs.d_data_url;
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
    Chunk() :
        d_data_url(""), d_query_marker(""), d_size(0), d_offset(0), d_bytes_read(0), d_read_buffer(0),
        d_read_buffer_size(0), d_is_read(false), d_is_inflated(false)
    {
    }

    /**
     * @brief Get a chunk initialized with values
     *
     * @param data_url Where to read this chunk's data
     * @param size The number of bytes to read
     * @param offset Read \arg size bytes starting from this offset
     * @param pia_str A string that provides the logical position of this chunk
     * in an Array. Has the syntax '[1,2,3,4]'.
     */
    Chunk(const std::string &data_url, unsigned long long size, unsigned long long offset, std::string pia_str = "") :
        d_data_url(data_url), d_query_marker(""), d_size(size), d_offset(offset), d_bytes_read(0), d_read_buffer(0),
        d_read_buffer_size(0), d_is_read(false), d_is_inflated(false)
    {
        add_tracking_query_param();
        set_position_in_array(pia_str);
    }

    /**
     * @brief Get a chunk initialized with values
     *
     * @param data_url Where to read this chunk's data
     * @param size The number of bytes to read
     * @param offset Read \arg size bytes starting from this offset
     * @param pia_vec The logical position of this chunk in an Array; a std::vector
     * of unsigned ints.
     */
    Chunk(const std::string &data_url, unsigned long long size, unsigned long long offset, const std::vector<unsigned int> &pia_vec) :
        d_data_url(data_url), d_query_marker(""), d_size(size), d_offset(offset), d_bytes_read(0), d_read_buffer(0),
        d_read_buffer_size(0), d_is_read(false), d_is_inflated(false)
    {
        add_tracking_query_param();
        set_position_in_array(pia_vec);
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
        if (this == &rhs) return *this;

        _duplicate(rhs);

        return *this;
    }

    /**
     * @brief Get the size of this Chunk's data block on disk
     */
    virtual unsigned long long get_size() const
    {
        return d_size;
    }

    /**
     * @brief Get the offset to this Chunk's data block
     */
    virtual unsigned long long get_offset() const
    {
        return d_offset;
    }

    /**
     * @brief Get the data url string for this Chunk's data block
     */
    virtual std::string get_data_url() const
    {
        // A conditional call to void Chunk::add_tracking_query_param()
        // here for the NASA cost model work THG's doing. jhrg 8/7/18

        if (!d_query_marker.empty()) {
            return d_data_url + d_query_marker;
        }

        return d_data_url;
    }

    /**
     * @brief Get the data url string for this Chunk's data block
     */
    virtual void set_data_url(const std::string &data_url)
    {
        d_data_url = data_url;
    }

    /**
     * @brief Get the number of bytes read so far for this Chunk.
     */
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
     * Calling this method will release any previously allocated read buffer
     * memory and then allocate a new memory block. The bytes_read counter is
     * reset to zero.
     */
    virtual void set_rbuf_to_size()
    {
        delete[] d_read_buffer;

        d_read_buffer = new char[d_size];
        d_read_buffer_size = d_size;
        set_bytes_read(0);
    }

    /**
     * Returns a pointer to the memory buffer for this Chunk. The
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
     * allocated buffer and take control of the one passed in with this method.
     * The size and number of bytes read are set to the value of 'size.'
     *
     * @param buf The new buffer to be used by this instance.
     * @param size The size of the new buffer.
     */
    virtual void set_rbuf(char *buf, unsigned int size)
    {
        delete[] d_read_buffer;

        d_read_buffer = buf;
        d_read_buffer_size = size;

        set_bytes_read(size);
    }

    /**
     * Returns the size, in bytes, of the read buffer for this Chunk.
     */
    virtual unsigned long long get_rbuf_size() const
    {
        return d_read_buffer_size;
    }

    /**
     * @return The chunk's position in the array, as a vector of ints.
     */
    virtual const std::vector<unsigned int> &get_position_in_array() const
    {
        return d_chunk_position_in_array;
    }

    virtual void add_tracking_query_param();

    virtual void set_position_in_array(const std::string &pia);
    virtual void set_position_in_array(const std::vector<unsigned int> &pia);

    virtual void read_chunk();

    virtual void inflate_chunk(bool deflate, bool shuffle, unsigned int chunk_size, unsigned int elem_width);

    virtual void set_is_read(bool state) { d_is_read = state; }

    virtual bool get_is_inflated() const { return d_is_inflated; }
    virtual void set_is_inflated(bool state) { d_is_inflated = state; }

    virtual std::string get_curl_range_arg_string();

    virtual void dump(std::ostream & strm) const;

    virtual std::string to_string() const;
};

/// Chunk data decompression function for use with pthreads
struct inflate_chunk_args {
    Chunk *chunk;
    bool deflate;
    bool shuffle;
    unsigned int chunk_size;
    unsigned int elem_width;

    inflate_chunk_args(Chunk *c, bool d, bool s, unsigned int c_size, unsigned int e_size):
        chunk(c), deflate(d), shuffle(s), chunk_size(c_size), elem_width(e_size) {}
};

void *inflate_chunk(void *args);

} // namespace dmrpp

#endif // _Chunk_h
