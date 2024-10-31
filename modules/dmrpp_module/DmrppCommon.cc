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

#include <libdap/BaseType.h>
#include <libdap/Str.h>
#include <libdap/Byte.h>
#include <libdap/D4Attributes.h>
#include <libdap/XMLWriter.h>
#include <libdap/util.h>


#define PUGIXML_NO_XPATH
#define PUGIXML_HEADER_ONLY
#include <pugixml.hpp>

#include "url_impl.h"
#include "BESIndent.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESInternalError.h"

#include "DmrppRequestHandler.h"
#include "DmrppCommon.h"
#include "Chunk.h"
#include "byteswap_compat.h"
#include "Base64.h"

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
 *
 * Use this to clean up resources if an exception is thrown in one thread. In that case
 * this code sweeps through all of the outstanding threads and makes sure they are joined.
 * It's tempting to detach and let the existing threads call exit, but that might lead to a
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
            }
            else if (error != NULL) {
                BESDEBUG(dmrpp_3, "Joined thread " << i << ", error exit: " << *error << endl);
            }
            else {
                BESDEBUG(dmrpp_3, "Joined thread " << i << ", successful exit." << endl);
            }
        }
    }
}

/// @brief Set the value of the filters property
void DmrppCommon::set_filter(const string &value) {
    if (DmrppRequestHandler::d_emulate_original_filter_order_behavior) {
        d_filters = "";
        if (value.find("shuffle") != string::npos)
            d_filters.append(" shuffle");
        if (value.find("deflate") != string::npos)
            d_filters.append(" deflate");
        if (value.find("fletcher32") != string::npos)
            d_filters.append(" fletcher32");

        BESUtil::removeLeadingAndTrailingBlanks(d_filters);
    }
    else {
        d_filters = value;
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

    // TODO Rewrite this to use split. jhrg 5/2/22
    string space(" ");
    size_t strPos = 0;
    string strVal;

    // Are there spaces or multiple values?
    if (chunk_dims.find(space) != string::npos) {
        // Process space delimited content
        while ((strPos = chunk_dims.find(space)) != string::npos) {
            strVal = chunk_dims.substr(0, strPos);
            // TODO stoull (CDS uses uint64_t) jhrg 5/2/22
            d_chunk_dimension_sizes.push_back(strtol(strVal.c_str(), nullptr, 10));
            chunk_dims.erase(0, strPos + space.size());
        }
    }

    // If it's multivalued there's still one more value left to process
    // If it's single valued the same is true, so let's ingest that.
    d_chunk_dimension_sizes.push_back(strtol(chunk_dims.c_str(), nullptr, 10));
}

/**
 * @brief Parses the text content of the XML element h4:chunkDimensionSizes
 * into the internal vector<unsigned int> representation.
 *
 * @param compression_type_string
 */
void DmrppCommon::ingest_compression_type(const string &compression_type_string)
{
    if (compression_type_string.empty()) return;
    set_filter(compression_type_string);
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


/**
 * @brief Adds a chunk to the vector of chunk refs (byteStreams) and returns the size of the chunks internal vector.
 *
 * @param data_url The URL, file or http(s) protocol, that identifies the location of the binary object that holds
 * the referenced chunk
 * @param byte_order The stored byte order of the chunk
 * @param size The sie of the chunk
 * @param offset Chunk offset in the target dataset binary object.
 * @param position_in_array A string description of the chunks position in the array
 * with a nominal format of [x,y,...,z] where x, ..., are unsigned integers
 * @return The number of chunks in the internal chunks vector for this variable.
 */
unsigned long DmrppCommon::add_chunk(
        shared_ptr<http::url> data_url,
        const string &byte_order,
        unsigned long long size,
        unsigned long long offset,
        const string &position_in_array)
{
    vector<unsigned long long> cpia_vector;
    Chunk::parse_chunk_position_in_array_string(position_in_array, cpia_vector);
    return add_chunk(std::move(data_url), byte_order, size, offset, cpia_vector);
}

unsigned long DmrppCommon::add_chunk(
        shared_ptr<http::url> data_url,
        const string &byte_order,
        unsigned long long size,
        unsigned long long offset,
        unsigned int filter_mask,
        const string &position_in_array)
{
    vector<unsigned long long> cpia_vector;
    Chunk::parse_chunk_position_in_array_string(position_in_array, cpia_vector);
    return add_chunk(std::move(data_url), byte_order, size, offset, filter_mask,cpia_vector);
}

/**
 * @brief Adds a chunk to the vector of chunk refs (byteStreams) and returns the size of the chunks internal vector.
 *
 * @param data_url The URL, file or http(s) protocol, that identifies the location of the binary object that holds
 * the referenced chunk
 * @param byte_order The stored byte order of the chunk
 * @param size The sie of the chunk
 * @param offset Chunk offset in the target dataset binary object.
 * @param position_in_array Described as a vector of unsigned long long values
 * @return The number of chunk refs (byteStreams) held in the internal chunks vector..
 */
unsigned long DmrppCommon::add_chunk(
        shared_ptr<http::url> data_url,
        const string &byte_order,
        unsigned long long size,
        unsigned long long offset,
        const vector<unsigned long long> &position_in_array)
{
    std::shared_ptr<Chunk> chunk(new Chunk(std::move(data_url), byte_order, size, offset, position_in_array));

    d_chunks.push_back(chunk);
    return d_chunks.size();
}

unsigned long DmrppCommon::add_chunk(
        shared_ptr<http::url> data_url,
        const string &byte_order,
        unsigned long long size,
        unsigned long long offset,
        unsigned int filter_mask,
        const vector<unsigned long long> &position_in_array)
{
    std::shared_ptr<Chunk> chunk(new Chunk(std::move(data_url), byte_order, size, offset, filter_mask,position_in_array));

    d_chunks.push_back(chunk);
    return d_chunks.size();
}

unsigned long DmrppCommon::add_chunk(
        shared_ptr<http::url> data_url,
        const string &byte_order,
        unsigned long long size,
        unsigned long long offset,
        bool linked_block,
        unsigned int linked_block_index)
{
    std::shared_ptr<Chunk> chunk(new Chunk(std::move(data_url), byte_order, size, offset, linked_block,linked_block_index));

    d_chunks.push_back(chunk);
    return d_chunks.size();
}

unsigned long DmrppCommon::add_chunk(
        const string &byte_order,
        unsigned long long size,
        unsigned long long offset,
        bool linked_block,
        unsigned int linked_block_index)
{
    std::shared_ptr<Chunk> chunk(new Chunk(byte_order, size, offset, linked_block,linked_block_index));

    d_chunks.push_back(chunk);
    return d_chunks.size();
}
/**
 * @brief Adds a chunk to the vector of chunk refs (byteStreams) and returns the size of the chunks internal vector.
 *
 * NB: This method is used by build_dmrpp during the production of the dmr++ file. The fact that the URL is
 * not set is fine in that circumstance because when the DMZ parser reads the dmr++ the chunks without
 * the explicit URL (dmrpp:href attribute) will inherit the primary URL (dmrpp:href) value from the top
 * level Dataset element.
 *
 * @param byte_order The stored byte order of the chunk
 * @param size The sie of the chunk
 * @param offset Chunk offset in the target dataset binary object.
 * @param position_in_array A string description of the chunks position in the array
 * with a nominal format of [x,y,...,z] where x, ..., are unsigned integers
 * @return The number of chunk refs (byteStreams) held in the internal chunks vector..
 */
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

/**
 * @brief Adds a chunk to the vector of chunk refs (byteStreams) and returns the size of the chunks internal vector.
 *
 * NB: This method is used by build_dmrpp during the production of the dmr++ file. The fact that the URL is
 * not set is fine in that circumstance because when the DMZ parser reads the dmr++ the chunks without
 * the explicit URL (dmrpp:href attributes) will inherit the primary URL (dmrpp:href) value from the top
 * level Dataset element.
 *
 * @param byte_order The stored byte order of the chunk
 * @param size The sie of the chunk
 * @param offset Chunk offset in the target dataset binary object.
 * @param position_in_array Described as a vector of unsigned long long values
 * @return The number of chunk refs (byteStreams) held in the internal chunks vector..
 */
unsigned long DmrppCommon::add_chunk(
        const string &byte_order,
        unsigned long long size,
        unsigned long long offset,
        const vector<unsigned long long> &position_in_array)
{
    shared_ptr<Chunk> chunk(new Chunk( byte_order, size, offset,  position_in_array));
    d_chunks.push_back(chunk);
    return d_chunks.size();
}


unsigned long DmrppCommon::add_chunk(
        const string &byte_order,
        unsigned long long size,
        unsigned long long offset,
        unsigned int filter_mask,
        const vector<unsigned long long> &position_in_array)
{
    shared_ptr<Chunk> chunk(new Chunk( byte_order, size, offset, filter_mask, position_in_array));
    d_chunks.push_back(chunk);
    return d_chunks.size();
}

unsigned long DmrppCommon::add_chunk(
        const string &byte_order,
        const string &fill_value,
        libdap::Type fv_type,
        unsigned long long chunk_size,
        const vector<unsigned long long> &position_in_array)
{
    shared_ptr<Chunk> chunk(new Chunk(byte_order, fill_value, fv_type, chunk_size, position_in_array));

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
    if (get_chunks_size() != 1)
        throw BESInternalError(string("Expected only a single chunk for variable ") + name, __FILE__, __LINE__);

    auto chunk = get_immutable_chunks()[0];

    chunk->read_chunk();

    return chunk->get_rbuf();
}

// Need to obtain the buffer size for scalar structure.
char * DmrppCommon::read_atomic(const string &name, size_t & buf_size)
{
    if (get_chunks_size() != 1)
        throw BESInternalError(string("Expected only a single chunk for variable ") + name, __FILE__, __LINE__);

    auto chunk = get_immutable_chunks()[0];

    chunk->read_chunk();
    buf_size = chunk->get_rbuf_size();
    return chunk->get_rbuf();
}
/**
 * @brief Print the Chunk information.
 *
 * @note Should not be called when the d_chunks vector has no elements because it
 * will write out a <chunks> element that is going to be empty when it might just
 * be the case that the chunks have not been read.
 *
 * @note Added support for chunks that use the HDF5 Fill Value system - Those chunks
 * have _no data_ to read and thus no offset or length. They do have a Chunk Position
 * in Array and Fill Value, however. Here's an example:
 *
 *       <dmrpp:chunk  fillValue="-32678" chunkPositionInArray="[...]"/>
 */
void
DmrppCommon::print_chunks_element(XMLWriter &xml, const string &name_space)
{
    // Start element "chunks" with dmrpp namespace and attributes:
    if (xmlTextWriterStartElementNS(xml.get_writer(), (const xmlChar*)name_space.c_str(), (const xmlChar*) "chunks", NULL) < 0)
        throw BESInternalError("Could not start chunks element.", __FILE__, __LINE__);

    if (!d_filters.empty()) {
        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "compressionType", (const xmlChar*) d_filters.c_str()) < 0)
            throw BESInternalError("Could not write compression attribute.", __FILE__, __LINE__);
        if (!deflate_levels.empty()) {

            ostringstream dls;
            for (unsigned int i = 0; i <deflate_levels.size(); i++) {
                if ( i != deflate_levels.size()-1)
                    dls<<deflate_levels[i]<<" ";
                else 
                    dls<<deflate_levels[i];

            }
            BESDEBUG(dmrpp_3, "Variable deflate levels: " << dls.str()<<endl);
            if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "deflateLevel", (const xmlChar*) dls.str().c_str()) < 0)
                throw BESInternalError("Could not write compression attribute.", __FILE__, __LINE__);
 
        }

    }

    if (d_uses_fill_value && !d_fill_value_str.empty()) {
        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "fillValue", (const xmlChar*) d_fill_value_str.c_str()) < 0)
            throw BESInternalError("Could not write fillValue attribute.", __FILE__, __LINE__);
    }

    if(!d_chunks.empty()) {
        auto first_chunk = get_immutable_chunks().front();
        if (!first_chunk->get_byte_order().empty()) {
            if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar *) "byteOrder",
                                        (const xmlChar *) first_chunk->get_byte_order().c_str()) < 0)
            throw BESInternalError("Could not write attribute byteOrder", __FILE__, __LINE__);
        }
    }

    // If the disable_dio flag is true, we will set the DIO=off attribute.

    if (!d_filters.empty() && d_disable_dio == true) {
        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar *) "DIO",
                                        (const xmlChar *) "off") < 0)
            throw BESInternalError("Could not write attribute byteOrder", __FILE__, __LINE__);
    }
 
    if (!d_chunk_dimension_sizes.empty()) { //d_chunk_dimension_sizes.size() > 0) {
        // Write element "chunkDimensionSizes" with dmrpp namespace:
        ostringstream oss;
        // Note: the ostream_iterator previously used unsigned int, which may siliently generate the wrong value when
        // the size is >4G. Fix it by using unsigned long long. KY 2023-02-09
        copy(d_chunk_dimension_sizes.begin(), d_chunk_dimension_sizes.end(), ostream_iterator<unsigned long long>(oss, " "));
        string sizes = oss.str();
        sizes.erase(sizes.size() - 1, 1);    // trim the trailing space

        if (xmlTextWriterWriteElementNS(xml.get_writer(), (const xmlChar*) name_space.c_str(), (const xmlChar*) "chunkDimensionSizes", NULL,
            (const xmlChar*) sizes.c_str()) < 0) throw BESInternalError("Could not write chunkDimensionSizes attribute.", __FILE__, __LINE__);
    }

    // Start elements "chunk" with dmrpp namespace and attributes:
    for(auto chunk: get_immutable_chunks()) {

        if (chunk->get_linked_block()) {
    
            if (xmlTextWriterStartElementNS(xml.get_writer(), (const xmlChar *) name_space.c_str(),
                                            (const xmlChar *) "block", nullptr) < 0)
                throw BESInternalError("Could not start element chunk", __FILE__, __LINE__);

            // Get offset string:
            ostringstream offset;
            offset << chunk->get_offset();
            if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar *) "offset",
                                            (const xmlChar *) offset.str().c_str()) < 0)
                throw BESInternalError("Could not write attribute offset", __FILE__, __LINE__);

            // Get nBytes string:
            ostringstream nBytes;
            nBytes << chunk->get_size();
            if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar *) "nBytes",
                                            (const xmlChar *) nBytes.str().c_str()) < 0)
                throw BESInternalError("Could not write attribute nBytes", __FILE__, __LINE__);
        }

        else {

            if (xmlTextWriterStartElementNS(xml.get_writer(), (const xmlChar *) name_space.c_str(),
                                            (const xmlChar *) "chunk", nullptr) < 0)
                throw BESInternalError("Could not start element chunk", __FILE__, __LINE__);

            // Get offset string:
            ostringstream offset;
            offset << chunk->get_offset();
            if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar *) "offset",
                                            (const xmlChar *) offset.str().c_str()) < 0)
                throw BESInternalError("Could not write attribute offset", __FILE__, __LINE__);

            // Get nBytes string:
            ostringstream nBytes;
            nBytes << chunk->get_size();
            if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar *) "nBytes",
                                            (const xmlChar *) nBytes.str().c_str()) < 0)
                throw BESInternalError("Could not write attribute nBytes", __FILE__, __LINE__);


            if (chunk->get_position_in_array().size() > 0) {
                // Get position in array string:
                vector<unsigned long long> pia = chunk->get_position_in_array();
                ostringstream oss;
                oss << "[";

                // Note: the ostream_iterator previously used unsigned int, which may siliently generate the wrong value when
                // the size is >4G. Fix it by using unsigned long long. KY 2023-02-09
                copy(pia.begin(), pia.end(), ostream_iterator<unsigned long long>(oss, ","));
                string pia_str = oss.str();
                if (pia.size() > 0) pia_str.replace(pia_str.size() - 1, 1, "]"); // replace the trailing ',' with ']'
                if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar *) "chunkPositionInArray",
                                                (const xmlChar *) pia_str.c_str()) < 0)
                    throw BESInternalError("Could not write attribute position in array", __FILE__, __LINE__);

                // Filter mask only applies to the chunking storage. So check here. Get the filter mask string if the filter mask is not zero.
                if (chunk->get_filter_mask() != 0) {
                    ostringstream fm;
                    fm << chunk->get_filter_mask();
                    if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar *) "fm",
                                                    (const xmlChar *) fm.str().c_str()) < 0)
                        throw BESInternalError("Could not write attribute fm(filter mask)", __FILE__, __LINE__);
                }

            }
        }

        // End element "chunk":
        if (xmlTextWriterEndElement(xml.get_writer()) < 0) throw BESInternalError("Could not end chunk element", __FILE__, __LINE__);
    }

    if (xmlTextWriterEndElement(xml.get_writer()) < 0) throw BESInternalError("Could not end chunks element", __FILE__, __LINE__);
}

/**
 * @brief Print the Compact base64-encoded information.
 * @see https://gnome.pages.gitlab.gnome.org/libxml2/devhelp/libxml2-xmlwriter.html#xmlTextWriterWriteElementNS
 */
void
DmrppCommon::print_compact_element(XMLWriter &xml, const string &name_space, const std::string &encoded) const
{
    if (xmlTextWriterWriteElementNS(xml.get_writer(), (const xmlChar *) name_space.c_str(),
                                    (const xmlChar *) "compact", NULL,
                                    (const xmlChar *) encoded.c_str()) < 0)
        throw BESInternalError("Could not write compact element.", __FILE__, __LINE__);
}

void
DmrppCommon::print_missing_data_element(const XMLWriter &xml, const string &name_space, const std::string &encoded) const
{

    if (xmlTextWriterWriteElementNS(xml.get_writer(), (const xmlChar *) name_space.c_str(),
                                    (const xmlChar *) "missingdata", nullptr,
                                    (const xmlChar *) encoded.c_str()) < 0)
        throw BESInternalError("Could not write missingdata element.", __FILE__, __LINE__);
}

void
DmrppCommon::print_special_structure_element(const XMLWriter &xml, const string &name_space, const std::string &encoded) const
{

    if (xmlTextWriterWriteElementNS(xml.get_writer(), (const xmlChar *) name_space.c_str(),
                                    (const xmlChar *) "specialstructuredata", nullptr,
                                    (const xmlChar *) encoded.c_str()) < 0)
        throw BESInternalError("Could not write special structure element.", __FILE__, __LINE__);

}
 
/**
 * @brief Print the DMR++ missing data XML element
 * @param xml
 * @param name_space
 * @param data
 * @param length
 */
void
DmrppCommon::print_missing_data_element(const XMLWriter &xml, const string &name_space, const char *data, int length) const
{

    if (xmlTextWriterStartElementNS(xml.get_writer(), (const xmlChar *) name_space.c_str(),
                                    (const xmlChar *) "missingdata", nullptr) < 0)
        throw BESInternalError("Could not start missingdata element.", __FILE__, __LINE__);

    if (xmlTextWriterWriteBase64(xml.get_writer(), data, 0, length) < 0)
        throw BESInternalError("Could not write the base 64 data for the  missingdata element.", __FILE__, __LINE__);

    if (xmlTextWriterEndElement(xml.get_writer()) < 0)
        throw BESInternalError("Could not end missingdata element.", __FILE__, __LINE__);
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

    if (!bt.name().empty()) {
        BESDEBUG(dmrpp_3, "Variable full path: " << bt.FQN() <<endl);
        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar*) "name", (const xmlChar*)bt.name().c_str()) < 0)
            throw InternalErr(__FILE__, __LINE__, "Could not write attribute for name");
    }

    if (bt.is_dap4())
        bt.attributes()->print_dap4(xml);

    if (!bt.is_dap4() && bt.get_attr_table().get_size() > 0)
        bt.get_attr_table().print_xml_writer(xml);

    // This is the code added to libdap::BaseType::print_dap4(). jhrg 5/10/18
    // If the scalar variable with contiguous contains a fillvalue, also needs to output. ky 07/12/22
    if (DmrppCommon::d_print_chunks && (get_chunks_size() > 0 || get_uses_fill_value()))
        print_chunks_element(xml, DmrppCommon::d_ns_prefix);

    // print scalar value for special missing variables. 
    if (DmrppCommon::d_print_chunks && is_missing_data() && bt.read_p())  {
        // Now we only handle dods_byte_c.
        if (bt.type() == dods_byte_c) {
            auto sca_var = dynamic_cast<libdap::Byte*>(this); 
            uint8_t sca_var_value = sca_var->value();
            string encoded = base64::Base64::encode(&sca_var_value, 1);
            print_missing_data_element(xml, DmrppCommon::d_ns_prefix, encoded);
        }
        else {
            string err_msg = "Bad type for scalar missing variable: " + bt.name();
            throw BESInternalError(err_msg, __FILE__, __LINE__);
        }
    }

    // print scalar value for compact storage.
    if (DmrppCommon::d_print_chunks && is_compact_layout() && bt.read_p())  {

        switch (bt.type()) {
            case dods_byte_c:
            case dods_char_c:
            case dods_int8_c:
            case dods_uint8_c:
            case dods_int16_c:
            case dods_uint16_c:
            case dods_int32_c:
            case dods_uint32_c:
            case dods_int64_c:
            case dods_uint64_c:

            case dods_enum_c:

            case dods_float32_c:
            case dods_float64_c: {
                u_int8_t *values = 0;
                try {
                    size_t size = bt.buf2val(reinterpret_cast<void **>(&values));
                    string encoded = base64::Base64::encode(values, size);
                    print_compact_element(xml, DmrppCommon::d_ns_prefix, encoded);
                    delete[] values;
                }
                catch (...) {
                    delete[] values;
                    throw;
                }
                break;
            }

            case dods_str_c: {
                
                auto str = dynamic_cast<libdap::Str*>(this); 
                string str_val = str->value();
                string encoded = base64::Base64::encode(reinterpret_cast<const u_int8_t *>(str_val.c_str()), str_val.size());
                print_compact_element(xml, DmrppCommon::d_ns_prefix, encoded);
 
                break;
            }

            default:
                throw InternalErr(__FILE__, __LINE__, "Vector::val2buf: bad type");
        }


    }
    if (xmlTextWriterEndElement(xml.get_writer()) < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not end " + bt.type_name() + " element");
}

void DmrppCommon::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "is_filters_empty:             " << (is_filters_empty() ? "true" : "false") << endl;
    strm << BESIndent::LMarg << "filters: " << (d_filters.c_str()) << endl;

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

/**
 * @brief Load chunk information for this variable.
 * @param btp Load the chunk information for/into this variable
 */
void
DmrppCommon::load_chunks(BaseType *btp) {
    d_dmz->load_chunks(btp);
}

/**
 * @brief Load the attribute information for this variable
 * @param btp
 */
void
DmrppCommon::load_attributes(libdap::BaseType *btp)
{
    d_dmz->load_attributes(btp);
}

} // namespace dmrpp

