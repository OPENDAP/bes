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
#include <cassert>

#include <BESDebug.h>
#include <BESInternalError.h>
#include <BESContextManager.h>

#include "Chunk.h"
#include "DmrppUtil.h"

const string debug = "dmrpp";

using namespace std;

namespace dmrpp {

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
 * @param pia The chunk position string.
 */
void Chunk::set_position_in_array(const string &pia)
{
    assert(!pia.empty());

    // Assume input is [x,y,...,z] where x, ..., is an integer
    // iss holds x , y , ... , z <eof>
    istringstream iss(pia.substr(1, pia.size() - 2));

    char c;
    unsigned int i;
    while (!iss.eof()) {
        iss >> i; // read an integer
        d_chunk_position_in_array.push_back(i);

        iss >> c; // read a separator or the ending ']'
        assert(c == ',' || iss.eof());
    }

    assert(d_chunk_position_in_array.size() > 0);
}

// FIXME Replace value param that is modified with a istringstream
void Chunk::ingest_position_in_array(string pia)
{
    BESDEBUG(debug, "Chunk::ingest_position_in_array() -  BEGIN" << " -  Parsing: " << pia << "'" << endl);

    if (!pia.empty()) {
        BESDEBUG(debug, "Chunk::ingest_position_in_array() -  string '"<< pia << "' is not empty." << endl);
        // Clear the thing if it's got stuff in it.
        if (d_chunk_position_in_array.size()) d_chunk_position_in_array.clear();

        string open_sqr_brckt("[");
        string close_sqr_brckt("]");
        string comma(",");
        size_t strPos = 0;
        string strVal;

        // Drop leading square bracket
        if (!pia.compare(0, 1, open_sqr_brckt)) {
            pia.erase(0, 1);
            BESDEBUG(debug,
                    "Chunk::ingest_position_in_array() -  dropping leading '[' result: '"<< pia << "'" << endl);
        }

        // Drop trailing square bracket
        if (!pia.compare(pia.length() - 1, 1, close_sqr_brckt)) {
            pia.erase(pia.length() - 1, 1);
            BESDEBUG(debug,
                    "Chunk::ingest_position_in_array() -  dropping trailing ']' result: '"<< pia << "'" << endl);
        }

        // Is it multi-valued? We check for commas  to find out.
        if ((strPos = pia.find(comma)) != string::npos) {
            BESDEBUG(debug,
                    "Chunk::ingest_position_in_array() -  Position string appears to contain multiple values..." << endl);
            // Process comma delimited content
            while ((strPos = pia.find(comma)) != string::npos) {
                strVal = pia.substr(0, strPos);
                BESDEBUG(debug, __PRETTY_FUNCTION__ << " -  Parsing: " << strVal << endl);
                d_chunk_position_in_array.push_back(strtol(strVal.c_str(), NULL, 10));
                pia.erase(0, strPos + comma.length());
            }
        }
        // A single value, remains after multi-valued processing, or there was only
        // Every a single value, so let's ingest that!
        BESDEBUG(debug,
                "Chunk::ingest_position_in_array() -  Position string appears to have a single value..." << endl);
        d_chunk_position_in_array.push_back(strtol(pia.c_str(), NULL, 10));
    }

    BESDEBUG(debug, "Chunk::ingest_position_in_array() -  END" << " -  Parsed " << pia << "'" << endl);
}

/**
 * @brief Returns a curl range argument.
 * The libcurl requires a string argument for range-ge activitys, this method
 * constructs one in the required syntax from the offset and size information
 * for this byteStream.
 *
 */
std::string Chunk::get_curl_range_arg_string()
{
    ostringstream range;   // range-get needs a string arg for the range
    range << d_offset << "-" << d_offset + d_size - 1;
    return range.str();
}

/**
 * @brief Modify the \arg data_access_url so that it include tracking info
 *
 * The tracking info is the value of the BESContext "cloudydap".
 *
 * @param data_access_url The URL to hack
 */
void Chunk::add_tracking_query_param(string &data_access_url)
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
     */
    std::string aws_s3_url("https://s3.amazonaws.com/");
    // Is it an AWS S3 access?
    if (!data_access_url.compare(0, aws_s3_url.size(), aws_s3_url)) {
        // Yup, headed to S3.
        string cloudydap_context("cloudydap");
        bool found;
        string cloudydap_context_value;
        cloudydap_context_value = BESContextManager::TheManager()->get_context(cloudydap_context, found);
        if (found) {
            data_access_url += "?cloudydap=" + cloudydap_context_value;
        }
    }
}

void Chunk::add_to_multi_read_queue(CURLM *multi_handle)
{
    BESDEBUG(debug,"Chunk::"<< __func__ <<"() - BEGIN  " << to_string() << endl);

    if (d_is_read || d_is_in_multi_queue) {
        BESDEBUG("dmrpp", "Chunk::"<< __func__ <<"() - Chunk has been " << (d_is_in_multi_queue?"queued to be ":"") << "read! Returning." << endl);
        return;
    }

    // This call uses the internal size param and allocates the buffer's memory
    set_rbuf_to_size();

    string data_access_url = get_data_url();

    BESDEBUG(debug,"Chunk::"<< __func__ <<"() - data_access_url "<< data_access_url << endl);

    add_tracking_query_param(data_access_url);

    string range = get_curl_range_arg_string();

    BESDEBUG(debug,
        __func__ <<" - Retrieve  " << get_size() << " bytes " "from "<< data_access_url << ": " << range << endl);

    CURL* curl = curl_easy_init();
    if (!curl) {
        throw BESInternalError("Unable to initialize curl handle", __FILE__, __LINE__);
    }

    CURLcode res = curl_easy_setopt(curl, CURLOPT_URL, data_access_url.c_str());
    if (res != CURLE_OK) throw BESInternalError("string(curl_easy_strerror(res))", __FILE__, __LINE__);

    // Use CURLOPT_ERRORBUFFER for a human-readable message
    //
    res = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, d_curl_error_buf);
    if (res != CURLE_OK) throw BESInternalError("string(curl_easy_strerror(res))", __FILE__, __LINE__);

    // get the offset to offset + size bytes
    if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str() /*"0-199"*/))
        throw BESInternalError(string("HTTP Error: ").append(d_curl_error_buf), __FILE__, __LINE__);

    // Pass all data to the 'write_data' function
    if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, chunk_write_data))
        throw BESInternalError(string("HTTP Error: ").append(d_curl_error_buf), __FILE__, __LINE__);

    // Pass this to write_data as the fourth argument
    if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEDATA, this))
        throw BESInternalError(string("HTTP Error: ").append(d_curl_error_buf), __FILE__, __LINE__);

    /* add the individual transfers */
    curl_multi_add_handle(multi_handle, curl);

    BESDEBUG(debug, "Chunk::"<< __func__ <<"() - Added to multi_handle: "<< to_string() << endl);

    /* we start some action by calling perform right away */
    // int still_running;
    //  curl_multi_perform(multi_handle, &still_running);
    d_curl_handle = curl;
    d_is_in_multi_queue = true;

    BESDEBUG(debug, __func__ <<"() - END  "<< to_string() << endl);
}

void Chunk::complete_read(bool deflate, bool shuffle, unsigned int chunk_size, unsigned int elem_width)
{

    // If the expected byte count was not read, it's an error.
    if (get_size() != get_bytes_read()) {
        ostringstream oss;
        oss << "Chunk: Wrong number of bytes read for '" << to_string() << "'; expected " << get_size()
                << " but found " << get_bytes_read() << endl;
        throw BESInternalError("oss.str()", __FILE__, __LINE__);
    }

    // If data are compressed/encoded, then decode them here.
    // At this point, the bytes_read property would be changed.
    // The file that implements the deflate filter is H5Zdeflate.c in the hdf5 source.
    // The file that implements the shuffle filter is H5Zshuffle.c.

    // TODO This code is pretty naive - there are apparently a number of
    // different ways HDF5 can compress data, and it does also use a scheme
    // where several algorithms can be applied in sequence. For now, get
    // simple zlib deflate working.jhrg 1/15/17
    // Added support for shuffle. Assuming unshuffle always is applied _after_
    // inflating the data (reversing the shuffle --> deflate process). It is
    // possible that data could just be deflated or shuffled (because we
    // have test data are use only shuffle). jhrg 1/20/17

    chunk_size *= elem_width;

    if (deflate) {
        char *dest = new char[chunk_size];  // TODO unique_ptr<>. jhrg 1/15/17
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

    d_is_read = true;
}

/**
 * @brief Read the chunk associated with this Chunk
 *
 * @param deflate True if we should deflate the data
 * @param shuffle_chunk True if the chunk was shuffled.
 * @param chunk_size The size of the chunk once deflated in elements; ignored when deflate is false
 * @param elem_width Number of bytes in an element; ignored when shuffle_chunk is false
 */
void Chunk::read(bool deflate, bool shuffle, unsigned int chunk_size, unsigned int elem_width)
{
    //  is_deflate, is_shuffle, chunk_size_in elements, var width)
    if (d_is_read) {
        BESDEBUG("dmrpp", "Chunk::"<< __func__ <<"() - Already been read! Returning." << endl);
        return;
    }

    if (!d_is_in_multi_queue) {
        // This call uses the internal size param and allocates the buffer's memory
        set_rbuf_to_size();

        string data_access_url = get_data_url();

        BESDEBUG(debug,"Chunk::"<< __func__ <<"() - data_access_url "<< data_access_url << endl);

        add_tracking_query_param(data_access_url);

        BESDEBUG(debug,
                "Chunk::"<< __func__ <<"() - Reading  " << get_size() << " bytes "
                        "from "<< data_access_url << ": " << get_curl_range_arg_string() << endl);

        curl_read_chunk(this);
    }

    complete_read(deflate, shuffle, chunk_size, elem_width);

    d_is_in_multi_queue = false;
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
    oss << "[is_in_multi_queue=" << d_is_in_multi_queue << "]";
}

string Chunk::to_string() const
{
    std::ostringstream oss;
    dump(oss);
    return oss.str();
}

} // namespace dmrpp

