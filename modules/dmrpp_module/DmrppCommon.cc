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
#include <vector>
#include <cstdlib>

#include <cstdlib>

#include <DapIndent.h>
#include <BESDebug.h>

#include "DmrppCommon.h"
#include "H4ByteStream.h"

using namespace std;
using namespace libdap;

namespace dmrpp {
/**
 * Interface for the size and offset information of data described by
 * DMR++ files.
 */

void DmrppCommon::ingest_chunk_dimension_sizes(std::string chunk_dim_sizes_string)
{
    if (!chunk_dim_sizes_string.length()) return;

    // Clear the thing if it's got stuff in it.
    if (d_chunk_dimension_sizes.size()) d_chunk_dimension_sizes.clear();

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

void DmrppCommon::ingest_compression_type(std::string compression_type_string)
{
    if (!compression_type_string.length()) return;

    // Clear previous state
    d_compression_type_deflate = false;
    d_compression_type_shuffle = false;

    string deflate("deflate");
    string shuffle("shuffle");
    size_t strPos = 0;

    // Process content
    if ((strPos = compression_type_string.find(deflate)) != string::npos) {
        d_compression_type_deflate = true;
    }

    if ((strPos = compression_type_string.find(shuffle)) != string::npos) {
        d_compression_type_shuffle = true;
    }

    BESDEBUG("dmrpp",
            __PRETTY_FUNCTION__ << " - Processed compressionType string. " "d_compression_type_shuffle: " << (d_compression_type_shuffle?"true":"false") << "d_compression_type_deflate: " << (d_compression_type_deflate?"true":"false") << endl);
}

/**
 * @brief Add a new chunk as defined by an h4:byteStream element
 * @return The number of chunk refs (byteStreams) held.
 */
unsigned long DmrppCommon::add_chunk(std::string data_url, unsigned long long size, unsigned long long offset,
        std::string md5, std::string uuid, std::string position_in_array)
{

    d_chunk_refs.push_back(H4ByteStream(data_url, size, offset, md5, uuid, position_in_array));

    BESDEBUG("dmrpp",
            "DmrppCommon::add_chunk() - Added chunk " << d_chunk_refs.size() << ": " << d_chunk_refs.back().to_string() << endl);

    return d_chunk_refs.size();
}

void DmrppCommon::dump(ostream & strm) const
{
    strm << DapIndent::LMarg << "is_deflate:             " << (is_deflate_compression() ? "true" : "false") << endl;
    strm << DapIndent::LMarg << "deflate_level:          " << (get_deflate_level() ? "true" : "false") << endl;
    strm << DapIndent::LMarg << "is_shuffle_compression: " << (is_shuffle_compression() ? "true" : "false") << endl;

    vector<unsigned int> chunk_dim_sizes = get_chunk_dimension_sizes();

    strm << DapIndent::LMarg << "chunk dimension sizes:  [";
    for (unsigned int i = 0; i < chunk_dim_sizes.size(); i++) {
        strm << (i ? "][" : "") << chunk_dim_sizes[i];
    }
    strm << "]" << endl;

    vector<H4ByteStream> chunk_refs = get_immutable_chunks();
    strm << DapIndent::LMarg << "H4ByteStreams (aka chunks):" << (chunk_refs.size() ? "" : "None Found.") << endl;
    DapIndent::Indent();
    for (unsigned int i = 0; i < chunk_refs.size(); i++) {
        strm << DapIndent::LMarg;
        chunk_refs[i].dump(strm);
        strm << endl;
    }
    DapIndent::UnIndent();
}

} // namepsace dmrpp

