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
#include <cstring>

#include <zlib.h>

#include <BESDebug.h>
#include <BESLog.h>
#include <BESInternalError.h>
#include <BESSyntaxUserError.h>
#include <BESForbiddenError.h>
#include <BESContextManager.h>
#include <BESUtil.h>

#define PUGIXML_NO_XPATH
#define PUGIXML_HEADER_ONLY
#include <pugixml.hpp>

#include "Chunk.h"
#include "CurlUtils.h"
#include "CurlHandlePool.h"
#include "EffectiveUrlCache.h"
#include "DmrppRequestHandler.h"
#include "DmrppNames.h"
#include "byteswap_compat.h"
#include "float_byteswap.h"

using namespace std;
using http::EffectiveUrlCache;

#define prolog std::string("Chunk::").append(__func__).append("() - ")

#define FLETCHER32_CHECKSUM 4               // Bytes in the fletcher32 checksum
#define ACTUALLY_USE_FLETCHER32_CHECKSUM 1  // Computing checksums takes time...

namespace dmrpp {

/**
 * @brief Read the response headers, save the Content-Type header
 * Look at the HTTP response headers and save the value of the Content-Type
 * header in the chunk object. This is a callback set up for the
 * curl easy handle.
 *
 * @param buffer One header line
 * @param size Always 1
 * @param nitems Number of bytes in this header line
 * @param data Pointer to the user data, configured using
 * @return The number of bytes read
 */
size_t chunk_header_callback(char *buffer, size_t /*size*/, size_t nitems, void *data) {
    // received header is nitems * size long in 'buffer' NOT ZERO TERMINATED
    // 'userdata' is set with CURLOPT_HEADERDATA
    // 'size' is always 1

    // -2 strips of the CRLF at the end of the header
    string header(buffer, buffer + nitems - 2);

    // Look for the content type header and store its value in the Chunk
    if (header.find("Content-Type") != string::npos) {
        // Header format 'Content-Type: <value>'
        auto c_ptr = reinterpret_cast<Chunk *>(data);
        c_ptr->set_response_content_type(header.substr(header.find_last_of(' ') + 1));
    }

    return nitems;
}

/**
 * @brief Extract something useful from the S3 error response, throw a BESError
 * @param data_url
 * @param xml_message
 */
void process_s3_error_response(const shared_ptr<http::url> &data_url, const string &xml_message)
{
    // See https://docs.aws.amazon.com/AmazonS3/latest/API/ErrorResponses.html
    // for the low-down on this XML document.
    pugi::xml_document error;
    pugi::xml_parse_result result = error.load_string(xml_message.c_str());
    if (!result)
        throw BESInternalError("The underlying data store returned an unintelligible error message.", __FILE__, __LINE__);

    pugi::xml_node err_elmnt = error.document_element();
    if (!err_elmnt || (strcmp(err_elmnt.name(), "Error") != 0))
        throw BESInternalError("The underlying data store returned a bogus error message.", __FILE__, __LINE__);

    string code = err_elmnt.child_value("Code");
    string message = err_elmnt.child_value("Message");

    // We might want to get the "Code" from the "Error" if these text messages
    // are not good enough. But the "Code" is not really suitable for normal humans...
    // jhrg 12/31/19

    if (code == "AccessDenied") {
        stringstream msg;
        msg << prolog << "ACCESS DENIED - The underlying object store has refused access to: "
            << data_url->protocol() << data_url->host() << data_url->path() << " Object Store Message: "
            << message;
        BESDEBUG(MODULE, msg.str() << endl);
        VERBOSE(msg.str() << endl);
        throw BESForbiddenError(msg.str(), __FILE__, __LINE__);
    }
    else {
        stringstream msg;
        msg << prolog << "The underlying object store returned an error. " << "(Tried: " << data_url->protocol()
            << "://" << data_url->host() << data_url->path() << ") Object Store Message: " << message;
        BESDEBUG(MODULE, msg.str() << endl);
        VERBOSE(msg.str() << endl);
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }
}

/**
 * @brief Callback passed to libcurl to handle reading bytes.
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
size_t chunk_write_data(void *buffer, size_t size, size_t nmemb, void *data) {
    BESDEBUG(MODULE, prolog << "BEGIN " << endl);
    size_t nbytes = size * nmemb;
    auto chunk = reinterpret_cast<Chunk *>(data);


    auto data_url = chunk->get_data_url();
    BESDEBUG(MODULE, prolog << "chunk->get_data_url():" << data_url << endl);

    // When Content-Type is 'application/xml,' that's an error. jhrg 6/9/20
    BESDEBUG(MODULE, prolog << "chunk->get_response_content_type():" << chunk->get_response_content_type() << endl);
    if (chunk->get_response_content_type().find("application/xml") != string::npos) {
        // At this point we no longer care about great performance - error msg readability
        // is more important. jhrg 12/30/19
        string xml_message = reinterpret_cast<const char *>(buffer);
        xml_message.erase(xml_message.find_last_not_of("\t\n\v\f\r 0") + 1);
        // Decode the AWS XML error message. In some cases this will fail because pub keys,
        // which maybe in this error text, may have < or > chars in them. the XML parser
        // will be sad if that happens. jhrg 12/30/19
        try {
            process_s3_error_response(data_url, xml_message);   // throws a BESError
        }
        catch (BESError) {
            // re-throw any BESError - added for the future if we make BESError a child
            // of std::exception as it should be. jhrg 12/30/19
            throw;
        }
        catch (std::exception &e) {
            stringstream msg;
            msg << prolog << "Caught std::exception when accessing object store data.";
            msg << " (Tried: " << data_url->str() << ")" << " Message: " << e.what();
            BESDEBUG(MODULE, msg.str() << endl);
            throw BESSyntaxUserError(msg.str(), __FILE__, __LINE__);
        }
    }

    // rbuf: |******++++++++++----------------------|
    //              ^        ^ bytes_read + nbytes
    //              | bytes_read

    unsigned long long bytes_read = chunk->get_bytes_read();

    // If this fails, the code will write beyond the buffer.
    if (bytes_read + nbytes > chunk->get_rbuf_size()) {
        stringstream msg;
        msg << prolog << "ERROR! The number of bytes_read: " << bytes_read << " plus the number of bytes to read: "
            << nbytes << " is larger than the target buffer size: " << chunk->get_rbuf_size();
        BESDEBUG(MODULE, msg.str() << endl);
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }

    memcpy(chunk->get_rbuf() + bytes_read, buffer, nbytes);
    chunk->set_bytes_read(bytes_read + nbytes);
    
    BESDEBUG(MODULE, prolog << "END" << endl);

    return nbytes;
}

/**
 * @brief Throws an exception if the information sent to the inflate function is invalid.
 *
 * @param destp The destination buffer
 * @param dest_len The number of bytes to inflate into
 * @param src The source buffer
 * @param src_len The number of bytes to inflate
 */
static void inflate_sanity_check(char **destp, unsigned long long dest_len, const char *src, unsigned long long src_len) {
    if (src_len == 0) {
        string msg = prolog + "ERROR! The number of bytes to inflate is zero.";
        BESDEBUG(MODULE, msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
    if (dest_len == 0) {
        string msg = prolog + "ERROR! The number of bytes to inflate into is zero.";
        BESDEBUG(MODULE, msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
    if (!destp || !*destp) {
        string msg = prolog + "ERROR! The destination buffer is NULL.";
        BESDEBUG(MODULE, msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
    if (!src) {
        string msg = prolog + "ERROR! The source buffer is NULL.";
        BESDEBUG(MODULE, msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
}

/**
 * @brief Deflate data. This is the zlib algorithm.
 *
 * @note Stolen from the HDF5 library and hacked to fit.
 *
 * @param destp A value-result parameter (pointer to a pointer) of the 'inflated' data 
 * @param dest_len Size of the destination buffer
 * @param src Compressed data
 * @param src_len Size of the compressed data
 * @return The number of bytes of the inflated data
 */
unsigned long long inflate(char **destp, unsigned long long dest_len, char *src, unsigned long long src_len) {
    inflate_sanity_check(destp, dest_len, src, src_len);

    /* Input; uncompress */
    z_stream z_strm; /* zlib parameters */

    /* Set the decompression parameters */
    memset(&z_strm, 0, sizeof(z_strm));
    z_strm.next_in = (Bytef *) src;
    z_strm.avail_in = src_len;
    z_strm.next_out = (Bytef *) (*destp);
    z_strm.avail_out = dest_len;

    size_t nalloc = dest_len;

    char *outbuf = *destp;

    /* Initialize the decompression routines */
    if (Z_OK != inflateInit(&z_strm))
        throw BESError("Failed to initialize inflate software.", BES_INTERNAL_ERROR, __FILE__, __LINE__);

    /* Loop to uncompress the buffer */
    int status = Z_OK;
    do {
        /* Uncompress some data */
        status = inflate(&z_strm, Z_SYNC_FLUSH);

        /* Check if we are done decompressing data */
        if (Z_STREAM_END == status) break; /*done*/

        /* Check for error */
        if (Z_OK != status) {
            stringstream err_msg;
            err_msg << "Failed to inflate data chunk.";
            char const *err_msg_cstr = z_strm.msg;
            if(err_msg_cstr)
                err_msg << " zlib message: " << err_msg_cstr;
            (void) inflateEnd(&z_strm);
            throw BESError(err_msg.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
        }
        else {
            // If we're not done and just ran out of buffer space, we need to extend the buffer.
            // We may encounter this case when the deflate filter is used twice. KY 2022-08-03
            if (0 == z_strm.avail_out) {

                /* Allocate a buffer twice as big */
                size_t outbuf_size = nalloc;
                nalloc *= 2;
                char *new_outbuf = new char[nalloc];
                memcpy((void*)new_outbuf,(void*)outbuf,outbuf_size);
                delete[] outbuf;
                outbuf = new_outbuf;

                /* Update pointers to buffer for next set of uncompressed data */
                z_strm.next_out = (unsigned char*) outbuf + z_strm.total_out;
                z_strm.avail_out = (uInt) (nalloc - z_strm.total_out);

            } /* end if */
        } /* end else */
    } while (true /* status == Z_OK */);    // Exit via the break statement after the call to inflate(). jhrg 11/8/21

    *destp = outbuf;
    outbuf = nullptr;
    /* Finish decompressing the stream */
    (void) inflateEnd(&z_strm);

    return z_strm.total_out;
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
void unshuffle(char *dest, const char *src, unsigned long long src_size, unsigned long long width) {
    unsigned long long elems = src_size / width;  // int division rounds down

    /* Don't do anything for 1-byte elements, or "fractional" elements */
    if (!(width > 1 && elems > 1)) {
        memcpy(dest, const_cast<char *>(src), src_size);
    }
    else {
        /* Get the pointer to the source buffer (Alias for source buffer) */
        char *_src = const_cast<char *>(src);
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
                        throw BESError("Internal error in unshuffle().", BES_INTERNAL_ERROR, __FILE__, __LINE__);

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
            memcpy((void *) _dest, (void *) _src, leftover);
        }
    } /* end if width and elems both > 1 */
}

/// Stolen from our friends at Stack Overflow and modified for our use.
/// This is far faster than the istringstream code it replaces (for one
/// test, run time for parse_chunk_position_in_array_string() dropped from
/// 20ms to ~3ms). It also fixes a test we could never get to pass.
/// jhrg 11/5/21
static void split_by_comma(const string &s, vector<unsigned long long> &res)
{
    const string delimiter = ",";
    const size_t delim_len = delimiter.size();

    size_t pos_start = 0, pos_end;

    while ((pos_end = s.find (delimiter, pos_start)) != string::npos) {
        res.push_back (stoull(s.substr(pos_start, pos_end - pos_start)));
        pos_start = pos_end + delim_len;
    }

    res.push_back (stoull(s.substr (pos_start)));
}

void Chunk::parse_chunk_position_in_array_string(const string &pia, vector<unsigned long long> &cpia_vect)
{
    if (pia.empty()) return;

    if (!cpia_vect.empty()) cpia_vect.clear();

    // Assume input is [x,y,...,z] where x, ..., are integers; modest syntax checking
    // [1] is a minimal 'position in array' string.
    if (pia.find('[') == string::npos || pia.find(']') == string::npos || pia.size() < 3)
        throw BESInternalError("while parsing a DMR++, chunk position string malformed", __FILE__, __LINE__);

    if (pia.find_first_not_of("[]1234567890,") != string::npos)
        throw BESInternalError("while parsing a DMR++, chunk position string illegal character(s)", __FILE__, __LINE__);

    try {
        split_by_comma(pia.substr(1, pia.size() - 2), cpia_vect);
    }
    catch(const std::invalid_argument &e) {
        throw BESInternalError(string("while parsing a DMR++, chunk position string illegal character(s): ").append(e.what()), __FILE__, __LINE__);
    }
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
void Chunk::set_position_in_array(const string &pia) {
    parse_chunk_position_in_array_string(pia,d_chunk_position_in_array);
}

/**
 * @brief Set the chunk's position in the Array
 *
 * Use this method when the vector<unsigned long long> is known.
 *
 * @see Chunk::set_position_in_array(const string &pia)
 * @param pia A vector of unsigned ints.
 */
void Chunk::set_position_in_array(const std::vector<unsigned long long> &pia) {
    if (pia.empty()) return;

    if (!d_chunk_position_in_array.empty()) d_chunk_position_in_array.clear();

    d_chunk_position_in_array = pia;
}

/**
 * @brief Returns a curl range argument.
 * The libcurl requires a string argument for range-ge activitys, this method
 * constructs one in the required syntax from the offset and size information
 * for this byteStream.
 *
 */
string Chunk::get_curl_range_arg_string() {
    return curl::get_range_arg_string(d_offset, d_size);
}

/**
 * @brief Modify this chunk's data URL so that it includes tracking info
 *
 * Add information to the Query string of a URL, intended primarily to aid in
 * tracking the origin of requests when reading data from S3. The information
 * added to the query string comes from a BES Context command sent to the BES
 * by a client (e.g., the OLFS). The addition takes the form
 * "tracking_context=<context value>". The method checks to see if the URL
 * already has a query string, if not it adds one: "?tracking_context=<context value>"
 * And if so it appends an additional parameter: "&tracking_context=<context value>"
 *
 * @note This is only added to data URLs that reference an S3 bucket.
 *
 * @note This adds a significant cost to the run-time behavior of the DMR++ code
 * and should only be used for testing and, even then, probably rarely since the
 * performance of the server will be reduced significantly.
 */
void Chunk::add_tracking_query_param() {

    // If there is no data url then there is nothing to add the parameter too.
    if(d_data_url == nullptr)
        return;

    bool found = false;
    string cloudydap_context_value = BESContextManager::TheManager()->get_context(S3_TRACKING_CONTEXT, found);
    if (!found)
        return;

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

    bool add_tracking = false;

    // All S3 buckets, virtual host style URL
    // Simpler regex that's likely equivalent:
    // ^https?:\/\/[a-z0-9]([-.a-z0-9]){1,61}[a-z0-9]\.s3[-.]us-(east|west)-[12])?\.amazonaws\.com\/.*$
    string s3_vh_regex_str = R"(^https?:\/\/([a-z]|[0-9])(([a-z]|[0-9]|\.|-){1,61})([a-z]|[0-9])\.s3((\.|-)us-(east|west)-(1|2))?\.amazonaws\.com\/.*$)";

    BESRegex s3_vh_regex(s3_vh_regex_str.c_str());
    int match_result = s3_vh_regex.match(d_data_url->str().c_str(), d_data_url->str().size());
    if(match_result>=0) {
        auto match_length = (unsigned int) match_result;
        if (match_length == d_data_url->str().size()) {
            BESDEBUG(MODULE,
                     prolog << "FULL MATCH. pattern: " << s3_vh_regex_str << " url: " << d_data_url->str() << endl);
            add_tracking = true;;
        }
    }

    if(!add_tracking){
        // All S3 buckets, path style URL
        string  s3_path_regex_str = R"(^https?:\/\/s3((\.|-)us-(east|west)-(1|2))?\.amazonaws\.com\/([a-z]|[0-9])(([a-z]|[0-9]|\.|-){1,61})([a-z]|[0-9])\/.*$)";
        BESRegex s3_path_regex(s3_path_regex_str.c_str());
        match_result = s3_path_regex.match(d_data_url->str().c_str(), d_data_url->str().size());
        if(match_result>=0) {
            auto match_length = (unsigned int) match_result;
            if (match_length == d_data_url->str().size()) {
                BESDEBUG(MODULE,
                         prolog << "FULL MATCH. pattern: " << s3_vh_regex_str << " url: " << d_data_url->str() << endl);
                add_tracking = true;;
            }
        }
    }

    if (add_tracking) {
        // Yup, headed to S3.
        d_query_marker.append(S3_TRACKING_CONTEXT).append("=").append(cloudydap_context_value);
    }
}

static void checksum_fletcher32_sanity_check(const void *_data, size_t _len) {
    // Sanity check
    if (!_data) {
        string msg = prolog + "ERROR! checksum_fletcher32_sanity_check: _data is NULL";
        BESDEBUG(MODULE, msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);

    }
    if (_len == 0) {
        string msg = prolog + "ERROR! checksum_fletcher32_sanity_check: _len is 0";
        BESDEBUG(MODULE, msg << endl);
        throw BESInternalError(msg, __FILE__, __LINE__);
    }
}
/**
 * @brief Compute the Fletcher32 checksum for a block of bytes
 * @param _data Pointer to a block of byte data
 * @param _len Number of bytes to checksum
 * @return The Fletcher32 checksum
 * Credit: The following code is adapted from the HDF5 library
 */
uint32_t
checksum_fletcher32(const void *_data, size_t _len)
{
    checksum_fletcher32_sanity_check(_data, _len);

    const auto *data = (const uint8_t *)_data;  // Pointer to the data to be summed
    size_t len = _len / 2;                      // Length in 16-bit words
    uint32_t sum1 = 0, sum2 = 0;

    // Compute checksum for pairs of bytes
    // (the magic "360" value is the largest number of sums that can be performed without numeric overflow)
    while (len) {
        size_t tlen = len > 360 ? 360 : len;
        len -= tlen;
        do {
            sum1 += (uint32_t)(((uint16_t)data[0]) << 8) | ((uint16_t)data[1]);
            data += 2;
            sum2 += sum1;
        } while (--tlen);
        sum1 = (sum1 & 0xffff) + (sum1 >> 16);
        sum2 = (sum2 & 0xffff) + (sum2 >> 16);
    }

    /* Check for odd # of bytes */
    if(_len % 2) {
        sum1 += (uint32_t)(((uint16_t)*data) << 8);
        sum2 += sum1;
        sum1 = (sum1 & 0xffff) + (sum1 >> 16);
        sum2 = (sum2 & 0xffff) + (sum2 >> 16);
    } /* end if */

    /* Second reduction step to reduce sums to 16 bits */
    sum1 = (sum1 & 0xffff) + (sum1 >> 16);
    sum2 = (sum2 & 0xffff) + (sum2 >> 16);

    return ((sum2 << 16) | sum1);
} /* end checksum_fletcher32() */

/**
 * @brief filter data in the chunk
 *
 * This method tracks if a chunk has already been decompressed, so, like read_chunk()
 * it can be called for a chunk that has already been decompressed without error.
 *
 * @param filters Space separated list of filters
 * @param chunk_size The _expected_ chunk size, in elements; used to allocate storage
 * @param elem_width The number of bytes per element
 */
void Chunk::filter_chunk(const string &filters, unsigned long long chunk_size, unsigned long long elem_width) {

    if (d_is_inflated)
        return;

    chunk_size *= elem_width;

    vector<string> filter_array = BESUtil::split(filters, ' ' );

    // We need to check if the filters that include the deflate filters are contiguous.
    // That is: the filters must be something like "deflate deflate deflate" instead of "deflate other_filter deflate"

    bool is_1st_deflate = true;
    unsigned cur_deflate_index = 0;
    unsigned num_deflate = 0;

    for (unsigned i = 0; i<filter_array.size(); i++) {

        if (filter_array[i] == "deflate") {
            if (is_1st_deflate == true) {
                cur_deflate_index = i;
                is_1st_deflate = false;
            }
            else if (i != (cur_deflate_index+1)) {
                throw BESInternalError("The deflate filters must be adjacent to each other",
                                       __FILE__, __LINE__);
            }
            else 
                cur_deflate_index = i;
            num_deflate++;
        }

    }

    // If there are >1 deflate filters applied, we want to localize the handling of the compressed data  in this function.
    unsigned deflate_index = 0;
    unsigned long long out_buf_size = 0;
    unsigned long long in_buf_size = 0;
    char**destp = nullptr;
    char* dest_deflate = nullptr;
    char* tmp_dest = nullptr;

    bool ignore_rest_deflate = false;

    for (auto i = filter_array.rbegin(), e = filter_array.rend(); i != e; ++i) {

        string filter = *i;

        if (filter == "deflate") {

            // Here we find that the deflate filter is applied twice. 
            // Note: we find one GHRSST file is using the deflate twice, 
            // However, for one chunk the deflate filter only applies once. 
            // The returned decompressed size of this chunk is equal to
            // the chunk size. So we have to skip the second "inflate" of this chunk by
            // checking if the inflated size is equal to the chunk size. KY 2022-08-07

            if (num_deflate > 1 && !ignore_rest_deflate) {

                dest_deflate = new char[chunk_size];
                try {
                    destp = &dest_deflate;
                    if (deflate_index == 0) {
                        // First inflate, receive the buffer and the corresponding info from
                        // the BES, save the inflated buffer into a tmp. buffer.
                        out_buf_size = inflate(destp, chunk_size, get_rbuf(), get_rbuf_size());
                        tmp_dest = *destp;
                    }
                    else {
                        // Obtain the buffer and the size locally starting from the second inflate.
                        // Remember to release the tmp_buf memory.
#if 0
                        tmp_buf = tmp_dest;
                        out_buf_size = inflate(destp, chunk_size, tmp_buf, in_buf_size);
                        tmp_dest = *destp;
                        delete[] tmp_buf;
#endif
                        out_buf_size = inflate(destp, chunk_size, tmp_dest, in_buf_size);
                        delete[] tmp_dest;
                        tmp_dest = *destp;

                    }
                    deflate_index ++;
                    in_buf_size = out_buf_size;
#if DMRPP_USE_SUPER_CHUNKS
                    // This is the last deflate filter, output the buffer.
                    if (in_buf_size == chunk_size)
                        ignore_rest_deflate = true;
                    if (ignore_rest_deflate || deflate_index == num_deflate) {
                        char* newdest = *destp;
                        // Need to use the out_buf_size insted of chunk_size since the size may be bigger than chunk_size. KY 2023-06-08
                        set_read_buffer(newdest, out_buf_size, chunk_size, true);
                    }
 
#else
                set_rbuf(dest_deflate, chunk_size);
#endif

                }
                catch (...) {
                    delete[] dest_deflate;
                    delete[] tmp_dest;
                    throw;
                }
 

            }
            else if(num_deflate == 1) {
                // The following is the same code as before. We need to use the double pointer
                // to pass the buffer. KY 2022-08-07
                dest_deflate = new char[chunk_size];
                destp = &dest_deflate;
                try {
                    out_buf_size = inflate(destp, chunk_size, get_rbuf(), get_rbuf_size());
                    if (out_buf_size == 0) {
                        throw BESError("inflate size should be greater than 0", BES_INTERNAL_ERROR, __FILE__, __LINE__);
                    }
                    // This replaces (and deletes) the original read_buffer with dest.
#if DMRPP_USE_SUPER_CHUNKS
                    char* new_dest=*destp;
                    set_read_buffer(new_dest, out_buf_size, chunk_size, true);
#else
                    set_rbuf(dest_deflate, chunk_size);
#endif
                }
                catch (...) {
                    delete[] dest_deflate;
                    throw;
                }
            }
        }// end filter is deflate
        else if (filter == "shuffle"){
            // The internal buffer is chunk's full size at this point.
            char *dest = new char[get_rbuf_size()];
            try {
                unshuffle(dest, get_rbuf(), get_rbuf_size(), elem_width);
#if DMRPP_USE_SUPER_CHUNKS
                set_read_buffer(dest,get_rbuf_size(),get_rbuf_size(), true);
#else
                set_rbuf(dest, get_rbuf_size());
#endif
            }
            catch (...) {
                delete[] dest;
                throw;
            }
        } //end filter is shuffle
        else if (filter == "fletcher32"){
            // Compute the fletcher32 checksum and compare to the value of the last four bytes of the chunk.
#if ACTUALLY_USE_FLETCHER32_CHECKSUM
            // Get the last four bytes of chunk's data (which is a byte array) and treat that as the four-byte
            // integer fletcher32 checksum. jhrg 10/15/21
            if (get_rbuf_size() <= FLETCHER32_CHECKSUM) {
                throw BESInternalError("fletcher32 filter: buffer size is less than the size of the checksum", __FILE__, __LINE__);
            }

            // Where is the checksum value?
            auto data_ptr = get_rbuf() + get_rbuf_size() - FLETCHER32_CHECKSUM;
            // Using a temporary variable ensures that the value is correctly positioned
            // on a 4 byte memory alignment. Casting data_ptr to a pointer to uint_32 does not.
            uint32_t f_checksum;
            memcpy(&f_checksum, data_ptr, FLETCHER32_CHECKSUM );

            // If the code should actually use the checksum (they can be expensive to compute), does it match
            // with once computed on the data actually read? Maybe make this a bes.conf parameter?
            // jhrg 10/15/21
            uint32_t calc_checksum = checksum_fletcher32((const void *)get_rbuf(), get_rbuf_size() - FLETCHER32_CHECKSUM);
            
            BESDEBUG(MODULE, prolog << "get_rbuf_size(): " << get_rbuf_size() << endl);
            BESDEBUG(MODULE, prolog << "calc_checksum: " << calc_checksum << endl);
            BESDEBUG(MODULE, prolog << "f_checksum: " << f_checksum << endl);
            if (f_checksum != calc_checksum) {
                throw BESInternalError("Data read from the DMR++ handler did not match the Fletcher32 checksum.",
                                       __FILE__, __LINE__);
            }
#endif
            if (d_read_buffer_size > FLETCHER32_CHECKSUM)
                d_read_buffer_size -= FLETCHER32_CHECKSUM;
            else {
                throw BESInternalError("Data filtered with fletcher32 don't include the four-byte checksum.",
                                       __FILE__, __LINE__);
            }
        } // end filter is fletcher32
    } // end for loop
    d_is_inflated = true;
}

static unsigned int get_value_size(libdap::Type type) 
{
    switch(type) {
        case libdap::dods_int8_c:
            return sizeof(int8_t);
            
        case libdap::dods_int16_c:
            return sizeof(int16_t);
            
        case libdap::dods_int32_c:
            return sizeof(int32_t);
            
        case libdap::dods_int64_c:
            return sizeof(int64_t);
            
        case libdap::dods_uint8_c:
        case libdap::dods_byte_c:
            return sizeof(uint8_t);
            
        case libdap::dods_uint16_c:
            return sizeof(uint16_t);
            
        case libdap::dods_uint32_c:
            return sizeof(uint32_t);
            
        case libdap::dods_uint64_c:
            return sizeof(uint64_t);

        case libdap::dods_float32_c:
            return sizeof(float);
            
        case libdap::dods_float64_c:
            return sizeof(double);

        default:
            throw BESInternalError("Unknown fill value type.", __FILE__, __LINE__);
    }
}

const char *get_value_ptr(fill_value &fv, libdap::Type type, const string &v, bool is_big_endian)
{
    switch(type) {
        case libdap::dods_int8_c:
            fv.int8 = (int8_t)stoi(v);
            return (const char *)&fv.int8;

        case libdap::dods_int16_c:
            fv.int16 = (int16_t)stoi(v);
            if (is_big_endian)
                fv.int16 = bswap_16(fv.int16);
            return (const char *)&fv.int16;

        case libdap::dods_int32_c:

            fv.int32 = (int32_t)stoi(v);
            if (is_big_endian)
                fv.int32 = bswap_32(fv.int32);
            return (const char *)&fv.int32;

        case libdap::dods_int64_c:
            fv.int64 = (int64_t)stoll(v);
            if (is_big_endian)
                fv.int64 = bswap_64(fv.int64);
           return (const char *)&fv.int64;

        case libdap::dods_uint8_c:
        case libdap::dods_byte_c:
            fv.uint8 = (uint8_t)stoi(v);
            return (const char *)&fv.uint8;

        case libdap::dods_uint16_c:
            fv.uint16 = (uint16_t)stoi(v);
            if (is_big_endian)
                fv.uint16 = bswap_16(fv.uint16);
           return (const char *)&fv.uint16;

        case libdap::dods_uint32_c:
            fv.uint32 = (uint32_t)stoul(v);
            if (is_big_endian)
                fv.uint32 = bswap_32(fv.uint32);
            return (const char *)&fv.uint32;

        case libdap::dods_uint64_c:
            fv.uint64 = (uint64_t)stoull(v);
            if (is_big_endian)
                fv.uint64 = bswap_32(fv.uint64);
            return (const char *)&fv.uint64;

        case libdap::dods_float32_c:
        {
            fv.f = stof(v);
            auto fv_float_p=(char *)&fv.f;
            if (is_big_endian) 
                swap_float32(fv_float_p,1);
            return (const char *)fv_float_p;
         }

        case libdap::dods_float64_c:
        {
            fv.d = stod(v);
            auto fv_double_p=(char *)&fv.d;
            if (is_big_endian)
                swap_float64 (fv_double_p,1);
            return (const char *)fv_double_p;
        }
        case libdap::dods_str_c:
            return v.c_str();

        default:
            throw BESInternalError("Unknown fill value type.", __FILE__, __LINE__);
    }
}

/**
 * @brief Load the chunk with fill values - temporary implementation
 */
void Chunk::load_fill_values() {

    fill_value fv;
    unsigned int value_size = 0;
    
    if (d_fill_value_type == libdap::dods_str_c) 
        value_size = (unsigned int)d_fill_value.size();
    else 
        value_size = get_value_size(d_fill_value_type);

    bool is_big_endian = false;
    if (d_byte_order == "BE")
        is_big_endian = true;
    const char *value = get_value_ptr(fv, d_fill_value_type, d_fill_value,is_big_endian);

    if(d_fill_value_type == libdap::dods_str_c && d_fill_value==""){
        d_fill_value = ' ';
        value_size = 1;
    }
    unsigned long long num_values = get_rbuf_size() / value_size;

    char *buffer = get_rbuf();

    for (unsigned long long  i = 0; i < num_values; ++i, buffer += value_size) {
        memcpy(buffer, value, value_size);
    }

    set_bytes_read(get_rbuf_size());
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
void Chunk::read_chunk() {
    if (d_is_read)
        return;

    // By default, d_read_buffer_is_mine is true. But if this is part of a SuperChunk
    // then the SuperChunk will have allocated memory and d_read_buffer_is_mine is false.
    if (d_read_buffer_is_mine)
        set_rbuf_to_size();

    if (d_uses_fill_value) {
        load_fill_values();
    }
    else {
        dmrpp_easy_handle *handle = DmrppRequestHandler::curl_handle_pool->get_easy_handle(this);
        if (!handle)
            throw BESInternalError(prolog + "No more libcurl handles.", __FILE__, __LINE__);

        try {
            handle->read_data();  // retries until success when appropriate, else throws
            DmrppRequestHandler::curl_handle_pool->release_handle(handle);
        }
        catch (...) {
            // TODO See https://bugs.earthdata.nasa.gov/browse/HYRAX-378
            //  It may be that this is the code that catches throws from
            //  chunk_write_data and based on read_data()'s behavior, the
            //  code should probably stop _all_ transfers, reclaim all
            //  handles and send a failure message up the call stack.
            //  jhrg 4/7/21
            DmrppRequestHandler::curl_handle_pool->release_handle(handle);
            throw;
        }
    }

    // If the expected byte count was not read, it's an error.
    if (get_size() != get_bytes_read()) {
        ostringstream oss;
        oss << "Wrong number of bytes read for chunk; read: " << get_bytes_read() << ", expected: " << get_size();
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }

    d_is_read = true;
}

// direct IO method that reads chunks.
void Chunk::read_chunk_dio() {

    // Read chunk for dio - use read_chunk() as a reference.
    if (d_is_read)
        return;

    // By default, d_read_buffer_is_mine is true. But if this is part of a SuperChunk
    // then the SuperChunk will have allocated memory and d_read_buffer_is_mine is false.
    if (d_read_buffer_is_mine)
        set_rbuf_to_size();

    dmrpp_easy_handle *handle = DmrppRequestHandler::curl_handle_pool->get_easy_handle(this);
    if (!handle)
        throw BESInternalError(prolog + "No more libcurl handles.", __FILE__, __LINE__);

    try {
        handle->read_data();  // retries until success when appropriate, else throws
        DmrppRequestHandler::curl_handle_pool->release_handle(handle);
    }
    catch (...) {
        // TODO See https://bugs.earthdata.nasa.gov/browse/HYRAX-378
        //  It may be that this is the code that catches throws from
        //  chunk_write_data and based on read_data()'s behavior, the
        //  code should probably stop _all_ transfers, reclaim all
        //  handles and send a failure message up the call stack.
        //  jhrg 4/7/21
        DmrppRequestHandler::curl_handle_pool->release_handle(handle);
        throw;
    }
    

    // If the expected byte count was not read, it's an error.
    if (get_size() != get_bytes_read()) {
        ostringstream oss;
        oss << "Wrong number of bytes read for chunk; read: " << get_bytes_read() << ", expected: " << get_size();
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }

    d_is_read = true;


}

/**
 *  unsigned long long d_size;
 *  unsigned long long d_offset;
 *  std::string d_md5;
 *  std::string d_uuid;
 *  std::string d_data_url;
 *  std::vector<unsigned int> d_chunk_position_in_array;
 *
 */
void Chunk::dump(ostream &oss) const {
    oss << "Chunk";
    oss << "[ptr='" << (void *) this << "']";
    oss << "[data_url='" << d_data_url->str() << "']";
    oss << "[offset=" << d_offset << "]";
    oss << "[size=" << d_size << "]";
    oss << "[chunk_position_in_array=(";
    for (unsigned long long i = 0; i < d_chunk_position_in_array.size(); i++) {
        if (i) oss << ",";
        oss << d_chunk_position_in_array[i];
    }
    oss << ")]";
    oss << "[is_read=" << d_is_read << "]";
    oss << "[is_inflated=" << d_is_inflated << "]";
}

string Chunk::to_string() const {
    std::ostringstream oss;
    dump(oss);
    return oss.str();
}

std::shared_ptr<http::url> Chunk::get_data_url() const {

    // The d_data_url may be nullptr(fillvalue case). 
    if (d_data_url == nullptr) 
        return d_data_url;
    std::shared_ptr<http::EffectiveUrl> effective_url = EffectiveUrlCache::TheCache()->get_effective_url(d_data_url);
    BESDEBUG(MODULE, prolog << "Using data_url: " << effective_url->str() << endl);

#if ENABLE_TRACKING_QUERY_PARAMETER
    //A conditional call to void Chunk::add_tracking_query_param()
    // here for the NASA cost model work THG's doing. jhrg 8/7/18
    if (!d_query_marker.empty()) {
        string url_str = effective_url->str();
        if(url_str.find('?') != string::npos){
            url_str.append("&");
        }
        else {
            url_str.append("?");
        }
        url_str += d_query_marker;
        shared_ptr<http::url> query_marker_url( new http::url(url_str));
        return query_marker_url;
    }
#endif

    return effective_url;
}

} // namespace dmrpp

