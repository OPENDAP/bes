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
#include <BESError.h>

#include "H4ByteStream.h"
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
void H4ByteStream::set_position_in_array(const string &pia)
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
void H4ByteStream::ingest_position_in_array(string pia)
{
    BESDEBUG(debug, "H4ByteStream::ingest_position_in_array() -  BEGIN" << " -  Parsing: " << pia << "'" << endl);

    if (!pia.empty()) {
        BESDEBUG(debug, "H4ByteStream::ingest_position_in_array() -  string '"<< pia << "' is not empty." << endl);
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
                    "H4ByteStream::ingest_position_in_array() -  dropping leading '[' result: '"<< pia << "'" << endl);
        }

        // Drop trailing square bracket
        if (!pia.compare(pia.length() - 1, 1, close_sqr_brckt)) {
            pia.erase(pia.length() - 1, 1);
            BESDEBUG(debug,
                    "H4ByteStream::ingest_position_in_array() -  dropping trailing ']' result: '"<< pia << "'" << endl);
        }

        // Is it multi-valued? We check for commas  to find out.
        if ((strPos = pia.find(comma)) != string::npos) {
            BESDEBUG(debug,
                    "H4ByteStream::ingest_position_in_array() -  Position string appears to contain multiple values..." << endl);
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
                "H4ByteStream::ingest_position_in_array() -  Position string appears to have a single value..." << endl);
        d_chunk_position_in_array.push_back(strtol(pia.c_str(), NULL, 10));
    }

    BESDEBUG(debug, "H4ByteStream::ingest_position_in_array() -  END" << " -  Parsed " << pia << "'" << endl);
}

/**
 * @brief Returns a curl range argument.
 * The libcurl requires a string argument for range-ge activitys, this method
 * constructs one in the required syntax from the offset and size information
 * for this byteStream.
 *
 */
std::string H4ByteStream::get_curl_range_arg_string()
{
    ostringstream range;   // range-get needs a string arg for the range
    range << d_offset << "-" << d_offset + d_size - 1;
    return range.str();
}

bool H4ByteStream::is_read()
{
    return d_is_read;
}

/**
 * @brief Read the chunk associated with this H4ByteStream
 *
 * @param deflate True if we should deflate the date; defaults to false
 * @param chunk_size The size of the chunk once deflated; defaults to 0
 */
void H4ByteStream::read(bool deflate_chunk, unsigned int chunk_size)
{
    if (d_is_read) {
        BESDEBUG("dmrpp", "H4ByteStream::"<< __func__ <<"() - Already been read! Returning." << endl);
        return;
    }

    set_rbuf_to_size();

    // First cut at subsetting; read the whole thing and then subset that.
    BESDEBUG(debug,
            "H4ByteStream::"<< __func__ <<"() - Reading  " << get_size() << " bytes from "<< get_data_url() << ": " << get_curl_range_arg_string() << endl);

    curl_read_byte_stream(get_data_url(), get_curl_range_arg_string(), this); //dynamic_cast<H4ByteStream*>(this));

#if 1
    // If data are compressed/encoded, then decode them here.
    // At this point, the bytes_read property would be changed.
    // The file that implements the deflate filter is H5Zdeflate.c in the hdf5 source.

    // TODO This code is pretty naive - there are apparently a number of
    // different ways HDF5 can compress data, and it does also use a scheme
    // where several algorithms can be applied in sequence. For now, get
    // simple zlib deflate working.jhrg 1/15/17
    if (deflate_chunk) {
        char *dest = new char[chunk_size];  // TODO unique_ptr<>. jhrg 1/15/17
        try {
            deflate(dest, chunk_size, get_rbuf(), get_rbuf_size());
            set_rbuf(dest, chunk_size);
        }
        catch (...) {
            delete[] dest;
            throw;
        }
    }
#endif

    // If the expected byte count was not read, it's an error.
    if (get_size() != get_bytes_read()) {
        ostringstream oss;
        oss << "H4ByteStream: Wrong number of bytes read for '" << to_string() << "'; expected " << get_size()
                << " but found " << get_bytes_read() << endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
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
void H4ByteStream::dump(ostream &oss) const
{
    oss << "H4ByteStream";
    oss << "[data_url='" << d_data_url << "']";
    oss << "[offset=" << d_offset << "]";
    oss << "[size=" << d_size << "]";
    oss << "[md5=" << d_md5 << "]";
    oss << "[uuid=" << d_uuid << "]";
    oss << "[chunk_position_in_array=(";
    for (unsigned long i = 0; i < d_chunk_position_in_array.size(); i++) {
        if (i) oss << ",";
        oss << d_chunk_position_in_array[i];
    }
    oss << ")]";
}

string H4ByteStream::to_string()
{
    std::ostringstream oss;
    dump(oss);
    return oss.str();
}

} // namespace dmrpp

