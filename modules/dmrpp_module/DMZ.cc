// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2021 OPeNDAP, Inc.
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

#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include <libdap/BaseType.h>
#include <libdap/Array.h>
#include <libdap/Type.h>
#include <libdap/D4Dimensions.h>
#include <libdap/D4Group.h>
#include <libdap/D4BaseTypeFactory.h>
#include <libdap/D4Enum.h>
#include <libdap/D4EnumDefs.h>
#include <libdap/D4Dimensions.h>
#include <libdap/DMR.h>
#include <libdap/util.h>        // is_simple_type()

#include "url_impl.h"           // see bes/http
#include "DMZ.h"                // this includes the rapidxml header
#include "BESInternalError.h"
#include "BESDebug.h"

using namespace rapidxml;
using namespace std;
using namespace libdap;

#define PARSER "dmz"
#define prolog std::string("DMZ::").append(__func__).append("() - ")

namespace dmrpp {

const std::set<std::string> variable_elements{"Byte", "Float32", "Int16"};

static const string dmrpp_namespace = "http://xml.opendap.org/dap/dmrpp/1.0.0#";

/**
 * @brief Build a DMZ object and initialize it using a DMR++ XML document
 * @param file_name The DMR++ XML document to parse.
 * @exception BESInternalError if file_name cannot be parsed
 */
DMZ::DMZ(const string &file_name)
{
    ifstream ifs(file_name, ios::in | ios::binary | ios::ate);
    if (!ifs)
        throw BESInternalError(string("Could not open DMR++ XML file: ").append(file_name), __FILE__, __LINE__);

    ifstream::pos_type file_size = ifs.tellg();
    if (file_size == 0)
        throw BESInternalError(string("DMR++ XML file is empty: ").append(file_name), __FILE__, __LINE__);

    ifs.seekg(0, ios::beg);

    d_xml_text.resize(file_size + 1LL);   // Add space for text and null termination
    ifs.read(d_xml_text.data(), file_size);
    if (!ifs)
        throw BESInternalError(string("DMR++ XML file seek or read failure: ").append(file_name), __FILE__, __LINE__);

    d_xml_text[file_size] = '\0';

    // TODO catch  rapidxml::parse_error
    d_xml_doc.parse<0>(d_xml_text.data());    // 0 means default parse flags

    if (!d_xml_doc.first_node())
        throw BESInternalError("No DMR++ data present.", __FILE__, __LINE__);
}

#if 0
void dmr_start_element(void *p, const xmlChar *l, const xmlChar *prefix, const xmlChar *URI,
                                        int nb_namespaces, const xmlChar **namespaces, int nb_attributes, int /*nb_defaulted*/, const xmlChar **attributes)
{
    DmrppParserSax2 *parser = static_cast<DmrppParserSax2*>(p);
    const char *localname = reinterpret_cast<const char *>(l);

    string this_element_ns_name(URI ? (char *) URI : "null");

    if (parser->get_state() != parser_error) {
        string dap4_ns_name = DapXmlNamspaces::getDapNamespaceString(DAP_4_0);
        BESDEBUG(PARSER, prolog << "dap4_ns_name:         " << dap4_ns_name << endl);

        if (this_element_ns_name == dmrpp_namespace) {
            if (strcmp(localname, "chunkDimensionSizes") == 0) {
                BESDEBUG(PARSER, prolog << "Found dmrpp:chunkDimensionSizes element. Pushing state." << endl);
                parser->push_state(inside_dmrpp_chunkDimensionSizes_element);
            }
            else if (strcmp(localname, "compact") == 0) {
                BESDEBUG(PARSER, prolog << "Found dmrpp:compact element. Pushing state." << endl);
                parser->push_state(inside_dmrpp_compact_element);
            }
            else {
                BESDEBUG(PARSER,
                         prolog << "Start of element in dmrpp namespace: " << localname << " detected." << endl);
                parser->push_state(inside_dmrpp_object);
            }
        }
        else if (this_element_ns_name != dap4_ns_name) {
            BESDEBUG(PARSER, prolog << "Start of non DAP4 element: " << localname << " detected." << endl);
            parser->push_state(not_dap4_element);
        }
    }

    BESDEBUG(PARSER, prolog << "Start element " << localname << "  prefix:  " << (prefix ? (char *) prefix : "null") << "  ns: "
                            << this_element_ns_name << " (state: " << states[parser->get_state()] << ")" << endl);

    switch (parser->get_state()) {
        case parser_start:
            if (is_not(localname, "Dataset"))
                DmrppParserSax2::dmr_error(parser, "Expected DMR to start with a Dataset element; found '%s' instead.",
                                           localname);

            parser->root_ns = URI ? (const char *) URI : "";

#if 0
            parser->transfer_xml_attrs(attributes, nb_attributes);
#endif

            if (parser->check_required_attribute(string("name"), attributes, nb_attributes)) parser->dmr()->set_name(parser->get_attribute_val("name", attributes, nb_attributes));

            if (parser->check_attribute("dapVersion", attributes, nb_attributes))
                parser->dmr()->set_dap_version(parser->get_attribute_val("dapVersion", attributes, nb_attributes));

            if (parser->check_attribute("dmrVersion", attributes, nb_attributes))
                parser->dmr()->set_dmr_version(parser->get_attribute_val("dmrVersion", attributes, nb_attributes));

            if (parser->check_attribute("base", attributes, nb_attributes)) {
                parser->dmr()->set_request_xml_base(parser->get_attribute_val("base", attributes, nb_attributes));
            }
            BESDEBUG(PARSER, prolog << "Dataset xml:base is set to '" << parser->dmr()->request_xml_base() << "'" << endl);

            if (parser->check_attribute("href", attributes, nb_attributes)) {
                bool trusted = false;
                if (parser->check_attribute("trust", attributes, nb_attributes)) {
                    string value = parser->get_attribute_val("trust", attributes, nb_attributes);
                    trusted = value == "true";
                }
                string href  = parser->get_attribute_val("href", attributes, nb_attributes);
                parser->dmrpp_dataset_href  = shared_ptr<http::url>(new http::url(href,trusted));
                BESDEBUG(PARSER, prolog << "Processed 'href' value into data_url. href: " << parser->dmrpp_dataset_href->str() << (trusted?"(trusted)":"") << endl);

                //######################################################################################################
                // Stop parser EffectiveUrl resolution (ndp - 08/27/2021)
                // I dropped this because:
                // - The Chunk::get_data_url() method calls EffectiveUrlCache::TheCache()->get_effective_url(data_url)
                // - EffectiveUrlCache::TheCache()->get_effective_url(data_url) method is thread safe
                // - By dropping these calls from the parser, which is in a single threaded section of the code we can
                //   resolve the URL during a multithreaded operation (reading the chunks) and reduce the overall
                //   time cost of resolving all of the chunk URLs with concurrency.
                // -----------------------------------------------------------------------------------------------------
                //BESDEBUG(PARSER, prolog << "Attempting to locate and cache the effective URL for Dataset URL: " << parser->dmrpp_dataset_href->str() << endl);
                //auto effective_url = EffectiveUrlCache::TheCache()->get_effective_url(parser->dmrpp_dataset_href);
                //BESDEBUG(PARSER, prolog << "EffectiveUrlCache::get_effective_url() returned: " << effective_url->str() << endl);
                //######################################################################################################

            }
            BESDEBUG(PARSER, prolog << "Dataset dmrpp:href is set to '" << parser->dmrpp_dataset_href->str() << "'" << endl);

            if (!parser->root_ns.empty()) parser->dmr()->set_namespace(parser->root_ns);

            // Push the root Group on the stack
            parser->push_group(parser->dmr()->root());

            parser->push_state(inside_dataset);

            break;

            // Both inside dataset and inside group can have the same stuff.
            // The difference is that the Dataset holds the root group, which
            // must be present; other groups are optional
        case inside_dataset:
        case inside_group:
            if (parser->process_enum_def(localname, attributes, nb_attributes))
                parser->push_state(inside_enum_def);
            else if (parser->process_dimension_def(localname, attributes, nb_attributes))
                parser->push_state(inside_dim_def);
            else if (parser->process_group(localname, attributes, nb_attributes))
                parser->push_state(inside_group);
            else if (parser->process_variable(localname, attributes, nb_attributes))
                // This will push either inside_simple_type or inside_structure
                // onto the parser state stack.
                break;
            else if (parser->process_attribute(localname, attributes, nb_attributes))
                // This will push either inside_attribute, inside_attribute_container
                // or inside_otherxml_attribute onto the parser state stack
                break;
            else
                DmrppParserSax2::dmr_error(parser,
                                           "Expected an Attribute, Enumeration, Dimension, Group or variable element; found '%s' instead.",
                                           localname);
            break;

        case inside_attribute_container:
            if (parser->process_attribute(localname, attributes, nb_attributes))
                break;
            else
                DmrppParserSax2::dmr_error(parser, "Expected an Attribute element; found '%s' instead.", localname);
            break;

        case inside_attribute:
            if (parser->process_attribute(localname, attributes, nb_attributes))
                break;
            else if (strcmp(localname, "Value") == 0)
                parser->push_state(inside_attribute_value);
            else
                dmr_error(parser, "Expected an 'Attribute' or 'Value' element; found '%s' instead.", localname);
            break;

        case inside_attribute_value:
            // Attribute values are processed by the end element code.
            break;

        case inside_other_xml_attribute:
            parser->other_xml_depth++;

            // Accumulate the elements here
            parser->other_xml.append("<");
            if (prefix) {
                parser->other_xml.append((const char *) prefix);
                parser->other_xml.append(":");
            }
            parser->other_xml.append(localname);

            if (nb_namespaces != 0) {
                parser->transfer_xml_ns(namespaces, nb_namespaces);

                for (map<string, string>::iterator i = parser->namespace_table.begin(); i != parser->namespace_table.end();
                     ++i) {
                    parser->other_xml.append(" xmlns");
                    if (!i->first.empty()) {
                        parser->other_xml.append(":");
                        parser->other_xml.append(i->first);
                    }
                    parser->other_xml.append("=\"");
                    parser->other_xml.append(i->second);
                    parser->other_xml.append("\"");
                }
            }

            if (nb_attributes != 0) {
#if 0
                parser->transfer_xml_attrs(attributes, nb_attributes);
#endif
                for (XMLAttrMap::iterator i = parser->xml_attr_begin(); i != parser->xml_attr_end(); ++i) {
                    parser->other_xml.append(" ");
                    if (!i->second.prefix.empty()) {
                        parser->other_xml.append(i->second.prefix);
                        parser->other_xml.append(":");
                    }
                    parser->other_xml.append(i->first);
                    parser->other_xml.append("=\"");
                    parser->other_xml.append(i->second.value);
                    parser->other_xml.append("\"");
                }
            }

            parser->other_xml.append(">");
            break;

        case inside_enum_def:
            // process an EnumConst element
            if (parser->process_enum_const(localname, attributes, nb_attributes))
                parser->push_state(inside_enum_const);
            else
                dmr_error(parser, "Expected an 'EnumConst' element; found '%s' instead.", localname);
            break;

        case inside_enum_const:
            // No content; nothing to do
            break;

        case inside_dim_def:
            // No content; nothing to do
            break;

        case inside_dim:
            // No content.
            break;

        case inside_map:
            // No content.
            break;

        case inside_simple_type:
            if (parser->process_attribute(localname, attributes, nb_attributes))
                break;
            else if (parser->process_dimension(localname, attributes, nb_attributes))
                parser->push_state(inside_dim);
            else if (parser->process_map(localname, attributes, nb_attributes))
                parser->push_state(inside_map);
            else
                dmr_error(parser, "Expected an 'Attribute', 'Dim' or 'Map' element; found '%s' instead.", localname);
            break;

        case inside_constructor:
            if (parser->process_variable(localname, attributes, nb_attributes))
                // This will push either inside_simple_type or inside_structure
                // onto the parser state stack.
                break;
            else if (parser->process_attribute(localname, attributes, nb_attributes))
                break;
            else if (parser->process_dimension(localname, attributes, nb_attributes))
                parser->push_state(inside_dim);
            else if (parser->process_map(localname, attributes, nb_attributes))
                parser->push_state(inside_map);
            else
                DmrppParserSax2::dmr_error(parser,
                                           "Expected an Attribute, Dim, Map or variable element; found '%s' instead.", localname);
            break;

        case not_dap4_element:
            BESDEBUG(PARSER, prolog << "SKIPPING unexpected element. localname: " << localname << "namespace: "
                                    << this_element_ns_name << endl);
            break;

        case inside_dmrpp_compact_element:
            if (parser->process_dmrpp_compact_start(localname)) {
                BESDEBUG(PARSER, prolog << "Call to parser->process_dmrpp_compact_start() completed." << endl);
            }
            break;

        case inside_dmrpp_object: {
            BESDEBUG(PARSER, prolog << "Inside dmrpp namespaced element. localname: " << localname << endl);
            assert(this_element_ns_name == dmrpp_namespace);

#if 0
            parser->transfer_xml_attrs(attributes, nb_attributes); // load up xml_attrs
#endif

            BaseType *bt = parser->top_basetype();
            if (!bt) throw BESInternalError("Could locate parent BaseType during parse operation.", __FILE__, __LINE__);

            DmrppCommon *dc = dynamic_cast<DmrppCommon*>(bt);   // Get the Dmrpp common info
            if (!dc)
                throw BESInternalError("Could not cast BaseType to DmrppType in the drmpp handler.", __FILE__, __LINE__);

            // Ingest the dmrpp:chunks element and it attributes
            if (strcmp(localname, "chunks") == 0) {
                BESDEBUG(PARSER, prolog << "DMR++ chunks element. localname: " << localname << endl);

                if (parser->check_attribute("compressionType", attributes, nb_attributes)) {
                    string compression_type_string(parser->get_attribute_val("compressionType", attributes, nb_attributes));
                    dc->ingest_compression_type(compression_type_string);

                    BESDEBUG(PARSER, prolog << "Processed attribute 'compressionType=\"" <<
                                            compression_type_string << "\"'" << endl);
                }
                else {
                    BESDEBUG(PARSER, prolog << "There was no 'compressionType' attribute associated with the variable '"
                                            << bt->type_name() << " " << bt->name() << "'" << endl);
                }

                if (parser->check_attribute("byteOrder", attributes, nb_attributes)) {
                    string byte_order_string(parser->get_attribute_val("byteOrder", attributes, nb_attributes));
                    dc->ingest_byte_order(byte_order_string);

                    BESDEBUG(PARSER, prolog << "Processed attribute 'byteOrder=\"" << byte_order_string << "\"'" << endl);
                }
                else {
                    BESDEBUG(PARSER, prolog << "There was no 'byteOrder' attribute associated with the variable '" << bt->type_name()
                                            << " " << bt->name() << "'" << endl);
                }
            }
                // Ingest an dmrpp:chunk element and its attributes
            else if (strcmp(localname, "chunk") == 0) {
                string data_url_str = "unknown_data_location";
                shared_ptr<http::url> data_url;

                if (parser->check_attribute("href", attributes, nb_attributes)) {
                    bool trusted = false;
                    if (parser->check_attribute("trust", attributes, nb_attributes)) {
                        string value = parser->get_attribute_val("trust", attributes, nb_attributes);
                        trusted = value == "true";
                    }

                    // This is the chunk elements href that we check.
                    data_url_str = parser->get_attribute_val("href", attributes, nb_attributes);
                    data_url = shared_ptr<http::url> ( new http::url(data_url_str,trusted));
                    BESDEBUG(PARSER, prolog << "Processed 'href' value into data_url. href: " << data_url->str() << (trusted?"":"(trusted)") << endl);
                    //######################################################################################################
                    // Stop parser EffectiveUrl resolution (ndp - 08/27/2021)
                    // I dropped this because:
                    // - The Chunk::get_data_url() method calls EffectiveUrlCache::TheCache()->get_effective_url(data_url)
                    // - EffectiveUrlCache::TheCache()->get_effective_url(data_url) method is thread safe
                    // - By dropping these calls from the parser, which is in a single threaded section of the code, we can
                    //   resolve the URL during a multi-threaded operation (reading the chunks) and reduce the overall
                    //   time cost of resolving all of the chunk URLs with concurrency.
                    // -----------------------------------------------------------------------------------------------------
                    // We may have to cache the last accessed/redirect URL for data_url here because this URL
                    // may be unique to this chunk.

                    //BESDEBUG(PARSER, prolog << "Attempting to locate and cache the effective URL for Chunk URL: " << data_url->str() << endl);
                    //auto effective_url = EffectiveUrlCache::TheCache()->get_effective_url(data_url);
                    //BESDEBUG(PARSER, prolog << "EffectiveUrlCache::get_effective_url() returned: " << effective_url->str() << endl);
                    //######################################################################################################

                }
                else {
                    BESDEBUG(PARSER, prolog << "No attribute 'href' located. Trying Dataset/@dmrpp:href..." << endl);
                    // This bit of magic sets the URL used to get the data and it's
                    // magic in part because it may be a file or an http URL
                    data_url = parser->dmrpp_dataset_href;
                    // We don't have to conditionally cache parser->dmrpp_dataset_href  here because that was
                    // done in the evaluation of the parser_start case.
                    BESDEBUG(PARSER, prolog << "Processing dmrpp:href into data_url. dmrpp:href='" << data_url->str() << "'" << endl);
                }

                if (data_url->protocol() != HTTP_PROTOCOL && data_url->protocol() != HTTPS_PROTOCOL && data_url->protocol() != FILE_PROTOCOL) {
                    BESDEBUG(PARSER, prolog << "data_url does NOT start with 'http://', 'https://' or 'file://'. "
                                               "Retrieving default catalog root directory" << endl);

                    // Now we try to find the default catalog. If we can't find it we punt and leave it be.
                    BESCatalog *defcat = BESCatalogList::TheCatalogList()->default_catalog();
                    if (!defcat) {
                        BESDEBUG(PARSER, prolog << "Not able to find the default catalog." << endl);
                    }
                    else {
                        // Found the catalog so we get the root dir; make a file URL.
                        BESCatalogUtils *utils = BESCatalogList::TheCatalogList()->default_catalog()->get_catalog_utils();

                        BESDEBUG(PARSER, prolog << "Found default catalog root_dir: '" << utils->get_root_dir() << "'" << endl);

                        data_url_str = BESUtil::assemblePath(utils->get_root_dir(), data_url_str, true);
                        data_url_str = FILE_PROTOCOL + data_url_str;
                        data_url = shared_ptr<http::url> ( new http::url(data_url_str));
                    }
                }

                BESDEBUG(PARSER, prolog << "Processed data_url: '" << data_url->str() << "'" << endl);

                unsigned long long offset = 0;
                unsigned long long size = 0;
                string chunk_position_in_array("");
                std::string byte_order = dc->get_byte_order();

                if (parser->check_required_attribute("offset", attributes, nb_attributes)) {
                    istringstream offset_ss(parser->get_attribute_val("offset", attributes, nb_attributes));
                    offset_ss >> offset;
                    BESDEBUG(PARSER, prolog << "Processed attribute 'offset=\"" << offset << "\"'" << endl);
                }
                else {
                    dmr_error(parser, "The hdf:byteStream element is missing the required attribute 'offset'.");
                }

                if (parser->check_required_attribute("nBytes", attributes, nb_attributes)) {
                    istringstream size_ss(parser->get_attribute_val("nBytes", attributes, nb_attributes));
                    size_ss >> size;
                    BESDEBUG(PARSER, prolog << "Processed attribute 'nBytes=\"" << size << "\"'" << endl);
                }
                else {
                    dmr_error(parser, "The hdf:byteStream element is missing the required attribute 'size'.");
                }

                if (parser->check_attribute("chunkPositionInArray", attributes, nb_attributes)) {
                    istringstream chunk_position_ss(parser->get_attribute_val("chunkPositionInArray", attributes, nb_attributes));
                    chunk_position_in_array = chunk_position_ss.str();
                    BESDEBUG(PARSER, prolog << "Found attribute 'chunkPositionInArray' value: " << chunk_position_ss.str() << endl);
                }
                else {
                    BESDEBUG(PARSER, prolog << "No attribute 'chunkPositionInArray' located" << endl);
                }

                dc->add_chunk(data_url, byte_order, size, offset, chunk_position_in_array);
            }
        }
            break;

        case inside_dmrpp_chunkDimensionSizes_element:
            // The dmrpp:chunkDimensionSizes value is processed by the end element code.
            break;

        case parser_unknown:
        case parser_error:
        case parser_fatal_error:
            break;

        case parser_end:
            // FIXME Error?
            break;
    }

    BESDEBUG(PARSER, prolog << "Start element exit state: " << states[parser->get_state()] << endl);
}
#endif

// NB: If we use this for string compares, we cannot use the 'fastest' parsing option
// of rapidxml. For that, we will need to modify this so that the length of 's1' is passed
// in (names, etc., are not null terminated with the fastest parsing option). jhrg 10/20/21
static inline bool is_eq(const char *value, const char *key)
{
    return strcmp(value, key) == 0;
}

#if 0
static inline bool is_not(const char *value, const char *key)
{
    return strcmp(value, key) != 0;
}
#endif

// 'process' functions from the sax parser.

#if 0
/** Check to see if the current tag is either an \c Attribute or an \c Alias
 start tag. This method is a glorified macro...

 @param name The start tag name
 @param attrs The tag's XML attributes
 @return True if the tag was an \c Attribute or \c Alias tag */
inline bool DmrppParserSax2::process_attribute(const char *name, const xmlChar **attrs, int nb_attributes)
{
    if (is_not(name, "Attribute")) return false;

#if 0
    // These methods set the state to parser_error if a problem is found.
    transfer_xml_attrs(attrs, nb_attributes);
#endif

    // add error
    if (!(check_required_attribute(string("name"), attrs, nb_attributes) && check_required_attribute(string("type"), attrs, nb_attributes))) {
        dmr_error(this, "The required attribute 'name' or 'type' was missing from an Attribute element.");
        return false;
    }

    if (get_attribute_val("type", attrs, nb_attributes) == "Container") {
        push_state(inside_attribute_container);

        BESDEBUG(PARSER, prolog << "Pushing attribute container " << get_attribute_val("name", attrs, nb_attributes) << endl);
        D4Attribute *child = new D4Attribute(get_attribute_val("name", attrs, nb_attributes), attr_container_c);

        D4Attributes *tos = top_attributes();
        // add return
        if (!tos) {
            delete child;
            dmr_fatal_error(this, "Expected an Attribute container on the top of the attribute stack.");
            return false;
        }

        tos->add_attribute_nocopy(child);
        push_attributes(child->attributes());
    }
    else if (get_attribute_val("type", attrs, nb_attributes) == "OtherXML") {
        push_state(inside_other_xml_attribute);

        dods_attr_name = get_attribute_val("name", attrs, nb_attributes);
        dods_attr_type = get_attribute_val("type", attrs, nb_attributes);
    }
    else {
        push_state(inside_attribute);

        dods_attr_name = get_attribute_val("name", attrs, nb_attributes);
        dods_attr_type = get_attribute_val("type", attrs, nb_attributes);
    }

    return true;
}

/** Check to see if the current tag is an \c Enumeration start tag.

 @param name The start tag name
 @param attrs The tag's XML attributes
 @return True if the tag was an \c Enumeration */
inline bool DmrppParserSax2::process_enum_def(const char *name, const xmlChar **attrs, int nb_attributes)
{
    if (is_not(name, "Enumeration")) return false;

#if 0
    transfer_xml_attrs(attrs, nb_attributes);
#endif

    if (!(check_required_attribute("name", attrs, nb_attributes) && check_required_attribute("basetype", attrs, nb_attributes))) {
        dmr_error(this, "The required attribute 'name' or 'basetype' was missing from an Enumeration element.");
        return false;
    }

    Type t = get_type(get_attribute_val("basetype", attrs, nb_attributes).c_str());
    if (!is_integer_type(t)) {
        dmr_error(this, "The Enumeration '%s' must have an integer type, instead the type '%s' was used.",
                  get_attribute_val("name", attrs, nb_attributes).c_str(), get_attribute_val("basetype", attrs, nb_attributes).c_str());
        return false;
    }

    // This getter allocates a new object if needed.
    string enum_def_path = get_attribute_val("name", attrs, nb_attributes);
#if 0
    // Use FQNs when things are referenced, not when they are defined
    if (xml_attrs["name"].value[0] != '/')
    enum_def_path = top_group()->FQN() + enum_def_path;
#endif
    enum_def()->set_name(enum_def_path);
    enum_def()->set_type(t);

    return true;
}

inline bool DmrppParserSax2::process_enum_const(const char *name, const xmlChar **attrs, int nb_attributes)
{
    if (is_not(name, "EnumConst")) return false;

#if 0
    // These methods set the state to parser_error if a problem is found.
    transfer_xml_attrs(attrs, nb_attributes);
#endif

    if (!(check_required_attribute("name", attrs, nb_attributes) && check_required_attribute("value", attrs, nb_attributes))) {
        dmr_error(this, "The required attribute 'name' or 'value' was missing from an EnumConst element.");
        return false;
    }

    istringstream iss(get_attribute_val("value", attrs, nb_attributes));
    long long value = 0;
    iss >> skipws >> value;
    if (iss.fail() || iss.bad()) {
        dmr_error(this, "Expected an integer value for an Enumeration constant, got '%s' instead.",
                  get_attribute_val("value", attrs, nb_attributes).c_str());
    }
    else if (!enum_def()->is_valid_enum_value(value)) {
        dmr_error(this, "In an Enumeration constant, the value '%s' cannot fit in a variable of type '%s'.",
                  get_attribute_val("value", attrs, nb_attributes).c_str(), D4type_name(d_enum_def->type()).c_str());
    }
    else {
        // unfortunate choice of names... args are 'label' and 'value'
        enum_def()->add_value(get_attribute_val("name", attrs, nb_attributes), value);
    }

    return true;
}
#endif

/**
 * @brief process a Dataset element
 * @param dmr
 */
void DMZ::process_dataset(DMR *dmr, xml_node<> *xml_root)
{
    // Process the attributes
    int required_attrs_found = 0;   // there are 1
    string href_attr;
    bool href_trusted = false;
    for (xml_attribute<> *attr = xml_root->first_attribute(); attr; attr = attr->next_attribute()) {
        if (is_eq(attr->name(), "name")) {
            ++required_attrs_found;
            dmr->set_name(attr->value());
        }
        else if (is_eq(attr->name(), "dapVersion")) {
            dmr->set_dap_version(attr->value());
        }
        else if (is_eq(attr->name(), "dmrVersion")) {
            dmr->set_dmr_version(attr->value());
        }
        else if (is_eq(attr->name(), "base")) {
            dmr->set_request_xml_base(attr->value());
            BESDEBUG(PARSER, prolog << "Dataset xml:base is set to '" << dmr->request_xml_base() << "'" << endl);
        }
        // TODO Namespaces? jhrg 10/2/0/21
        else if (is_eq(attr->name(), "xmlns")) {
            dmr->set_namespace(attr->value());
        }
        // TODO what to do with namespaced attributes? jhrg 10/20/21
        else if (is_eq(attr->name(), "dmrpp:href")) {
            href_attr = attr->value();
        }
        else if (is_eq(attr->name(), "trust")) {
            href_trusted = is_eq(attr->value(), "true");
        }
        // We allow other, non recognized attributes, so there is no 'else' jhrg 10/20/21
    }

    if (required_attrs_found != 1)
        throw BESInternalError("DMR++ XML dataset element missing one or more required attributes.", __FILE__, __LINE__);

    d_dataset_elem_href = new http::url(href_attr, href_trusted);
    BESDEBUG(PARSER, prolog << "Dataset dmrpp:href is set to '" << d_dataset_elem_href->str() << "'" << endl);
}

void DMZ::process_dimension(D4Group *grp, xml_node<> *dimension_node)
{
    string name_value;
    string size_value;
    for (xml_attribute<> *attr = dimension_node->first_attribute(); attr; attr = attr->next_attribute()) {
        if (is_eq(attr->name(), "name")) {
            name_value = attr->value();
        }
        else if (is_eq(attr->name(), "size")) {
            size_value = attr->value();
        }
    }

    if (name_value.empty() || size_value.empty())
        throw BESInternalError("The required attribute 'name' or 'size' was missing from a Dimension element.", __FILE__, __LINE__);

    // This getter (dim_def) allocates a new object if needed.
    try {
        auto *dimension = new D4Dimension();
        dimension->set_name(name_value);
        dimension->set_size(size_value);
        grp->dims()->add_dim_nocopy(dimension);
    }
    catch (Error &e) {
        throw BESInternalError(e.get_error_message(), __FILE__, __LINE__);
    }
}

/**
 * @brief Process a Dim node. Add it to the given BaseType
 * @param dmr
 * @param grp The group we are currently inside (could be the root group)
 * @param array The variable we are inside
 * @param dim_node The Dim node itself
 */
void DMZ::process_dim(DMR *dmr, D4Group *grp, Array *array, xml_node<> *dim_node)
{
    assert(array->is_vector_type());

    string name_value;
    string size_value;
    for (xml_attribute<> *attr = dim_node->first_attribute(); attr; attr = attr->next_attribute()) {
        if (is_eq(attr->name(), "name")) {
            name_value = attr->value();
        }
        else if (is_eq(attr->name(), "size")) {
            size_value = attr->value();
        }
    }

    if (name_value.empty() && size_value.empty())
        throw BESInternalError("Either 'size' or 'name' must be used in a Dim element.", __FILE__, __LINE__);
    if (!name_value.empty() && !size_value.empty())
        throw BESInternalError("Only one of 'size' and 'name' are allowed in a Dim element, but both were used.", __FILE__, __LINE__);

    if (!size_value.empty()) {
        BESDEBUG(PARSER, prolog << "Processing nameless Dim of size: " << stoi(size_value) << endl);
#if 0
        D4Dimension *d = new D4Dimension();
        d->set_size(size_value);
#endif
        array->append_dim(stoi(size_value));
    }
    else if (!name_value.empty()) {
        BESDEBUG(PARSER, prolog << "Processing Dim with named Dimension reference: " << name_value << endl);

        D4Dimension *dim = nullptr;
        if (name_value[0] == '/')		// lookup the Dimension in the root group
            dim = dmr->root()->find_dim(name_value);
        else
            // get enclosing Group and lookup Dimension there
            dim = grp->find_dim(name_value);

        if (!dim)
            throw BESInternalError("The dimension '" + name_value + "' was not found while parsing the variable '" + array->name() + "'.",__FILE__,__LINE__);

        array->append_dim(dim);
    }
}

static inline bool has_dim_nodes(xml_node<> *var_node)
{
    for (auto *child = var_node->first_node(); child; child = child->next_sibling()) {
        if (is_eq(child->name(), "Dim"))    // just one is enough
            return true;
    }

    return false;
}

/**
 * @brief Check to see if the current element is the start of a variable declaration.
 * @param dmr
 * @param grp
 * @param btp
 * @param var_node
 */
void DMZ::process_variable(DMR *dmr, D4Group *grp, xml_node<> *var_node)
{
    // Variables are declared using nodes with type names (e.g., <Float32...>)
    // Variables are arrays if they have one or more <Dim...> child nodes.
    Type t = get_type(var_node->name());
    bool is_array_type = has_dim_nodes(var_node);
    if (is_simple_type(t)) {
        if (is_array_type) {
            add_array_variable(dmr, grp, t, var_node);
        }
        else {
            add_scalar_variable(dmr, grp, t, var_node);
        }
    }
    else {
        switch (t) {
            case dods_structure_c:
            case dods_sequence_c:

            default:
                throw BESInternalError(string("The variable type '") + var_node->name() + "' is unknown.", __FILE__, __LINE__);
        }
    }
}

/**
 * @brief helper code to build a BaseType that may wind up a scalar or an array
 * @param dmr
 * @param grp
 * @param t
 * @param var_node
 */
BaseType *DMZ::build_scalar_variable(DMR *dmr, D4Group *grp, Type t, xml_node<> *var_node)
{
    assert(dmr->factory());

    string name_value;
    string enum_value;
    for (xml_attribute<> *attr = var_node->first_attribute(); attr; attr = attr->next_attribute()) {
        if (is_eq(attr->name(), "name")) {
            name_value = attr->value();
        }
        if (is_eq(attr->name(), "enum")) {
            enum_value = attr->value();
        }
    }

    if (name_value.empty())
        throw BESInternalError("The variable 'name' attribute was missing.", __FILE__, __LINE__);

    BaseType *btp = dmr->factory()->NewVariable(t, name_value);
    if (!btp)
        throw BESInternalError("Could not instantiate the variable ' "+ name_value +"'.", __FILE__, __LINE__);

    btp->set_is_dap4(true);

    if (t == dods_enum_c) {
        if (enum_value.empty())
            throw BESInternalError("The variable ' " + name_value + "' lacks an 'enum' attribute.", __FILE__, __LINE__);

        D4EnumDef *enum_def = nullptr;
        if (enum_value[0] == '/')
            enum_def = dmr->root()->find_enum_def(enum_value);
        else
            enum_def = grp->find_enum_def(enum_value);

        if (!enum_def)
            throw BESInternalError("Could not find the Enumeration definition '" + enum_value + "'.", __FILE__, __LINE__);

        dynamic_cast<D4Enum&>(*btp).set_enumeration(enum_def);
    }

    return btp;
}
/**
 * Given that a tag which opens a variable declaration has just been read,
 * create the variable.
 * @param dmr
 * @param grp
 * @param t
 * @param var_node
 * @return The new variable
 */
void DMZ::add_scalar_variable(DMR *dmr, D4Group *grp, Type t, xml_node<> *var_node)
{
    BaseType *btp = build_scalar_variable(dmr, grp,t, var_node);
    grp->add_var_nocopy(btp);
}

void DMZ::add_array_variable(DMR *dmr, D4Group *grp, Type t, xml_node<> *var_node)
{
    BaseType *btp = build_scalar_variable(dmr, grp,t, var_node);

    // Transform the scalar to an array
    Array *array = static_cast<Array*>(dmr->factory()->NewVariable(dods_array_c, btp->name()));
    array->set_is_dap4(true);
    array->add_var_nocopy(btp);

    // The SAX parser set up the parse of attributes here. For the thin DMR, we won't
    // parse those from the DMR now. jhrg 10/21/21

    // Now grab the dimension elements
    for (auto *child = var_node->first_node(); child; child = child->next_sibling()) {
        if (is_eq(child->name(), "Dim")) {
            process_dim(dmr, grp, array, child);
        }
    }

    grp->add_var_nocopy(array);
}

static inline bool member_of(const set<string> &elements_set, const string &element_name)
{
    return elements_set.find(element_name) != elements_set.end();
}

void DMZ::process_group(DMR *dmr, D4Group *parent, xml_node<> *var_node)
{
    string name_value;
    for (xml_attribute<> *attr = var_node->first_attribute(); attr; attr = attr->next_attribute()) {
        if (is_eq(attr->name(), "name")) {
            name_value = attr->value();
        }
    }

    if (name_value.empty())
        throw BESInternalError("The required attribute 'name' was missing from a Group element.", __FILE__, __LINE__);


    BaseType *btp = dmr->factory()->NewVariable(dods_group_c, name_value);
    if (!btp)
        throw BESInternalError("Could not instantiate the Group '" + name_value + "'.", __FILE__, __LINE__);

    auto new_group = dynamic_cast<D4Group*>(btp);

    // Need to set this to get the D4Attribute behavior in the type classes
    // shared between DAP2 and DAP4. jhrg 4/18/13
    new_group->set_is_dap4(true);

    // link it up and change the current group
    new_group->set_parent(parent);
    parent->add_group_nocopy(new_group);

    // Now parse all the child nodes of the Group.
    // NB: this is the same block of code as in build_thin_dmr(); refactor. jhrg 10/21/21
    for (auto *child = var_node->first_node(); child; child = child->next_sibling()) {
        if (is_eq(child->name(), "Dimension")) {
            process_dimension(new_group, child);
        }
        else if (is_eq(child->name(), "Group")) {
            process_group(dmr, new_group, child);
        }
        else if (member_of(variable_elements, child->name())) {
            process_variable(dmr, new_group, child);
        }
    }
}

#if 0
static void print_xml_node(xml_node<> *node)
{
    cerr << "Node " << node->name() <<" has value " << node->value() << endl;
    for (xml_attribute<> *attr = node->first_attribute(); attr; attr = attr->next_attribute()) {
        cerr << "Node has attribute " << attr->name() << " ";
        cerr << "with value " << attr->value() << endl;
    }
}
#endif

/**
 * @brief populate the DMR instance as a 'thin DMR'
 * @note Assume the DMZ holds valid DMR++ metadata.
 * @param dmr Pointer to a DMR instnace that should be populated
 */
void DMZ::build_thin_dmr(DMR *dmr)
{
    auto xml_root_node = d_xml_doc.first_node();

    process_dataset(dmr, xml_root_node);

    auto root_group = dmr->root();
    for (auto *child = xml_root_node->first_node(); child; child = child->next_sibling()) {
        if (is_eq(child->name(), "Dimension")) {
            process_dimension(root_group, child);
        }
        else if (is_eq(child->name(), "Group")) {
            process_group(dmr, root_group, child);
        }
        else if (member_of(variable_elements, child->name())) {
            process_variable(dmr, root_group, child);
        }
    }
}

std::string get_variable_xml(std::string /*path*/)
{
    return "";
}

} // namespace dmrpp
