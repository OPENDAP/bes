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
#include <cassert>

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
        msg << prolog << "ACCESS DENIED - The underlying object store has refused access to: ";
        msg << data_url->str() << " Object Store Message: " << message;
        BESDEBUG(MODULE, msg.str() << endl);
        VERBOSE(msg.str() << endl);
        throw BESForbiddenError(msg.str(), __FILE__, __LINE__);
    }
    else {
        stringstream msg;
        msg << prolog << "ERROR - The underlying object store returned an error. ";
        msg << "(Tried: " << data_url->str() << ") Object Store Message: " << message;
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
        DmrppRequestHandler::curl_handle_pool->release_all_handles();
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }

    memcpy(chunk->get_rbuf() + bytes_read, buffer, nbytes);
    chunk->set_bytes_read(bytes_read + nbytes);

    BESDEBUG(MODULE, prolog << "END" << endl);

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
unsigned long long  inflate(char **destp, unsigned long long dest_len, char *src, unsigned long long src_len) {
    /* Sanity check */
    assert(src_len > 0);
    assert(src);
    assert(dest_len > 0);
    assert(destp);
    assert(*destp);

//cerr<<"dest_len is "<<dest_len <<endl;
//cerr<<"src_len is "<<src_len <<endl;
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
//cerr<<"total_out is "<<z_strm.total_out <<endl;

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
    const size_t delim_len = delimiter.length();

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
    if (pia.find('[') == string::npos || pia.find(']') == string::npos || pia.length() < 3)
        throw BESInternalError("while parsing a DMR++, chunk position string malformed", __FILE__, __LINE__);

    if (pia.find_first_not_of("[]1234567890,") != string::npos)
        throw BESInternalError("while parsing a DMR++, chunk position string illegal character(s)", __FILE__, __LINE__);

    try {
        split_by_comma(pia.substr(1, pia.length() - 2), cpia_vect);
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
    int match_result = s3_vh_regex.match(d_data_url->str().c_str(), d_data_url->str().length());
    if(match_result>=0) {
        auto match_length = (unsigned int) match_result;
        if (match_length == d_data_url->str().length()) {
            BESDEBUG(MODULE,
                     prolog << "FULL MATCH. pattern: " << s3_vh_regex_str << " url: " << d_data_url->str() << endl);
            add_tracking = true;;
        }
    }

    if(!add_tracking){
        // All S3 buckets, path style URL
        string  s3_path_regex_str = R"(^https?:\/\/s3((\.|-)us-(east|west)-(1|2))?\.amazonaws\.com\/([a-z]|[0-9])(([a-z]|[0-9]|\.|-){1,61})([a-z]|[0-9])\/.*$)";
        BESRegex s3_path_regex(s3_path_regex_str.c_str());
        match_result = s3_path_regex.match(d_data_url->str().c_str(), d_data_url->str().length());
        if(match_result>=0) {
            auto match_length = (unsigned int) match_result;
            if (match_length == d_data_url->str().length()) {
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

/**
 * @brief Compute the Fletcher32 checksum for a block of bytes
 * @param _data Pointer to a block of byte data
 * @param _len Number of bytes to checksum
 * @return The Fletcher32 checksum
 */
uint32_t
checksum_fletcher32(const void *_data, size_t _len)
{
    const auto *data = (const uint8_t *)_data;  // Pointer to the data to be summed
    size_t len = _len / 2;                      // Length in 16-bit words
    uint32_t sum1 = 0, sum2 = 0;

    // Sanity check
    assert(_data);
    assert(_len > 0);

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
} /* end H5_checksum_fletcher32() */

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

//cerr<<"num_deflate is "<<num_deflate <<endl;
    unsigned deflate_index = 0;
    unsigned long long out_buf_size = 0;
    unsigned long long in_buf_size = 0;
    char**destp = nullptr;
    char* dest = nullptr;
    char* tmp_buf = nullptr;
    char* tmp_dest = nullptr;
    for (auto i = filter_array.rbegin(), e = filter_array.rend(); i != e; ++i) {

        string filter = *i;

        if (filter == "deflate") {

            
            if (num_deflate > 1) {

                dest = new char[chunk_size];
                try {
                    destp = &dest;
                    if (deflate_index == 0) {
                        out_buf_size = inflate(destp, chunk_size, get_rbuf(), get_rbuf_size());
cerr<<"index 0: input buf size is "<<get_rbuf_size() <<endl;
                        tmp_dest = *destp;
                    }
                    else {
cerr<<"index 1: input buf size is "<<in_buf_size <<endl;
                        tmp_buf = tmp_dest;
                        out_buf_size = inflate(destp, chunk_size, tmp_buf, in_buf_size);
cerr<<"out buf size is "<<out_buf_size <<endl;
                        tmp_dest = *destp;
                        delete[] tmp_buf;
                    }
                    deflate_index ++;
                    in_buf_size = out_buf_size;
#if DMRPP_USE_SUPER_CHUNKS
                    // This is the last deflate filter, output the buffer.
                    if (deflate_index == num_deflate) {
                        char* newdest = *destp;
                        set_read_buffer(newdest, chunk_size, chunk_size, true);
                    }
 
#else
                set_rbuf(dest, chunk_size);
#endif

                }
                catch (...) {
                    delete[] dest;
                    delete[] tmp_buf;
                    throw;
                }
 

            }
            else {
                dest = new char[chunk_size];
                destp = &dest;
                try {
                    if (inflate(destp, chunk_size, get_rbuf(), get_rbuf_size()) <0) {
                        throw BESError("inflate size is <0", BES_INTERNAL_ERROR, __FILE__, __LINE__);
                    }
                    // This replaces (and deletes) the original read_buffer with dest.
#if DMRPP_USE_SUPER_CHUNKS
                    //set_read_buffer(dest, chunk_size, chunk_size, true);
                    char* new_dest=*destp;
                    //set_read_buffer(new_dest, out_buf_size, chunk_size, true);
                    set_read_buffer(new_dest, chunk_size, chunk_size, true);
                    //set_read_buffer(new_dest, out_buf_size, out_buf_size, true);
#else
                    set_rbuf(dest, chunk_size);
#endif
                }
                catch (...) {
                    delete[] dest;
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
            assert(get_rbuf_size() > FLETCHER32_CHECKSUM);
            //assert((get_rbuf_size() - FLETCHER32_CHECKSUM) % 4 == 0); //probably wrong
            auto f_checksum = *(uint32_t *)(get_rbuf() + get_rbuf_size() - FLETCHER32_CHECKSUM);

            // If the code should actually use the checksum (they can be expensive to compute), does it match
            // with once computed on the data actually read? Maybe make this a bes.conf parameter?
            // jhrg 10/15/21
            uint32_t calc_checksum = checksum_fletcher32((const void *)get_rbuf(), get_rbuf_size() - FLETCHER32_CHECKSUM);
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

const char *get_value_ptr(fill_value &fv, libdap::Type type, const string &v)
{
    switch(type) {
        case libdap::dods_int8_c:
            fv.int8 = (int8_t)stoi(v);
            return (const char *)&fv.int8;

        case libdap::dods_int16_c:
            fv.int16 = (int16_t)stoi(v);
            return (const char *)&fv.int16;

        case libdap::dods_int32_c:
            fv.int32 = (int32_t)stoi(v);
            return (const char *)&fv.int32;

        case libdap::dods_int64_c:
            fv.int64 = (int64_t)stoll(v);
            return (const char *)&fv.int64;

        case libdap::dods_uint8_c:
        case libdap::dods_byte_c:
            fv.uint8 = (uint8_t)stoi(v);
            return (const char *)&fv.uint8;

        case libdap::dods_uint16_c:
            fv.uint16 = (uint16_t)stoi(v);
            return (const char *)&fv.uint16;

        case libdap::dods_uint32_c:
            fv.uint32 = (uint32_t)stoul(v);
            return (const char *)&fv.uint32;

        case libdap::dods_uint64_c:
            fv.uint64 = (uint64_t)stoull(v);
            return (const char *)&fv.uint64;

        case libdap::dods_float32_c:
            fv.f = stof(v);
            return (const char *)&fv.f;

        case libdap::dods_float64_c:
            fv.d = stod(v);
            return (const char *)&fv.d;

        default:
            throw BESInternalError("Unknown fill value type.", __FILE__, __LINE__);
    }
}

/**
 * @brief Load the chunk with fill values - temporary implementation
 */
void Chunk::load_fill_values() {
    fill_value fv;
    const char *value = get_value_ptr(fv, d_fill_value_type, d_fill_value);
    unsigned int value_size = get_value_size(d_fill_value_type);

    unsigned long long num_values = get_rbuf_size() / value_size;
    char *buffer = get_rbuf();

    for (int i = 0; i < num_values; ++i, buffer += value_size) {
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
    for (unsigned long i = 0; i < d_chunk_position_in_array.size(); i++) {
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

