// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2023 OPeNDAP
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
// Created by ndp on 11/11/23.
//

#ifndef BES_VLSA_UTIL_H
#define BES_VLSA_UTIL_H

#include <string>
#include <sstream>
#include <zlib.h>

#define PUGIXML_NO_XPATH
#define PUGIXML_HEADER_ONLY
#include <pugixml.hpp>

#include <libdap/XMLWriter.h>

#include "vlsa_util.h"
#include "DmrppArray.h"
#include "DmrppNames.h"
#include "Base64.h"
#include "BESInternalError.h"
#include "BESDebug.h"
#include "BESLog.h"


#define prolog std::string("vlsa_util::").append(__func__).append("() - ")
#define VLSA "vlsa"
#define VLSA_VERBOSE "vlsa:verbose"
#define VLSA_VERBOSE "vlsa:verbose"
#define VALUE_COMPRESSION_THRESHOLD 300

using namespace std;

namespace vlsa {

string vlsa_element_name(DMRPP_VLSA_ELEMENT);
string vlsa_value_element_name(DMRPP_VLSA_VALUE_ELEMENT);
string vlsa_value_size_attr_name(DMRPP_VLSA_VALUE_SIZE_ATTR);

/**
 * @brief Maps zlib return code to a string value;
 * @param retval The zlib return value;
 * @return A string representation of the return code's meaning.
 */
string zlib_msg(int retval)
{
    string msg;
    switch (retval) {
        case Z_OK:
            msg = "Z_OK";
            break;
        case Z_STREAM_END:
            msg = "Z_STREAM_END";
            break;
        case Z_NEED_DICT:
            msg = "Z_NEED_DICT";
            break;
        case Z_ERRNO:
            msg = "Z_ERRNO";
            break;
        case Z_STREAM_ERROR:
            msg = "Z_STREAM_ERROR";
            break;
        case Z_DATA_ERROR:
            msg = "Z_DATA_ERROR";
            break;
        case Z_MEM_ERROR:
            msg = "Z_MEM_ERROR";
            break;
        case Z_BUF_ERROR:
            msg = "Z_BUF_ERROR";
            break;
        case Z_VERSION_ERROR:
            msg = "Z_VERSION_ERROR";
            break;
        default:
            msg = "UNKNOWN ZLIB RETURN CODE";
            break;
    }
    return msg;
}

constexpr unsigned int W = 10;
constexpr unsigned int R = 8;

/** _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._
 * @brief Attempts to compress the source_string bytes and then base64 encode the compressed bytes.
 * The compress function has some minimum size requirement of about 100 bytes, but the size of the
 * compressed bytes isn't reliably smaller then the source_string until about 300 bytes.
 *
 * @param source_string The string to encode.
 * @return The source string compressed and then base64 encoded.
 */
std::string encode(const std::string &source_string) {
    BESDEBUG(VLSA, prolog << "BEGIN\n");

    string encoded;
    auto source_size = source_string.size();
    // Copy the stuff into a vector...
    vector<Bytef> src(source_string.begin(), source_string.end());
    BESDEBUG(VLSA, prolog << "     source_string.size(): " << source_string.size() << " bytes. \n");
    BESDEBUG(VLSA, prolog << " vector<Bytef> src.size(): " << src.size() << " bytes.\n");

#ifndef NDEBUG
    if (BESDebug::IsSet(VLSA_VERBOSE)) {
        stringstream dbg_msg;
        dbg_msg << prolog << "Source document copied to vector<Bytef>: \n";
        for (uint64_t i = 0; i < source_size; i++) {
            dbg_msg << (char) src[i];
        }
        dbg_msg << "\n";
        BESDEBUG(VLSA_VERBOSE, dbg_msg.str());
    }
#endif
    vector<Bytef> compressed_src;
    compressed_src.resize(source_size);
    unsigned long compressed_src_size = source_size;

    int retval = compress(compressed_src.data(), &compressed_src_size, src.data(), source_size);
    BESDEBUG(VLSA, prolog << "        compress() retval: " << setw(W) << retval
                          << " (" << zlib_msg(retval) << ")\n");

    if (retval != 0) {
        stringstream msg;
        msg << "Failed to compress source string. \n";
        msg << "   compress() retval: " << retval << " (" << zlib_msg(retval) << ")\n";
        msg << "         source_size: " << source_size << "\n";
        msg << " compressed_src_size: " << compressed_src_size << "\n";
        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }

    double c_ratio = ((double) source_size) / ((double) compressed_src_size);

    BESDEBUG(VLSA, prolog << "                source len: " << setw(W) << source_size << "\n");
    BESDEBUG(VLSA, prolog << "  compressed source binary: " << setw(W) << compressed_src_size <<
                          " src:csb=" << setw(R) << c_ratio << "\n");

    string base64_compressed_payload;
    base64_compressed_payload = base64::Base64::encode(compressed_src.data(), compressed_src_size);
    double base64_compressed_ratio = ((double) source_size) / ((double) base64_compressed_payload.size());
    BESDEBUG(VLSA, prolog << " base64_encoded_compressed: " << setw(W) << base64_compressed_payload.size() <<
                          " src:b64=" << setw(R) << base64_compressed_ratio << "\n");

    BESDEBUG(VLSA_VERBOSE, prolog << "base64_compressed_payload:\n" << base64_compressed_payload << "\n");
    BESDEBUG(VLSA, prolog << "END\n");
    return base64_compressed_payload;
}

/** _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._ _._
 *
 * @param encoded
 * @param expected_size
 * @return
 */
string decode(const string &encoded, uint64_t expected_size) {
    BESDEBUG(VLSA, prolog << "BEGIN\n");
    BESDEBUG(VLSA, prolog << "             expected_size: " << setw(W) << expected_size << "\n");
    BESDEBUG(VLSA, prolog << "            encoded.size(): " << setw(W) << encoded.size() << "\n");

    string decoded_string;
    std::vector<u_int8_t> decoded = base64::Base64::decode(encoded);
    BESDEBUG(VLSA, prolog << "   (base64) decoded.size(): " << setw(W) << decoded.size() << "\n");

    vector<Bytef> result_bytes;
    uLongf result_size = expected_size;

    BESDEBUG(VLSA, prolog << "               result_size: " << setw(W) << result_size << "\n");

    result_bytes.resize(result_size);
    BESDEBUG(VLSA, prolog << "       result_bytes.size(): " << setw(W) << result_bytes.size() << "\n");

    int retval = uncompress(result_bytes.data(), &result_size, decoded.data(), decoded.size());
    if(retval !=0){
        stringstream msg;
        msg << prolog << "Failed to decompress payload. \n";
        msg << "                 retval: " << retval << " (" << zlib_msg(retval) << ")\n";
        msg << "            result_size: " << result_size << "\n";
        msg << "          expected_size: " << expected_size << "\n";
        msg << "    result_bytes.size(): " << expected_size << "\n";

        throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }
    BESDEBUG(VLSA, prolog << "  uncompress() result_size: " << setw(W) << result_size << "\n");
    BESDEBUG(VLSA, prolog << "             expected_size: " << setw(W) << expected_size << "\n");

    std::string result_string(result_bytes.begin(), result_bytes.end());
    BESDEBUG(VLSA, prolog << "      result_string.size(): " << setw(W) << result_string.size() << "\n");
    if(result_string.size() != expected_size){
        stringstream msg;
        msg << prolog << "Result size " << result_string.size() << " does not match expected size " << expected_size;
                throw BESInternalError(msg.str(), __FILE__, __LINE__);
    }

    BESDEBUG(VLSA_VERBOSE, prolog << "RESULT DOC: \n" << result_string << "\n");
    BESDEBUG(VLSA, prolog << "END\n");
    return result_string;
}



void write_value(libdap::XMLWriter &xml, const std::string &value)
{

    if (xmlTextWriterStartElement(xml.get_writer(), (const xmlChar *) vlsa_value_element_name.c_str()) < 0) {
        stringstream msg;
        msg <<  prolog << "Could not begin '" + vlsa_value_element_name + "' element.";
        throw BESInternalError( msg.str(), __FILE__, __LINE__);
    }

    if(value.size() > VALUE_COMPRESSION_THRESHOLD) {

        if (xmlTextWriterWriteAttribute(xml.get_writer(), (const xmlChar *) vlsa_value_size_attr_name.c_str(),
                                        (const xmlChar *) to_string(value.size()).c_str()) < 0) {
            stringstream msg;
            msg << prolog << "Could not write '" << vlsa_value_size_attr_name << "' (size) attribute.";
            throw BESInternalError( msg.str(), __FILE__, __LINE__);
        }
        string encoded = encode(value);

        if (xmlTextWriterWriteString(xml.get_writer(), (const xmlChar *) encoded.c_str()) < 0) {
            stringstream msg;
            msg << prolog <<  "Could not write text into element '" << vlsa_value_element_name << "'";
            throw BESInternalError( msg.str(), __FILE__, __LINE__);
        }
    }
    else {
        if (xmlTextWriterWriteString(xml.get_writer(), (const xmlChar *) value.c_str()) < 0) {
            stringstream msg;
            msg << prolog <<  "Could not write text into element '"<< vlsa_value_element_name << "'";
            throw BESInternalError( msg.str(), __FILE__, __LINE__);
        }
    }

    if (xmlTextWriterEndElement(xml.get_writer()) < 0) {
        stringstream msg;
        msg << prolog << "Could not end '" + vlsa_value_element_name + "' element";
        throw BESInternalError( msg.str(), __FILE__, __LINE__);
    }


}

void write(libdap::XMLWriter &xml, dmrpp::DmrppArray &a)
{
    auto values = a.get_str();

    if (xmlTextWriterStartElement(xml.get_writer(), (const xmlChar *) vlsa_element_name.c_str()) < 0) {
        stringstream msg;
        msg << prolog << "Could not write " << vlsa_element_name << " element";
        throw BESInternalError( msg.str(), __FILE__, __LINE__);
    }

    for(const auto &value : values) {
        write_value(xml,value);
    }

    if (xmlTextWriterEndElement(xml.get_writer()) < 0) {
        stringstream msg;
        msg << prolog << "Could not end " << vlsa_element_name << " element";
        throw BESInternalError( msg.str(), __FILE__, __LINE__);
    }
}

/**
 * @brief Reads the vale of a vlsa value 'v', element.
 * TODO Is there an advanage to passing the ref or would returning the string value be equivalent?
 * @param v The element to evaluate
 * @param value The (decoded) string value of v
 */
void read_value(const pugi::xml_node &v, std::string &value)
{
    if (v.name() == vlsa_value_element_name ) {
        // We check for the presence of the size attribut, vlsa_value_size_attr_name
        // If present it means the content was compressed and the base64 encoded
        // and we have to decode it.
        pugi::xml_attribute esize = v.attribute(vlsa_value_size_attr_name.c_str());
        if (esize) {
            uint64_t value_size = stoull(esize.value());
            value = decode(v.child_value(), value_size);
        } else {
            value = v.child_value();
        }
        BESDEBUG(VLSA, prolog << "value: '" << value << "'");
    }
}
void read(const pugi::xml_node &vlsa_element, std::vector<std::string> &entries)
{
    if (vlsa_element.name() != vlsa_element_name ) { return; }

    // Chunks for this node will be held in the var_node siblings.
    for (auto v = vlsa_element.child(vlsa_value_element_name.c_str()); v; v = v.next_sibling()) {
        string value;
        read_value(v, value);
        entries.emplace_back(value);
    }
}

} // namespace vlsa
#endif //BES_VLSA_UTIL_H