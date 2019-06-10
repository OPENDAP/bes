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

#include "config.h"

#include <iostream>
#include <sstream>

#include <cstring>
#include <cstdarg>
#include <cassert>

#include <libxml/parserInternals.h>

#include <DMR.h>

#include <BaseType.h>
#include <Array.h>
#include <D4Group.h>
#include <D4Attributes.h>
#include <D4Maps.h>
#include <D4Enum.h>
#include <D4BaseTypeFactory.h>

#include <DapXmlNamespaces.h>
#include <util.h>

#include <BESInternalError.h>
#include <BESDebug.h>
#include <BESCatalog.h>
#include <BESCatalogUtils.h>
#include <BESCatalogList.h>
#include <BESUtil.h>

#include "DmrppParserSax2.h"
#include "DmrppCommon.h"

#define FIVE_12K  524288;
#define ONE_MB   1048576;
#define MAX_INPUT_LINE_LENGTH ONE_MB;


static const string module = "dmrpp:2";
static const string dmrpp_namespace = "http://xml.opendap.org/dap/dmrpp/1.0.0#";

using namespace libdap;
using namespace std;

namespace dmrpp {

static const char *states[] = { "parser_start",

"inside_dataset",

// inside_group is the state just after parsing the start of a Group
// element.
    "inside_group",

    "inside_attribute_container", "inside_attribute", "inside_attribute_value", "inside_other_xml_attribute",

    "inside_enum_def", "inside_enum_const",

    "inside_dim_def",

    // This covers Byte, ..., Url, Opaque
    "inside_simple_type",

    // "inside_array",
    "inside_dim", "inside_map",

    "inside_constructor",

    "not_dap4_element", "inside_dmrpp_object", "inside_dmrpp_chunkDimensionSizes_element",

    "parser_unknown", "parser_error", "parser_fatal_error",

    "parser_end" };

static bool is_not(const char *name, const char *tag)
{
    return strcmp(name, tag) != 0;
}

/** @brief Return the current Enumeration definition
 * Allocate the Enumeration definition if needed and return it. Once parsing the current
 * enumeration definition is complete, the pointer allocated/returned by this method will
 * be copied into the current Group and this internal storage will be 'reset' using
 * clear_enum_def().
 *
 * @return
 */
D4EnumDef *
DmrppParserSax2::enum_def()
{
    if (!d_enum_def) d_enum_def = new D4EnumDef;

    return d_enum_def;
}

/** @brief Return the current Dimension definition
 * Allocate the Dimension definition if needed and return it.
 * @see enum_def() for an explanation of how this is used by the parser.
 *
 * @return
 */
D4Dimension *
DmrppParserSax2::dim_def()
{
    if (!d_dim_def) d_dim_def = new D4Dimension;

    return d_dim_def;
}

/* Search through the attribute array for a given attribute name.
 * If the name is found, return the string value for that attribute
 * @param name: Search for this name
 * @param attributes: Array that holds the attribute values to search
 * @param num_attributes: Number of attributes
 * @return string value of attribute; the empty string if the name was not found
 */
string DmrppParserSax2::get_attribute_val(const string &name, const xmlChar **attributes, int num_attributes)
{
	unsigned int index = 0;
	for (int i = 0; i < num_attributes; ++i, index += 5) {
		if (strncmp(name.c_str(), (const char *)attributes[index], name.length()) == 0) {
			return string((const char *)attributes[index+3],  (const char *)attributes[index+4]);
		}
	}
	return "";
}

#if 0
/** Dump XML attributes to local store so they can be easily manipulated.
 * XML attribute names are always folded to lower case.
 * @param attributes The XML attribute array
 * @param nb_attributes The number of attributes
 */
void DmrppParserSax2::transfer_xml_attrs(const xmlChar **attributes, int nb_attributes)
{
    if (!xml_attrs.empty()) xml_attrs.clear(); // erase old attributes

    // Make a value using the attribute name and the prefix, namespace URI
    // and the value. The prefix might be null.
    unsigned int index = 0;
    for (int i = 0; i < nb_attributes; ++i, index += 5) {
        xml_attrs.insert(
            map<string, XMLAttribute>::value_type(string((const char *) attributes[index]),
                XMLAttribute(attributes + index + 1)));

        BESDEBUG(module,
            "XML Attribute '" << (const char *)attributes[index] << "': " << xml_attrs[(const char *)attributes[index]].value << endl);
    }
}
#endif

/** Transfer the XML namespaces to the local store so they can be manipulated
 * more easily.
 *
 * @param namespaces Array of xmlChar*
 * @param nb_namespaces The number of namespaces in the array.
 */
void DmrppParserSax2::transfer_xml_ns(const xmlChar **namespaces, int nb_namespaces)
{
    // make a value with the prefix and namespace URI. The prefix might be null.
    for (int i = 0; i < nb_namespaces; ++i) {
        namespace_table.insert(
            map<string, string>::value_type(namespaces[i * 2] != 0 ? (const char *) namespaces[i * 2] : "",
                (const char *) namespaces[i * 2 + 1]));
    }
}

#if 0
/** Is a required XML attribute present? Attribute names are always lower case.
 * @note To use this method, first call transfer_xml_attrs.
 * @param attr The XML attribute
 * @return True if the XML attribute was present in the last tag, otherwise
 * it sets the global error state and returns false.
 */
bool DmrppParserSax2::check_required_attribute(const string & attr)
{
    if (xml_attrs.find(attr) == xml_attrs.end()) {
        dmr_error(this, "Required attribute '%s' not found.", attr.c_str());
        return false;
    }
    else
        return true;
}
#endif

/*
 * An improved version of the previous check_required_attribute.
 * Searches for an attribute name within the attribute array.
 * @param name: The attribute name to search for
 * @param attributes: The attribute array
 * @param num_attributes: The number of attributes
 * @return success: true
 * 		   failure: false
 */
bool DmrppParserSax2::check_required_attribute(const string &name, const xmlChar **attributes, int num_attributes)
{
	unsigned int index = 0;
	for (int i = 0; i < num_attributes; ++i, index += 5) {
		if (strncmp(name.c_str(), (const char *)attributes[index], name.length()) == 0) {
			return true;
		}
	}

	dmr_error(this, "Required attribute '%s' not found.", name.c_str());
	return false;
}

#if 0
/** Is a XML attribute present? Attribute names are always lower case.
 * @note To use this method, first call transfer_xml_attrs.
 * @param attr The XML attribute
 * @return True if the XML attribute was present in the last/current tag,
 * false otherwise.
 */
bool DmrppParserSax2::check_attribute(const string & attr)
{
    return (xml_attrs.find(attr) != xml_attrs.end());
}
#endif

/**
 * An improved version of the check_attribute function.
 * Search for an attribute name to see if it is already present in the
 * provided attribute array.
 * @param name: The attribute name to search for
 * @param attributes: The attribute array
 * @param num_attributes: The number of attributes
 * @return success: true
 * 		   failure: false
 */
bool DmrppParserSax2::check_attribute(const string &name, const xmlChar **attributes, int num_attributes)
{
	unsigned int index = 0;
	for (int i = 0; i < num_attributes; ++i, index += 5) {
		if (strncmp(name.c_str(), (const char *)attributes[index], name.length()) == 0) {
			return true;
		}
	}
	return false;
}

bool DmrppParserSax2::process_dimension_def(const char *name, const xmlChar **attrs, int nb_attributes)
{
    if (is_not(name, "Dimension")) return false;

#if 0
    transfer_xml_attrs(attrs, nb_attributes);
#endif

    if (!(check_required_attribute("name", attrs, nb_attributes) && check_required_attribute("size", attrs, nb_attributes))) {
        dmr_error(this, "The required attribute 'name' or 'size' was missing from a Dimension element.");
        return false;
    }

    // This getter (dim_def) allocates a new object if needed.
    dim_def()->set_name(get_attribute_val("name", attrs, nb_attributes));
    try {
        dim_def()->set_size(get_attribute_val("size", attrs, nb_attributes));
    }
    catch (Error &e) {
        dmr_error(this, e.get_error_message().c_str());
        return false;
    }

    return true;
}

/**
 * @brief Process a Dim element.
 * If a Dim element is found, the current variable is an Array. If the BaseType
 * on the TOS is not already an Array, make it one. Append the dimension
 * information to the Array variable on the TOS.
 *
 * @note Dim elements can have two attributes: name or size. The latter defines
 * an 'anonymous' dimension (one without a name that does not reference a
 * shared dimension object. If the \c name attribute is used, then the shared
 * dimension used is the one defined by the enclosing group or found using the
 * fully qualified name. The \name and \c size attributes are mutually exclusive.
 *
 * @param name XML element name; must be Dim
 * @param attrs XML Attributes
 * @param nb_attributes The number of XML Attributes
 * @return True if the element is a Dim, false otherwise.
 */
bool DmrppParserSax2::process_dimension(const char *name, const xmlChar **attrs, int nb_attributes)
{
    if (is_not(name, "Dim")) return false;

#if 0
    transfer_xml_attrs(attrs, nb_attributes);
#endif

    if (check_attribute("size", attrs, nb_attributes) && check_attribute("name", attrs, nb_attributes)) {
        dmr_error(this, "Only one of 'size' and 'name' are allowed in a Dim element, but both were used.");
        return false;
    }
    if (!(check_attribute("size", attrs, nb_attributes) || check_attribute("name", attrs, nb_attributes))) {
        dmr_error(this, "Either 'size' or 'name' must be used in a Dim element.");
        return false;
    }

    if (!top_basetype()->is_vector_type()) {
        // Make the top BaseType* an array
        BaseType *b = top_basetype();
        pop_basetype();

        Array *a = static_cast<Array*>(dmr()->factory()->NewVariable(dods_array_c, b->name()));
        a->set_is_dap4(true);
        a->add_var_nocopy(b);
        a->set_attributes_nocopy(b->attributes());
        // trick: instead of popping b's attributes, copying them and then pushing
        // a's copy, just move the pointer (but make sure there's only one object that
        // references that pointer).
        b->set_attributes_nocopy(0);

        push_basetype(a);
    }

    assert(top_basetype()->is_vector_type());

    Array *a = static_cast<Array*>(top_basetype());
    if (check_attribute("size", attrs, nb_attributes)) {
        a->append_dim(stoi(get_attribute_val("size", attrs, nb_attributes))); // low budget code for now. jhrg 8/20/13, modified to use new function. kln 9/7/19
        return true;
    }
    else if (check_attribute("name", attrs, nb_attributes)) {
        string name = get_attribute_val("name", attrs, nb_attributes);

        D4Dimension *dim = 0;
        if (name[0] == '/')		// lookup the Dimension in the root group
            dim = dmr()->root()->find_dim(name);
        else
            // get enclosing Group and lookup Dimension there
            dim = top_group()->find_dim(name);

        if (!dim)
            throw Error("The dimension '" + name + "' was not found while parsing the variable '" + a->name() + "'.");
        a->append_dim(dim);
        return true;
    }

    return false;
}

bool DmrppParserSax2::process_map(const char *name, const xmlChar **attrs, int nb_attributes)
{
    if (is_not(name, "Map")) return false;

#if 0
    transfer_xml_attrs(attrs, nb_attributes);
#endif

    if (!check_attribute("name", attrs, nb_attributes)) {
        dmr_error(this, "The 'name' attribute must be used in a Map element.");
        return false;
    }

    if (!top_basetype()->is_vector_type()) {
        // Make the top BaseType* an array
        BaseType *b = top_basetype();
        pop_basetype();

        Array *a = static_cast<Array*>(dmr()->factory()->NewVariable(dods_array_c, b->name()));
        a->set_is_dap4(true);
        a->add_var_nocopy(b);
        a->set_attributes_nocopy(b->attributes());
        // trick: instead of popping b's attributes, copying them and then pushing
        // a's copy, just move the pointer (but make sure there's only one object that
        // references that pointer).
        b->set_attributes_nocopy(0);

        push_basetype(a);
    }

    assert(top_basetype()->is_vector_type());

    Array *a = static_cast<Array*>(top_basetype());

    string map_name = get_attribute_val("name", attrs, nb_attributes);
    if (get_attribute_val("name", attrs, nb_attributes).at(0) != '/') map_name = top_group()->FQN() + map_name;

    Array *map_source = 0;	// The array variable that holds the data for the Map

    if (map_name[0] == '/')		// lookup the Map in the root group
        map_source = dmr()->root()->find_map_source(map_name);
    else
        // get enclosing Group and lookup Map there
        map_source = top_group()->find_map_source(map_name);

    // Change: If the parser is in 'strict' mode (the default) and the Array named by
    // the Map cannot be fond, it is an error. If 'strict' mode is false (permissive
    // mode), then this is not an error. However, the Array referenced by the Map will
    // be null. This is a change in the parser's behavior to accommodate requests for
    // Arrays that include Maps that do not also include the Map(s) in the request.
    // See https://opendap.atlassian.net/browse/HYRAX-98. jhrg 4/13/16
    if (!map_source && d_strict)
        throw Error("The Map '" + map_name + "' was not found while parsing the variable '" + a->name() + "'.");

    a->maps()->add_map(new D4Map(map_name, map_source));

    return true;
}

bool DmrppParserSax2::process_group(const char *name, const xmlChar **attrs, int nb_attributes)
{
    if (is_not(name, "Group")) return false;

#if 0
    transfer_xml_attrs(attrs, nb_attributes);
#endif

    if (!check_required_attribute("name", attrs, nb_attributes)) {
        dmr_error(this, "The required attribute 'name' was missing from a Group element.");
        return false;
    }

    BaseType *btp = dmr()->factory()->NewVariable(dods_group_c, get_attribute_val("name", attrs, nb_attributes));
    if (!btp) {
        dmr_fatal_error(this, "Could not instantiate the Group '%s'.", get_attribute_val("name", attrs, nb_attributes).c_str());
        return false;
    }

    D4Group *grp = static_cast<D4Group*>(btp);

    // Need to set this to get the D4Attribute behavior in the type classes
    // shared between DAP2 and DAP4. jhrg 4/18/13
    grp->set_is_dap4(true);

    // link it up and change the current group
    D4Group *parent = top_group();
    if (!parent) {
        dmr_fatal_error(this, "No Group on the Group stack.");
        return false;
    }

    grp->set_parent(parent);
    parent->add_group_nocopy(grp);

    push_group(grp);
    push_attributes(grp->attributes());
    return true;
}

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

        BESDEBUG(module, "Pushing attribute container " << get_attribute_val("name", attrs, nb_attributes) << endl);
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

/** Check to see if the current element is the start of a variable declaration.
 If so, process it. A glorified macro...
 @param name The start element name
 @param attrs The element's XML attributes
 @return True if the element was a variable */
inline bool DmrppParserSax2::process_variable(const char *name, const xmlChar **attrs, int nb_attributes)
{
    Type t = get_type(name);
    if (is_simple_type(t)) {
        process_variable_helper(t, inside_simple_type, attrs, nb_attributes);
        return true;
    }
    else {
        switch (t) {
        case dods_structure_c:
            process_variable_helper(t, inside_constructor, attrs, nb_attributes);
            return true;

        case dods_sequence_c:
            process_variable_helper(t, inside_constructor, attrs, nb_attributes);
            return true;

        default:
            return false;
        }
    }
}

/** Given that a tag which opens a variable declaration has just been read,
 create the variable. Once created, push the variable onto the stack of
 variables, push that variable's attribute table onto the attribute table
 stack and update the state of the parser.
 @param t The type of variable to create.
 @param s The next state of the parser (e.g., inside_simple_type, ...)
 @param attrs the attributes read with the tag */
void DmrppParserSax2::process_variable_helper(Type t, ParseState s, const xmlChar **attrs, int nb_attributes)
{
#if 0
	transfer_xml_attrs(attrs, nb_attributes);
#endif

    if (check_required_attribute("name", attrs, nb_attributes)) {
        BaseType *btp = dmr()->factory()->NewVariable(t, get_attribute_val("name", attrs, nb_attributes));
        if (!btp) {
            dmr_fatal_error(this, "Could not instantiate the variable '%s'.", xml_attrs["name"].value.c_str());
            return;
        }

        if ((t == dods_enum_c) && check_required_attribute("enum", attrs, nb_attributes)) {
            D4EnumDef *enum_def = 0;
            string enum_path = get_attribute_val("enum", attrs, nb_attributes);
            if (enum_path[0] == '/')
                enum_def = dmr()->root()->find_enum_def(enum_path);
            else
                enum_def = top_group()->find_enum_def(enum_path);

            if (!enum_def) dmr_fatal_error(this, "Could not find the Enumeration definition '%s'.", enum_path.c_str());

            static_cast<D4Enum*>(btp)->set_enumeration(enum_def);
        }

        btp->set_is_dap4(true); // see comment above
        push_basetype(btp);

        push_attributes(btp->attributes());

        push_state(s);
    }
}

/** @name SAX Parser Callbacks

 These methods are declared static in the class header. This gives them C
 linkage which allows them to be used as callbacks by the SAX parser
 engine. */
//@{
/** Initialize the SAX parser state object. This object is passed to each
 callback as a void pointer. The initial state is parser_start.

 @param p The SAX parser  */
void DmrppParserSax2::dmr_start_document(void * p)
{
    DmrppParserSax2 *parser = static_cast<DmrppParserSax2*>(p);
    parser->error_msg = "";
    parser->char_data = "";

    // Set this in intern_helper so that the loop test for the parser_end
    // state works for the first iteration. It seems like XMLParseChunk calls this
    // function on it's first run. jhrg 9/16/13
    // parser->push_state(parser_start);

    parser->push_attributes(parser->dmr()->root()->attributes());

    if (parser->debug()) cerr << "Parser start state: " << states[parser->get_state()] << endl;
}

/** Clean up after finishing a parse.
 @param p The SAX parser  */
void DmrppParserSax2::dmr_end_document(void * p)
{
    DmrppParserSax2 *parser = static_cast<DmrppParserSax2*>(p);

    if (parser->debug()) cerr << "Parser end state: " << states[parser->get_state()] << endl;

    if (parser->get_state() != parser_end)
        DmrppParserSax2::dmr_error(parser, "The document contained unbalanced tags.");

    // If we've found any sort of error, don't make the DMR; intern() will
    // take care of the error.
    if (parser->get_state() == parser_error || parser->get_state() == parser_fatal_error) return;

    if (!parser->empty_basetype() || parser->empty_group())
        DmrppParserSax2::dmr_error(parser,
            "The document did not contain a valid root Group or contained unbalanced tags.");

    if (parser->debug()) parser->top_group()->dump(cerr);

    parser->pop_group();     // leave the stack 'clean'
    parser->pop_attributes();
}

void DmrppParserSax2::dmr_start_element(void *p, const xmlChar *l, const xmlChar *prefix, const xmlChar *URI,
    int nb_namespaces, const xmlChar **namespaces, int nb_attributes, int /*nb_defaulted*/, const xmlChar **attributes)
{
    DmrppParserSax2 *parser = static_cast<DmrppParserSax2*>(p);
    const char *localname = reinterpret_cast<const char *>(l);

    string this_element_ns_name(URI ? (char *) URI : "null");

    if (parser->get_state() != parser_error) {
        string dap4_ns_name = DapXmlNamspaces::getDapNamespaceString(DAP_4_0);
        if (parser->debug()) cerr << "dap4_ns_name:         " << dap4_ns_name << endl;

        if (this_element_ns_name == dmrpp_namespace) {
            if (strcmp(localname, "chunkDimensionSizes") == 0) {
                if (parser->debug()) cerr << "Found dmrpp:chunkDimensionSizes element. Pushing state." << endl;
                parser->push_state(inside_dmrpp_chunkDimensionSizes_element);
            }
            else {
                if (parser->debug())
                    cerr << "Start of element in dmrpp namespace: " << localname << " detected." << endl;
                parser->push_state(inside_dmrpp_object);
                // Ingest the dmrpp namespaced element text content
            }

        }
        else if (this_element_ns_name != dap4_ns_name) {
            if (parser->debug()) cerr << "Start of non DAP4 element: " << localname << " detected." << endl;
            parser->push_state(not_dap4_element);
        }
    }

    if (parser->debug())
        cerr << "Start element " << localname << "  prefix:  " << (prefix ? (char *) prefix : "null") << "  ns: "
            << this_element_ns_name << " (state: " << states[parser->get_state()] << ")" << endl;

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
        if (parser->debug()) cerr << "Dataset xml:base is set to '" << parser->dmr()->request_xml_base() << "'" << endl;

        if (parser->check_attribute("href", attributes, nb_attributes)) {
            parser->dmrpp_dataset_href = parser->get_attribute_val("href", attributes, nb_attributes);
        }
        if (parser->debug()) cerr << "Dataset dmrpp:href is set to '" << parser->dmrpp_dataset_href << "'" << endl;

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
        if (parser->debug())
            cerr << "SKIPPING unexpected element. localname: " << localname << "namespace: " << this_element_ns_name
                << endl;
        break;

    case inside_dmrpp_object: {
        if (parser->debug()) cerr << "Inside dmrpp namespaced element. localname: " << localname << endl;
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
            if (parser->debug()) cerr << "DMR++ chunks element. localname: " << localname << endl;

            if (parser->check_attribute("compressionType", attributes, nb_attributes)) {
                string compression_type_string(parser->get_attribute_val("compressionType", attributes, nb_attributes));
                dc->ingest_compression_type(compression_type_string);

                if (parser->debug())
                    cerr << "Processed attribute 'compressionType=\"" << compression_type_string << "\"'" << endl;
            }
            else {
                if (parser->debug())
                    cerr << "There was no 'compressionType' attribute associated with the variable '" << bt->type_name()
                        << " " << bt->name() << "'" << endl;
            }
        }
        // Ingest an dmrpp:chunk element and its attributes
        else if (strcmp(localname, "chunk") == 0) {
            string data_url = "unknown_data_location";
            if (parser->check_attribute("href", attributes, nb_attributes)) {
#if 0
                istringstream data_url_ss(parser->xml_attrs["href"].value);
                data_url = data_url_ss.str();
                if (parser->debug())
                cerr << "Processing 'href' value into data_url. href: " << data_url_ss.str() << endl;
#endif

                data_url = parser->get_attribute_val("href", attributes, nb_attributes);
                if (parser->debug())
                    cerr << "Processing 'href' value into data_url. href: " << data_url << endl;
            }
            else {
                if (parser->debug()) cerr << "No attribute 'href' located. Trying Dataset/@dmrpp:href..." << endl;
                // This bit of magic sets the URL used to get the data and it's
                // magic in part because it may be a file or an http URL
                data_url = parser->dmrpp_dataset_href;
                if (parser->debug())
                    cerr << "Processing dmrpp:href into data_url. dmrpp:href='" << data_url << "'" << endl;
            }
            // First we see if it's an HTTP URL, and if not we
            // make a local file url based on the Catalog Root
#if 0
            std::string http("http://");
            std::string https("https://");
            std::string file("file://");
            if (data_url.compare(0, http.size(), http) && data_url.compare(0, https.size(), https)
                && data_url.compare(0, file.size(), file)) {
#endif

            if (data_url.find("http://") != 0 && data_url.find("https://") != 0 && data_url.find("file://") != 0) {
                if (parser->debug()) cerr << "data_url does NOT start with 'http://', 'https://' or 'file://'. "
                    "Retrieving default catalog root directory" << endl;

                // Now we try to find the default catalog. If we can't find it we punt and leave it be.
                BESCatalog *defcat = BESCatalogList::TheCatalogList()->default_catalog();
                if (!defcat) {
                    if (parser->debug()) cerr << "Not able to find the default catalog." << endl;
                }
                else {
                    // Found the catalog so we get the root dir; make a file URL.
                    BESCatalogUtils *utils = BESCatalogList::TheCatalogList()->default_catalog()->get_catalog_utils();

                    if (parser->debug())
                        cerr << "Found default catalog root_dir: '" << utils->get_root_dir() << "'" << endl;

                    data_url = BESUtil::assemblePath(utils->get_root_dir(), data_url, true);
                    data_url = "file://" + data_url;
                }
            }

            if (parser->debug()) cerr << "Processed data_url: '" << data_url << "'" << endl;

            unsigned long long offset = 0;
            unsigned long long size = 0;
            string chunk_position_in_array("");

            if (parser->check_required_attribute("offset", attributes, nb_attributes)) {
                istringstream offset_ss(parser->get_attribute_val("offset", attributes, nb_attributes));
                offset_ss >> offset;
                if (parser->debug()) cerr << "Processed attribute 'offset=\"" << offset << "\"'" << endl;
            }
            else {
                dmr_error(parser, "The hdf:byteStream element is missing the required attribute 'offset'.");
            }

            if (parser->check_required_attribute("nBytes", attributes, nb_attributes)) {
                istringstream size_ss(parser->get_attribute_val("nBytes", attributes, nb_attributes));
                size_ss >> size;
                if (parser->debug()) cerr << "Processed attribute 'nBytes=\"" << size << "\"'" << endl;
            }
            else {
                dmr_error(parser, "The hdf:byteStream element is missing the required attribute 'size'.");
            }

            if (parser->check_attribute("chunkPositionInArray", attributes, nb_attributes)) {
                istringstream chunk_position_ss(parser->get_attribute_val("chunkPositionInArray", attributes, nb_attributes));
                chunk_position_in_array = chunk_position_ss.str();
                if (parser->debug())
                    cerr << "Found attribute 'chunkPositionInArray' value: " << chunk_position_ss.str() << endl;
            }
            else {
                if (parser->debug()) cerr << "No attribute 'chunkPositionInArray' located" << endl;
            }

            dc->add_chunk(data_url, size, offset, chunk_position_in_array);
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

    if (parser->debug()) cerr << "Start element exit state: " << states[parser->get_state()] << endl;
}

void DmrppParserSax2::dmr_end_element(void *p, const xmlChar *l, const xmlChar *prefix, const xmlChar *URI)
{
    DmrppParserSax2 *parser = static_cast<DmrppParserSax2*>(p);
    const char *localname = (const char *) l;

    if (parser->debug())
        cerr << "End element " << localname << " (state " << states[parser->get_state()] << ")" << endl;

    switch (parser->get_state()) {
    case parser_start:
        dmr_fatal_error(parser, "Unexpected state, inside start state while processing element '%s'.", localname);
        break;

    case inside_dataset:
        if (is_not(localname, "Dataset"))
            DmrppParserSax2::dmr_error(parser, "Expected an end Dataset tag; found '%s' instead.", localname);

        parser->pop_state();
        if (parser->get_state() != parser_start)
            dmr_fatal_error(parser, "Unexpected state, expected start state.");
        else {
            parser->pop_state();
            parser->push_state(parser_end);
        }
        break;

    case inside_group: {
        if (is_not(localname, "Group"))
            DmrppParserSax2::dmr_error(parser, "Expected an end tag for a Group; found '%s' instead.", localname);

        if (!parser->empty_basetype() || parser->empty_group())
            DmrppParserSax2::dmr_error(parser,
                "The document did not contain a valid root Group or contained unbalanced tags.");

        parser->pop_group();
        parser->pop_state();
        break;
    }

    case inside_attribute_container:
        if (is_not(localname, "Attribute"))
            DmrppParserSax2::dmr_error(parser, "Expected an end Attribute tag; found '%s' instead.", localname);

        parser->pop_state();
        parser->pop_attributes();
        break;

    case inside_attribute:
        if (is_not(localname, "Attribute"))
            DmrppParserSax2::dmr_error(parser, "Expected an end Attribute tag; found '%s' instead.", localname);

        parser->pop_state();
        break;

    case inside_attribute_value: {
        if (is_not(localname, "Value"))
            DmrppParserSax2::dmr_error(parser, "Expected an end value tag; found '%s' instead.", localname);

        parser->pop_state();

        // The old code added more values using the name and type as
        // indexes to find the correct attribute. Use get() for that
        // now. Or fix this code to keep a pointer to the to attribute...
        D4Attributes *attrs = parser->top_attributes();
        D4Attribute *attr = attrs->get(parser->dods_attr_name);
        if (!attr) {
            attr = new D4Attribute(parser->dods_attr_name, StringToD4AttributeType(parser->dods_attr_type));
            attrs->add_attribute_nocopy(attr);
        }
        attr->add_value(parser->char_data);

        parser->char_data = ""; // Null this after use.
        break;
    }

    case inside_other_xml_attribute: {
        if (strcmp(localname, "Attribute") == 0 && parser->root_ns == (const char *) URI) {
            parser->pop_state();

            // The old code added more values using the name and type as
            // indexes to find the correct attribute. Use get() for that
            // now. Or fix this code to keep a pointer to the to attribute...
            D4Attributes *attrs = parser->top_attributes();
            D4Attribute *attr = attrs->get(parser->dods_attr_name);
            if (!attr) {
                attr = new D4Attribute(parser->dods_attr_name, StringToD4AttributeType(parser->dods_attr_type));
                attrs->add_attribute_nocopy(attr);
            }
            attr->add_value(parser->other_xml);

            parser->other_xml = ""; // Null this after use.
        }
        else {
            if (parser->other_xml_depth == 0) {
                DmrppParserSax2::dmr_error(parser, "Expected an OtherXML attribute to end! Instead I found '%s'",
                    localname);
                break;
            }
            parser->other_xml_depth--;

            parser->other_xml.append("</");
            if (prefix) {
                parser->other_xml.append((const char *) prefix);
                parser->other_xml.append(":");
            }
            parser->other_xml.append(localname);
            parser->other_xml.append(">");
        }
        break;
    }

    case inside_enum_def:
        if (is_not(localname, "Enumeration"))
            DmrppParserSax2::dmr_error(parser, "Expected an end Enumeration tag; found '%s' instead.", localname);
        if (!parser->top_group())
            DmrppParserSax2::dmr_fatal_error(parser,
                "Expected a Group to be the current item, while finishing up an Enumeration.");
        else {
            // copy the pointer; not a deep copy
            parser->top_group()->enum_defs()->add_enum_nocopy(parser->enum_def());
            // Set the enum_def to null; next call to enum_def() will
            // allocate a new object
            parser->clear_enum_def();
            parser->pop_state();
        }
        break;

    case inside_enum_const:
        if (is_not(localname, "EnumConst"))
            DmrppParserSax2::dmr_error(parser, "Expected an end EnumConst tag; found '%s' instead.", localname);

        parser->pop_state();
        break;

    case inside_dim_def: {
        if (is_not(localname, "Dimension"))
            DmrppParserSax2::dmr_error(parser, "Expected an end Dimension tag; found '%s' instead.", localname);

        if (!parser->top_group())
            DmrppParserSax2::dmr_error(parser,
                "Expected a Group to be the current item, while finishing up an Dimension.");

        parser->top_group()->dims()->add_dim_nocopy(parser->dim_def());
        // Set the dim_def to null; next call to dim_def() will
        // allocate a new object. Calling 'clear' is important because
        // the cleanup method will free dim_def if it's not null and
        // we just copied the pointer in the add_dim_nocopy() call
        // above.
        parser->clear_dim_def();
        parser->pop_state();
        break;
    }

    case inside_simple_type:
        if (is_simple_type(get_type(localname))) {
            BaseType *btp = parser->top_basetype();
            parser->pop_basetype();
            parser->pop_attributes();

            BaseType *parent = 0;
            if (!parser->empty_basetype())
                parent = parser->top_basetype();
            else if (!parser->empty_group())
                parent = parser->top_group();
            else {
                dmr_fatal_error(parser, "Both the Variable and Groups stacks are empty while closing a %s element.",
                    localname);
                delete btp;
                parser->pop_state();
                break;
            }

            if (parent->type() == dods_array_c)
                static_cast<Array*>(parent)->prototype()->add_var_nocopy(btp);
            else
                parent->add_var_nocopy(btp);
        }
        else
            DmrppParserSax2::dmr_error(parser, "Expected an end tag for a simple type; found '%s' instead.", localname);

        parser->pop_state();
        break;

    case inside_dim:
        if (is_not(localname, "Dim"))
            DmrppParserSax2::dmr_fatal_error(parser, "Expected an end Dim tag; found '%s' instead.", localname);

        parser->pop_state();
        break;

    case inside_map:
        if (is_not(localname, "Map"))
            DmrppParserSax2::dmr_fatal_error(parser, "Expected an end Map tag; found '%s' instead.", localname);

        parser->pop_state();
        break;

    case inside_constructor: {
        if (strcmp(localname, "Structure") != 0 && strcmp(localname, "Sequence") != 0) {
            DmrppParserSax2::dmr_error(parser, "Expected an end tag for a constructor; found '%s' instead.", localname);
            return;
        }

        BaseType *btp = parser->top_basetype();
        parser->pop_basetype();
        parser->pop_attributes();

        BaseType *parent = 0;
        if (!parser->empty_basetype())
            parent = parser->top_basetype();
        else if (!parser->empty_group())
            parent = parser->top_group();
        else {
            dmr_fatal_error(parser, "Both the Variable and Groups stacks are empty while closing a %s element.",
                localname);
            delete btp;
            parser->pop_state();
            break;
        }

        // TODO Why doesn't this code mirror the simple_var case and test
        // for the parent being an array? jhrg 10/13/13
        parent->add_var_nocopy(btp);
        parser->pop_state();
        break;
    }

    case not_dap4_element:
        if (parser->debug()) cerr << "End of non DAP4 element: " << localname << endl;
        parser->pop_state();
        break;

    case inside_dmrpp_object:
        if (parser->debug()) cerr << "End of dmrpp namespace element: " << localname << endl;
        parser->pop_state();
        break;

    case inside_dmrpp_chunkDimensionSizes_element: {
        if (parser->debug()) cerr << "End of chunkDimensionSizes element. localname: " << localname << endl;

        if (is_not(localname, "chunkDimensionSizes"))
            DmrppParserSax2::dmr_error(parser, "Expected an end value tag; found '%s' instead.", localname);
        DmrppCommon *dc = dynamic_cast<DmrppCommon*>(parser->top_basetype());   // Get the Dmrpp common info
        if (!dc)
            throw BESInternalError("Could not cast BaseType to DmrppType in the drmpp handler.", __FILE__, __LINE__);
        string element_text(parser->char_data);
        if (parser->debug()) cerr << "chunkDimensionSizes element_text: '" << element_text << "'" << endl;
        dc->parse_chunk_dimension_sizes(element_text);
        parser->char_data = ""; // Null this after use.
        parser->pop_state();
        break;
    }

    case parser_unknown:
        parser->pop_state();
        break;

    case parser_error:
    case parser_fatal_error:
        break;

    case parser_end:
        // FIXME Error?
        break;
    }

    if (parser->debug()) cerr << "End element exit state: " << states[parser->get_state()] << endl;
}

/** Process/accumulate character data. This may be called more than once for
 one logical clump of data. Only save character data when processing
 'value' elements; throw away all other characters. */
void DmrppParserSax2::dmr_get_characters(void * p, const xmlChar * ch, int len)
{
    DmrppParserSax2 *parser = static_cast<DmrppParserSax2*>(p);

    switch (parser->get_state()) {
    case inside_attribute_value:
    case inside_dmrpp_chunkDimensionSizes_element:
        parser->char_data.append((const char *) (ch), len);
        BESDEBUG(module, "Characters[" << parser->char_data.size() << "]" << parser->char_data << "'" << endl);
        break;

    case inside_other_xml_attribute:
        parser->other_xml.append((const char *) (ch), len);
        BESDEBUG(module, "Other XML Characters: '" << parser->other_xml << "'" << endl);
        break;

    default:
        break;
    }
}

/** Read whitespace that's not really important for content. This is used
 only for the OtherXML attribute type to preserve formating of the XML.
 Doing so makes the attribute value far easier to read.
 */
void DmrppParserSax2::dmr_ignoreable_whitespace(void *p, const xmlChar *ch, int len)
{
    DmrppParserSax2 *parser = static_cast<DmrppParserSax2*>(p);

    switch (parser->get_state()) {
    case inside_other_xml_attribute:
        parser->other_xml.append((const char *) (ch), len);
        break;

    default:
        break;
    }
}

/** Get characters in a cdata block. DAP does not use CData, but XML in an
 OtherXML attribute (the value of that DAP attribute) might use it. This
 callback also allows CData when the parser is in the 'parser_unknown'
 state since some future DAP element might use it.
 */
void DmrppParserSax2::dmr_get_cdata(void *p, const xmlChar *value, int len)
{
    DmrppParserSax2 *parser = static_cast<DmrppParserSax2*>(p);

    switch (parser->get_state()) {
    case inside_other_xml_attribute:
        parser->other_xml.append((const char *) (value), len);
        break;

    case parser_unknown:
        break;

    default:
        DmrppParserSax2::dmr_error(parser, "Found a CData block but none are allowed by DAP4.");

        break;
    }
}

/** Handle the standard XML entities.

 @param parser The SAX parser
 @param name The XML entity. */
xmlEntityPtr DmrppParserSax2::dmr_get_entity(void *, const xmlChar * name)
{
    return xmlGetPredefinedEntity(name);
}

/** Process an XML fatal error. Note that SAX provides for warnings, errors
 and fatal errors. This code treats them all as fatal errors since there's
 typically no way to tell a user about the error since there's often no
 user interface for this software.

 @note This static function does not throw an exception or otherwise
 alter flow of control except for altering the parser state.

 @param p The SAX parser
 @param msg A printf-style format string. */
void DmrppParserSax2::dmr_fatal_error(void * p, const char *msg, ...)
{
    va_list args;
    DmrppParserSax2 *parser = static_cast<DmrppParserSax2*>(p);

    parser->push_state(parser_fatal_error);

    va_start(args, msg);
    char str[1024];
    vsnprintf(str, 1024, msg, args);
    va_end(args);

    int line = xmlSAX2GetLineNumber(parser->context);

    if (!parser->error_msg.empty()) parser->error_msg += "\n";
    parser->error_msg += "At line " + long_to_string(line) + ": " + string(str);
}

void DmrppParserSax2::dmr_error(void *p, const char *msg, ...)
{
    va_list args;
    DmrppParserSax2 *parser = static_cast<DmrppParserSax2*>(p);

    parser->push_state(parser_error);

    va_start(args, msg);
    char str[1024];
    vsnprintf(str, 1024, msg, args);
    va_end(args);

    int line = xmlSAX2GetLineNumber(parser->context);

    if (!parser->error_msg.empty()) parser->error_msg += "\n";
    parser->error_msg += "At line " + long_to_string(line) + ": " + string(str);
}
//@}

/** Clean up after a parse operation. If the parser encountered an error,
 * throw either an Error or InternalErr object.
 */
void DmrppParserSax2::cleanup_parse()
{
    bool wellFormed = context->wellFormed;
    bool valid = context->valid;

    context->sax = NULL;
    xmlFreeParserCtxt(context);

    delete d_enum_def;
    d_enum_def = 0;

    delete d_dim_def;
    d_dim_def = 0;

    // If there's an error, there may still be items on the stack at the
    // end of the parse.
    while (!btp_stack.empty()) {
        delete top_basetype();
        pop_basetype();
    }

    if (!wellFormed)
        throw Error("The DMR was not well formed. " + error_msg);
    else if (!valid)
        throw Error("The DMR was not valid." + error_msg);
    else if (get_state() == parser_error)
        throw Error(error_msg);
    else if (get_state() == parser_fatal_error) throw InternalErr(error_msg);
}

/**
 * Read the DMR from a stream.
 *
 * @param f The input stream
 * @param dest_dmr Value-result parameter. Pass a pointer to a DMR in and
 * the information in the DMR will be added to it.
 * @param boundary If not empty, use this as the boundary tag in a MPM document
 * that marks the end of the part hat holds the DMR. Stop reading when the
 * boundary is found.
 * @param debug If true, ouput helpful debugging messages, False by default.
 *
 * @exception Error Thrown if the XML document could not be read or parsed.
 * @exception InternalErr Thrown if an internal error is found.
 */
void DmrppParserSax2::intern(istream &f, DMR *dest_dmr, bool debug)
{
    d_debug = debug;

    // Code example from libxml2 docs re: read from a stream.

    if (!f.good()) throw Error("Input stream not open or read error");
    if (!dest_dmr) throw InternalErr(__FILE__, __LINE__, "DMR object is null");

    d_dmr = dest_dmr; // dump values here

    int line_num = 1;
    string line;

    // Get the <xml ... ?> line
    getline(f, line);
    if (line.length() == 0) throw Error("No input found while parsing the DMR.");

    if (debug) cerr << "line: (" << line_num << "): " << endl << line << endl << endl;

    context = xmlCreatePushParserCtxt(&dmrpp_sax_parser, this, line.c_str(), line.length(), "stream");
    context->validate = true;
    push_state(parser_start);

    // Get the first line of stuff
    getline(f, line);
    ++line_num;

    if (debug) cerr << "line: (" << line_num << "): " << endl << line << endl << endl;

    while (!f.eof() && (get_state() != parser_end)) {
        xmlParseChunk(context, line.c_str(), line.length(), 0);
        // Get the next line
        getline(f, line);
        ++line_num;
        if (debug) cerr << "line: (" << line_num << "): " << endl << line << endl << endl;
    }

    // This call ends the parse.
    xmlParseChunk(context, line.c_str(), 0, 1/*terminate*/);

    // This checks that the state on the parser stack is parser_end and throws
    // an exception if it's not (i.e., the loop exited with gcount() == 0).
    cleanup_parse();
}



/** Parse a DMR document stored in a string.
 *
 * @param document Read the DMR from this string.
 * @param dest_dmr Value/result parameter; dumps the information to this DMR
 * instance.
 * @param debug If true, ouput helpful debugging messages, False by default
 *
 * @exception Error Thrown if the XML document could not be read or parsed.
 * @exception InternalErr Thrown if an internal error is found.
 */
void DmrppParserSax2::intern(const string &document, DMR *dest_dmr, bool debug)
{
    intern(document.c_str(), document.length(), dest_dmr, debug);
}

/** Parse a DMR document stored in a char *buffer.
 *
 * @param document Read the DMR from this string.
 * @param dest_dmr Value/result parameter; dumps the information to this DMR
 * instance.
 * @param debug If true, output helpful debugging messages, False by default
 *
 * @exception Error Thrown if the XML document could not be read or parsed.
 * @exception InternalErr Thrown if an internal error is found.
 */
void DmrppParserSax2::intern(const char *buffer, int size, DMR *dest_dmr, bool debug)
{
    if (!(size > 0)) return;

    d_debug = debug;

    // Code example from libxml2 docs re: read from a stream.

    if (!dest_dmr) throw InternalErr(__FILE__, __LINE__, "DMR object is null");
    d_dmr = dest_dmr; // dump values in dest_dmr

    push_state(parser_start);
    context = xmlCreatePushParserCtxt(&dmrpp_sax_parser, this, buffer, size, "stream");
    context->validate = true;
    //push_state(parser_start);
    //xmlParseChunk(context, buffer, size, 0);

    // This call ends the parse.
    xmlParseChunk(context, buffer, 0, 1/*terminate*/);

    // This checks that the state on the parser stack is parser_end and throws
    // an exception if it's not (i.e., the loop exited with gcount() == 0).
    cleanup_parse();
}

} // namespace dmrpp
