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

// NB: If we use this for string compares, we cannot use the 'fastest' parsing option
// of rapidxml. For that, we will need to modify this so that the length of 's1' is passed
// in (names, etc., are not null terminated with the fastest parsing option). jhrg 10/20/21
static inline bool is_eq(const char *value, const char *key)
{
    return strcmp(value, key) == 0;
}

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
 *
 * Given the root node of a DOM tree for the DMR++ XML document, populate the
 * libdap a DMR instance with information from that Dataset node. This sets the
 * stage for the parse of rest of the information in the DOM tree.
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

/**
 * @brief Parse information from a Dimension element
 * @param grp
 * @param dimension_node
 */
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
 * @brief Process a Dim element and add it to the given Array
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

/// Are any of the child nodes 'Dim' elements?
static inline bool has_dim_nodes(xml_node<> *var_node)
{
    for (auto *child = var_node->first_node(); child; child = child->next_sibling()) {
        if (is_eq(child->name(), "Dim"))    // just one is enough
            return true;
    }

    return false;
}

/// Simple set membership; used to test for variable elements
static inline bool member_of(const set<string> &elements_set, const string &element_name)
{
    return elements_set.find(element_name) != elements_set.end();
}

/**
 * @brief Process an element that is a variable
 *
 * This method processes both scalar and array variables, but not Structures
 * or Sequences or Groups - those are handled separately. This method does
 * process all of the elements contained by the scalar or array variable element,
 * however.
 *
 * @note Only one of group or parent can be non-null
 *
 * @param dmr
 * @param group If not null, add the new variable to this group
 * @param parent If not null add this new variable to this constructor
 * @param var_node
 */
void DMZ::process_variable(DMR *dmr, D4Group *group, Constructor *parent, xml_node<> *var_node)
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
            // for each of var_node's child nodes, call process_variable with group null and parent set
            // NB: For an array of a Constructor, add children to the Constructor, not the array
            auto parent = dynamic_cast<Constructor*>(btp->var());
            assert(parent);
            for (auto *child = var_node->first_node(); child; child = child->next_sibling()) {
                if (member_of(variable_elements, child->name()))
                    process_variable(dmr, group, parent, child);
            }
        }
    }
    else {
        btp = add_scalar_variable(dmr, group, parent, t, var_node);
        if (t == dods_structure_c || t == dods_sequence_c) {
            assert(btp->type() == t);
            // for each of var_node's child nodes, call process_variable with group null and parent set
            auto parent = dynamic_cast<Constructor*>(btp);
            assert(parent);
            for (auto *child = var_node->first_node(); child; child = child->next_sibling()) {
                if (member_of(variable_elements, child->name()))
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
BaseType *DMZ::build_variable(DMR *dmr, D4Group *group, Type t, xml_node<> *var_node)
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
BaseType *DMZ::add_scalar_variable(DMR *dmr, D4Group *group, Constructor *parent, Type t, xml_node<> *var_node)
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

BaseType *DMZ::add_array_variable(DMR *dmr, D4Group *group, Constructor *parent, Type t, xml_node<> *var_node)
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
    for (auto *child = var_node->first_node(); child; child = child->next_sibling()) {
        if (is_eq(child->name(), "Dim")) {
            process_dim(dmr, group, array, child);
        }
    }

    if (parent)
        parent->add_var_nocopy(array);
    else
        group->add_var_nocopy(array);

    return array;
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
            process_variable(dmr, new_group, nullptr, child);
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
            process_variable(dmr, root_group, nullptr, child);
        }
    }
}

} // namespace dmrpp
