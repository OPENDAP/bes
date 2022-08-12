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

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <unordered_set>
#include <cstring>

#include <libdap/BaseType.h>
#include <libdap/Array.h>
#include <libdap/Type.h>
#include <libdap/D4Dimensions.h>
#include <libdap/D4Group.h>
#include <libdap/D4BaseTypeFactory.h>
#include <libdap/D4Enum.h>
#include <libdap/D4EnumDefs.h>
#include <libdap/D4Attributes.h>
#include <libdap/D4Maps.h>
#include <libdap/DMR.h>
#include <libdap/util.h>        // is_simple_type()

#define PUGIXML_NO_XPATH
#define PUGIXML_HEADER_ONLY
#include <pugixml.hpp>

#include "url_impl.h"           // see bes/http
#include "DMRpp.h"
#include "DMZ.h"                // this includes the pugixml header
#include "Chunk.h"
#include "DmrppCommon.h"
#include "DmrppArray.h"
#include "DmrppStr.h"
#include "DmrppUrl.h"
#include "DmrppD4Group.h"
#include "Base64.h"
#include "DmrppRequestHandler.h"
#include "DmrppChunkOdometer.h"
#include "BESInternalError.h"
#include "BESDebug.h"

using namespace pugi;
using namespace std;
using namespace libdap;

// The pugixml library does not grok namespaces. So, for a tag named 'dmrpp:chunks'
// if TREAT_NAMESPACES_AS_LITERALS is '1' the parser matches the whole string. If it
// is '0' the parser only matches the characters after the colon. In both cases the
// namespace (as XML intends) is not used. Using '1' is a bit more efficient.
// jhrg 11/2/21
#define TREAT_NAMESPACES_AS_LITERALS 1

// THe code can either search for a DAP variable's information in the XML, or it can
// record that during the parse process. Set this when/if the code does the latter.
// Using this simplifies the lazy-load process, particularly for the DAP2 DDS and
// data responses (which have not yet been coded completely). jhrg 11/17/21
#define USE_CACHED_XML_NODE 1

#define SUPPORT_FILL_VALUE_CHUNKS 1

#define PARSER "dmz"
#define prolog std::string("DMZ::").append(__func__).append("() - ")

namespace dmrpp {

using shape = std::vector<unsigned long long>;

#if 1
const std::set<std::string> DMZ::variable_elements{"Byte", "Int8", "Int16", "Int32", "Int64", "UInt8", "UInt16", "UInt32",
                                              "UInt64", "Float32", "Float64", "String", "Structure", "Sequence",
                                              "Enum", "Opaque"};
#endif

/// @brief Are the C-style strings equal?
static inline bool is_eq(const char *value, const char *key)
{
#if TREAT_NAMESPACES_AS_LITERALS
    return strcmp(value, key) == 0;
#else
    if (strcmp(value, key) == 0) {
        return true;
    }
    else {
        const char* colon = strchr(value, ':');
        return colon && strcmp(colon + 1, key) == 0;
    }
#endif
}

/// @brief Are any of the child nodes 'Dim' elements?
static inline bool has_dim_nodes(const xml_node &var_node)
{
    return var_node.child("Dim"); // just one is enough
}

/// @brief Simple set membership; used to test for variable elements, et cetera.
static inline bool member_of(const set<string> &elements_set, const string &element_name)
{
    return elements_set.find(element_name) != elements_set.end();
}

/// @brief syntactic sugar for a dynamic cast to DmrppCommon
static inline DmrppCommon *dc(BaseType *btp)
{
    auto *dc = dynamic_cast<DmrppCommon*>(btp);
    if (!dc)
        throw BESInternalError(string("Expected a BaseType that was also a DmrppCommon instance (")
                                       .append((btp) ? btp->name() : "unknown").append(")."), __FILE__, __LINE__);
    return dc;
}

/**
 * @brief Build a DMZ object and initialize it using a DMR++ XML document
 * @param file_name The DMR++ XML document to parse.
 * @exception BESInternalError if file_name cannot be parsed
 */
DMZ::DMZ(const string &file_name)
{
    parse_xml_doc(file_name);
}

/**
 * @brief Build the DOM tree for a DMR++ XML document
 * @param file_name
 */
void
DMZ::parse_xml_doc(const std::string &file_name)
{
    std::ifstream stream(file_name);

    // Free memory used by a previously parsed document.
    d_xml_doc.reset();

    // parse_ws_pcdata_single will include the space when it appears in a <Value> </Value>
    // DAP Attribute element. jhrg 11/3/21
    pugi::xml_parse_result result = d_xml_doc.load(stream,  pugi::parse_default | pugi::parse_ws_pcdata_single);

    if (!result)
        throw BESInternalError(string("DMR++ parse error: ").append(result.description()), __FILE__, __LINE__);

    if (!d_xml_doc.document_element())
        throw BESInternalError("No DMR++ data present.", __FILE__, __LINE__);
}

/**
 * @brief process a Dataset element
 *
 * Given the root node of a DOM tree for the DMR++ XML document, populate the
 * libdap DMR instance with information from that Dataset node. This sets the
 * stage for the parse of rest of the information in the DOM tree.
 * @param dmr Dump the information in this instance of DMR
 * @param xml_root The root node of the DOM tree
 */
void DMZ::process_dataset(DMR *dmr, const xml_node &xml_root)
{
    // Process the attributes
    int required_attrs_found = 0;   // there are 1
    string href_attr;
    bool href_trusted = false;
    string dmrpp_version;           // empty or holds a value if dmrpp::version is present
    for (xml_attribute attr = xml_root.first_attribute(); attr; attr = attr.next_attribute()) {
        if (is_eq(attr.name(), "name")) {
            ++required_attrs_found;
            dmr->set_name(attr.value());
        }
        else if (is_eq(attr.name(), "dapVersion")) {
            dmr->set_dap_version(attr.value());
        }
        else if (is_eq(attr.name(), "dmrVersion")) {
            dmr->set_dmr_version(attr.value());
        }
        else if (is_eq(attr.name(), "base")) {
            dmr->set_request_xml_base(attr.value());
            BESDEBUG(PARSER, prolog << "Dataset xml:base is set to '" << dmr->request_xml_base() << "'" << endl);
        }
        // The pugixml library does not use XML namespaces AFAIK. jhrg 11/2/21
        else if (is_eq(attr.name(), "xmlns")) {
            dmr->set_namespace(attr.value());
        }
        // This code does not use namespaces. By default, we assume the DMR++ elements
        // all use the namespace prefix 'dmrpp'. jhrg 11/2/21
        else if (is_eq(attr.name(), "dmrpp:href")) {
            href_attr = attr.value();
        }
        else if (is_eq(attr.name(), "dmrpp:trust")) {
            href_trusted = is_eq(attr.value(), "true");
        }
        else if (is_eq(attr.name(), "dmrpp:version")) {
            dmrpp_version = attr.value();
        }
        // We allow other, non recognized attributes, so there is no 'else' jhrg 10/20/21
    }

    if (dmrpp_version.empty()) {    // old style DMR++, set enable-kludge flag
        DmrppRequestHandler::d_emulate_original_filter_order_behavior = true;
    }
    else {
        auto dmrpp = dynamic_cast<DMRpp*>(dmr);
        if (dmrpp) {
            dmrpp->set_version(dmrpp_version);
        }
    }

    if (required_attrs_found != 1)
        throw BESInternalError("DMR++ XML dataset element missing one or more required attributes.", __FILE__, __LINE__);

    d_dataset_elem_href.reset(new http::url(href_attr, href_trusted));
}

/**
 * @brief Parse information from a Dimension element
 * @param grp The group we are currently processing (could be the root group)
 * @param dimension_node The node in the DOM tree of the <Dimension> element
 */
void DMZ::process_dimension(D4Group *grp, const xml_node &dimension_node)
{
    string name_value;
    string size_value;
    for (xml_attribute attr = dimension_node.first_attribute(); attr; attr = attr.next_attribute()) {
        if (is_eq(attr.name(), "name")) {
            name_value = attr.value();
        }
        else if (is_eq(attr.name(), "size")) {
            size_value = attr.value();
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
 * @brief Process a Dim element and add it to the given Array
 * @param dmr
 * @param grp The group we are currently processing (could be the root group)
 * @param array The variable we are processing
 * @param dim_node The node in the DOM tree of the Dim element
 */
void DMZ::process_dim(DMR *dmr, D4Group *grp, Array *array, const xml_node &dim_node)
{
    assert(array->is_vector_type());

    string name_value;
    string size_value;
    for (xml_attribute attr = dim_node.first_attribute(); attr; attr = attr.next_attribute()) {
        if (is_eq(attr.name(), "name")) {
            name_value = attr.value();
        }
        else if (is_eq(attr.name(), "size")) {
            size_value = attr.value();
        }
    }

    if (name_value.empty() && size_value.empty())
        throw BESInternalError("Either 'size' or 'name' must be used in a Dim element.", __FILE__, __LINE__);
    if (!name_value.empty() && !size_value.empty())
        throw BESInternalError("Only one of 'size' and 'name' are allowed in a Dim element, but both were used.", __FILE__, __LINE__);

    if (!size_value.empty()) {
        BESDEBUG(PARSER, prolog << "Processing nameless Dim of size: " << stoi(size_value) << endl);
        array->append_dim(stoi(size_value));
    }
    else if (!name_value.empty()) {
        BESDEBUG(PARSER, prolog << "Processing Dim with named Dimension reference: " << name_value << endl);

        D4Dimension *dim;
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

void DMZ::process_map(DMR *dmr, D4Group *grp, Array *array, const xml_node &map_node)
{
    assert(array->is_vector_type());

    string name_value;
    string size_value;
    for (xml_attribute attr = map_node.first_attribute(); attr; attr = attr.next_attribute()) {
        if (is_eq(attr.name(), "name")) {
            name_value = attr.value();
        }
    }

    // All map names are FQNs. If we get one that isn't, assume it's within the most current group.
    if (name_value[0] != '/')
        name_value = grp->FQN() + name_value;

    // The array variable that holds the data for the Map
    Array *map_source = dmr->root()->find_map_source(name_value);

    // In the SAX2 parser, we had 'strict' and 'permissive' modes. For Maps, permissive
    // allowed the DAP variable for a Map to be missing so that users could request just
    // the data with the maps. I'm implementing that behavior. Below is the original
    // comment from DmrppParserSAX2.cc. jhrg 11/3/21

    // Change: If the parser is in 'strict' mode (the default) and the Array named by
    // the Map cannot be fond, it is an error. If 'strict' mode is false (permissive
    // mode), then this is not an error. However, the Array referenced by the Map will
    // be null. This is a change in the parser's behavior to accommodate requests for
    // Arrays that include Maps that do not also include the Map(s) in the request.
    // See https://opendap.atlassian.net/browse/HYRAX-98. jhrg 4/13/16

    array->maps()->add_map(new D4Map(name_value, map_source));
}

/**
 * @brief Process an element that is a variable
 *
 * This method processes both scalar and array variables, but not Groups since
 * those are handled separately. This method  process all of the elements contained
 * by the scalar or array variable element.
 *
 * @note Only one of group or parent can be non-null
 *
 * @param dmr
 * @param group If not null, add the new variable to this group
 * @param parent If not null add this new variable to this constructor
 * @param var_node
 */
void DMZ::process_variable(DMR *dmr, D4Group *group, Constructor *parent, const xml_node &var_node)
{
    assert(group);

    // Variables are declared using nodes with type names (e.g., <Float32...>)
    // Variables are arrays if they have one or more <Dim...> child nodes.
    Type t = get_type(var_node.name());

    assert(t != dods_group_c);  // Groups are special and handled elsewhere

    bool is_array_type = has_dim_nodes(var_node);
    BaseType *btp;
    if (is_array_type) {
        btp = add_array_variable(dmr, group, parent, t, var_node);
        if (t == dods_structure_c || t == dods_sequence_c) {
            assert(btp->type() == dods_array_c && btp->var()->type() == t);
            // NB: For an array of a Constructor, add children to the Constructor, not the array
            parent = dynamic_cast<Constructor*>(btp->var());
            assert(parent);
            for (auto child = var_node.first_child(); child; child = child.next_sibling()) {
                if (member_of(variable_elements, child.name()))
                    process_variable(dmr, group, parent, child);
            }
        }
    }
    else {
        btp = add_scalar_variable(dmr, group, parent, t, var_node);
        if (t == dods_structure_c || t == dods_sequence_c) {
            assert(btp->type() == t);
            parent = dynamic_cast<Constructor*>(btp);
            assert(parent);
            for (auto child = var_node.first_child(); child; child = child.next_sibling()) {
                if (member_of(variable_elements, child.name()))
                    process_variable(dmr, group, parent, child);
            }
        }
    }

    dc(btp)->set_xml_node(var_node);
}

/**
 * @brief helper code to build a BaseType that may wind up a scalar or an array
 * @param dmr
 * @param group If this is an enum, look in this group for am enum definition with a relative path
 * @param t What DAP type is the new variable?
 * @param var_node
 */
BaseType *DMZ::build_variable(DMR *dmr, D4Group *group, Type t, const xml_node &var_node)
{
    assert(dmr->factory());

    string name_value;
    string enum_value;
    for (xml_attribute attr = var_node.first_attribute(); attr; attr = attr.next_attribute()) {
        if (is_eq(attr.name(), "name")) {
            name_value = attr.value();
        }
        if (is_eq(attr.name(), "enum")) {
            enum_value = attr.value();
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

        D4EnumDef *enum_def;
        if (enum_value[0] == '/')
            enum_def = dmr->root()->find_enum_def(enum_value);
        else
            enum_def = group->find_enum_def(enum_value);

        if (!enum_def)
            throw BESInternalError("Could not find the Enumeration definition '" + enum_value + "'.", __FILE__, __LINE__);

        dynamic_cast<D4Enum&>(*btp).set_enumeration(enum_def);
    }

    return btp;
}

/**
 * Given that a tag which opens a variable declaration has just been read,
 * create the variable. This is used when the variable is a scalar.
 * @param dmr
 * @param group If parent is null, add the new var to this group
 * @param parent If non-null, add the new var to this constructor
 * @param t What DAP type is the variable?
 * @param var_node
 * @return The new variable
 */
BaseType *DMZ::add_scalar_variable(DMR *dmr, D4Group *group, Constructor *parent, Type t, const xml_node &var_node)
{
    assert(group);

    BaseType *btp = build_variable(dmr, group, t, var_node);

    // if parent is non-null, the code should add the new var to a constructor,
    // else add the new var to the group.
    if (parent)
        parent->add_var_nocopy(btp);
    else
        group->add_var_nocopy(btp);

    return btp;
}

/**
 * Given that a tag which opens a variable declaration has just been read,
 * create the variable. This is used when the variable is an Array.
 *
 * This follows the same basic logic as add_scalar_variable() but adds
 * processing of an Array's Dim and Map elements.
 *
 * @param dmr
 * @param group
 * @param parent
 * @param t
 * @param var_node
 * @return
 */
BaseType *DMZ::add_array_variable(DMR *dmr, D4Group *group, Constructor *parent, Type t, const xml_node &var_node)
{
    assert(group);

    BaseType *btp = build_variable(dmr, group, t, var_node);

    // Transform the scalar to an array
    auto *array = static_cast<Array*>(dmr->factory()->NewVariable(dods_array_c, btp->name()));
    array->set_is_dap4(true);
    array->add_var_nocopy(btp);

    // The SAX parser set up the parse of attributes here. For the thin DMR, we won't
    // parse those from the DMR now. jhrg 10/21/21

    // Now grab the dimension elements
    for (auto child = var_node.first_child(); child; child = child.next_sibling()) {
        if (is_eq(child.name(), "Dim")) {
            process_dim(dmr, group, array, child);
        }
        else if (is_eq(child.name(), "Map")) {
            process_map(dmr, group, array, child);
        }
    }

    if (parent)
        parent->add_var_nocopy(array);
    else
        group->add_var_nocopy(array);

    return array;
}

/**
 * @brief Process a Group element
 * This processes the information in a Group element and then processes the contained
 * Dimension, Group and Variable elements.
 * @param dmr
 * @param parent
 * @param var_node
 */
void DMZ::process_group(DMR *dmr, D4Group *parent, const xml_node &var_node)
{
    string name_value;
    for (xml_attribute attr = var_node.first_attribute(); attr; attr = attr.next_attribute()) {
        if (is_eq(attr.name(), "name")) {
            name_value = attr.value();
        }
    }

    if (name_value.empty())
        throw BESInternalError("The required attribute 'name' was missing from a Group element.", __FILE__, __LINE__);

    BaseType *btp = dmr->factory()->NewVariable(dods_group_c, name_value);
    if (!btp)
        throw BESInternalError("Could not instantiate the Group '" + name_value + "'.", __FILE__, __LINE__);

    auto new_group = dynamic_cast<DmrppD4Group*>(btp);

    // Need to set this to get the D4Attribute behavior in the type classes
    // shared between DAP2 and DAP4. jhrg 4/18/13
    new_group->set_is_dap4(true);

    // link it up and change the current group
    new_group->set_parent(parent);
    parent->add_group_nocopy(new_group);

    // Save the xml_node so that we can later find unprocessed XML without searching
    new_group->set_xml_node(var_node);

    // Now parse all the child nodes of the Group.
    // NB: this is the same block of code as in build_thin_dmr(); refactor. jhrg 10/21/21
    for (auto child = var_node.first_child(); child; child = child.next_sibling()) {
        if (is_eq(child.name(), "Dimension")) {
            process_dimension(new_group, child);
        }
        else if (is_eq(child.name(), "Group")) {
            process_group(dmr, new_group, child);
        }
        else if (member_of(variable_elements, child.name())) {
            process_variable(dmr, new_group, nullptr, child);
        }
    }
}

/**
 * @brief populate the DMR instance as a 'thin DMR'
 * @note Assume the DMZ holds valid DMR++ metadata.
 * @param dmr Pointer to a DMR instance that should be populated
 */
void DMZ::build_thin_dmr(DMR *dmr)
{
    auto xml_root_node = d_xml_doc.first_child();

    process_dataset(dmr, xml_root_node);

    auto root_group = dmr->root();

    auto *dg = dynamic_cast<DmrppD4Group*>(root_group);
    if (!dg)
        throw BESInternalError("Expected the root group to also be an instance of DmrppD4Group.", __FILE__, __LINE__);

    dg->set_xml_node(xml_root_node);

    for (auto child = xml_root_node.first_child(); child; child = child.next_sibling()) {
        if (is_eq(child.name(), "Dimension")) {
            process_dimension(dg, child);
        }
        else if (is_eq(child.name(), "Group")) {
            process_group(dmr, dg, child);
        }
        // TODO Add EnumDef
        else if (member_of(variable_elements, child.name())) {
            process_variable(dmr, dg, nullptr, child);
        }
    }
}

/**
 * Check to see if the current tag is either an \c Attribute or an \c Alias
 * start tag. This method is a glorified macro...
 *
 * @param name The start tag name
 * @param attrs The tag's XML attributes
 * @return True if the tag was an \c Attribute or \c Alias tag
 */
void DMZ::process_attribute(D4Attributes *attributes, const xml_node &dap_attr_node)
{
    string name_value;
    string type_value;
    for (xml_attribute attr = dap_attr_node.first_attribute(); attr; attr = attr.next_attribute()) {
        if (is_eq(attr.name(), "name")) {
            name_value = attr.value();
        }
        if (is_eq(attr.name(), "type")) {
            type_value = attr.value();
        }
    }

    if (name_value.empty() || type_value.empty())
        throw BESInternalError("The required attribute 'name' or 'type' was missing from an Attribute element.", __FILE__, __LINE__);

    if (type_value == "Container") {
        // Make the new attribute container and add it to current container
        auto *dap_attr_cont = new D4Attribute(name_value, attr_container_c);
        attributes->add_attribute_nocopy(dap_attr_cont);
        // In this call, 'attributes()' will allocate the D4Attributes object
        // that will hold the container's attributes.
        // Test to see if there really are child "Attribute" nodes - empty containers
        // are allowed. jhrg 11/4/21
        if (dap_attr_node.first_child()) {
            for (auto attr_node: dap_attr_node.children("Attribute")) {
                process_attribute(dap_attr_cont->attributes(), attr_node);
            }
        }
    }
    else if (type_value == "OtherXML") {
        // TODO Add support for OtherXML
    }
    else {
        // Make the D4Attribute and add it to the D4Attributes attribute container
        auto *attribute = new D4Attribute(name_value, StringToD4AttributeType(type_value));
        attributes->add_attribute_nocopy(attribute);
        // Process one or more Value elements
        for (auto value_elem = dap_attr_node.first_child(); value_elem; value_elem = value_elem.next_sibling()) {
            if (is_eq(value_elem.name(), "Value")) {
                attribute->add_value(value_elem.child_value());  // returns the text of the first data node
            }
        }
     }
}

/**
 * @brief Build the lineage of a variable on a stack
 *
 * This function builds a stack of BaseTypes so that the top-most
 * variable will be at the top level of the DOM tree (a child of
 * the Dataset node) and each successive variable can be found
 * by looking further in the tree.
 *
 * @note Because this function works by starting with the leaf node
 * and visiting each parent in succession, it will include Array BaseTypes
 * but the XML DOM tree _does not_. This function could be improved
 * by making sure that Array objects are not pushed on the stack.
 * See get_variable_xml_node_helper() below.
 *
 * @param btp Push thisBaseType on the stack, then push it's ancestors
 * @param bt The stack
 */
void DMZ::build_basetype_chain(BaseType *btp, stack<BaseType*> &bt)
{
    auto parent = btp->get_parent();
    bt.push(btp);

    // The parent must be non-null and not the root group (the root group has no parent).
    if (parent && !(parent->type() == dods_group_c && parent->get_parent() == nullptr))
        build_basetype_chain(parent, bt);
}

xml_node DMZ::get_variable_xml_node_helper(const xml_node &/*parent_node*/, stack<BaseType*> &/*bt*/)
{
#if !USE_CACHED_XML_NODE
    // When we have an array of Structure or Sequence, both the Array and the
    // Structure BaseType are pushed on the stack. This happens because, for
    // constructors, other variables reference them as a parent node (while that's
    // not the case for the cardinal types held by an array). Here we pop the
    // Array off the stack. A better solution might be to better control what gets
    // pushed by build_basetype_chain(). jhrg 10/24/21
    if (bt.top()->type() == dods_array_c && bt.top()->var()->is_constructor_type())
        bt.pop();

    // The DMR XML stores both scalar and array variables using XML elements
    // named for the cardinal type. For an array, that is the type of the
    // element, so we use BaseType->var()->type_name() for an Array.
    string type_name = bt.top()->type() == dods_array_c ? bt.top()->var()->type_name(): bt.top()->type_name();
    string var_name = bt.top()->name();
    bt.pop();

    // Now look for the node with the correct element type and matching name
    for (auto node = parent_node.child(type_name.c_str()); node; node = node.next_sibling()) {
        for (xml_attribute attr = node.first_attribute(); attr; attr = attr.next_attribute()) {
            if (is_eq(attr.name(), "name") && is_eq(attr.value(), var_name.c_str())) {
                // if this is the last BaseType on the stack, return the node
                if (bt.empty())
                    return node;
                else
                    return get_variable_xml_node_helper(node, bt);
            }
        }
    }

    return xml_node();      // return an empty node
#else
    return xml_node();      // return an empty node
#endif
}

/**
 * @brief For a given variable find its corresponding xml_node
 * @param btp The variable (nominally, a child node from the DMR
 * that corresponds to the DMR++ XML document this class manages).
 * @return The xml_node pointer
 */
xml_node DMZ::get_variable_xml_node(BaseType *btp)
{
#if USE_CACHED_XML_NODE
    auto node = dc(btp)->get_xml_node();
    if (node == nullptr)
        throw BESInternalError(string("The xml_node for '").append(btp->name()).append("' was not recorded."), __FILE__, __LINE__);

    return node;
#else
    // load the BaseType objects onto a stack, since we start at the leaf and
    // go backward using its 'parent' pointer, the order of BaseTypes on the
    // stack will match the order in the hierarchy of the DOM tree.
    stack<BaseType*> bt;
    build_basetype_chain(btp, bt);

    xml_node dataset = d_xml_doc.first_child();
    if (!dataset || !is_eq(dataset.name(), "Dataset"))
        throw BESInternalError("No DMR++ has been parsed.", __FILE__, __LINE__);

    auto node = get_variable_xml_node_helper(dataset, bt);
    return node;
#endif
}

/// @name Attributes
/// These are functions specific to loading attributes. Originally intended to
/// be part of a lazy-load scheme, this code is used to load attribute data into
/// a 'thin DMR.' jhrg 11/23/21
/// @{

/**
 * @brief Load the DAP attributes from the DMR++ XML for a variable
 *
 * This method assumes the DMR++ XML has already been parsed and that
 * the BaseType* points to a variable defined in that XML.
 *
 * Use build_thin_dmr() to load the variable information in a DNR, then
 * use D4Group::find_var()() to get a BaseType* to a particular variable.
 *
 * @param btp The variable
 */
void
DMZ::load_attributes(BaseType *btp)
{
    if (dc(btp)->get_attributes_loaded())
        return;

    load_attributes(btp, get_variable_xml_node(btp));

    // TODO Remove redundant
    dc(btp)->set_attributes_loaded(true);

    switch (btp->type()) {
        // When we load attributes for an Array, the set_send_p() method
        // is called for its 'template' variable, but that call fails (and
        // the attributes are already loaded). This block marks the attributes
        // as loaded so the 'var_node == nullptr' exception above does not
        // get thrown. Maybe a better fix would be to mark 'child variables'
        // as having their attributes loaded. jhrg 11/16/21
        case dods_array_c: {
            dc(btp->var())->set_attributes_loaded(true);
            break;
        }

        // FIXME There are no tests for this code. The above block for Array
        //  was needed, so it seems likely that this will be too, but ...
        //  jhrg 11/16/21
        case dods_structure_c:
        case dods_sequence_c:
        case dods_grid_c: {
            auto *c = dynamic_cast<Constructor*>(btp);
            if (c) {
                for (auto i = c->var_begin(), e = c->var_end(); i != e; i++) {
                    dc(btp->var())->set_attributes_loaded(true);
                }
                break;
            }
        }

        default:
            break;
    }
}

/**
 * @brief Skip looking for the location in the DOM tree of the DAP Attributes
 * @param btp
 * @param var_node
 */
void
DMZ::load_attributes(BaseType *btp, xml_node var_node) const
{
    if (dc(btp)->get_attributes_loaded())
        return;
    
    // Attributes for this node will be held in the var_node siblings.
    // NB: Make an explict call to the BaseType implementation in case
    // the attributes() method is specialized for this DMR++ code to
    // trigger a lazy-load of the variables' attributes. jhrg 10/24/21
    // Could also use BaseType::set_attributes(). jhrg
    auto attributes = btp->BaseType::attributes();
    for (auto child = var_node.first_child(); child; child = child.next_sibling()) {
        if (is_eq(child.name(), "Attribute")) {
            process_attribute(attributes, child);
        }
    }

    dc(btp)->set_attributes_loaded(true);
}

/**
 * @brief
 * @param constructor
 */
void
DMZ::load_attributes(Constructor *constructor)
{
    load_attributes(constructor,  get_variable_xml_node(constructor));
    for (auto i = constructor->var_begin(), e = constructor->var_end(); i != e; ++i) {
        // Groups are not allowed inside a Constructor
        assert((*i)->type() != dods_group_c);
        load_attributes(*i);
    }
}

void
DMZ::load_attributes(D4Group *group) {
    // The root group is special; look for its DAP Attributes in the Dataset element
    if (group->get_parent() == nullptr) {
        xml_node dataset = d_xml_doc.child("Dataset");
        if (!dataset)
            throw BESInternalError("Could not find the 'Dataset' element in the DMR++ XML document.", __FILE__, __LINE__);
        load_attributes(group, dataset);
    }
    else {
        load_attributes(group, get_variable_xml_node(group));
    }

    for (auto i = group->var_begin(), e = group->var_end(); i != e; ++i) {
        // Even though is_constructor_type() returns true for instances of D4Group,
        // Groups are kept under a separate container from variables because they
        // have a different function than the Structure and Sequence types (Groups
        // never hold data).
        assert((*i)->type() != dods_group_c);
        load_attributes(*i);
    }

    for (auto i = group->grp_begin(), e = group->grp_end(); i != e; ++i) {
        load_attributes(*i);
    }
}

void DMZ::load_all_attributes(libdap::DMR *dmr)
{
    assert(d_xml_doc != nullptr);
    load_attributes(dmr->root());
}

/// @}

/// @name Chunks
/// These are the functions specific to loading chunk data. They implement
/// the lazy-load feature for DMR++ chunk information.
/// @{

/**
 * @brief Process HDF5 COMPACT data
 * HDF5 stores some data - smaller amounts of information - locally to save time
 * given that the chunking scheme(s) can have too much overhead to be practical
 * for small variables. This code processes those DMR++ XML elements and loads
 * the data into the BaseType variable. Effectively, this reads the data.
 * @param btp The BaseType that will hold the data values
 * @param compact The location in the DMR++ of the Base64 encoded values
 */
void
DMZ::process_compact(BaseType *btp, const xml_node &compact)
{
    dc(btp)->set_compact(true);

    auto char_data = compact.child_value();
    if (!char_data)
        throw BESInternalError("The dmrpp::compact is missing data values.",__FILE__,__LINE__);

    std::vector <u_int8_t> decoded = base64::Base64::decode(char_data);

    // Current not support structure, sequence and grid for compact storage.
    if (btp->type()== dods_structure_c || btp->type() == dods_sequence_c || btp->type() == dods_grid_c) 
        throw BESInternalError("The dmrpp::compact element must be the child of an array or a scalar variable", __FILE__, __LINE__);

    // Obtain the datatype for array and scalar.
    Type dtype =btp->type();
    if (dtype == dods_array_c)
        dtype = btp->var()->type();

    switch (dtype) {
        case dods_array_c:
            throw BESInternalError("DMR++ document fail: An Array may not be the template for an Array.", __FILE__, __LINE__);

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
        case dods_float64_c:
            btp->val2buf(reinterpret_cast<void *>(decoded.data()));
            btp->set_read_p(true);
            break;

        case dods_str_c:
        case dods_url_c: {
         
            std::string str(decoded.begin(), decoded.end());
            if (btp->type() == dods_array_c) {
                auto *st = static_cast<DmrppArray *>(btp);
                // Although val2buf() takes a void*, for DAP Str and Url types, it casts
                // that to std::string*. jhrg 11/4/21
                st->val2buf(&str);
                st->set_read_p(true);
            }
            else {// Scalar
                if(btp->type() == dods_str_c) {
                    auto *st = static_cast<DmrppStr *>(btp);
                    st->val2buf(&str);
                    st->set_read_p(true);
                }
                else {
                    auto *st = static_cast<DmrppUrl *>(btp);
                    st->val2buf(&str);
                    st->set_read_p(true);
                }
                
            }
            break;
        }

        default:
            throw BESInternalError("Unsupported COMPACT storage variable type in the drmpp handler.", __FILE__, __LINE__);
    }
}

/**
 * @brief Parse a chunk node
 * There are several different forms a chunk node can take and this handles
 * all of them.
 * @param dc
 * @param chunk
 */
void DMZ::process_chunk(DmrppCommon *dc, const xml_node &chunk) const
{
    string href;
    string trust;
    string offset;
    string size;
    string chunk_position_in_array;
    bool href_trusted = false;

    for (xml_attribute attr = chunk.first_attribute(); attr; attr = attr.next_attribute()) {
        if (is_eq(attr.name(), "href")) {
            href = attr.value();
        }
        else if (is_eq(attr.name(), "trust") || is_eq(attr.name(), "dmrpp:trust")) {
            href_trusted = is_eq(attr.value(), "true");
        }
        else if (is_eq(attr.name(), "offset")) {
            offset = attr.value();
        }
        else if (is_eq(attr.name(), "nBytes")) {
            size = attr.value();
        }
        else if (is_eq(attr.name(), "chunkPositionInArray")) {
            chunk_position_in_array = attr.value();
        }
    }

    if (offset.empty() || size.empty())
        throw BESInternalError("Both size and offset are required for a chunk node.", __FILE__, __LINE__);

    if (!href.empty()) {
        shared_ptr<http::url> data_url(new http::url(href, href_trusted));
        dc->add_chunk(data_url, dc->get_byte_order(), stoull(size), stoull(offset), chunk_position_in_array);
    }
    else {
        dc->add_chunk(d_dataset_elem_href, dc->get_byte_order(), stoull(size), stoull(offset), chunk_position_in_array);
    }
}

/**
 * @brief find the first chunkDimensionSizes node and use its value
 * This method ignores any 'extra' chunkDimensionSizes nodes.
 * @param dc
 * @param chunks
 */
void DMZ::process_cds_node(DmrppCommon *dc, const xml_node &chunks)
{
    for (auto child = chunks.child("dmrpp:chunkDimensionSizes"); child; child = child.next_sibling()) {
        if (is_eq(child.name(), "dmrpp:chunkDimensionSizes")) {
            string sizes = child.child_value();
            dc->parse_chunk_dimension_sizes(sizes);
        }
    }
}

static void add_fill_value_information(DmrppCommon *dc, const string &value_string, libdap::Type fv_type)
{
    dc->set_fill_value_string(value_string);
    dc->set_fill_value_type(fv_type);
    dc->set_uses_fill_value(true);
 }

// a 'dmrpp:chunks' node has a chunkDimensionSizes node and then one or more chunks
// nodes, and they have to be in that order.
void DMZ::process_chunks(BaseType *btp, const xml_node &chunks) const
{
    
    bool has_fill_value = false;
    for (xml_attribute attr = chunks.first_attribute(); attr; attr = attr.next_attribute()) {
        if (is_eq(attr.name(), "compressionType")) {
            dc(btp)->set_filter(attr.value());
        }
        else if (is_eq(attr.name(), "fillValue")) {

            has_fill_value = true;

            // Fill values are only supported for Arrays and scalar numeric datatypes (7/12/22)
            if (btp->type()==dods_url_c || btp->type()== dods_structure_c 
               || btp->type() == dods_sequence_c || btp->type() == dods_grid_c)
                throw BESInternalError("Fill Value chunks are only supported for Arrays and numeric datatypes.", __FILE__, __LINE__);

            // Here we should not add fill value for string since this is not handled. 
            // Why not issuing an error since the string fill value is assigned as "unsupported-string" in the get_value_as_string()
            // in build_dmrpp_util.cc.  HDF5 string variables rarely contain any fill value even if fill value is set.
            // So to avoid the error of processing string variables, we just ignore the string fillvalues.
            if (btp->type() !=dods_str_c) { 
               if (btp->type() == dods_array_c) {
                   auto array = dynamic_cast<libdap::Array*>(btp);
                   add_fill_value_information(dc(btp), attr.value(), array->var()->type());
               }
               else 
                   add_fill_value_information(dc(btp), attr.value(), btp->type());
            }
        }
        else if (is_eq(attr.name(), "byteOrder")) 
            dc(btp)->ingest_byte_order(attr.value());
    }

    // reset one_chunk_fillvalue to false if has_fill_value = false
    if (has_fill_value == false && dc(btp)->get_one_chunk_fill_value() == true) // reset fillvalue 
        dc(btp)->set_one_chunk_fill_value(false);

    // Look for the chunksDimensionSizes element - it will not be present for contiguous data
    process_cds_node(dc(btp), chunks);

    // Chunks for this node will be held in the var_node siblings.
    for (auto chunk = chunks.child("dmrpp:chunk"); chunk; chunk = chunk.next_sibling()) {
        if (is_eq(chunk.name(), "dmrpp:chunk")) {
            process_chunk(dc(btp), chunk);
        }
    }
}

/**
 * @brief Get a vector describing the shape and size of an Array
 * @param array The Array
 * @return A vector of integers that holds the size of each dimension of the Array
 */
vector<unsigned long long> DMZ::get_array_dims(Array *array)
{
    vector<unsigned long long> array_dim_sizes;
    for (auto i= array->dim_begin(), e = array->dim_end(); i != e; ++i) {
        array_dim_sizes.push_back(array->dimension_size(i));
    }

    return array_dim_sizes;
}

/**
 * @brief Compute the number of chunks assuming none are elided using fill values
 * Note that it's possible to use a chunk size that does not divide the array
 * dimensions evenly. For example, if an array (1D) has 40,000 elements and a
 * chunk size of 9501, then there are 5 'logical chunks,' 4 with 9501 elements and
 * one with the remainder (1966). To find this number this method uses ceil().
 * @param array_dim_sizes A vector of the array dimension sizes
 * @param dc A pointer to the DmrppCommon information
 * @return The number of logical chunks.
 */
size_t DMZ::logical_chunks(const vector <unsigned long long> &array_dim_sizes, const DmrppCommon *dc)
{
    auto const& chunk_dim_sizes = dc->get_chunk_dimension_sizes();
    if (chunk_dim_sizes.size() != array_dim_sizes.size()) {
        ostringstream oss;
        oss << "Expected the chunk and array rank to match (chunk: " << chunk_dim_sizes.size() << ", array: "
            << array_dim_sizes.size() << ")";
        throw BESInternalError(oss.str(), __FILE__, __LINE__);
    }

    size_t num_logical_chunks = 1;
    auto i = array_dim_sizes.begin();
    for (auto chunk_dim_size: chunk_dim_sizes) {
        auto array_dim_size = *i++;
        num_logical_chunks *= (size_t)ceil((float)array_dim_size / (float)chunk_dim_size);
    }

    return num_logical_chunks;
}

/**
 * @brief Get a set<> of vectors that describe this variables actual chunks
 * In an HDF5 file, each chunked variable may have some chunks that hold only
 * fill values. As an optimization, HDF5 does not waste space in the data file
 * by writing those values to the file. This code builds a set< vector<> > that
 * describes the indices of each chunk in a variable.
 *
 * The 'chunk map' can be used, along with an exhaustive enumeration of all _possible_
 * chunks, to find those 'fill value chunks.'
 *
 * @param chunks The list of chunk objects parsed from the DMR++ for this variable
 * @return A set where each element is a vector of chunk indices. The number of
 * elements in the set should equal the number of chunks in the _chunks_ parameter.
 */
set< vector<unsigned long long> > DMZ::get_chunk_map(const vector<shared_ptr<Chunk>> &chunks)
{
    set< vector<unsigned long long> > chunk_map;
    for (auto const &chunk: chunks) {
        chunk_map.insert(chunk->get_position_in_array());
    }

    return chunk_map;
}

/**
 * @brief Add missing chunks as 'fill value chunks'
 * @param dc
 * @param chunk_map A set<> with one element for each chunk
 * @param chunk_shape The shape of a chunk for this array
 * @param array_shape The shape of the array
 * @param chunk_size the number of bytes in the chunk
 */
void DMZ::process_fill_value_chunks(DmrppCommon *dc, const set<shape> &chunk_map, const shape &chunk_shape,
                                    const shape &array_shape, unsigned long long chunk_size)
{
    // Use an Odometer to walk over each potential chunk
    DmrppChunkOdometer odometer(array_shape, chunk_shape);
    do {
        const auto &s = odometer.indices();
        if (chunk_map.find(s) == chunk_map.end()) {
            // Fill Value chunk
            // what we need byte order, pia, fill value
            dc->add_chunk(dc->get_byte_order(), dc->get_fill_value(), dc->get_fill_value_type(), chunk_size, s);
        }
    } while (odometer.next());
}

/**
 * @brief Load the chunk information into a variable
 *
 * Process the chunks, chunk, etc., elements from the DMR++ information. Once this is called,
 * the variable's read() method should be able to read the data for this variable.
 *
 * @param btp The variable
 */
void DMZ::load_chunks(BaseType *btp)
{
    if (dc(btp)->get_chunks_loaded())
        return;

    // goto the DOM tree node for this variable
    xml_node var_node = get_variable_xml_node(btp);
    if (var_node == nullptr)
        throw BESInternalError("Could not find location of variable in the DMR++ XML document.", __FILE__, __LINE__);

    // Chunks for this node will be held in the var_node siblings. For a given BaseType, there should
    // be only one chunks node xor one chunk node.
    int chunks_found = 0;
    int chunk_found = 0;
    int compact_found = 0;

    // Chunked data
    auto child = var_node.child("dmrpp:chunks");
    if (child) {
        chunks_found = 1;
        process_chunks(btp, child);
        auto array = dynamic_cast<Array*>(btp);
        // It's possible to have a chunk, but not have a chunk dimension sizes element
        // when there is only one chunk (e.g., with HDF5 Contiguous storage). jhrg 5/5/22
        if (array && !dc(btp)->get_chunk_dimension_sizes().empty()) {
            auto const &array_shape = get_array_dims(array);
            size_t num_logical_chunks = logical_chunks(array_shape, dc(btp));
            // do we need to run this code?
            if (num_logical_chunks != dc(btp)->get_chunks_size()) {
                auto const &chunk_map = get_chunk_map(dc(btp)->get_immutable_chunks());
                // Since the variable has some chunks that hold only fill values, add those chunks
                // to the vector of chunks.
                auto const &chunk_shape = dc(btp)->get_chunk_dimension_sizes();
                unsigned long long chunk_size_bytes = array->var()->width(); // start with the element size in bytes
                for (auto dim_size: chunk_shape)
                    chunk_size_bytes *= dim_size;
                process_fill_value_chunks(dc(btp), chunk_map, dc(btp)->get_chunk_dimension_sizes(),
                                          array_shape, chunk_size_bytes);
                // Now we need to check if this var only contains one chunk.
                // If yes, we will go ahead to set one_chunk_fill_value be true. 
                // While later in process_chunks(), we will check if fillValue is defined and adjust the value.
                if (num_logical_chunks == 1) 
                    dc(btp)->set_one_chunk_fill_value(true);

                
            }
        }
        // If both chunks and chunk_dimension_sizes are empty, this is contiguous storage
        // with nothing but fill values. Make a single chunk that can hold the fill values.
        else if (array && dc(btp)->get_immutable_chunks().empty()) {
            auto const &array_shape = get_array_dims(array);
            // Since there is one chunk, the chunk size and array size are one and the same.
            unsigned long long array_size_bytes = 1;
            for (auto dim_size: array_shape)
                array_size_bytes *= dim_size;
            // array size above is in _elements_, multiply by the element width to get bytes
            array_size_bytes *= array->var()->width();
            // Position in array is 0, 0, ..., 0 were the number of zeros is the number of array dimensions
            shape pia(0,array_shape.size());
            auto dcp = dc(btp);
            dcp->add_chunk(dcp->get_byte_order(), dcp->get_fill_value(), dcp->get_fill_value_type(), array_size_bytes, pia);
        }
        // This is the case when the scalar variable that holds the fill value with the contiguous storage comes. 
        // Note we only support numeric datatype now. KY 2022-07-12
        else if (btp->type()!=dods_array_c && dc(btp)->get_immutable_chunks().empty()) {
            if (btp->type() == dods_grid_c || btp->type() == dods_sequence_c || btp->type() ==dods_url_c) { 
                ostringstream oss;
                oss << " For scalar variable with the contiguous storage that holds the fillvalue, only numeric" 
                    << " types are supported.";
                throw BESInternalError(oss.str(), __FILE__, __LINE__);
            }
            shape pia;
            auto dcp = dc(btp);
            dcp->add_chunk(dcp->get_byte_order(), dcp->get_fill_value(), dcp->get_fill_value_type(), btp->width(), pia);
        }
    }

    // Contiguous data
    auto chunk = var_node.child("dmrpp:chunk");
    if (chunk) {
        chunk_found = 1;
        process_chunk(dc(btp), chunk);
    }

    auto compact = var_node.child("dmrpp:compact");
    if (compact) {
        compact_found = 1;
        process_compact(btp, compact);
    }

    // Here we (optionally) check that exactly one of the three types of node was found
    if (DmrppRequestHandler::d_require_chunks) {
        int elements_found = chunks_found + chunk_found + compact_found;
        if (elements_found != 1) {
            ostringstream oss;
            oss << "Expected chunk, chunks or compact information in the DMR++ data. Found " << elements_found
                << " types of nodes.";
            throw BESInternalError(oss.str(), __FILE__, __LINE__);
        }
    }

    dc(btp)->set_chunks_loaded(true);
}

/// @}

} // namespace dmrpp
