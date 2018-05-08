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

#include <curl/curl.h>

#include "BESIndent.h"
#include "BESDebug.h"
#include "BESInternalError.h"

#include "DmrppRequestHandler.h"
#include "DmrppCommon.h"
#include "Chunk.h"
#include <XMLWriter.h>

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
//            BESDEBUG("dmrpp", __PRETTY_FUNCTION__ << " -  Parsing: " << strVal << endl);
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
    d_deflate = false;
    d_shuffle = false;

    string deflate("deflate");
    string shuffle("shuffle");

    // Process content
    if (compression_type_string.find(deflate) != string::npos) {
        d_deflate = true;
    }

    if (compression_type_string.find(shuffle) != string::npos) {
        d_shuffle = true;
    }

    BESDEBUG("dmrpp", "Processed compressionType string. " "d_compression_type_shuffle: "
        << (d_shuffle?"true":"false") << "d_compression_type_deflate: "
        << (d_deflate?"true":"false") << endl);
}

/**
 * @brief Add a new chunk as defined by an h4:byteStream element
 * @return The number of chunk refs (byteStreams) held.
 */
unsigned long DmrppCommon::add_chunk(string data_url, unsigned long long size, unsigned long long offset,
    string position_in_array)
{
    d_chunks.push_back(Chunk(data_url, size, offset, position_in_array));

    return d_chunks.size();
}
unsigned long DmrppCommon::add_chunk(string data_url, unsigned long long size, unsigned long long offset,
    const vector<unsigned int> &position_in_array)
{
    d_chunks.push_back(Chunk(data_url, size, offset, position_in_array));

    return d_chunks.size();
}

/**
 * @brief read method for the atomic types
 *
 * This method is used by the specializations of BaseType::read() in the
 * 'atomic' type classes (libdap::Byte, libdap::In32, ...) to read data
 * when those data are contained in a single chunk (i.e., using HDF5
 * contiguous storage).
 *
 * @note It is assumed that these data are never compressed. However,
 * it is possible to call Chunk::inflate_chunk(...) after calling this
 * method and then call Chunk::get_rbuf() to access the decompressed
 * data.
 *
 * @param name The name of the variable, used for error messages
 * @return Pointer to a char buffer holding the data.
 * @exception BESInternalError on error.
 */
char *
DmrppCommon::read_atomic(const string &name)
{
    vector<Chunk> &chunk_refs = get_chunk_vec();

    if (chunk_refs.size() != 1)
        throw BESInternalError(string("Expected only a single chunk for variable ") + name, __FILE__, __LINE__);

    Chunk &chunk = chunk_refs[0];

    chunk.read_chunk();

    return chunk.get_rbuf();
}

void DmrppCommon::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "is_deflate:             " << (is_deflate_compression() ? "true" : "false") << endl;
    strm << BESIndent::LMarg << "is_shuffle_compression: " << (is_shuffle_compression() ? "true" : "false") << endl;

    const vector<unsigned int> &chunk_dim_sizes = get_chunk_dimension_sizes();

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

void print_dmrpp(libdap::XMLWriter &xml)
{
    if (xmlTextWriterStartElement(xml.get_writer(), (const xmlChar*)"Dataset") < 0)
            throw BESInternalError("Could not write Dataset element", __FILE__, __LINE__);
}

} // namepsace dmrpp

