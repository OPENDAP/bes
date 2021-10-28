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

#include <cstring>

#include <libdap/BaseType.h>
#include <libdap/Array.h>
#include <libdap/Type.h>
#include <libdap/D4Dimensions.h>
#include <libdap/D4Group.h>
#include <libdap/D4BaseTypeFactory.h>
#include <libdap/D4Enum.h>
#include <libdap/D4EnumDefs.h>
#include <libdap/D4Dimensions.h>
#include <libdap/D4Attributes.h>
#include <libdap/DMR.h>
#include <libdap/util.h>        // is_simple_type()

#include "url_impl.h"           // see bes/http
#include "DMZ.h"                // this includes the pugixml header
#include "DmrppCommon.h"
#include "BESInternalError.h"
#include "BESDebug.h"

using namespace pugi;
using namespace std;
using namespace libdap;

#define PARSER "dmz"
#define prolog std::string("DMZ::").append(__func__).append("() - ")

namespace dmrpp {

const std::set<std::string> variable_elements{"Byte", "Int8", "Int16", "Int32", "Int64", "UInt8", "UInt16", "UInt32",
                                              "UInt64", "Float32", "Float64", "String", "Structure", "Sequence",
                                              "Enum", "Opaque"};

static const string dmrpp_namespace = "http://xml.opendap.org/dap/dmrpp/1.0.0#";

/**
 * @brief Build a DMZ object and initialize it using a DMR++ XML document
 * @param file_name The DMR++ XML document to parse.
 * @exception BESInternalError if file_name cannot be parsed
 */
DMZ::DMZ(const string &file_name)
{
#if 0
    ifstream ifs(file_name, ios::in | ios::binary | ios::ate);
    if (!ifs)
        throw BESInternalError(string("Could not open DMR++ XML file: ").append(file_name), __FILE__, __LINE__);

    ifstream::pos_type file_size = ifs.tellg();
    if (file_size == 0)
        throw BESInternalError(string("DMR++ XML file is empty: ").append(file_name), __FILE__, __LINE__);

    ifs.seekg(0, ios::beg);

    d_xml_text.resize(file_size + ifstream::pos_type(1LL));   // Add space for text and null termination
    ifs.read(d_xml_text.data(), file_size);
    if (!ifs)
        throw BESInternalError(string("DMR++ XML file seek or read failure: ").append(file_name), __FILE__, __LINE__);

    d_xml_text[file_size] = '\0';

    // TODO catch  rapidxml::parse_error
    //d_xml_doc.parse<0>(d_xml_text.data());    // 0 means default parse flags
    d_xml_doc.load_buffer_inplace(d_xml_text.data());
#endif
    std::ifstream stream(file_name);
    pugi::xml_parse_result result = d_xml_doc.load(stream);

    if (!d_xml_doc.document_element())
        throw BESInternalError("No DMR++ data present.", __FILE__, __LINE__);
}

// NB: If we use this for string compares, we cannot use the 'fastest' parsing option
// of rapidxml. For that, we will need to modify this so that the length of 's1' is passed
// in (names, etc., are not null terminated with the fastest parsing option). jhrg 10/20/21
static inline bool is_eq(const char *value, const char *key)
{
    return strcmp(value, key) == 0;
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
void DMZ::process_dataset(DMR *dmr, xml_node *xml_root)
{
    // Process the attributes
    int required_attrs_found = 0;   // there are 1
    string href_attr;
    bool href_trusted = false;
    for (xml_attribute attr = xml_root->first_attribute(); attr; attr = attr.next_attribute()) {
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
        // TODO Namespaces? jhrg 10/2/0/21
        else if (is_eq(attr.name(), "xmlns")) {
            dmr->set_namespace(attr.value());
        }
        // TODO what to do with namespaced attributes? jhrg 10/20/21
        else if (is_eq(attr.name(), "dmrpp:href")) {
            href_attr = attr.value();
        }
        else if (is_eq(attr.name(), "trust")) {
            href_trusted = is_eq(attr.value(), "true");
        }
        // We allow other, non recognized attributes, so there is no 'else' jhrg 10/20/21
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
void DMZ::process_dimension(D4Group *grp, xml_node *dimension_node)
{
    string name_value;
    string size_value;
    for (xml_attribute attr = dimension_node->first_attribute(); attr; attr = attr.next_attribute()) {
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
void DMZ::process_dim(DMR *dmr, D4Group *grp, Array *array, xml_node *dim_node)
{
    assert(array->is_vector_type());

    string name_value;
    string size_value;
    for (xml_attribute attr = dim_node->first_attribute(); attr; attr = attr.next_attribute()) {
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

/// @brief Are any of the child nodes 'Dim' elements?
static inline bool has_dim_nodes(xml_node *var_node)
{
    return  var_node->child("Dim"); // just one is enough
#if 0
    for (auto child = var_node->child("Dim"); child; child = child.next_sibling()) {
        if (is_eq(child.name(), "Dim"))    // just one is enough
            return true;
    }

    return false;
#endif
}

/// @brief Simple set membership; used to test for variable elements, et cetera.
static inline bool member_of(const set<string> &elements_set, const string &element_name)
{
    return elements_set.find(element_name) != elements_set.end();
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
void DMZ::process_variable(DMR *dmr, D4Group *group, Constructor *parent, xml_node *var_node)
{
    assert(group);

    // Variables are declared using nodes with type names (e.g., <Float32...>)
    // Variables are arrays if they have one or more <Dim...> child nodes.
    Type t = get_type(var_node->name());

    assert(t != dods_group_c);  // Groups are special and handled elsewhere

    bool is_array_type = has_dim_nodes(var_node);
    BaseType *btp;
    if (is_array_type) {
        btp = add_array_variable(dmr, group, parent, t, var_node);
        if (t == dods_structure_c || t == dods_sequence_c) {
            assert(btp->type() == dods_array_c && btp->var()->type() == t);
            // NB: For an array of a Constructor, add children to the Constructor, not the array
            auto parent = dynamic_cast<Constructor*>(btp->var());
            assert(parent);
            for (auto child = var_node->first_child(); child; child = child.next_sibling()) {
                if (member_of(variable_elements, child.name()))
                    process_variable(dmr, group, parent, child);
            }
        }
    }
    else {
        btp = add_scalar_variable(dmr, group, parent, t, var_node);
        if (t == dods_structure_c || t == dods_sequence_c) {
            assert(btp->type() == t);
            auto parent = dynamic_cast<Constructor*>(btp);
            assert(parent);
            for (auto child = var_node->first_child(); child; child = child.next_sibling()) {
                if (member_of(variable_elements, child.name()))
                    process_variable(dmr, group, parent, child);
            }
        }
    }
}

/**
 * @brief helper code to build a BaseType that may wind up a scalar or an array
 * @param dmr
 * @param group If this is an enum, look in this group for am enum definition with a relative path
 * @param t What DAP type is the new variable?
 * @param var_node
 */
BaseType *DMZ::build_variable(DMR *dmr, D4Group *group, Type t, xml_node *var_node)
{
    assert(dmr->factory());

    string name_value;
    string enum_value;
    for (xml_attribute attr = var_node->first_attribute(); attr; attr = attr.next_attribute()) {
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

        D4EnumDef *enum_def = nullptr;
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
 * create the variable.
 * @param dmr
 * @param group If parent is null, add the new var to this group
 * @param parent If non-null, add the new var to this constructor
 * @param t What DAP type is the variable?
 * @param var_node
 * @return The new variable
 */
BaseType *DMZ::add_scalar_variable(DMR *dmr, D4Group *group, Constructor *parent, Type t, xml_node *var_node)
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

BaseType *DMZ::add_array_variable(DMR *dmr, D4Group *group, Constructor *parent, Type t, xml_node *var_node)
{
    assert(group);

    BaseType *btp = build_variable(dmr, group, t, var_node);

    // Transform the scalar to an array
    Array *array = static_cast<Array*>(dmr->factory()->NewVariable(dods_array_c, btp->name()));
    array->set_is_dap4(true);
    array->add_var_nocopy(btp);

    // The SAX parser set up the parse of attributes here. For the thin DMR, we won't
    // parse those from the DMR now. jhrg 10/21/21

    // Now grab the dimension elements
    for (auto child = var_node->first_child(); child; child = child.next_sibling()) {
        if (is_eq(child.name(), "Dim")) {
            process_dim(dmr, group, array, child);
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
 * Dimension, Group and variable elements.
 * @param dmr
 * @param parent
 * @param var_node
 */
void DMZ::process_group(DMR *dmr, D4Group *parent, xml_node *var_node)
{
    string name_value;
    for (xml_attribute attr = var_node->first_attribute(); attr; attr = attr.next_attribute()) {
        if (is_eq(attr.name(), "name")) {
            name_value = attr.value();
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
    for (auto child = var_node->first_child(); child; child = child.next_sibling()) {
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
    for (auto child = xml_root_node->first_child(); child; child = child.next_sibling()) {
        if (is_eq(child.name(), "Dimension")) {
            process_dimension(root_group, child);
        }
        else if (is_eq(child.name(), "Group")) {
            process_group(dmr, root_group, child);
        }
        else if (member_of(variable_elements, child.name())) {
            process_variable(dmr, root_group, nullptr, child);
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
void DMZ::process_attribute(D4Attributes *attributes, xml_node *dap_attr_node)
{
    string name_value;
    string type_value;
    for (xml_attribute attr = dap_attr_node->first_attribute(); attr; attr = attr.next_attribute()) {
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
        D4Attribute *child = new D4Attribute(name_value, attr_container_c);
        attributes->add_attribute_nocopy(child);
        // In this call, 'attributes()' will allocate the D4Attributes object
        // that will hold the container's attributes.
        process_attribute(child.attributes(), dap_attr_node->first_child());
    }
    else if (type_value == "OtherXML") {
        // TODO Add support for this
    }
    else {
        // Make the D4Attribute and add it to the D4Attributes attribute container
        D4Attribute *attribute = new D4Attribute(name_value, StringToD4AttributeType(type_value));
        attributes->add_attribute_nocopy(attribute);
        // Process one or more Value elements
        for (auto value_elem = dap_attr_node->first_child(); value_elem; value_elem = value_elem->next_sibling()) {
            if (is_eq(value_elem->name(), "Value")) {
                attribute->add_value(value_elem->value());  // returns the text of the first data node
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

xml_node *DMZ::get_variable_xml_node_helper(xml_node *parent_node, stack<BaseType*> &bt)
{
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
    for (auto node = parent_node->first_child(type_name.c_str()); node; node = node->next_sibling()) {
        for (xml_attribute attr = node->first_attribute(); attr; attr = attr.next_attribute()) {
            if (is_eq(attr.name(), "name") && is_eq(attr.value(), var_name.c_str())) {
                // if this is the last BaseType on the stack, return the node
                if (bt.empty())
                    return node;
                else
                    return get_variable_xml_node_helper(node, bt);
            }
        }
    }

    return nullptr;
}

/**
 * @brief For a given variable find its corresponding xml_node
 * @param btp The variable (nominally, a child node from the DMR
 * that corresponds to the DMR++ XML document this class manages).
 * @return The xml_node pointer
 */
xml_node *DMZ::get_variable_xml_node(BaseType *btp)
{
    // load the BaseType objects onto a stack, since we start at the leaf and
    // go backward using its 'parent' pointer, the order of BaseTypes on the
    // stack will match the order in the hierarchy of the DOM tree.
    stack<BaseType*> bt;
    build_basetype_chain(btp, bt);

    xml_node *dataset = d_xml_doc.first_child();
    if (!dataset || !is_eq(dataset->name(), "Dataset"))
        throw BESInternalError("No DMR++ has been parsed.", __FILE__, __LINE__);

    auto node = get_variable_xml_node_helper(dataset, bt);
    return node;
}

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
void DMZ::load_attributes(BaseType *btp)
{
    // goto the DOM tree node for this variable
    xml_node *var_node = get_variable_xml_node(btp);
    if (var_node == nullptr)
        throw BESInternalError("Could not find location of variable in the DMR++ XML document.", __FILE__, __LINE__);

    // Attributes for this node will be held in the var_node siblings.
    // NB: Make an explict call to the BaseType implementation in case
    // the attributes() method is specialized for this DMR++ code to
    // trigger a lazy-load of the variables' attributes. jhrg 10/24/21
    // Could also use BaseType::set_attributes(). jhrg
    auto attributes = btp->BaseType::attributes(); // new D4Attributes();
    for (auto child = var_node->first_child(); child; child = child.next_sibling()) {
        if (is_eq(child.name(), "Attribute")) {
            process_attribute(attributes, child);
        }
    }
}

/**
 * @brief Parse a chunk node
 * There are several different forms a chunk node can take and this handles
 * all of them.
 * @param dc
 * @param chunk
 */
void DMZ::process_chunk(DmrppCommon *dc, xml_node *chunk)
{
    string href;
    string trust;
    string offset;
    string size;
    string chunk_position_in_array;

    bool href_trusted = false;

    for (xml_attribute attr = chunk->first_attribute(); attr; attr = attr.next_attribute()) {
        if (is_eq(attr.name(), "href")) {
            href = attr.value();
        }
        else if (is_eq(attr.name(), "trust")) {
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
        dc->add_chunk(data_url, dc->get_byte_order(), stoi(size), stoi(offset), chunk_position_in_array);
    }
    else {
        dc->add_chunk(d_dataset_elem_href, dc->get_byte_order(), stoi(size), stoi(offset), chunk_position_in_array);
    }
}

/**
 * @brief find the first chunkDimensionSizes node and use its value
 * This method ignores any 'extra' chunkDimensionSizes nodes.
 * @param dc
 * @param chunks
 */
void DMZ::process_cds_node(DmrppCommon *dc, xml_node *chunks)
{
    bool cds_found = false;
    for (auto child = chunks->first_child("dmrpp:chunkDimensionSizes"); child && !cds_found; child = child.next_sibling()) {
        if (is_eq(child.name(), "dmrpp:chunkDimensionSizes")) {
            string sizes = child.value();
            dc->parse_chunk_dimension_sizes(sizes);
            cds_found = true;
        }
    }

    if (!cds_found)
        throw BESInternalError("Could not find a chunkDimensionSizes node.", __FILE__, __LINE__);
}

// a 'dmrpp:chunks' node has a chunkDimensionSizes node and then one or more chunks
// nodes, and they have to be in that order.
void DMZ::process_chunks(BaseType *btp, xml_node *chunks)
{
    auto *dc = dynamic_cast<DmrppCommon*>(btp);   // Get the Dmrpp common info
    if (!dc)
        throw BESInternalError("Could not cast BaseType to DmrppType in the DMR++ handler.", __FILE__, __LINE__);

    process_cds_node(dc, chunks);

    // Chunks for this node will be held in the var_node siblings.
    for (auto *chunk = chunks->first_child("dmrpp:chunk"); chunk; chunk = chunk->next_sibling()) {
        if (is_eq(chunk->name(), "dmrpp:chunk")) {
            process_chunk(dc, chunk);
        }
    }
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
    // goto the DOM tree node for this variable
    xml_node *var_node = get_variable_xml_node(btp);
    if (var_node == nullptr)
        throw BESInternalError("Could not find location of variable in the DMR++ XML document.", __FILE__, __LINE__);

    // Chunks for this node will be held in the var_node siblings. For a given BaseType, there should
    // be only one chunks node xor one chunk node.
    int chunks_found = 0;
    int chunk_found = 0;
    for (auto child = var_node->first_child("dmrpp:chunks"); child; child = child.next_sibling()) {
        if (is_eq(child.name(), "dmrpp:chunks")) {
            chunks_found++;
            process_chunks(btp, child);
        }
    }

    for (auto *chunk = var_node->first_child("dmrpp:chunk"); chunk; chunk = chunk->next_sibling()) {
        if (is_eq(chunk->name(), "dmrpp:chunk")) {
            auto *dc = dynamic_cast<DmrppCommon*>(btp);   // Get the Dmrpp common info
            if (!dc)
                throw BESInternalError("Could not cast BaseType to DmrppCommon in the DMR++ handler.", __FILE__, __LINE__);
            chunk_found++;
            process_chunk(dc, chunk);
        }
    }

    // TODO Add support for compact. jhrg 10/25/21
    //  Compact data is stored in the XML using <dmrpp:compact> elements. These are at the same level
    //  as the <dmrpp:chunk> (and <Attribute>) elements.

    if (chunks_found > 1 || chunk_found > 1 || (chunks_found && chunk_found) || !(chunks_found || chunk_found))
        throw BESInternalError("Unsupported chunk information in the DMR++ data.", __FILE__, __LINE__);
}

} // namespace dmrpp
