
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2012 OPeNDAP, Inc.
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

#ifndef dmrpp_parser_sax2_h
#define dmrpp_parser_sax2_h

#define ATTR 1

#include <cstring>

#include <memory>
#include <string>
#include <iostream>
#include <map>
#include <unordered_map>
#include <stack>

#include <libxml/parserInternals.h>

#include <libdap/Type.h>   // from libdap
#include "BESRegex.h"
#include "url_impl.h"
#include "EffectiveUrlCache.h"

#define CRLF "\r\n"
#define D4_PARSE_BUFF_SIZE 1048576

namespace libdap {
class DMR;
class BaseType;
class D4BaseTypeFactory;
class D4Group;
class D4Attributes;
class D4EnumDef;
class D4Dimension;
}

namespace dmrpp {
/**
 * Hacked DMR parser copied from libdap. This version of the parser processes
 * dmrpp:chunk elements and stores their information (xml attributes) in
 * Dmrpp BaseTypes.
 */
class DmrppParserSax2
{
private:
    /** States used by DDXParserDAP4State. These are the states of the SAX parser
    state-machine. */
    enum ParseState {
        parser_start,

        inside_dataset,

        // inside_group is the state just after parsing the start of a Group element.
        inside_group,

        inside_attribute_container,
        inside_attribute,
        inside_attribute_value,
        inside_other_xml_attribute,

        inside_enum_def,
        inside_enum_const,

        inside_dim_def,

        // This covers Byte, ..., Url, Opaque
        inside_simple_type,

        // inside_array,
        inside_dim,
        inside_map,

        inside_constructor,

        not_dap4_element,
        inside_dmrpp_object,
        inside_dmrpp_chunkDimensionSizes_element,
        inside_dmrpp_compact_element,

        parser_unknown,
        parser_error,
        parser_fatal_error,

        parser_end
    };

    char d_parse_buffer[D4_PARSE_BUFF_SIZE+1]; // Buff size plus one byte for NULL termination.

    xmlSAXHandler dmrpp_sax_parser;

    // The results of the parse operation are stored in these fields.
    // This is passed into the parser using the intern() methods.
    libdap::DMR *d_dmr;   // dump DMR here
    libdap::DMR *dmr() const { return d_dmr; }

    // These stacks hold the state of the parse as it progresses.
    std::stack<ParseState> s; // Current parse state
    void push_state(DmrppParserSax2::ParseState state) { s.push(state); }
    DmrppParserSax2::ParseState get_state() const { return s.top(); }
    void pop_state() { s.pop(); }
    bool empty_state() const { return s.empty(); }

    std::stack<libdap::BaseType*> btp_stack; // current variable(s)
    void push_basetype(libdap::BaseType *btp) { btp_stack.push(btp); }
    libdap::BaseType *top_basetype() const { return btp_stack.top(); }
    void pop_basetype() { btp_stack.pop(); }
    bool empty_basetype() const { return btp_stack.empty(); }

    std::stack<libdap::D4Group*> grp_stack; // current groups(s)
    void push_group(libdap::D4Group *grp) { grp_stack.push(grp); }
    libdap::D4Group *top_group() const { return grp_stack.top(); }
    void pop_group() { grp_stack.pop(); }
    bool empty_group() const { return grp_stack.empty(); }

    std::stack<libdap::D4Attributes*> d_attrs_stack; // DAP4 Attributes
    void push_attributes(libdap::D4Attributes *attr) { d_attrs_stack.push(attr); }
    libdap::D4Attributes *top_attributes() const { return d_attrs_stack.top(); }
    void pop_attributes() { d_attrs_stack.pop(); }
    bool empty_attributes() const { return d_attrs_stack.empty(); }

    libdap::D4EnumDef *d_enum_def;
    libdap::D4EnumDef *enum_def();
    void clear_enum_def() { d_enum_def = 0; }

    libdap::D4Dimension *d_dim_def;
    libdap::D4Dimension *dim_def();
    void clear_dim_def() { d_dim_def = 0; }

    // Accumulate stuff inside an 'OtherXML' DAP attribute here
    std::string other_xml;

    // When we're parsing unknown XML, how deeply is it nested? This is used
    // for the OtherXML DAP attributes.
    unsigned int other_xml_depth;
    unsigned int unknown_depth;

    // These are used for processing errors.
    std::string error_msg;  // Error message(s), if any.
    xmlParserCtxtPtr context; // used for error message line numbers

    // These hold temporary values read during the parse.
    std::string dods_attr_name; // DAP4 attributes, not XML attributes
    std::string dods_attr_type; // ... not XML ...
    std::string char_data;  // char data in value elements; null after use
    std::string root_ns;     // What is the namespace of the root node (Group)

    bool d_strict;

    std::shared_ptr<http::url> dmrpp_dataset_href;

    class XMLAttribute {
        public:
        std::string prefix;
        std::string nsURI;
        std::string value;

        void clone(const XMLAttribute &src) {
            prefix = src.prefix;
            nsURI = src.nsURI;
            value = src.value;
        }

        XMLAttribute() : prefix(""), nsURI(""), value("") {}
        XMLAttribute(const std::string &p, const std::string &ns, const std::string &v)
            : prefix(p), nsURI(ns), value(v) {}
        // 'attributes' as passed from libxml2 is a five element array but this
        // ctor gets the back four elements.
        XMLAttribute(const xmlChar **attributes/*[4]*/) {
            prefix = attributes[0] != 0 ? (const char *)attributes[0]: "";
            nsURI = attributes[1] != 0 ? (const char *)attributes[1]: "";
            value = std::string((const char *)attributes[2], (const char *)attributes[3]);
        }
        XMLAttribute(const XMLAttribute &rhs) {
            clone(rhs);
        }
        XMLAttribute &operator=(const XMLAttribute &rhs) {
            if (this == &rhs)
                return *this;
            clone(rhs);
            return *this;
        }
    };

    typedef std::unordered_map<std::string, XMLAttribute> XMLAttrMap;
    XMLAttrMap xml_attrs; // dump XML attributes here

    XMLAttrMap::iterator xml_attr_begin() { return xml_attrs.begin(); }

    XMLAttrMap::iterator xml_attr_end() {  return xml_attrs.end(); }

    std::map<std::string, std::string> namespace_table;

    void cleanup_parse();

    /** @name Parser Actions

    These methods are the 'actions' carried out by the start_element and
    end_element callbacks. Most of what takes place in those has been
    factored out to this set of functions. */
    //@{
#if 0
    void transfer_xml_attrs(const xmlChar **attrs, int nb_attributes);
#endif
    std::string get_attribute_val(const std::string &name, const xmlChar **attributes, int num_attributes);
    void transfer_xml_ns(const xmlChar **namespaces, int nb_namespaces);
    bool check_required_attribute(const std::string &attr);
    bool check_required_attribute(const std::string &attr, const xmlChar **attributes, int num_attributes);
    bool check_attribute(const std::string & attr);
    bool check_attribute(const std::string &name, const xmlChar **attributes, int num_attributes);
    void process_variable_helper(libdap::Type t, ParseState s, const xmlChar **attrs, int nb_attributes);

    void process_enum_const_helper(const xmlChar **attrs, int nb_attributes);
    void process_enum_def_helper(const xmlChar **attrs, int nb_attributes);

    bool process_dmrpp_compact_start(const char *name);
    void process_dmrpp_compact_end(const char *localname);
    bool process_dimension(const char *name, const xmlChar **attrs, int nb_attrs);
    bool process_dimension_def(const char *name, const xmlChar **attrs, int nb_attrs);
    bool process_map(const char *name, const xmlChar **attrs, int nb_attributes);
    bool process_attribute(const char *name, const xmlChar **attrs, int nb_attributes);
    bool process_variable(const char *name, const xmlChar **attrs, int nb_attributes);
    bool process_group(const char *name, const xmlChar **attrs, int nb_attributes);
    bool process_enum_def(const char *name, const xmlChar **attrs, int nb_attributes);
    bool process_enum_const(const char *name, const xmlChar **attrs, int nb_attributes);
    bool process_dmrpp_object(const char *name, const xmlChar **attrs, int nb_attributes);

    void finish_variable(const char *tag, libdap::Type t, const char *expected);
    //@}

    friend class DmrppParserSax2Test;

public:
    DmrppParserSax2() :
            d_dmr(0), d_enum_def(0), d_dim_def(0),
            other_xml(""), other_xml_depth(0), unknown_depth(0),
            error_msg(""), context(0),
            dods_attr_name(""), dods_attr_type(""),
            char_data(""), root_ns(""), d_strict(true),
            dmrpp_dataset_href(nullptr)
    {
        //xmlSAXHandler ddx_sax_parser;
        memset(&dmrpp_sax_parser, 0, sizeof(xmlSAXHandler));

        dmrpp_sax_parser.getEntity = &DmrppParserSax2::dmr_get_entity;
        dmrpp_sax_parser.startDocument = &DmrppParserSax2::dmr_start_document;
        dmrpp_sax_parser.endDocument = &DmrppParserSax2::dmr_end_document;
        dmrpp_sax_parser.characters = &DmrppParserSax2::dmr_get_characters;
        dmrpp_sax_parser.ignorableWhitespace = &DmrppParserSax2::dmr_ignoreable_whitespace;
        dmrpp_sax_parser.cdataBlock = &DmrppParserSax2::dmr_get_cdata;
        dmrpp_sax_parser.warning = &DmrppParserSax2::dmr_error;
        dmrpp_sax_parser.error = &DmrppParserSax2::dmr_error;
        dmrpp_sax_parser.fatalError = &DmrppParserSax2::dmr_fatal_error;
        dmrpp_sax_parser.initialized = XML_SAX2_MAGIC;
        dmrpp_sax_parser.startElementNs = &DmrppParserSax2::dmr_start_element;
        dmrpp_sax_parser.endElementNs = &DmrppParserSax2::dmr_end_element;
    }

    ~DmrppParserSax2(){}

    void intern(std::istream &f, libdap::DMR *dest_dmr);
    void intern(const std::string &document, libdap::DMR *dest_dmr);
    void intern(const char *buffer, int size, libdap::DMR *dest_dmr);

    /**
     * @defgroup strict The 'strict' mode
     * @{
     * The strict mode of the parser is the default. In this mode any error
     * will result in an exception and parsing will stop. When strict mode
     * is set to false (forgiving mode?), some errors will be silently ignored.
     * The list of allowed errors is:
     *     The Array named by a Map element is not required to be in the DMR.
     *     There are no other allowed errors at this time (4/13/16)
     */

    /** @brief Set the 'strict' mode to true or false. */
    void set_strict(bool s) { d_strict = s; }
    /** @brief Get the setting of the 'strict' mode.
     * @return True or False.
     */
    bool get_strict() const { return d_strict; }
    /** @} */

    static void dmr_start_document(void *parser);
    static void dmr_end_document(void *parser);

    static void dmr_start_element(void *parser,
            const xmlChar *localname, const xmlChar *prefix, const xmlChar *URI,
            int nb_namespaces, const xmlChar **namespaces, int nb_attributes,
            int nb_defaulted, const xmlChar **attributes);
    static void dmr_end_element(void *parser, const xmlChar *localname,
            const xmlChar *prefix, const xmlChar *URI);

    static void dmr_get_characters(void *parser, const xmlChar *ch, int len);
    static void dmr_ignoreable_whitespace(void *parser,
            const xmlChar * ch, int len);
    static void dmr_get_cdata(void *parser, const xmlChar *value, int len);

    static xmlEntityPtr dmr_get_entity(void *parser, const xmlChar *name);
    static void dmr_fatal_error(void *parser, const char *msg, ...);
    static void dmr_error(void *parser, const char *msg, ...);

    };

} // namespace dmrpp

#endif // dmrpp_parser_sax2_h
