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

#include "config.h"

#include <string.h>

#include <string>
#include <cassert>

#include <curl/curl.h>

#include <zlib.h>

#include <BESError.h>
#include <BESDebug.h>

#include "DmrppCommon.h"
#include "H4ByteStream.h"
#include "DmrppUtil.h"

using namespace std;

namespace dmrpp {

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
static size_t h4bytestream_write_data(void *buffer, size_t size, size_t nmemb, void *data)
{
    H4ByteStream *h4bs = reinterpret_cast<H4ByteStream*>(data);

    // rbuf: |******++++++++++----------------------|
    //              ^        ^ bytes_read + nbytes
    //              | bytes_read

    unsigned long long bytes_read = h4bs->get_bytes_read();
    size_t nbytes = size * nmemb;

    BESDEBUG("dmrpp",
            "h4bytestream_write_data() - bytes_read: " << bytes_read << ", nbytes: " << nbytes << ", rbuf_size: " << h4bs->get_rbuf_size() << endl);

    // If this fails, the code will write beyond the buffer.
    assert(bytes_read + nbytes <= h4bs->get_rbuf_size());

    memcpy(h4bs->get_rbuf() + bytes_read, buffer, nbytes);

    h4bs->set_bytes_read(bytes_read + nbytes);

    return nbytes;
}

/**
 * @brief Read data using HTTP/File Range GET
 *
 * @see https://curl.haxx.se/libcurl/c/libcurl.html
 * @param url Get dat from this URL
 * @param range ...and this byte range
 * @param user_data A pinter to a DmrppCommon instance
 */
void curl_read_byte_stream(const string &url, const string &range, void *user_data)
{
    // See https://curl.haxx.se/libcurl/c/CURLOPT_RANGE.html, etc.

    CURL* curl = curl_easy_init();
    if (curl) {
        CURLcode res = curl_easy_setopt(curl, CURLOPT_URL, url.c_str() /*"http://example.com"*/);
        if (res != CURLE_OK) throw BESError(string(curl_easy_strerror(res)), BES_INTERNAL_ERROR, __FILE__, __LINE__);

        // Use CURLOPT_ERRORBUFFER for a human-readable message
        char buf[CURL_ERROR_SIZE];
        res = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, buf);
        if (res != CURLE_OK) throw BESError(string(curl_easy_strerror(res)), BES_INTERNAL_ERROR, __FILE__, __LINE__);

        // get the offset to offset + size bytes
        if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str() /*"0-199"*/)) throw BESError(
                string("HTTP Error: ").append(buf), BES_INTERNAL_ERROR, __FILE__, __LINE__);

        // Pass all data to the 'write_data' function
        if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, h4bytestream_write_data)) throw BESError(
                string("HTTP Error: ").append(buf), BES_INTERNAL_ERROR, __FILE__, __LINE__);

        // Pass this to write_data as the fourth argument
        if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEDATA, user_data)) throw BESError(
                string("HTTP Error: ").append(buf), BES_INTERNAL_ERROR, __FILE__, __LINE__);

        // Perform the request
        if (CURLE_OK != curl_easy_perform(curl)) {
            curl_easy_cleanup(curl);
            throw BESError(string("HTTP Error: ").append(buf), BES_INTERNAL_ERROR, __FILE__, __LINE__);
        }

        curl_easy_cleanup(curl);
    }
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
        throw BESError("Failed to initialize deflate software.", BES_INTERNAL_ERROR, __FILE__, __LINE__);

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
            throw BESError("Failed to deflate data chunk.", BES_INTERNAL_ERROR, __FILE__, __LINE__);
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
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0, "memory allocation failed for deflate uncompression")
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

// TODO #define this to enable the duff's device loop unrolling code.
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
void unshuffle(unsigned char *dest, const unsigned char *src, unsigned int src_size, unsigned int width)
{
    unsigned int elems = src_size / width;  // int division rounds down

    /* Don't do anything for 1-byte elements, or "fractional" elements */
    if (!(width > 1 && elems > 1)) {
        memcpy(dest, const_cast<unsigned char*>(src), src_size);
    }
    else {
        /* Get the pointer to the source buffer (Alias for source buffer) */
        unsigned char *_src = const_cast<unsigned char*>(src);
        unsigned char *_dest = 0;   // Alias for destination buffer

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

}    // namespace dmrpp
