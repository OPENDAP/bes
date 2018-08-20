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

#include "config.h"

#include <sstream>
#include <cstdlib>
#include <cstring>
#include <cassert>

#include <zlib.h>

#include <BESDebug.h>
#include <BESInternalError.h>
#include <BESContextManager.h>

#include "Chunk.h"
#include "CurlHandlePool.h"
#include "DmrppRequestHandler.h"

const string debug = "dmrpp";

using namespace std;

namespace dmrpp {

const std::string Chunk::tracking_context = "cloudydap";

/**
 * @brief Callback passed to libcurl to handle reading a single byte.
 *
 * This callback assumes that the size of the data is small enough
 * that all of the bytes will be either read at once or that a local
 * temporary buffer can be used to build up the values.
 *
 * @param buffer Data from libcurl
 * @param size Number of bytes
 * @param nmemb Total size of data in this call is 'size * nmemb'
 * @param data Pointer to this
 * @return The number of bytes read
 */
size_t chunk_write_data(void *buffer, size_t size, size_t nmemb, void *data)
{
    Chunk *c_ptr = reinterpret_cast<Chunk*>(data);

    // rbuf: |******++++++++++----------------------|
    //              ^        ^ bytes_read + nbytes
    //              | bytes_read

    unsigned long long bytes_read = c_ptr->get_bytes_read();
    size_t nbytes = size * nmemb;

    // If this fails, the code will write beyond the buffer.
    assert(bytes_read + nbytes <= c_ptr->get_rbuf_size());

    memcpy(c_ptr->get_rbuf() + bytes_read, buffer, nbytes);

    c_ptr->set_bytes_read(bytes_read + nbytes);

    return nbytes;
}

/**
 * @brief Deflate data. This is the zlib algorithm.
 *
 * @note Stolen from the HDF5 library and hacked to fit.
 *
 * @param dest Write the 'inflated' data here
 * @param dest_len Size of the destination buffer
 * @param src Compressed data
 * @param src_len Size of the compressed data
 */
void inflate(char *dest, unsigned int dest_len, char *src, unsigned int src_len)
{
    /* Sanity check */
    assert(src_len > 0);
    assert(src);
    assert(dest_len > 0);
    assert(dest);

    /* Input; uncompress */
    z_stream z_strm; /* zlib parameters */

    /* Set the uncompression parameters */
    memset(&z_strm, 0, sizeof(z_strm));
    z_strm.next_in = (Bytef *) src;
    z_strm.avail_in = src_len;
    z_strm.next_out = (Bytef *) dest;
    z_strm.avail_out = dest_len;

    /* Initialize the uncompression routines */
    if (Z_OK != inflateInit(&z_strm))
        throw BESError("Failed to initialize inflate software.", BES_INTERNAL_ERROR, __FILE__, __LINE__);

    /* Loop to uncompress the buffer */
    int status = Z_OK;
    do {
        /* Uncompress some data */
        status = inflate(&z_strm, Z_SYNC_FLUSH);

        /* Check if we are done uncompressing data */
        if (Z_STREAM_END == status) break; /*done*/

        /* Check for error */
        if (Z_OK != status) {
            (void) inflateEnd(&z_strm);
            throw BESError("Failed to inflate data chunk.", BES_INTERNAL_ERROR, __FILE__, __LINE__);
        }
        else {
            /* If we're not done and just ran out of buffer space, it's an error.
             * The HDF5 library code would extend the buffer as needed, but for
             * this handler, we always know the size of the uncompressed chunk.
             */
            if (0 == z_strm.avail_out) {
                throw BESError("Data buffer is not big enough for uncompressed data.", BES_INTERNAL_ERROR, __FILE__, __LINE__);
#if 0
                /* Here's how to extend the buffer if needed. This might be useful someday... */
                void *new_outbuf; /* Pointer to new output buffer */

                /* Allocate a buffer twice as big */
                nalloc *= 2;
                if (NULL == (new_outbuf = H5MM_realloc(outbuf, nalloc))) {
                    (void) inflateEnd(&z_strm);
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0, "memory allocation failed for inflate decompression")
                } /* end if */
                outbuf = new_outbuf;

                /* Update pointers to buffer for next set of uncompressed data */
                z_strm.next_out = (unsigned char*) outbuf + z_strm.total_out;
                z_strm.avail_out = (uInt) (nalloc - z_strm.total_out);
#endif
            } /* end if */
        } /* end else */
    } while (status == Z_OK);

    /* Finish uncompressing the stream */
    (void) inflateEnd(&z_strm);
}

// #define this to enable the duff's device loop unrolling code.
// jhrg 1/19/17
#define DUFFS_DEVICE

/**
 * @brief Un-shuffle data.
 *
 * @note Stolen from HDF5 and hacked to fit
 *
 * @note We use src size as a param because the buffer might be larger than
 * elems * width (e.g., 1020 byte buffer will hold 127 doubles with 4 extra).
 * If we used elems * width, the the buffer size will be too small for those
 * extra bytes. Code at the end of this function will transfer them.
 *
 * @note Do not call this when the number of elements or the element width
 * is 1. In the HDF5 library chunks that fit that description are never shuffled
 * (because there really is nothing to shuffle). The function will handle that
 * case, but by not calling it you can save the allocation of a buffer and a
 * call to memcpy.
 *
 * @param dest Put the result here.
 * @param src Shuffled data source
 * @param src_size Number of bytes in both src and dest
 * @param width Number of bytes in an element
 */
void unshuffle(char *dest, const char *src, unsigned int src_size, unsigned int width)
{
    unsigned int elems = src_size / width;  // int division rounds down

    /* Don't do anything for 1-byte elements, or "fractional" elements */
    if (!(width > 1 && elems > 1)) {
        memcpy(dest, const_cast<char*>(src), src_size);
    }
    else {
        /* Get the pointer to the source buffer (Alias for source buffer) */
        char *_src = const_cast<char*>(src);
        char *_dest = 0;   // Alias for destination buffer

        /* Input; unshuffle */
        for (unsigned int i = 0; i < width; i++) {
            _dest = dest + i;
#ifndef DUFFS_DEVICE
            size_t j = elems;
            while(j > 0) {
                *_dest = *_src++;
                _dest += width;

                j--;
            }
#else /* DUFFS_DEVICE */
            {
                size_t duffs_index = (elems + 7) / 8;   /* Counting index for Duff's device */
                switch (elems % 8) {
                default:
                    assert(0 && "This Should never be executed!");
                    break;
                case 0:
                    do {
                        // This macro saves repeating the same line 8 times
#define DUFF_GUTS       *_dest = *_src++; _dest += width;

                        DUFF_GUTS
                        case 7:
                        DUFF_GUTS
                        case 6:
                        DUFF_GUTS
                        case 5:
                        DUFF_GUTS
                        case 4:
                        DUFF_GUTS
                        case 3:
                        DUFF_GUTS
                        case 2:
                        DUFF_GUTS
                        case 1:
                        DUFF_GUTS
                    } while (--duffs_index > 0);
                } /* end switch */
            } /* end block */
#endif /* DUFFS_DEVICE */

        } /* end for i = 0 to width*/

        /* Compute the leftover bytes if there are any */
        size_t leftover = src_size % width;

        /* Add leftover to the end of data */
        if (leftover > 0) {
            /* Adjust back to end of shuffled bytes */
            _dest -= (width - 1); /*lint !e794 _dest is initialized */
            memcpy((void*) _dest, (void*) _src, leftover);
        }
    } /* end if width and elems both > 1 */
}

/**
 * @brief parse the chunk position string
 *
 * Extract information from the chunk position string and store as a
 * vector of integers in the instance
 *
 * @note If we can change the DMR++ syntax to be less verbose and use
 * a list of ints with whitespace as a separator, then the parsing code
 * will be much simpler since istringstream is designed to deal with exactly
 * that form of input.
 *
 * @param pia The chunk position string. Syntax parsed: "[1,2,3,4]"
 */
void Chunk::set_position_in_array(const string &pia)
{
    if (pia.empty()) return;

    if (d_chunk_position_in_array.size()) d_chunk_position_in_array.clear();

    // Assume input is [x,y,...,z] where x, ..., are integers; modest syntax checking
    // [1] is a minimal 'position in array' string.
    if (pia.find('[') == string::npos || pia.find(']') == string::npos || pia.length() < 3)
        throw BESInternalError("while parsing a DMR++, chunk position string malformed", __FILE__, __LINE__);

    if (pia.find_first_not_of("[]1234567890,") != string::npos)
        throw BESInternalError("while parsing a DMR++, chunk position string illegal character(s)", __FILE__, __LINE__);

    // string off []; iss holds x,y,...,z
    istringstream iss(pia.substr(1, pia.length()-2));

    char c;
    unsigned int i;
    while (!iss.eof() ) {
        iss >> i; // read an integer

        d_chunk_position_in_array.push_back(i);

        iss >> c; // read a separator (,)
    }
}

/**
 * @brief Set the chunk's position in the Array
 *
 * Use this method when the vector<unsigned int> is known.
 *
 * @see Chunk::set_position_in_array(const string &pia)
 * @param pia A vector of unsigned ints.
 */
void Chunk::set_position_in_array(const std::vector<unsigned int> &pia)
{
    if (pia.size() == 0) return;

    if (d_chunk_position_in_array.size()) d_chunk_position_in_array.clear();

    d_chunk_position_in_array = pia;
}

/**
 * @brief Returns a curl range argument.
 * The libcurl requires a string argument for range-ge activitys, this method
 * constructs one in the required syntax from the offset and size information
 * for this byteStream.
 *
 */
string Chunk::get_curl_range_arg_string()
{
    ostringstream range;   // range-get needs a string arg for the range
    range << d_offset << "-" << d_offset + d_size - 1;
    return range.str();
}

/**
 * @brief Modify this chunk's data URL so that it includes tracking info
 *
 * Add information to the Query string of a URL, intended primarily to aid in
 * tracking the origin of requests when reading data from S3. The information
 * added to the query string comes from a BES Context command sent to the BES
 * by a client (e.g., the OLFS). The addition takes the form
 * "?tracking_context=<context value>".
 *
 * @note This is only added to data URLs that reference S3.
 */
void Chunk::add_tracking_query_param()
{
    /** - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     * Cloudydap test hack where we tag the S3 URLs with a query string for the S3 log
     * in order to track S3 requests. The tag is submitted as a BESContext with the
     * request. Here we check to see if the request is for an AWS S3 object, if
     * it is AND we have the magic BESContext "cloudydap" then we add a query
     * parameter to the S3 URL for tracking purposes.
     *
     * Should this be a function? FFS why? This is the ONLY place where this needs
     * happen, as close to the curl call as possible and we can just turn it off
     * down the road. - ndp 1/20/17 (EOD)
     *
     * Well, it's a function now... ;-) jhrg 8/6/18
     */

    string aws_s3_url_https("https://s3.amazonaws.com/");
    string aws_s3_url_http("http://s3.amazonaws.com/");

    // Is it an AWS S3 access? (y.find(x) returns 0 when y starts with x)
    if (d_data_url.find(aws_s3_url_https) == 0 || d_data_url.find(aws_s3_url_http) == 0) {
        // Yup, headed to S3.
        bool found = false;
        string cloudydap_context_value = BESContextManager::TheManager()->get_context(tracking_context, found);
        if (found) {
            d_query_marker.append("?").append(tracking_context).append("=").append(cloudydap_context_value);
        }
    }
}

/**
 * @brief function version of Chunk::inflate_chunk for use with pthreads
 *
 * @note Only use this with child threads
 * @todo Rewrite this as glue to the method?
 *
 * @param arg_list Pointer to an inflate_chunk_args instance. That struct contains
 * The Chunk object, booleans that describe if the chunk is compressed or shuffled,
 * the expected chunk size and the element size (chunk size is in elements, not bytes).
 * @see Chunk::inflate_chunk()
 */
void *inflate_chunk(void *arg_list /*Chunk *chunk, bool deflate, bool shuffle, unsigned int chunk_size, unsigned int elem_width*/)
{
    // This code is pretty naive - there are apparently a number of
    // different ways HDF5 can compress data, and it does also use a scheme
    // where several algorithms can be applied in sequence. For now, get
    // simple zlib deflate working.jhrg 1/15/17
    // Added support for shuffle. Assuming unshuffle always is applied _after_
    // inflating the data (reversing the shuffle --> deflate process). It is
    // possible that data could just be deflated or shuffled (because we
    // have test data are use only shuffle). jhrg 1/20/17
    // The file that implements the deflate filter is H5Zdeflate.c in the hdf5 source.
    // The file that implements the shuffle filter is H5Zshuffle.c.

    inflate_chunk_args *args = reinterpret_cast<inflate_chunk_args*>(arg_list);

    if (args->chunk->get_is_inflated()) {
#if USE_PTHREADS
        pthread_exit(args);
#else
        // This should only be called using pthread_create(). This is here because I
        // tested this code in the main thread. jhrg 8/19/18
        return NULL;
#endif
    }

    args->chunk_size *= args->elem_width;

    if (args->deflate) {
        char *dest = new char[args->chunk_size];
        try {
            inflate(dest, args->chunk_size, args->chunk->get_rbuf(), args->chunk->get_rbuf_size());
            // This replaces (and deletes) the original read_buffer with dest.
            args->chunk->set_rbuf(dest, args->chunk_size);
        }
        catch (...) {
            delete[] dest;
            throw;
        }
    }

    if (args->shuffle) {
        // The internal buffer is chunk's full size at this point.
        char *dest = new char[args->chunk->get_rbuf_size()];
        try {
            unshuffle(dest, args->chunk->get_rbuf(), args->chunk->get_rbuf_size(), args->elem_width);
            args->chunk->set_rbuf(dest, args->chunk->get_rbuf_size());
        }
        catch (...) {
            delete[] dest;
            throw;
        }
    }

    args->chunk->set_is_inflated(true);

#if USE_PTHREADS
    pthread_exit(args);
#else
    return NULL;
#endif
}

/**
 * @brief Decompress data in the chunk, managing the Chunk's data buffers
 *
 * This method tracks if a chunk has already been decompressed, so, like read_chunk()
 * it can be called for a chunk that has already been decompressed without error.
 *
 * @param deflate True if the chunk should be 'inflated'
 * @param shuffle True if the chunk should be 'unshuffled'
 * @param chunk_size The _expected_ chunk size, in elements; used to allocate storage
 * @param elem_width The number of bytes per element
 */
void Chunk::inflate_chunk(bool deflate, bool shuffle, unsigned int chunk_size, unsigned int elem_width)
{
    // This code is pretty naive - there are apparently a number of
    // different ways HDF5 can compress data, and it does also use a scheme
    // where several algorithms can be applied in sequence. For now, get
    // simple zlib deflate working.jhrg 1/15/17
    // Added support for shuffle. Assuming unshuffle always is applied _after_
    // inflating the data (reversing the shuffle --> deflate process). It is
    // possible that data could just be deflated or shuffled (because we
    // have test data are use only shuffle). jhrg 1/20/17
    // The file that implements the deflate filter is H5Zdeflate.c in the hdf5 source.
    // The file that implements the shuffle filter is H5Zshuffle.c.

    if (d_is_inflated)
        return;

    chunk_size *= elem_width;

    if (deflate) {
        char *dest = new char[chunk_size];
        try {
            inflate(dest, chunk_size, get_rbuf(), get_rbuf_size());
            // This replaces (and deletes) the original read_buffer with dest.
            set_rbuf(dest, chunk_size);
        }
        catch (...) {
            delete[] dest;
            throw;
        }
    }

    if (shuffle) {
        // The internal buffer is chunk's full size at this point.
        char *dest = new char[get_rbuf_size()];
        try {
            unshuffle(dest, get_rbuf(), get_rbuf_size(), elem_width);
            set_rbuf(dest, get_rbuf_size());
        }
        catch (...) {
            delete[] dest;
            throw;
        }
    }

    d_is_inflated = true;

#if 0 // This was handy during development for debugging. Keep it for awhile (year or two) before we drop it ndp - 01/18/17
    if(BESDebug::IsSet("dmrpp")) {
        unsigned long long chunk_buf_size = get_rbuf_size();
        dods_float32 *vals = (dods_float32 *) get_rbuf();
        ostream *os = BESDebug::GetStrm();
        (*os) << std::fixed << std::setfill('_') << std::setw(10) << std::setprecision(0);
        (*os) << "DmrppArray::"<< __func__ <<"() - Chunk[" << i << "]: " << endl;
        for(unsigned long long k=0; k< chunk_buf_size/prototype()->width(); k++) {
            (*os) << vals[k] << ", " << ((k==0)|((k+1)%10)?"":"\n");
        }
    }
#endif
}

/**
 * This method is for reading one chunk after the other, using a CURL handle
 * from the CurlHandlePool.
 *
 * @param deflate
 * @param shuffle
 * @param chunk_size
 * @param elem_width
 */
void Chunk::read_chunk()
{
    if (d_is_read) {
        BESDEBUG("dmrpp", "Chunk::"<< __func__ <<"() - Already been read! Returning." << endl);
        return;
    }

    set_rbuf_to_size();

    dmrpp_easy_handle *handle = DmrppRequestHandler::curl_handle_pool->get_easy_handle(this);
    if (!handle)
        throw BESInternalError("No more libcurl handles.", __FILE__, __LINE__);

    handle->read_data();  // throws BESInternalError if error

    DmrppRequestHandler::curl_handle_pool->release_handle(handle);

    // If the expected byte count was not read, it's an error.
    if (get_size() != get_bytes_read()) {
        ostringstream oss;
        oss << "Wrong number of bytes read for chunk; read: " << get_bytes_read() << ", expected: " << get_size();
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }

    d_is_read = true;
}

/**
 *
 *  unsigned long long d_size;
 *  unsigned long long d_offset;
 *  std::string d_md5;
 *  std::string d_uuid;
 *  std::string d_data_url;
 *  std::vector<unsigned int> d_chunk_position_in_array;
 *
 */
void Chunk::dump(ostream &oss) const
{
    oss << "Chunk";
    oss << "[ptr='" << (void *)this << "']";
    oss << "[data_url='" << d_data_url << "']";
    oss << "[offset=" << d_offset << "]";
    oss << "[size=" << d_size << "]";
    oss << "[chunk_position_in_array=(";
    for (unsigned long i = 0; i < d_chunk_position_in_array.size(); i++) {
        if (i) oss << ",";
        oss << d_chunk_position_in_array[i];
    }
    oss << ")]";
    oss << "[is_read=" << d_is_read << "]";
    oss << "[is_inflated=" << d_is_inflated << "]";
}

string Chunk::to_string() const
{
    std::ostringstream oss;
    dump(oss);
    return oss.str();
}

} // namespace dmrpp

