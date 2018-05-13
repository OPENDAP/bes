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

#include <BaseType.h>
#include <D4Attributes.h>
#include <XMLWriter.h>

#include <BESIndent.h>
#include <BESDebug.h>
#include <BESInternalError.h>

#include "DmrppRequestHandler.h"
#include "DmrppCommon.h"
#include "Chunk.h"

using namespace std;
using namespace libdap;

namespace dmrpp {

bool DmrppCommon::d_print_chunks = false;
string DmrppCommon::d_dmrpp_ns = "http://xml.opendap.org/dap/dmrpp/1.0.0#";
string DmrppCommon::d_ns_prefix = "dmrpp";

/**
 * @brief Set the dimension sizes for a chunk
 *
 * The string argument holds a space-separated list of integers that
 * represent the dimensions of a chunk. Parse that string and store
 * the integers in this instance.
 *
 * @param chunk_dims The sizes as a list of integers separated by spaces, e.g., '50 50'
 */
void DmrppCommon::parse_chunk_dimension_sizes(string chunk_dims)
{
    d_chunk_dimension_sizes.clear();

    if (chunk_dims.empty()) return;

    // If the input is anything other than integers and spaces, throw
    if (chunk_dims.find_first_not_of("1234567890 ") != string::npos)
        throw BESInternalError("while processing chunk dimension information, illegal character(s)", __FILE__, __LINE__);

    // istringstream can parse this kind of input more easily. jhrg 4/10/18

    string space(" ");
    size_t strPos = 0;
    string strVal;

    // Are there spaces or multiple values?
    if (chunk_dims.find(space) != string::npos) {
        // Process space delimited content
        while ((strPos = chunk_dims.find(space)) != string::npos) {
            strVal = chunk_dims.substr(0, strPos);

            d_chunk_dimension_sizes.push_back(strtol(strVal.c_str(), NULL, 10));
            chunk_dims.erase(0, strPos + space.length());
        }
    }

    // If it's multi valued there's still one more value left to process
    // If it's single valued the same is true, so let's ingest that.
    d_chunk_dimension_sizes.push_back(strtol(chunk_dims.c_str(), NULL, 10));
}

/**
 * @brief Parses the text content of the XML element h4:chunkDimensionSizes
 * into the internal vector<unsigned int> representation.
 *
 * @param compression_type_string One of "deflate" or "shuffle."
 */
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
}

/**
 * @brief Add a new chunk as defined by an h4:byteStream element
 * @return The number of chunk refs (byteStreams) held.
 */
unsigned long DmrppCommon::add_chunk(const string &data_url, unsigned long long size, unsigned long long offset,
    string position_in_array)
{
    d_chunks.push_back(Chunk(data_url, size, offset, position_in_array));

    return d_chunks.size();
}

unsigned long DmrppCommon::add_chunk(const string &data_url, unsigned long long size, unsigned long long offset,
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

/**
 * @brief Print the Chunk information.
 */
void
DmrppCommon::print_chunks_element(XMLWriter &xml, const string &name_space)
{
    // Start element "chunks" with dmrpp namespace and attributes:
    if (xmlTextWriterStartElementNS(xml.get_writer(), (const xmlChar*)name_space.c_str(), (const xmlChar*) "chunks", NULL) < 0)
        throw BESInternalError("Could not start chunks element.", __FILE__, __LINE__);

    string compression = "";
    if (is_shuffle_compression() && is_deflate_compression())
        compression = "deflate shuffle";
    else if (is_shuffle_compression())
        compression.append("shuffle");
    else if (is_deflate_compression())
        compression.append("deflate");

    if (!compression.empty())
        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "compressionType", (const xmlChar*) compression.c_str()) < 0)
            throw BESInternalError("Could not write compression attribute.", __FILE__, __LINE__);

    if (d_chunk_dimension_sizes.size() > 0) {
        // Write element "chunkDimensionSizes" with dmrpp namespace:
        ostringstream oss;
        copy(d_chunk_dimension_sizes.begin(), d_chunk_dimension_sizes.end(), ostream_iterator<unsigned int>(oss, " "));
        string sizes = oss.str();
        sizes.erase(sizes.size() - 1, 1);    // trim the trailing space

        if (xmlTextWriterWriteElementNS(xml.get_writer(), (const xmlChar*) name_space.c_str(), (const xmlChar*) "chunkDimensionSizes", NULL,
            (const xmlChar*) sizes.c_str()) < 0) throw BESInternalError("Could not write chunkDimensionSizes attribute.", __FILE__, __LINE__);
    }

    // Start elements "chunk" with dmrpp namespace and attributes:
    for (vector<Chunk>::iterator i = get_chunk_vec().begin(), e = get_chunk_vec().end(); i != e; ++i) {
        Chunk &chunk = *i;

        if (xmlTextWriterStartElementNS(xml.get_writer(), (const xmlChar*)name_space.c_str(), (const xmlChar*) "chunk", NULL) < 0)
            throw BESInternalError("Could not start element chunk", __FILE__, __LINE__);

        // Get offset string:
        ostringstream offset;
        offset << chunk.get_offset();
        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "offset", (const xmlChar*) offset.str().c_str()) < 0)
            throw BESInternalError("Could not write attribute offset", __FILE__, __LINE__);

        // Get nBytes string:
        ostringstream nBytes;
        nBytes << chunk.get_size();
        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "nBytes", (const xmlChar*) nBytes.str().c_str()) < 0)
            throw BESInternalError("Could not write attribute nBytes", __FILE__, __LINE__);

        if (chunk.get_position_in_array().size() > 0) {
            // Get position in array string:
            vector<unsigned int> pia = chunk.get_position_in_array();
            ostringstream oss;
            oss << "[";
            copy(pia.begin(), pia.end(), ostream_iterator<unsigned int>(oss, ","));
            string pia_str = oss.str();
            if (pia.size() > 0) pia_str.replace(pia_str.size() - 1, 1, "]"); // replace the trailing ',' with ']'
            if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "chunkPositionInArray", (const xmlChar*) pia_str.c_str()) < 0)
                throw BESInternalError("Could not write attribute position in array", __FILE__, __LINE__);
        }

        // End element "chunk":
        if (xmlTextWriterEndElement(xml.get_writer()) < 0) throw BESInternalError("Could not end chunk element", __FILE__, __LINE__);
    }

    if (xmlTextWriterEndElement(xml.get_writer()) < 0) throw BESInternalError("Could not end chunks element", __FILE__, __LINE__);
}

/**
 * @brief Print the DMR++ response for the Scalar types
 *
 * @note See DmrppArray::print_dap4() for a discussion about the design of
 * this, and related, method.
 *
 * @param xml Write the XML to this instance of XMLWriter
 * @param constrained If true, print the constrained DMR. False by default.
 * @see DmrppArray::print_dap4()
 */
void DmrppCommon::print_dmrpp(XMLWriter &xml, bool constrained /*false*/)
{
    BaseType &bt = dynamic_cast<BaseType&>(*this);
    if (constrained && !bt.send_p())
        return;

    if (xmlTextWriterStartElement(xml.get_writer(), (const xmlChar*)bt.type_name().c_str()) < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not write " + bt.type_name() + " element");

    if (!bt.name().empty())
        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "name", (const xmlChar*)bt.name().c_str()) < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not write attribute for name");

    if (bt.is_dap4())
        bt.attributes()->print_dap4(xml);

    if (!bt.is_dap4() && bt.get_attr_table().get_size() > 0)
        bt.get_attr_table().print_xml_writer(xml);

    // This is the code added to libdap::BaseType::print_dap4(). jhrg 5/10/18
    if (DmrppCommon::d_print_chunks && get_immutable_chunks().size() > 0)
        print_chunks_element(xml, DmrppCommon::d_ns_prefix);

    if (xmlTextWriterEndElement(xml.get_writer()) < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not end " + bt.type_name() + " element");
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

} // namepsace dmrpp

