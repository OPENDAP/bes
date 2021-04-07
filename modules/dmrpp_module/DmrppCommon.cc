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

#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <cstdlib>
#include <cstring>

#include <curl/curl.h>

#include "BaseType.h"
#include "D4Attributes.h"
#include "XMLWriter.h"

#include "BESIndent.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESLog.h"
#include "BESInternalError.h"

#include "DmrppRequestHandler.h"
#include "DmrppCommon.h"
#include "DmrppArray.h"
#include "Chunk.h"
#include "util.h"

using namespace std;
using namespace libdap;

#define prolog std::string("DmrppCommon::").append(__func__).append("() - ")

namespace dmrpp {

// Used with BESDEBUG
static const string dmrpp_3 = "dmrpp:3";
static const string dmrpp_4 = "dmrpp:4";

bool DmrppCommon::d_print_chunks = false;
string DmrppCommon::d_dmrpp_ns = "http://xml.opendap.org/dap/dmrpp/1.0.0#";
string DmrppCommon::d_ns_prefix = "dmrpp";

/**
 * @brief Join with all the 'outstanding' threads
 * Use this to clean up resources if an exception is thrown in one thread. In that case
 * this code sweeps through all of the outstanding threads and makes sure they are joined.
 * It's tempting to detach and let the existing threads call exit, but might lead to a
 * double use error, since two threads might be working with the same libcurl handle.
 *
 * @param threads Array of pthread_t structures; null values indicate an unused item
 * @param num_threads Total number of elements in threads.
 */
void join_threads(pthread_t threads[], unsigned int num_threads)
{
    int status;
    for (unsigned int i = 0; i < num_threads; ++i) {
        if (threads[i]) {
            BESDEBUG(dmrpp_3, "Join thread " << i << " after an exception was caught." << endl);
            string *error = NULL;
            if ((status = pthread_join(threads[i], (void **) &error)) < 0) {
                BESDEBUG(dmrpp_3, "Could not join thread " << i << ", " << strerror(status)<< endl);
                // ERROR_LOG("Failed to join thread " << i << " during clean up from an exception: " << strerror(status) << endl);
            }
            else if (error != NULL) {
                BESDEBUG(dmrpp_3, "Joined thread " << i << ", error exit: " << *error << endl);
                // ERROR_LOG("Joined thread " << i << ", error exit" << *error << endl);
            }
            else {
                BESDEBUG(dmrpp_3, "Joined thread " << i << ", successful exit." << endl);
            }
        }
    }
}

/**
 * @brief Set the dimension sizes for a chunk
 *
 * The string argument holds a space-separated list of integers that
 * represent the dimensions of a chunk. Parse that string and store
 * the integers in this instance.
 *
 * @param chunk_dims The sizes as a list of integers separated by spaces, e.g., '50 50'
 */
void DmrppCommon::parse_chunk_dimension_sizes(const string &chunk_dims_string)
{
    d_chunk_dimension_sizes.clear();

    if (chunk_dims_string.empty()) return;

    string chunk_dims = chunk_dims_string;
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

            d_chunk_dimension_sizes.push_back(strtol(strVal.c_str(), nullptr, 10));
            chunk_dims.erase(0, strPos + space.length());
        }
    }

    // If it's multi valued there's still one more value left to process
    // If it's single valued the same is true, so let's ingest that.
    d_chunk_dimension_sizes.push_back(strtol(chunk_dims.c_str(), nullptr, 10));
}

/**
 * @brief Parses the text content of the XML element h4:chunkDimensionSizes
 * into the internal vector<unsigned int> representation.
 *
 * @param compression_type_string One of "deflate" or "shuffle."
 */
void DmrppCommon::ingest_compression_type(const string &compression_type_string)
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
 * @brief Parses the text content of the XML element chunks:byteOrder.
 *
 * @param byte_order_string One of "LE", "BE"
 */
    void DmrppCommon::ingest_byte_order(const string &byte_order_string) {

        if (byte_order_string.empty()) return;

        // Process content
        if (byte_order_string.compare("LE") == 0) {
            d_byte_order = "LE";
            d_twiddle_bytes = is_host_big_endian();
        } else {
            if (byte_order_string.compare("BE") == 0) {
                d_byte_order = "BE";
                d_twiddle_bytes = !(is_host_big_endian());
            } else {
                throw BESInternalError("Did not recognize byteOrder.", __FILE__, __LINE__);
            }
        }
    }

#if 0
std::string DmrppCommon::get_byte_order()
    {
        return d_byte_order;
    }
#endif

/**
 * @brief Add a new chunk as defined by an h4:byteStream element
 * @return The number of chunk refs (byteStreams) held.
 */
unsigned long DmrppCommon::add_chunk(
        std::shared_ptr<http::url> data_url,
        const string &byte_order,
        unsigned long long size,
        unsigned long long offset,
        const string &position_in_array)

{
    vector<unsigned long long> cpia_vector;
    Chunk::parse_chunk_position_in_array_string(position_in_array, cpia_vector);
    return add_chunk(data_url, byte_order, size, offset, cpia_vector);
}

unsigned long DmrppCommon::add_chunk(
        std::shared_ptr<http::url> data_url,
        const string &byte_order,
        unsigned long long size,
        unsigned long long offset,
        const vector<unsigned long long> &position_in_array)
{
    std::shared_ptr<Chunk> chunk(new Chunk(data_url, byte_order, size, offset, position_in_array));
#if 0
    auto array = dynamic_cast<dmrpp::DmrppArray *>(this);
    if(!array){
        stringstream msg;
        msg << prolog << "ERROR  DmrrpCommon::add_chunk() may only be called on an instance of DmrppArray. ";
        msg << "The variable";
        auto bt = dynamic_cast<libdap::BaseType *>(this);
        if(bt){
            msg  << " " << bt->type_name() << " " << bt->name();
        }
        msg << " is not an instance of DmrppArray.";
        msg << "this: " << (void **) this << " ";
        msg << "byte_order: " << byte_order << " ";
        msg << "size: " << size << " ";
        msg << "offset: " << offset << " ";
        throw BESInternalError(msg.str(),__FILE__, __LINE__);
    }

    if(d_super_chunks.empty())
        d_super_chunks.push_back( shared_ptr<SuperChunk>(new SuperChunk()));

    auto currentSuperChunk = d_super_chunks.back();

    bool chunk_was_added = currentSuperChunk->add_chunk(chunk);
    if(!chunk_was_added){
        if(currentSuperChunk->empty()){
            stringstream msg;
            msg << prolog << "ERROR! Failed to add a Chunk to an empty SuperChunk. This should not happen.";
            throw BESInternalError(msg.str(),__FILE__,__LINE__);
        }
        // This means that the chunk was not contiguous with the currentSuperChunk
        currentSuperChunk = shared_ptr<SuperChunk>(new SuperChunk());
        chunk_was_added = currentSuperChunk->add_chunk(chunk);
        if(!chunk_was_added) {
            stringstream msg;
            msg << prolog << "ERROR! Failed to add a Chunk to an empty SuperChunk. This should not happen.";
            throw BESInternalError(msg.str(),__FILE__,__LINE__);
        }
        d_super_chunks.push_back(currentSuperChunk);
    }
#endif

    d_chunks.push_back(chunk);
    return d_chunks.size();
}




unsigned long DmrppCommon::add_chunk(
        const string &byte_order,
        unsigned long long size,
        unsigned long long offset,
        const string &position_in_array)

{
    vector<unsigned long long> cpia_vector;
    Chunk::parse_chunk_position_in_array_string(position_in_array, cpia_vector);
    return add_chunk(byte_order, size, offset, cpia_vector);
}

unsigned long DmrppCommon::add_chunk(
        const string &byte_order,
        unsigned long long size,
        unsigned long long offset,
        const vector<unsigned long long> &position_in_array)
{
    std::shared_ptr<Chunk> chunk(new Chunk( byte_order, size, offset, position_in_array));

    d_chunks.push_back(chunk);
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
    auto chunk_refs = get_chunks();

    if (chunk_refs.size() != 1)
        throw BESInternalError(string("Expected only a single chunk for variable ") + name, __FILE__, __LINE__);

    auto chunk = chunk_refs[0];

    chunk->read_chunk();

    return chunk->get_rbuf();
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


    if(!get_chunks().empty()){
        auto first_chunk = get_chunks().front();
        if (!first_chunk->get_byte_order().empty()) {
            if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar *) "byteOrder",
                                        (const xmlChar *) first_chunk->get_byte_order().c_str()) < 0)
            throw BESInternalError("Could not write attribute byteOrder", __FILE__, __LINE__);
        }
    }

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
    // for (vector<Chunk>::iterator i = get_chunks().begin(), e = get_chunks().end(); i != e; ++i) {

    for(auto chunk: get_chunks()){

        if (xmlTextWriterStartElementNS(xml.get_writer(), (const xmlChar*)name_space.c_str(), (const xmlChar*) "chunk", NULL) < 0)
            throw BESInternalError("Could not start element chunk", __FILE__, __LINE__);

        // Get offset string:
        ostringstream offset;
        offset << chunk->get_offset();
        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "offset", (const xmlChar*) offset.str().c_str()) < 0)
            throw BESInternalError("Could not write attribute offset", __FILE__, __LINE__);

        // Get nBytes string:
        ostringstream nBytes;
        nBytes << chunk->get_size();
        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "nBytes", (const xmlChar*) nBytes.str().c_str()) < 0)
            throw BESInternalError("Could not write attribute nBytes", __FILE__, __LINE__);

        if (chunk->get_position_in_array().size() > 0) {
            // Get position in array string:
            vector<unsigned long long> pia = chunk->get_position_in_array();
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
 * @brief Print the Compact base64-encoded information.
 */
void
DmrppCommon::print_compact_element(XMLWriter &xml, const string &name_space, const std::string &encoded)
{
    // Write element "compact" with dmrpp namespace:
    ostringstream oss;
    copy(encoded.begin(), encoded.end(), ostream_iterator<char>(oss, ""));
    string sizes = oss.str();

    if (xmlTextWriterWriteElementNS(xml.get_writer(), (const xmlChar *) name_space.c_str(),
                                    (const xmlChar *) "compact", NULL,
                                    (const xmlChar *) sizes.c_str()) < 0)
        throw BESInternalError("Could not write compact element.", __FILE__, __LINE__);
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

    const vector<unsigned long long> &chunk_dim_sizes = get_chunk_dimension_sizes();

    strm << BESIndent::LMarg << "chunk dimension sizes:  [";
    for (unsigned int i = 0; i < chunk_dim_sizes.size(); i++) {
        strm << (i ? "][" : "") << chunk_dim_sizes[i];
    }
    strm << "]" << endl;

    auto chunk_refs = get_immutable_chunks();
    strm << BESIndent::LMarg << "Chunks (aka chunks):" << (chunk_refs.size() ? "" : "None Found.") << endl;
    BESIndent::Indent();
    for (auto & chunk_ref : chunk_refs) {
        strm << BESIndent::LMarg;
        chunk_ref->dump(strm);
        strm << endl;
    }
}

} // namepsace dmrpp

