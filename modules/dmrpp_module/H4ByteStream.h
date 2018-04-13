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

#ifndef _H4ByteStream_h
#define _H4ByteStream_h 1

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
class H4ByteStream {
private:
    std::string d_data_url;
    unsigned long long d_size;
    unsigned long long d_offset;
#if 0
    // TODO Remove these
    std::string d_md5;
    std::string d_uuid;
#endif

    bool d_is_read;
    std::vector<unsigned int> d_chunk_position_in_array;

    // These are used only during the libcurl callback;
    // they are not duplicated by the copy ctor or assignment
    // operator.
    unsigned long long d_bytes_read;
    char *d_read_buffer;
    unsigned long long d_read_buffer_size;

#if 0
    unsigned long long d_read_pointer;
#endif


    CURL *d_curl_handle;
    char d_curl_error_buf[CURL_ERROR_SIZE];

    bool d_is_in_multi_queue;

    void add_tracking_query_param(std::string& data_access_url);

    friend class H4ByteStreamTest;

protected:

    void _duplicate(const H4ByteStream &bs)
    {
        // See above
        d_bytes_read = 0;
        d_read_buffer = 0;
        d_read_buffer_size = 0;
#if 0
        d_read_pointer = 0;
#endif

        d_is_read = false;
        d_curl_handle = 0;
        d_is_in_multi_queue = false;

#if 0
        // For some reason, the assignment of a vector<H4ByteStream> fails
        // but an assignment of a reference to a vector<H4ByteStream> works.
        // I thought it was something in the code above - something missing
        // in _duplicate(), but no. jhrg 4/10/18
        d_bytes_read = bs.d_bytes_read;

        d_read_buffer = new char[bs.d_read_buffer_size];
        d_read_buffer_size = bs.d_read_buffer_size;
        memcpy(d_read_buffer, bs.d_read_buffer, bs.d_read_buffer_size);

        d_read_pointer = bs.d_read_pointer;
        d_is_read = bs.d_is_read;

        d_curl_handle = bs.d_curl_handle;
        d_is_in_multi_queue = bs.d_is_in_multi_queue;
#endif

        // These vars are easy to duplicate.
        d_size = bs.d_size;
        d_offset = bs.d_offset;
#if 0
        d_md5 = bs.d_md5;
        d_uuid = bs.d_uuid;
#endif
        d_data_url = bs.d_data_url;
        d_chunk_position_in_array = bs.d_chunk_position_in_array;
    }

public:

    H4ByteStream() :
            d_data_url(""), d_size(0), d_offset(0), // TODO d_md5(""), d_uuid(""),
            d_is_read(false), d_bytes_read(0),
            d_read_buffer(0), d_read_buffer_size(0), // TODO d_read_pointer(0),
            d_curl_handle(0), d_is_in_multi_queue(false)
    {
    }

    H4ByteStream(std::string data_url, unsigned long long size, unsigned long long offset, std::string position_in_array = "") :
            d_data_url(data_url), d_size(size), d_offset(offset), // TODO d_md5(""), d_uuid(""),
            d_is_read(false), d_bytes_read(0), d_read_buffer(0), d_read_buffer_size(0),
            d_curl_handle(0), d_is_in_multi_queue(false)
    {
        ingest_position_in_array(position_in_array);
    }

#if 0
    H4ByteStream(std::string data_url, unsigned long long size, unsigned long long offset, std::string md5,
        std::string uuid, std::string position_in_array = "") :
    d_data_url(data_url), d_size(size), d_offset(offset), d_md5(md5), d_uuid(uuid),
    d_is_read(false), d_bytes_read(0), d_read_buffer(0), d_read_buffer_size(0), // TODO  d_read_pointer(0),
    d_curl_handle(0), d_is_in_multi_queue(false)
    {
        ingest_position_in_array(position_in_array);
    }
#endif


    H4ByteStream(const H4ByteStream &h4bs)
    {
        _duplicate(h4bs);
    }

    virtual ~H4ByteStream()
    {
        delete[] d_read_buffer;
    }

    /// I think this is broken. vector<H4ByteStream> assignment fails
    /// in the read_atomic() method but 'assignment' using a reference
    /// works. This bug shows up in DmrppCommnon::read_atomic().
    /// jhrg 4/10/18
    H4ByteStream &operator=(const H4ByteStream &rhs)
    {
        if (this == &rhs)
        return *this;

        _duplicate(rhs);

        return *this;
    }

#if 0
    void reset_read_pointer()
    {
        d_read_pointer = 0;
    }

    void increment_read_pointer(unsigned long long size)
    {
        d_read_pointer += size;
    }

    char *get_read_pointer()
    {
        return get_rbuf() + d_read_pointer;
    }
#endif


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
     * @brief Get the md5 string for this byteStream's data block
     */
    virtual std::string get_md5() const
    {
        return ""; // TODO d_md5;

    }
    /**
     * @brief Get the uuid string for this byteStream's data block
     */
    virtual std::string get_uuid() const
    {
        return ""; // TODO d_uuid;
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

#if 0
    /**
     * @brief Sets the size of the internal read buffer.
     * @deprecated
     * @see set_rbuf_to_size()
     * @param size Size of the internal read buffer.
     */
    virtual void rbuf_size(unsigned long long size)
    {
        // Calling delete on a null pointer is fine, so we don't need to check
        // to see if this is the first call.
        delete[] d_read_buffer;
        d_read_buffer_size = 0;

        d_read_buffer = new char[size];
        d_read_buffer_size = size;
        set_bytes_read(0);
    }
#endif


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
#if 0
        rbuf_size(d_size);
#endif

        // Calling delete on a null pointer is fine, so we don't need to check
        // to see if this is the first call.
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
        // Calling delete on a null pointer is fine, so we don't need to check
        // to see if this is the first call.
        delete[] d_read_buffer;
        d_read_buffer_size = 0;

        d_read_buffer = buf;
        d_read_buffer_size = size;

        // FIXME Setting d_byes_read to 'size' may break code that tests to see that the
        // correct number of bytes were actually read. I think this is patched, but we
        // should think about what this field really means - the number of bytes read or
        // the size of the current read_buffer? For now this works given a mod in
        //H4ByteStream::read(...). jhrg 1/18/17
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
     * @brief Parse the dimension string store the sizes in an internal vector.
     *
     * The dimension information is a string of the form '[x,y,...,z]' Parse the
     * integer dimension sizes and store them in the d_position_in_array vector.
     * Since this is essentially a setter method any previous postion_in_array
     * content is discarded. If the passed string parameter is empty then nothing
     * is done.
     *
     * @param pia The dimension string
     */
    virtual void ingest_position_in_array(std::string pia);

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
     */
    virtual void read() {
        read(false, 0, false, 0);   // default values for no compression
    }

    virtual void read(bool deflate, unsigned int chunk_size, bool shuffle, unsigned int elem_size);

    virtual bool is_started() const { return d_is_in_multi_queue; };
    virtual bool is_read() const { return d_is_read;  }

    virtual void set_is_read(bool state) { d_is_read = state; }

    virtual std::string get_curl_range_arg_string();

    virtual void add_to_multi_read_queue(CURLM *multi_handle);

    void complete_read(bool deflate, unsigned int chunk_size, bool shuffle, unsigned int elem_width);

    virtual void dump(std::ostream & strm) const;

    virtual std::string to_string() const;
};

} // namespace dmrpp

#endif // _H4ByteStream_h
