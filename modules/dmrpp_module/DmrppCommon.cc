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

#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>

#include <cstdlib>

#include "BESIndent.h"
#include "BESDebug.h"
#include "BESInternalError.h"

#include "DmrppCommon.h"
#include "DmrppUtil.h"
#include "Chunk.h"

using namespace std;

namespace dmrpp {

/**
 * @brief Set the dimension sizes for a chunk
 *
 * The string argument holds a space-separated list of integers that
 * represent the dimensions of a chunk. Parse that string and store
 * the integers in this instance.
 *
 * @param chunk_dim_sizes_string
 */
void DmrppCommon::ingest_chunk_dimension_sizes(string chunk_dim_sizes_string)
{
    if (chunk_dim_sizes_string.empty()) return;

    // Clear the thing if it's got stuff in it.
#if 0
    if (d_chunk_dimension_sizes.size()) d_chunk_dimension_sizes.clear();
#endif

    d_chunk_dimension_sizes.clear();

    // TODO use istringstream. jhrg 4/10/18

    string space(" ");
    size_t strPos = 0;
    string strVal;

    // Are there spaces or multiple values?
    if (chunk_dim_sizes_string.find(space) != string::npos) {
        // Process space delimited content
        while ((strPos = chunk_dim_sizes_string.find(space)) != string::npos) {
            strVal = chunk_dim_sizes_string.substr(0, strPos);
            BESDEBUG("dmrpp", __PRETTY_FUNCTION__ << " -  Parsing: " << strVal << endl);
            d_chunk_dimension_sizes.push_back(strtol(strVal.c_str(), NULL, 10));
            chunk_dim_sizes_string.erase(0, strPos + space.length());
        }
    }

    // If it's multi valued there's still one more value left to process
    // If it's single valued the same is true, so let's ingest that.
    d_chunk_dimension_sizes.push_back(strtol(chunk_dim_sizes_string.c_str(), NULL, 10));
}

void DmrppCommon::ingest_compression_type(string compression_type_string)
{
    if (compression_type_string.empty()) return;

    // Clear previous state
    d_compression_type_deflate = false;
    d_compression_type_shuffle = false;

    string deflate("deflate");
    string shuffle("shuffle");

    // Process content
    if (compression_type_string.find(deflate) != string::npos) {
        d_compression_type_deflate = true;
    }

    if (compression_type_string.find(shuffle) != string::npos) {
        d_compression_type_shuffle = true;
    }

    BESDEBUG("dmrpp", "Processed compressionType string. " "d_compression_type_shuffle: "
        << (d_compression_type_shuffle?"true":"false") << "d_compression_type_deflate: "
        << (d_compression_type_deflate?"true":"false") << endl);
}

/**
 * @brief Add a new chunk as defined by an h4:byteStream element
 * @return The number of chunk refs (byteStreams) held.
 */
unsigned long DmrppCommon::add_chunk(std::string data_url, unsigned long long size, unsigned long long offset,
        /*std::string md5, std::string uuid,*/ std::string position_in_array)
{

    d_chunk_refs.push_back(Chunk(data_url, size, offset, /*md5, uuid,*/ position_in_array));

    BESDEBUG("dmrpp",
            "DmrppCommon::add_chunk() - Added chunk " << d_chunk_refs.size() << ": " << d_chunk_refs.back().to_string() << endl);

    return d_chunk_refs.size();
}

/**
 * @brief read method for the atomic types
 *
 * @param name The name of the variable, used for error messages
 * @return Pointer to a char buffer holding the data.
 * @exception BESInternalError on error.
 */
char *
DmrppCommon::read_atomic(const string &name)
{
    vector<Chunk> &chunk_refs = get_chunk_vec();

    if (chunk_refs.size() == 0) {
        throw BESInternalError(string("Unable to obtain ByteStream objects for ") + name, __FILE__, __LINE__);
    }

    // For now we only handle the one chunk case.
    Chunk &h4_byte_stream = chunk_refs[0];
    h4_byte_stream.set_rbuf_to_size();

    // First cut at subsetting; read the whole thing and then subset that.
    curl_read_byte_stream(h4_byte_stream.get_data_url(), h4_byte_stream.get_curl_range_arg_string(), &h4_byte_stream);

    // If the expected byte count was not read, it's an error.
    if (h4_byte_stream.get_size() != h4_byte_stream.get_bytes_read()) {
        ostringstream oss;
        oss << "Wrong number of bytes read for '" << name << "'; expected " << h4_byte_stream.get_size()
            << " but found " << h4_byte_stream.get_bytes_read();
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }

    return h4_byte_stream.get_rbuf();
}

void DmrppCommon::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "is_deflate:             " << (is_deflate_compression() ? "true" : "false") << endl;

#if 0
    strm << BESIndent::LMarg << "deflate_level:          " << (get_deflate_level() ? "true" : "false") << endl;
#endif

    strm << BESIndent::LMarg << "is_shuffle_compression: " << (is_shuffle_compression() ? "true" : "false") << endl;

    vector<unsigned int> chunk_dim_sizes = get_chunk_dimension_sizes();

    strm << BESIndent::LMarg << "chunk dimension sizes:  [";
    for (unsigned int i = 0; i < chunk_dim_sizes.size(); i++) {
        strm << (i ? "][" : "") << chunk_dim_sizes[i];
    }
    strm << "]" << endl;

    const vector<Chunk> &chunk_refs = get_immutable_chunks();
    strm << BESIndent::LMarg << "Chunks (aka chunks):" << (chunk_refs.size() ? "" : "None Found.") << endl;
    BESIndent::Indent();
    for (unsigned int i = 0; i < chunk_refs.size(); i++) {
        strm << BESIndent::LMarg;
        chunk_refs[i].dump(strm);
        strm << endl;
    }
    BESIndent::UnIndent();
}

} // namepsace dmrpp

