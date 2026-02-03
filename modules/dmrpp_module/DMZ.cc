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
#include <unordered_set>
#include <stack>
#include <string>
#include <fstream>
#include <cstring>
#include <zlib.h>

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

#include "DmrppNames.h"

#define PUGIXML_NO_XPATH
#define PUGIXML_HEADER_ONLY
#include <pugixml.hpp>

#include "url_impl.h"           // see bes/http
#include "DMRpp.h"
#include "DMZ.h"                // this includes the pugixml header
#include "Chunk.h"
#include "DmrppCommon.h"
#include "DmrppArray.h"
#include "DmrppStructure.h"
#include "DmrppByte.h"
#include "DmrppStr.h"
#include "DmrppUrl.h"
#include "DmrppD4Group.h"
#include "Base64.h"
#include "DmrppRequestHandler.h"
#include "DmrppChunkOdometer.h"
#include "TheBESKeys.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESLog.h"
#include "vlsa_util.h"

using namespace pugi;
using namespace std;
using namespace libdap;

// The pugixml library does not grok namespaces. So, for a tag named 'dmrpp:chunks'
// if TREAT_NAMESPACES_AS_LITERALS is '1' the parser matches the whole string. If it
// is '0,' the parser only matches the characters after the colon. In both cases the
// namespace (as XML intends) is not used. Using '1' is a bit more efficient.
// jhrg 11/2/21
#define TREAT_NAMESPACES_AS_LITERALS 1

// THe code can either search for a DAP variable's information in the XML, or it can
// record that during the parse process. Set this when/if the code does the latter.
// Using this simplifies the lazy-load process, particularly for the DAP2 DDS and
// data responses (which have not yet been coded completely). jhrg 11/17/21
#define USE_CACHED_XML_NODE 1

#define SUPPORT_FILL_VALUE_CHUNKS 1

#define prolog std::string("DMZ::").append(__func__).append("() - ")

namespace dmrpp {

using shape = std::vector<unsigned long long>;

// The original unsupported fillValue flags from 4/22
constexpr static const auto UNSUPPORTED_STRING = "unsupported-string";
constexpr static const auto UNSUPPORTED_ARRAY = "unsupported-array";
constexpr static const auto UNSUPPORTED_COMPOUND = "unsupported-compound";
// Added when Arrays Of Fixed Length Strings. The unsupported-string value was dropped at that time.
constexpr static const auto UNSUPPORTED_VARIABLE_LENGTH_STRING = "unsupported-variable-length-string";

constexpr static const auto ELIDE_UNSUPPORTED_KEY = "DMRPP.Elide.Unsupported";

bool DMZ::d_elide_unsupported = true;

const std::set<std::string> DMZ::variable_elements{
    "Byte", "Int8", "Int16", "Int32", "Int64", "UInt8", "UInt16", "UInt32",
    "UInt64", "Float32", "Float64", "String", "Structure", "Sequence",
    "Enum", "Opaque"
};

/// @brief Are the C-style strings equal?
static inline bool is_eq(const char *value, const char *key) {
#if TREAT_NAMESPACES_AS_LITERALS
    return strcmp(value, key) == 0;
#else
    if (strcmp(value, key) == 0) {
        return true;
    }
    else {
        const char *colon = strchr(value, ':');
        return colon && strcmp(colon + 1, key) == 0;
    }
#endif
}

/// @brief Are any of the child nodes 'Dim' elements?
static inline bool has_dim_nodes(const xml_node &var_node) {
    return var_node.child("Dim"); // just one is enough
}

/// @brief Simple set membership; used to test for variable elements, et cetera.
static inline bool member_of(const set<string> &elements_set, const string &element_name) {
    return elements_set.find(element_name) != elements_set.end();
}

/// @brief syntactic sugar for a dynamic cast to DmrppCommon
static inline DmrppCommon *dc(BaseType *btp) {
    auto *dc = dynamic_cast<DmrppCommon *>(btp);
    if (!dc)
        throw BESInternalError(string("Expected a BaseType that was also a DmrppCommon instance (")
                               .append((btp) ? btp->name() : "unknown").append(")."), __FILE__, __LINE__);
    return dc;
}


/**
 * Loads configuration state from TheBESKeys
 *
 */
void DMZ::load_config_from_keys() {
    // ########################################################################
    // Loads the ELIDE_UNSUPPORTED_KEY (see top of file for key definition)
    // And if it's set and set to true, then we set the eliding flag to true.
    d_elide_unsupported = TheBESKeys::read_bool_key(ELIDE_UNSUPPORTED_KEY, false);
}

/**
 * @brief Build a DMZ object and initialize it using a DMR++ XML document
 * @param file_name The DMR++ XML document to parse.
 * @exception BESInternalError if file_name cannot be parsed
 */
DMZ::DMZ(const string &file_name) {
    load_config_from_keys();
    parse_xml_doc(file_name);
}

/**
 * @brief Build the DOM tree for a DMR++ XML document
 * @param file_name
 */
void
DMZ::parse_xml_doc(const string &file_name) {
    std::ifstream stream(file_name);

    // Free memory used by a previously parsed document.
    d_xml_doc.reset();

    // parse_ws_pcdata_single will include the space when it appears in a <Value> </Value>
    // DAP Attribute element. jhrg 11/3/21
    pugi::xml_parse_result result = d_xml_doc.load(stream, pugi::parse_default | pugi::parse_ws_pcdata_single);

    if (!result)
        throw BESInternalError(string("DMR++ parse error: ").append(result.description()), __FILE__, __LINE__);

    if (!d_xml_doc.document_element())
        throw BESInternalError("No DMR++ data present.", __FILE__, __LINE__);
}


/**
 *
 * @param var_node
 * @param unsupported_flag
 * @return
 */
bool flagged_as_unsupported_type(const xml_node var_node, string &unsupported_flag) {
    if (var_node == nullptr) {
        throw BESInternalError(prolog + "Received null valued xml_node in the DMR++ XML document.", __FILE__, __LINE__);
    }

    // We'll start assuming it's not flagged as unsupported
    bool is_unsupported_type = false;

    // We know the unsupported flag is held in the fillValue attribute of the dmrpp:chunks element.
    auto chunks = var_node.child("dmrpp:chunks");
    if (!chunks) {
        // No dmrpp:chunks? Then no fillValue and we can be done, it's supported.
        return is_unsupported_type;
    }

    xml_attribute fillValue_attr = chunks.attribute("fillValue");
    if (!fillValue_attr) {
        // No fillValue attribute? Then we can be done, it's supported.
        return is_unsupported_type;
    }

    // We found th fillValue attribute, So now we have to deal with its various tragic values...
    if (is_eq(fillValue_attr.value(), UNSUPPORTED_STRING)) {
        // UNSUPPORTED_STRING is the older, indeterminate, tag that might label a truly
        // unsupported VariableLengthString, or it could be a labeling FixedLengthString.
        // To find out, we need to look in XML DOM to determine if this is an Array, and
        // if so, to see if it's the FixedLengthString case:
        //    <dmrpp:FixedLengthStringArray string_length ... />
        // This should be a child of var_node.

        // Start by making it unsupported and then check each of the exceptions.
        is_unsupported_type = true;

        const auto dim_node = var_node.child("Dim");
        if (!dim_node) {
            // No dims? Then this is a scalar String and it's cool.
            // We dump the BS fillValue for one that makes some sense in Stringville
            fillValue_attr.set_value("");
            is_unsupported_type = false;
        }
        else {
            // It's an array, so is it a FixedLengthStringArray??
            const auto flsa_node = var_node.child("dmrpp:FixedLengthStringArray");
            if (flsa_node) {
                // FixedLengthStringArray arrays work!
                // We dump the BS fillValue for one that makes some sense in Stringville
                fillValue_attr.set_value("");
                is_unsupported_type = false;
            }
        }
    }
    else if (is_eq(fillValue_attr.value(), UNSUPPORTED_VARIABLE_LENGTH_STRING)) {
        unsupported_flag = fillValue_attr.value();
        is_unsupported_type = true;
    }
    else if (is_eq(fillValue_attr.value(), UNSUPPORTED_ARRAY)) {
        unsupported_flag = fillValue_attr.value();
        is_unsupported_type = true;
    }
    else if (is_eq(fillValue_attr.value(), UNSUPPORTED_COMPOUND)) {
        unsupported_flag = fillValue_attr.value();
        is_unsupported_type = true;
    }

    return is_unsupported_type;
}


/**
 * @brief Build a DOM tree for a DMR++ using content from a string.
 * @note This method was added to support parsing DMR++ 'documents'
 * that are cached as strings. The caching nominally takes place in
 * the NgapContainer code over in the ngap_module (but it doesn't
 * have to). See DmrppRequestHandler.cc for places where this method
 * is called.
 * @param source The string that contains the DMR++ content.
 */
void
DMZ::parse_xml_string(const string &source) {
    pugi::xml_parse_result result = d_xml_doc.load_string(source.c_str());

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
 * stage for the parse of the rest of the information in the DOM tree.
 * @param dmr Dump the information in this instance of DMR
 * @param xml_root The root node of the DOM tree
 */
void DMZ::process_dataset(DMR *dmr, const xml_node &xml_root) {
    // Process the attributes
    int required_attrs_found = 0; // there are 1
    string href_attr;
    bool href_trusted = false;
    string dmrpp_version; // empty or holds a value if dmrpp::version is present
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
        // We allow other, unrecognized attributes, so there is no 'else' jhrg 10/20/21
    }

    if (dmrpp_version.empty()) {
        // old style DMR++, set enable-kludge flag
        DmrppRequestHandler::d_emulate_original_filter_order_behavior = true;
    }
    else {
        auto dmrpp = dynamic_cast<DMRpp *>(dmr);
        if (dmrpp) {
            dmrpp->set_version(dmrpp_version);
        }
    }

    if (required_attrs_found != 1)
        throw BESInternalError("DMR++ XML dataset element missing one or more required attributes.", __FILE__,
                               __LINE__);

    if (href_attr.empty())
        throw BESInternalError("DMR++ XML dataset element dmrpp:href is missing. ", __FILE__, __LINE__);

    d_dataset_elem_href.reset(new http::url(href_attr, href_trusted));
}

/**
 * @brief Parse information from a Dimension element
 * @param grp The group we are currently processing (could be the root group)
 * @param dimension_node The node in the DOM tree of the <Dimension> element
 */
void DMZ::process_dimension(D4Group *grp, const xml_node &dimension_node) {
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
        throw BESInternalError("The required attribute 'name' or 'size' was missing from a Dimension element.",
                               __FILE__, __LINE__);

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
void DMZ::process_dim(DMR *dmr, D4Group *grp, Array *array, const xml_node &dim_node) {
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
        throw BESInternalError("Only one of 'size' and 'name' are allowed in a Dim element, but both were used.",
                               __FILE__, __LINE__);

    if (!size_value.empty()) {
        BESDEBUG(PARSER, prolog << "Processing nameless Dim of size: " << stoll(size_value) << endl);
        array->append_dim_ll(stoll(size_value));
    }
    else if (!name_value.empty()) {
        BESDEBUG(PARSER, prolog << "Processing Dim with named Dimension reference: " << name_value << endl);

        D4Dimension *dim;
        if (name_value[0] == '/') // lookup the Dimension in the root group
            dim = dmr->root()->find_dim(name_value);
        else
            // get enclosing Group and lookup Dimension there
            dim = grp->find_dim(name_value);

        if (!dim)
            throw BESInternalError(
                "The dimension '" + name_value + "' was not found while parsing the variable '" + array->name() + "'.",
                __FILE__,__LINE__);

        array->append_dim(dim);
    }
}

void DMZ::process_map(DMR *dmr, D4Group *grp, Array *array, const xml_node &map_node) {
    string name_value;
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
void DMZ::process_variable(DMR *dmr, D4Group *group, Constructor *parent, const xml_node &var_node) {
    if (!group) {
        throw BESInternalError(
            prolog + "Received a null valued Group pointer!", __FILE__, __LINE__);
    }

    string unsupported_flag;
    if (d_elide_unsupported && flagged_as_unsupported_type(var_node, unsupported_flag)) {
        // And in this way we elide the unsupported types - we don't process the DAP object
        // if it's got the unsupported bits in fillValue
        auto var_name = var_node.attribute("name");
        auto var_type = var_node.name();
        INFO_LOG(
            prolog + "Unsupported Type Encountered: " + var_type + " " + var_name.value() + "; flag: '" +
            unsupported_flag + "'\n");
        return;
    }

    // Variables are declared using nodes with type names (e.g., <Float32...>)
    // Variables are arrays if they have one or more <Dim...> child nodes.
    Type t = get_type(var_node.name());

    if (t == dods_group_c) {
        // Groups are special and handled elsewhere
        throw BESInternalError(
            prolog + "ERROR - The variable node to process is a Group type! "
            "This is handled elsewhere, not here. Parser State Issue!!", __FILE__, __LINE__);
    }

    BaseType *btp;
    if (has_dim_nodes(var_node)) {
        // If it has Dim nodes then it's an array!
        btp = add_array_variable(dmr, group, parent, t, var_node);
        if (t == dods_structure_c || t == dods_sequence_c) {
            if (btp->type() != dods_array_c || btp->var()->type() != t) {
                throw BESInternalError(
                    prolog + "Failed to create an array variable for " + var_node.name(), __FILE__, __LINE__);
            }
            // NB: For an array of a Constructor, add children to the Constructor, not the array
            parent = dynamic_cast<Constructor *>(btp->var());
            if (!parent) {
                throw BESInternalError(
                    prolog + "Failed to cast  " + btp->var()->type_name() + " " + btp->name() +
                    " to an instance of Constructor.", __FILE__, __LINE__);
            }
            for (auto child = var_node.first_child(); child; child = child.next_sibling()) {
                if (member_of(variable_elements, child.name()))
                    process_variable(dmr, group, parent, child);
            }
        }
    }
    else {
        // Things not arrays must be scalars...
        btp = add_scalar_variable(dmr, group, parent, t, var_node);
        if (t == dods_structure_c || t == dods_sequence_c) {
            if (btp->type() != t) {
                throw BESInternalError(
                    prolog + "Failed to create a scalar variable for " + var_node.name(), __FILE__, __LINE__);
            }
            parent = dynamic_cast<Constructor *>(btp);
            if (!parent) {
                throw BESInternalError(
                    prolog + "Failed to cast  " + btp->var()->type_name() + " " + btp->name() +
                    " to an instance of Constructor.", __FILE__, __LINE__);
            }
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
BaseType *DMZ::build_variable(DMR *dmr, D4Group *group, Type t, const xml_node &var_node) {
    if (!dmr->factory()) {
        throw BESInternalError(prolog + "ERROR - Received a DMR without a class factory!", __FILE__, __LINE__);
    }

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
        throw BESInternalError("Could not instantiate the variable ' " + name_value + "'.", __FILE__, __LINE__);

    btp->set_is_dap4(true);

    // I cannot find a test on the code on enum. Is this part of code really tested? KY 2023-12-21
    if (t == dods_enum_c) {
        if (enum_value.empty())
            throw BESInternalError("The variable ' " + name_value + "' lacks an 'enum' attribute.", __FILE__, __LINE__);
        D4EnumDef *enum_def = nullptr;
        if (enum_value[0] == '/')
            enum_def = dmr->root()->find_enum_def(enum_value);
        else
            enum_def = group->find_enum_def(enum_value);

        if (!enum_def)
            throw BESInternalError("Could not find the Enumeration definition '" + enum_value + "'.", __FILE__,
                                   __LINE__);

        dynamic_cast<D4Enum &>(*btp).set_enumeration(enum_def);
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
BaseType *DMZ::add_scalar_variable(DMR *dmr, D4Group *group, Constructor *parent, Type t, const xml_node &var_node) {
    if (!group) {
        throw BESInternalError(prolog + "ERROR - Received a null valued Group pointer!", __FILE__, __LINE__);
    }

    BaseType *btp = build_variable(dmr, group, t, var_node);

    // if the parent is non-null, the code should add the new var to a constructor,
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
BaseType *DMZ::add_array_variable(DMR *dmr, D4Group *group, Constructor *parent, Type t, const xml_node &var_node) {
    if (!group) {
        throw BESInternalError(prolog + "ERROR - Received a null valued Group pointer!", __FILE__, __LINE__);
    }

    BaseType *btp = build_variable(dmr, group, t, var_node);

    // Transform the scalar to an array
    auto *array = static_cast<DmrppArray *>(dmr->factory()->NewVariable(dods_array_c, btp->name()));
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
        else if (is_eq(child.name(), DMRPP_FIXED_LENGTH_STRING_ARRAY_ELEMENT)) {
            BESDEBUG(PARSER,
                     prolog << "Variable has been marked with a " << DMRPP_FIXED_LENGTH_STRING_ARRAY_ELEMENT << endl);
            // <dmrpp:FixedLengthStringArray string_length="8" pad="null"/>
            array->set_is_flsa(true);
            for (xml_attribute attr = child.first_attribute(); attr; attr = attr.next_attribute()) {
                if (is_eq(attr.name(), DMRPP_FIXED_LENGTH_STRING_LENGTH_ATTR)) {
                    auto length = array->set_fixed_string_length(attr.value());
                    BESDEBUG(PARSER, prolog << "Fixed length string array string length: " << length << endl);
                }
                else if (is_eq(attr.name(), DMRPP_FIXED_LENGTH_STRING_PAD_ATTR)) {
                    string_pad_type pad = array->set_fixed_length_string_pad_type(attr.value());
                    BESDEBUG(PARSER, prolog << "Fixed length string array padding scheme: " << pad << " (" <<
                             array->get_fixed_length_string_pad_str() << ")" << endl);
                }
            }
        }
        else if (is_eq(child.name(), DMRPP_VLSA_ELEMENT)) {
            BESDEBUG(PARSER, prolog << "Variable has been marked with a " << DMRPP_VLSA_ELEMENT << endl);
            array->set_is_vlsa(true);
        }
    }

    if (parent)
        parent->add_var_nocopy(array);
    else
        group->add_var_nocopy(array);

    return array;
}

/**
 * @brief Process an Enumeration element
 * @param d4g
  * @param var_node
 */
void DMZ::process_enum_def(D4Group *d4g, const xml_node &var_node) {
    string enum_def_name;
    string basetype_value;
    for (xml_attribute attr = var_node.first_attribute(); attr; attr = attr.next_attribute()) {
        if (is_eq(attr.name(), "name")) {
            enum_def_name = attr.value();
        }
        else if (is_eq(attr.name(), "basetype")) {
            basetype_value = attr.value();
        }
    }

    if (enum_def_name.empty())
        throw BESInternalError("The required attribute 'name' was missing from an enumeration element.", __FILE__,
                               __LINE__);

    if (basetype_value.empty())
        throw BESInternalError("The required attribute 'basetype' was missing from an enumeration element.", __FILE__,
                               __LINE__);

    Type enum_def_type = get_type(basetype_value.c_str());
    if (!is_integer_type(enum_def_type)) {
        string err_msg = "The enumeration '" + enum_def_name + "' must have an integer type, instead the type '" +
                         basetype_value;
        err_msg += "' is used.";
        throw BESInternalError(err_msg, __FILE__, __LINE__);
    }

    D4EnumDefs *d4enumdefs = d4g->enum_defs();
    vector<string> labels;
    vector<int64_t> label_values;
    for (auto child = var_node.first_child(); child; child = child.next_sibling()) {
        if (is_eq(child.name(), "EnumConst")) {
            string enum_const_def_name;
            string enum_const_def_value;
            for (xml_attribute attr = child.first_attribute(); attr; attr = attr.next_attribute()) {
                if (is_eq(attr.name(), "name")) {
                    enum_const_def_name = attr.value();
                }
                else if (is_eq(attr.name(), "value")) {
                    enum_const_def_value = attr.value();
                }
            }

            if (enum_const_def_name.empty())
                throw BESInternalError("The enum const name is missing.", __FILE__, __LINE__);

            if (enum_const_def_value.empty())
                throw BESInternalError("The enum const value is missing.", __FILE__, __LINE__);
            labels.push_back(enum_const_def_name);
            label_values.push_back(stoll(enum_const_def_value));
        }
    }
    auto enum_def_unique = make_unique<D4EnumDef>(enum_def_name, enum_def_type);
    auto enum_def = enum_def_unique.get();
    for (unsigned i = 0; i < labels.size(); i++)
        enum_def->add_value(labels[i], label_values[i]);
    d4enumdefs->add_enum_nocopy(enum_def_unique.release());
}


/**
 * @brief Process a Group element
 * This processes the information in a Group element and then processes the contained
 * Dimension, Group and Variable elements.
 * @param dmr
 * @param parent
 * @param var_node
 */
void DMZ::process_group(DMR *dmr, D4Group *parent, const xml_node &var_node) {
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

    auto new_group = dynamic_cast<DmrppD4Group *>(btp);

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
        else if (is_eq(child.name(), "Enumeration")) {
            process_enum_def(new_group, child);
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
void DMZ::build_thin_dmr(DMR *dmr) {
    auto xml_root_node = d_xml_doc.first_child();

    process_dataset(dmr, xml_root_node);

    auto root_group = dmr->root();

    auto *dg = dynamic_cast<DmrppD4Group *>(root_group);
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
        else if (is_eq(child.name(), "Enumeration")) {
            process_enum_def(dg, child);
        }
        else if (member_of(variable_elements, child.name())) {
            process_variable(dmr, dg, nullptr, child);
        }
    }
}


// This method will check if any variable in this file can apply the direct IO feature.
// If there is none,a global dio flag will be set to false. By checking the  global flag,
// the fileout netCDF module may not need to check every variable in the file to see if
// the direct IO can be applied.
bool DMZ::set_up_all_direct_io_flags_phase_1(DMR *dmr) {
    if (d_xml_doc == nullptr) {
        throw BESInternalError(prolog + "Received a null DMR pointer.", __FILE__, __LINE__);
    }

    bool dio_flag_value = set_up_direct_io_flag_phase_1(dmr->root());

    dmr->set_global_dio_flag(dio_flag_value);
    return dio_flag_value;
}

bool DMZ::set_up_direct_io_flag_phase_1(D4Group *group) {
    bool ret_value = false;
    for (auto i = group->var_begin(), e = group->var_end(); i != e; ++i) {
        BESDEBUG("dmrpp", "Inside set_up_direct_io_flag: var name is "<<(*i)->name()<<endl);
        if ((*i)->type() == dods_array_c) {
            if (true == set_up_direct_io_flag_phase_1(*i)) {
                ret_value = true;
                break;
            }
        }
    }

    if (ret_value == false) {
        for (auto gi = group->grp_begin(), ge = group->grp_end(); gi != ge; ++gi) {
            if (true == set_up_direct_io_flag_phase_1(*gi)) {
                ret_value = true;
                break;
            }
        }
    }
    return ret_value;
}

bool DMZ::set_up_direct_io_flag_phase_1(BaseType *btp) {
    // goto the DOM tree node for this variable
    xml_node var_node = get_variable_xml_node(btp);
    if (var_node == nullptr)
        throw BESInternalError("Could not find location of variable in the DMR++ XML document.", __FILE__, __LINE__);

    auto chunks = var_node.child("dmrpp:chunks");
    if (!chunks)
        return false;

    bool ret_value = false;
    for (xml_attribute attr = chunks.first_attribute(); attr; attr = attr.next_attribute()) {
        if (is_eq(attr.name(), "deflateLevel")) {
            ret_value = true;
            break;
        }
    }
    return ret_value;
}

void DMZ::set_up_all_direct_io_flags_phase_2(DMR *dmr) {
    if (d_xml_doc == nullptr) {
        throw BESInternalError(prolog + "Received a null DMR pointer.", __FILE__, __LINE__);
    }

    set_up_direct_io_flag_phase_2(dmr->root());
}

void DMZ::set_up_direct_io_flag_phase_2(D4Group *group) {
    for (auto i = group->var_begin(), e = group->var_end(); i != e; ++i) {
        if ((*i)->type() == dods_array_c)
            set_up_direct_io_flag_phase_2((*i));
    }

    for (auto gi = group->grp_begin(), ge = group->grp_end(); gi != ge; ++gi)
        set_up_direct_io_flag_phase_2((*gi));
}

void DMZ::set_up_direct_io_flag_phase_2(BaseType *btp) {
    bool is_integer_float = false;
    Array *t_a = nullptr;

    Type t = btp->type();
    if (t == dods_array_c) {
        t_a = dynamic_cast<Array *>(btp);
        Type t_var = t_a->var()->type();
        if (libdap::is_simple_type(t_var) && t_var != dods_str_c && t_var != dods_url_c && t_var != dods_enum_c && t_var
            != dods_opaque_c)
            is_integer_float = true;
    }

    // If the var is not an integer or float array, don't support the direct IO.
    if (is_integer_float == false)
        return;


    // goto the DOM tree node for this variable
    xml_node var_node = get_variable_xml_node(btp);
    if (var_node == nullptr)
        throw BESInternalError("Could not find location of variable in the DMR++ XML document.", __FILE__, __LINE__);

    auto chunks = var_node.child("dmrpp:chunks");

    // No chunks,no need to check the rest.
    if (!chunks)
        return;


    bool has_deflate_filter = false;
    string filter;
    vector<unsigned int> deflate_levels;

    bool is_le = false;

    for (xml_attribute attr = chunks.first_attribute(); attr; attr = attr.next_attribute()) {
        if (!has_deflate_filter && is_eq(attr.name(), "compressionType")) {
            filter = attr.value();
            if (filter.find("deflate") == string::npos)
                break;
            else
                has_deflate_filter = true;
        }
        else if (has_deflate_filter && deflate_levels.empty()) {
            if (is_eq(attr.name(), "deflateLevel")) {
                string def_lev_str = attr.value();

                // decompose the string.
                vector<string> def_lev_str_vec = BESUtil::split(def_lev_str, ' ');
                for (const auto &def_lev: def_lev_str_vec)
                    deflate_levels.push_back(stoul(def_lev));
            }
        }
        else if (is_eq(attr.name(), "byteOrder")) {
            string endian_str = attr.value();
            if (endian_str == "LE")
                is_le = true;
        }

        else if (is_eq(attr.name(), "DIO") && is_eq(attr.value(), "off")) {
            dc(btp)->set_disable_dio(true);
            BESDEBUG(PARSER, prolog << "direct IO is disabled : the variable name is: " <<btp->name() << endl);
        }
    }

    // If no deflate filter is used or the deflate_levels is not defined, cannot do the direct IO. return.
    if (!has_deflate_filter || (deflate_levels.empty()))
        return;

    // If the datatype is not little-endian, cannot do the direct IO. return.
    // The big-endian IEEE-floating-point data also needs byteswap. So we cannot do direct IO. KY 2024-03-03
    if (!is_le)
        return;

    if (dc(btp)->is_disable_dio())
        return;

    // Now we need to read the first child of dmrpp:chunks to obtain the chunk sizes.
    vector<unsigned long long> chunk_dim_sizes;
    for (auto child = chunks.child("dmrpp:chunkDimensionSizes"); child; child = child.next_sibling()) {
        if (is_eq(child.name(), "dmrpp:chunkDimensionSizes")) {
            string chunk_sizes_str = child.child_value();
            vector<string> chunk_sizes_str_vec = BESUtil::split(chunk_sizes_str, ' ');
            for (const auto &chunk_size: chunk_sizes_str_vec)
                chunk_dim_sizes.push_back(stoull(chunk_size));
            break;
        }
    }

    // Since the deflate filter is always associated with the chunk storage,
    // the chunkDimensionSizes should always exist for the direct IO case. If not, return.
    if (chunk_dim_sizes.empty())
        return;

    // Now we need to count the number of children with the name <dmrpp:chunk> inside the <dmrpp:chunks>.
    size_t num_chunks_children = 0;
    for (auto child = chunks.first_child(); child; child = child.next_sibling())
        num_chunks_children++;

    // If the only child is dmrpp::chunkDimensionSizes, no chunk is found. This is not direct IO case.
    if (num_chunks_children == 1)
        return;

    // Now we need to check the special case if the chunk size is greater than the dimension size for any dimension.
    // If this is the case, we will not use the direct chunk IO since netCDF-4 doesn't allow this.
    vector<unsigned long long> dim_sizes;
    auto p = t_a->dim_begin();
    while (p != t_a->dim_end()) {
        dim_sizes.push_back((unsigned long long) (t_a->dimension_size_ll(p)));
        ++p;
    }

    bool chunk_less_dim = true;
    if (chunk_dim_sizes.size() == dim_sizes.size()) {
        for (unsigned int i = 0; i < dim_sizes.size(); i++) {
            if (chunk_dim_sizes[i] > dim_sizes[i]) {
                chunk_less_dim = false;
                break;
            }
        }
    }
    else
        chunk_less_dim = false;

    if (!chunk_less_dim)
        return;

    // Another special case is that some chunks are only filled with the fvalues. This case cannot be handled by direct IO.
    // First calculate the number of logical chunks.
    // Also up to this step, the size of chunk_dim_sizes must be the same as the size of dim_sizes. No need to double check.
    size_t num_logical_chunks = 1;
    for (unsigned int i = 0; i < dim_sizes.size(); i++)
        num_logical_chunks *= (size_t) ceil((float) dim_sizes[i] / (float) chunk_dim_sizes[i]);
    if (num_logical_chunks != (num_chunks_children - 1))
        return;

    // Now we should provide the variable info for the define mode inside the fileout netCDF module.
    // The chunk offset/length etc. information will be provided after load_chunk() is called in the read().

    BESDEBUG(PARSER, prolog << "Can do direct IO: the variable name is: " <<btp->name() << endl);

    // Adding the dio information in the variable level. This information is needed for the define mode in the fileout netcdf module.
    // Fill in the chunk information so that the fileout netcdf can retrieve.
    Array::var_storage_info dmrpp_vs_info;

    // Add the filter info.
    dmrpp_vs_info.filter = filter;

    // Provide the deflate compression levels.
    for (const auto &def_lev: deflate_levels)
        dmrpp_vs_info.deflate_levels.push_back(def_lev);

    // Provide the chunk dimension sizes.
    for (const auto &chunk_dim: chunk_dim_sizes)
        dmrpp_vs_info.chunk_dims.push_back(chunk_dim);

    t_a->set_var_storage_info(dmrpp_vs_info);
    t_a->set_dio_flag();
}


/**
 * Check to see if the current tag is either an \c Attribute or an \c Alias
 * start tag. This method is a glorified macro...
 *
 * @param attributes The start tag name
 * @param dap_attr_node The tag's XML attributes
 * @return True if the tag was an \c Attribute or \c Alias tag
 */
void DMZ::process_attribute(D4Attributes *attributes, const xml_node &dap_attr_node) {
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
        throw BESInternalError("The required attribute 'name' or 'type' was missing from an Attribute element.",
                               __FILE__, __LINE__);

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
                attribute->add_value(value_elem.child_value()); // returns the text of the first data node
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
void DMZ::build_basetype_chain(BaseType *btp, stack<BaseType *> &bt) {
    auto parent = btp->get_parent();
    bt.push(btp);

    // The parent must be non-null and not the root group (the root group has no parent).
    if (parent && !(parent->type() == dods_group_c && parent->get_parent() == nullptr))
        build_basetype_chain(parent, bt);
}

xml_node DMZ::get_variable_xml_node_helper(const xml_node &/*parent_node*/, stack<BaseType *> &/*bt*/) {
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
    string type_name = bt.top()->type() == dods_array_c ? bt.top()->var()->type_name() : bt.top()->type_name();
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

    return xml_node(); // return an empty node
#else
    return {}; // return an empty node
#endif
}

/**
 * @brief For a given variable find its corresponding xml_node
 * @param btp The variable (nominally, a child node from the DMR
 * that corresponds to the DMR++ XML document this class manages).
 * @return The xml_node pointer
 */
xml_node DMZ::get_variable_xml_node(BaseType *btp) {
#if USE_CACHED_XML_NODE
    auto node = dc(btp)->get_xml_node();
    if (node == nullptr)
        throw BESInternalError(string("The xml_node for '").append(btp->name()).append("' was not recorded."), __FILE__,
                               __LINE__);

    return node;
#else
    // load the BaseType objects onto a stack, since we start at the leaf and
    // go backward using its 'parent' pointer, the order of BaseTypes on the
    // stack will match the order in the hierarchy of the DOM tree.
    stack<BaseType *> bt;
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
DMZ::load_attributes(BaseType *btp) {
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
            auto *c = dynamic_cast<Constructor *>(btp);
            if (c) {
                for (auto i = c->var_begin(), e = c->var_end(); i != e; i++) {
                    if ((*i)->type() == dods_array_c)
                        dc((*i)->var())->set_attributes_loaded(true);
                    else
                        dc(*i)->set_attributes_loaded(true);
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
DMZ::load_attributes(BaseType *btp, xml_node var_node) const {
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
DMZ::load_attributes(Constructor *constructor) {
    load_attributes(constructor, get_variable_xml_node(constructor));
    for (auto i = constructor->var_begin(), e = constructor->var_end(); i != e; ++i) {
        // Groups are not allowed inside a Constructor
        if ((*i)->type() == dods_group_c) {
            throw BESInternalError(
                prolog + "Found a Group as a member of a " + constructor->type_name() + " data type. " +
                "This violates the DAP4 data model and cannot be processed!", __FILE__, __LINE__);
        }
        load_attributes(*i);
    }
}

void
DMZ::load_attributes(D4Group *group) {
    // The root group is special; look for its DAP Attributes in the Dataset element
    if (group->get_parent() == nullptr) {
        xml_node dataset = d_xml_doc.child("Dataset");
        if (!dataset)
            throw BESInternalError("Could not find the 'Dataset' element in the DMR++ XML document.", __FILE__,
                                   __LINE__);
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
        if ((*i)->type() == dods_group_c) {
            throw BESInternalError(
                prolog + "Found a Group instance in the variables collection for Group " + group->name() + ". " +
                "This violates the DAP4 data model and cannot be processed!", __FILE__, __LINE__);
        }
        load_attributes(*i);
    }

    for (auto i = group->grp_begin(), e = group->grp_end(); i != e; ++i) {
        load_attributes(*i);
    }
}

void DMZ::load_all_attributes(libdap::DMR *dmr) {
    if (d_xml_doc == nullptr) {
        throw BESInternalError(prolog + "Received a null DMR pointer.", __FILE__, __LINE__);
    }
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
DMZ::process_compact(BaseType *btp, const xml_node &compact) {
    dc(btp)->set_compact(true);

    auto char_data = compact.child_value();
    if (!char_data)
        throw BESInternalError("The dmrpp::compact is missing data values.",__FILE__,__LINE__);

    std::vector<u_int8_t> decoded = base64::Base64::decode(char_data);

    // Current not support structure, sequence and grid for compact storage.
    if (btp->type() == dods_structure_c || btp->type() == dods_sequence_c || btp->type() == dods_grid_c)
        throw BESInternalError("The dmrpp::compact element must be the child of an array or a scalar variable",
                               __FILE__, __LINE__);

    // Obtain the datatype for array and scalar.
    Type dtype = btp->type();
    bool is_array_subset = false;
    if (dtype == dods_array_c) {
        auto *da = dynamic_cast<DmrppArray *>(btp);
        if (da->is_projected())
            is_array_subset = true;
        else
            dtype = btp->var()->type();
    }

    if (is_array_subset) {
        auto *da = dynamic_cast<DmrppArray *>(btp);
        process_compact_subset(da, decoded);
        return;
    }

    switch (dtype) {
        case dods_array_c:
            throw BESInternalError("DMR++ document fail: An Array may not be the template for an Array.", __FILE__,
                                   __LINE__);

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
                auto *array = dynamic_cast<DmrppArray *>(btp);
                if (!array) {
                    throw BESInternalError("Internal state error. Object claims to be array but is not.",__FILE__,
                                           __LINE__);
                }
                if (array->is_flsa()) {
                    // It's an array of Fixed Length Strings
                    auto fls_length = array->get_fixed_string_length();
                    auto pad_type = array->get_fixed_length_string_pad();
                    auto str_start = reinterpret_cast<char *>(decoded.data());
                    vector<string> fls_values;
                    while (fls_values.size() < btp->length_ll()) {
                        string aValue = DmrppArray::ingest_fixed_length_string(str_start, fls_length, pad_type);
                        fls_values.emplace_back(aValue);
                        str_start += fls_length;
                    }
                    array->set_value(fls_values, (int) fls_values.size());
                    array->set_read_p(true);
                }
                else {
                    // It's an array of Variable Length Strings
                    throw BESInternalError("Variable Length Strings are not yet supported.",__FILE__,__LINE__);
                }
            }
            else {
                // Scalar
                if (btp->type() == dods_str_c) {
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
            throw BESInternalError("Unsupported COMPACT storage variable type in the drmpp handler.", __FILE__,
                                   __LINE__);
        case dods_null_c:
            break;
        case dods_structure_c:
            break;
        case dods_sequence_c:
            break;
        case dods_grid_c:
            break;
        case dods_opaque_c:
            break;
        case dods_group_c:
            break;
    }
}

void DMZ::process_compact_subset(DmrppArray *da, std::vector<u_int8_t> &decoded) {
    if (da->var()->type() == dods_str_c || da->var()->type() == dods_url_c)
        throw BESInternalError("Currently we don't support the subset for the compacted array of string",__FILE__,
                               __LINE__);

    int64_t num_buf_bytes = da->width_ll(true);
    vector<unsigned char> buf_bytes;
    buf_bytes.resize(num_buf_bytes);
    vector<unsigned long long> da_dims = da->get_shape(false);
    unsigned long subset_index = 0;
    vector<unsigned long long> subset_pos;
    handle_subset(da, da->dim_begin(), subset_index, subset_pos, buf_bytes, decoded);

    da->val2buf(reinterpret_cast<void *>(buf_bytes.data()));

    da->set_read_p(true);
}

void DMZ::process_vlsa(libdap::BaseType *btp, const pugi::xml_node &vlsa_element) {
    //---------------------------------------------------------------------------
    // Input Sanitization
    // We do the QC here and not in all the functions called, like endlessly...
    //
    if (btp->type() != dods_array_c) {
        throw BESInternalError(prolog + "Received an unexpected " + btp->type_name() +
                               " Expected an instance of DmrppArray!", __FILE__, __LINE__);
    }
    auto *array = dynamic_cast<DmrppArray *>(btp);
    if (!array) {
        throw BESInternalError("Internal state error. "
                               "Object claims to be array but is not.", __FILE__, __LINE__);
    }
    if (array->var()->type() != dods_str_c && array->var()->type() != dods_url_c) {
        throw BESInternalError(prolog + "Internal state error. "
                               "Expected array of dods_str_c, got " +
                               array->var()->type_name(), __FILE__, __LINE__);
    }

    vector<string> entries;
    vlsa::read(vlsa_element, entries);

    array->set_is_vlsa(true);
    array->set_value(entries, (int) entries.size());
    array->set_read_p(true);
}

void
DMZ::process_missing_data(BaseType *btp, const xml_node &missing_data) {
    BESDEBUG(PARSER, prolog << "Coming to process_missing_data() " << endl);
    dc(btp)->set_missing_data(true);

    auto char_data = missing_data.child_value();
    if (!char_data)
        throw BESInternalError("The dmrpp::missing_data doesn't contain  missing data values.",__FILE__,__LINE__);

    std::vector<u_int8_t> decoded = base64::Base64::decode(char_data);

    if (btp->type() != dods_array_c && btp->type() != dods_byte_c)
        throw BESInternalError(
            "The dmrpp::missing_data element must be the child of an array or a unsigned char scalar variable",
            __FILE__, __LINE__);

    if (btp->type() == dods_byte_c) {
        auto db = dynamic_cast<DmrppByte *>(btp);
        db->set_value(decoded[0]);
        db->set_read_p(true);
        return;
    }
    auto *da = dynamic_cast<DmrppArray *>(btp);

    vector<Bytef> result_bytes;

    // We need to obtain the total buffer size to retrieve the whole array. 
    // We cannot use width_ll() since it will return the number of selected elements.
    auto result_size = (uLongf) (da->get_size(false) * da->prototype()->width());
    result_bytes.resize(result_size);

    if (da->get_size(false) == 1)
        memcpy(result_bytes.data(), decoded.data(), result_size);
    else {
#if 0
        // TODO: somehow using uncompress2 causes some existing tests failed. We need to
        //       investigate how to compress/decompress a >4GB buffer with compress2/uncompress2. KY 2026-01-21
        auto sourceLenP = (uLong*)(decoded.size());
        int retval = uncompress2(result_bytes.data(), &result_size, decoded.data(), sourceLenP);
#endif 
        int retval = uncompress(result_bytes.data(), &result_size, decoded.data(), decoded.size());
        if (retval != 0)
            throw BESInternalError("The dmrpp::missing_data - fail to uncompress the mssing data.", __FILE__, __LINE__);
    }

    if (da->is_projected()) {
        int64_t num_buf_bytes = da->width_ll(true);
        vector<unsigned char> buf_bytes;
        buf_bytes.resize(num_buf_bytes);
        vector<unsigned long long> da_dims = da->get_shape(false);
        unsigned long subset_index = 0;
        vector<unsigned long long> subset_pos;
        handle_subset(da, da->dim_begin(), subset_index, subset_pos, buf_bytes, result_bytes);

        da->val2buf(reinterpret_cast<void *>(buf_bytes.data()));
    }
    else
        da->val2buf(reinterpret_cast<void *>(result_bytes.data()));

    da->set_read_p(true);
}

bool
DMZ::supported_special_structure_type_internal(Constructor *var_ctor) {
    bool ret_value = true;
    for (auto vi = var_ctor->var_begin(), ve = var_ctor->var_end(); vi != ve; ++vi) {
        BaseType *bt = *vi;
        Type t_bt = bt->type();

        // Only support array or scalar of float/int/string.
        if (libdap::is_simple_type(t_bt) == false) {
            if (t_bt != dods_array_c) {
                ret_value = false;
                break;
            }
            else {
                auto t_a = dynamic_cast<Array *>(bt);
                Type t_array_var = t_a->var()->type();
                if (!libdap::is_simple_type(t_array_var) || t_array_var == dods_url_c || t_array_var == dods_enum_c ||
                    t_array_var == dods_opaque_c) {
                    ret_value = false;
                    break;
                }
            }
        }
        else if (t_bt == dods_url_c || t_bt == dods_enum_c || t_bt == dods_opaque_c) {
            ret_value = false;
            break;
        }
    }

    return ret_value;
}

bool
DMZ::supported_special_structure_type(BaseType *btp) {
    bool ret_value = false;
    Type t = btp->type();
    if ((t == dods_array_c && btp->var()->type() == dods_structure_c) || t == dods_structure_c) {
        Constructor *var_constructor = nullptr;
        if (t == dods_structure_c)
            var_constructor = dynamic_cast<Constructor *>(btp);
        else
            var_constructor = dynamic_cast<Constructor *>(btp->var());
        if (!var_constructor) {
            throw BESInternalError(
                prolog + "Failed to cast  " + btp->var()->type_name() + " " + btp->name() +
                " to an instance of Constructor.", __FILE__, __LINE__);
        }

        ret_value = supported_special_structure_type_internal(var_constructor);
    }
    return ret_value;
}

void
DMZ::process_special_structure_data(BaseType *btp, const xml_node &special_structure_data) {
    BESDEBUG(PARSER, prolog << "Coming to process_special_structure_data() " << endl);

    if (supported_special_structure_type(btp) == false)
        throw BESInternalError("The dmrpp::the datatype is not a supported special  structure variable", __FILE__,
                               __LINE__);

    auto char_data = special_structure_data.child_value();
    if (!char_data)
        throw BESInternalError("The dmrpp::special_structure_data doesn't contain special structure data values.",
                               __FILE__,__LINE__);

    std::vector<u_int8_t> values = base64::Base64::decode(char_data);
    size_t total_value_size = values.size();

    if (btp->type() == dods_array_c) {
        auto ar = dynamic_cast<DmrppArray *>(btp);
        if (ar->is_projected())
            throw BESInternalError("The dmrpp::currently we don't support subsetting of special_structure_data.",
                                   __FILE__,__LINE__);

        int64_t nelms = ar->length_ll();
        size_t values_offset = 0;

        for (int64_t element = 0; element < nelms; ++element) {
            auto dmrpp_s = dynamic_cast<DmrppStructure *>(ar->var()->ptr_duplicate());
            if (!dmrpp_s)
                throw InternalErr(__FILE__, __LINE__, "Cannot obtain the structure pointer.");

            process_special_structure_data_internal(dmrpp_s, values, total_value_size, values_offset);
            ar->set_vec_ll((uint64_t) element, dmrpp_s);
            delete dmrpp_s;
        }
    }
    else {
        size_t values_offset = 0;
        auto dmrpp_s = dynamic_cast<DmrppStructure *>(btp);
        if (!dmrpp_s)
            throw InternalErr(__FILE__, __LINE__, "Cannot obtain the structure pointer.");
        process_special_structure_data_internal(dmrpp_s, values, total_value_size, values_offset);
    }

    btp->set_read_p(true);
}

void DMZ::process_special_structure_data_internal(DmrppStructure *dmrpp_s, std::vector<u_int8_t> &values,
                                                  size_t total_value_size, size_t &values_offset) {

    for (auto vi = dmrpp_s->var_begin(), ve = dmrpp_s->var_end(); vi != ve; ++vi) {
        BaseType *bt = *vi;
        Type t_bt = bt->type();
        if (libdap::is_simple_type(t_bt) && t_bt != dods_str_c && t_bt != dods_url_c && t_bt != dods_enum_c && t_bt !=
            dods_opaque_c) {
            BESDEBUG("dmrpp", "var name is: " << bt->name() << "'" << endl);
            BESDEBUG("dmrpp", "var values_offset is: " << values_offset << "'" << endl);
            bt->val2buf(values.data() + values_offset);
            values_offset += bt->width_ll();
        }
        else if (t_bt == dods_str_c) {
            BESDEBUG("dmrpp", "var string name is: " << bt->name() << "'" << endl);
            BESDEBUG("dmrpp", "var string values_offset is: " << values_offset << "'" << endl);
            if (total_value_size < values_offset)
                throw InternalErr(__FILE__, __LINE__, "The offset of the retrieved value is out of the boundary.");
            size_t rest_buf_size = total_value_size - values_offset;
            u_int8_t *start_pointer = values.data() + values_offset;
            vector<char> temp_buf;
            temp_buf.resize(rest_buf_size);
            memcpy(temp_buf.data(), (void *) start_pointer, rest_buf_size);
            // find the index of first ";", the separator
            size_t string_stop_index = 0;
            vector<char> string_value;
            for (size_t i = 0; i < rest_buf_size; i++) {
                if (temp_buf[i] == ';') {
                    string_stop_index = i;
                    break;
                }
                else
                    string_value.push_back(temp_buf[i]);
            }
            string encoded_str(string_value.begin(), string_value.end());
            vector<u_int8_t> decoded_str = base64::Base64::decode(encoded_str);
            vector<char> decoded_vec;
            decoded_vec.resize(decoded_str.size());
            memcpy(decoded_vec.data(), (void *) decoded_str.data(), decoded_str.size());
            string final_str(decoded_vec.begin(), decoded_vec.end());
            bt->val2buf(&final_str);
            values_offset = values_offset + string_stop_index + 1;
        }

        else if (t_bt == dods_array_c) {
            BESDEBUG("dmrpp", "var array name is: " << bt->name() << "'" << endl);
            BESDEBUG("dmrpp", "var array values_offset is: " << values_offset << "'" << endl);

            auto t_a = dynamic_cast<Array *>(bt);
            Type ar_basetype = t_a->var()->type();
            if (libdap::is_simple_type(ar_basetype) && ar_basetype != dods_str_c && ar_basetype != dods_url_c &&
                ar_basetype != dods_enum_c && ar_basetype != dods_opaque_c) {
                bt->val2buf(values.data() + values_offset);
                values_offset += bt->width_ll();
            }
            else if (ar_basetype == dods_str_c) {
                if (total_value_size < values_offset)
                    throw InternalErr(__FILE__, __LINE__, "The offset of the retrieved value is out of the boundary.");

                size_t rest_buf_size = total_value_size - values_offset;
                u_int8_t *start_pointer = values.data() + values_offset;
                vector<char> temp_buf;
                temp_buf.resize(rest_buf_size);
                memcpy(temp_buf.data(), (void *) start_pointer, rest_buf_size);

                int64_t num_ar_elems = t_a->length_ll();

                // We need to create a vector of string to pass the string array.
                // Each string's encoded value is separated by ';'.
                vector<string> encoded_str;
                encoded_str.resize(num_ar_elems);

                unsigned int str_index = 0;
                size_t string_stop_index = 0;
                for (size_t i = 0; i < rest_buf_size; i++) {
                    if (temp_buf[i] != ';')
                        encoded_str[str_index].push_back(temp_buf[i]);
                    else {
                        str_index++;
                        if (str_index == num_ar_elems) {
                            string_stop_index = i;
                            break;
                        }
                    }
                }

                vector<string> final_str;
                final_str.resize(num_ar_elems);

                // decode the encoded string
                for (size_t i = 0; i < num_ar_elems; i++) {
                    string temp_encoded_str(encoded_str[i].begin(), encoded_str[i].end());
                    vector<u_int8_t> decoded_str = base64::Base64::decode(temp_encoded_str);
                    vector<char> decoded_vec;
                    decoded_vec.resize(decoded_str.size());
                    memcpy(decoded_vec.data(), (void *) decoded_str.data(), decoded_str.size());
                    string temp_final_str(decoded_vec.begin(), decoded_vec.end());
                    final_str[i] = temp_final_str;
                }

                t_a->set_value_ll(final_str, num_ar_elems);
                values_offset = values_offset + string_stop_index + 1;
            }
            else
                throw InternalErr(
                    __FILE__, __LINE__,
                    "The base type of this structure is not integer or float or string.  Currently it is not supported.");
        }
    }
    dmrpp_s->set_read_p(true);
}

/**
 * @brief Parse a chunk node
 * There are several different forms a chunk node can take and this handles
 * all of them.
 * @param dc
 * @param chunk
 */
void DMZ::process_chunk(DmrppCommon *dc, const xml_node &chunk) const {
    string href;
    string offset;
    string size;
    string chunk_position_in_array;
    string filter_mask;
    bool href_trusted = false;

    for (xml_attribute attr = chunk.first_attribute(); attr; attr = attr.next_attribute()) {
        if (is_eq(attr.name(), "offset")) {
            offset = attr.value();
        }
        else if (is_eq(attr.name(), "nBytes")) {
            size = attr.value();
        }
        else if (is_eq(attr.name(), "chunkPositionInArray")) {
            chunk_position_in_array = attr.value();
        }
        else if (is_eq(attr.name(), "fm")) {
            filter_mask = attr.value();
        }
        else if (is_eq(attr.name(), "href")) {
            href = attr.value();
        }
        else if (is_eq(attr.name(), "trust") || is_eq(attr.name(), "dmrpp:trust")) {
            href_trusted = is_eq(attr.value(), "true");
        }
    }

    if (offset.empty() || size.empty())
        throw BESInternalError("Both size and offset are required for a chunk node.", __FILE__, __LINE__);
    if (!href.empty()) {
        shared_ptr<http::url> data_url(new http::url(href, href_trusted));
        if (filter_mask.empty())
            dc->add_chunk(data_url, dc->get_byte_order(), stoull(size), stoull(offset), chunk_position_in_array);
        else
            dc->add_chunk(data_url, dc->get_byte_order(), stoull(size), stoull(offset), stoul(filter_mask),
                          chunk_position_in_array);
    }
    else {
        if (filter_mask.empty())
            dc->add_chunk(d_dataset_elem_href, dc->get_byte_order(), stoull(size), stoull(offset),
                          chunk_position_in_array);
        else
            dc->add_chunk(d_dataset_elem_href, dc->get_byte_order(), stoull(size), stoull(offset), stoul(filter_mask),
                          chunk_position_in_array);
    }

    dc->accumlate_storage_size(stoull(size));
}

void DMZ::process_block(DmrppCommon *dc, const xml_node &chunk, unsigned int block_count) const {
    string href;
    string offset;
    string size;
    bool href_trusted = false;

    for (xml_attribute attr = chunk.first_attribute(); attr; attr = attr.next_attribute()) {
        if (is_eq(attr.name(), "offset")) {
            offset = attr.value();
        }
        else if (is_eq(attr.name(), "nBytes")) {
            size = attr.value();
        }
        else if (is_eq(attr.name(), "href")) {
            href = attr.value();
        }
        else if (is_eq(attr.name(), "trust") || is_eq(attr.name(), "dmrpp:trust")) {
            href_trusted = is_eq(attr.value(), "true");
        }
    }

    if (offset.empty() || size.empty())
        throw BESInternalError("Both size and offset are required for a block node.", __FILE__, __LINE__);
    if (!href.empty()) {
        shared_ptr<http::url> data_url(new http::url(href, href_trusted));
        dc->add_chunk(data_url, dc->get_byte_order(), stoull(size), stoull(offset), true, block_count);
    }
    else
        dc->add_chunk(d_dataset_elem_href, dc->get_byte_order(), stoull(size), stoull(offset), true, block_count);


    dc->accumlate_storage_size(stoull(size));
}

/**
 * @brief find the first chunkDimensionSizes node and use its value
 * This method ignores any 'extra' chunkDimensionSizes nodes.
 * @param dc
 * @param chunks
 */
void DMZ::process_cds_node(DmrppCommon *dc, const xml_node &chunks) {
    for (auto child = chunks.child("dmrpp:chunkDimensionSizes"); child; child = child.next_sibling()) {
        if (is_eq(child.name(), "dmrpp:chunkDimensionSizes")) {
            string sizes = child.child_value();
            dc->parse_chunk_dimension_sizes(sizes);
        }
    }
}

static void add_fill_value_information(DmrppCommon *dc, const string &value_string, libdap::Type fv_type) {
    dc->set_fill_value_string(value_string);
    dc->set_fill_value_type(fv_type);
    dc->set_uses_fill_value(true);
}

/**
 * A 'dmrpp:chunks' node has a chunkDimensionSizes node and then one or more chunks
 * nodes, and they have to be in that order.
 *
 * @param btp
 * @param var_node
 * @return True when the dmrpp:chunks element was located, false otherwise.
 */
 bool DMZ::process_chunks(BaseType *btp, const xml_node &var_node) const {
     auto chunks = var_node.child("dmrpp:chunks");
     if (!chunks)
         return false;

     bool has_fill_value = false;

     unsigned int block_count = 0;
     bool is_multi_lb_chunks = false;

     for (xml_attribute attr = chunks.first_attribute(); attr; attr = attr.next_attribute()) {
         if (is_eq(attr.name(), "compressionType")) {
             dc(btp)->set_filter(attr.value());
         }
         else if (is_eq(attr.name(), "deflateLevel")) {
             string def_lev_str = attr.value();
             // decompose the string.
             vector<string> def_lev_str_vec = BESUtil::split(def_lev_str, ' ');
             vector<unsigned int> def_levels;
             def_levels.reserve(def_lev_str_vec.size());
             for (const auto &def_lev: def_lev_str_vec)
                 def_levels.push_back(stoul(def_lev));
             dc(btp)->set_deflate_levels(def_levels);
         }
         else if (is_eq(attr.name(), "fillValue")) {
             // Throws BESInternalError when unsupported types detected.
             string unsupported_type;
             if (flagged_as_unsupported_type(var_node, unsupported_type)) {
                 stringstream msg;
                 msg << prolog << "Found a dmrpp:chunk/@fillValue with a value of ";
                 msg << "'" << unsupported_type << "' this means that ";
                 msg << "the Hyrax service is unable to process this variable/dataset.";
                 throw BESInternalError(msg.str(),__FILE__,__LINE__);
             }

             has_fill_value = true;

             // Fill values are only supported for Arrays and scalar numeric datatypes (7/12/22)
             if (btp->type() == dods_url_c
                 || btp->type() == dods_sequence_c || btp->type() == dods_grid_c)
                 throw BESInternalError("Fill Value chunks are unsupported for URL, sequence and grid types.", __FILE__,
                                        __LINE__);
             if (btp->type() == dods_array_c) {
                 auto array = dynamic_cast<libdap::Array *>(btp);
                 add_fill_value_information(dc(btp), attr.value(), array->var()->type());
             }
             else
                 add_fill_value_information(dc(btp), attr.value(), btp->type());
         }
         else if (is_eq(attr.name(), "byteOrder"))
             dc(btp)->ingest_byte_order(attr.value());

             // Here we don't need to check the structOffset attribute if the datatype is not dods_structure_c or array of dods_structure_c.
             // But since most variables won't have the structOffset attribute, the code will NOT even go to the following "else if block" after
             // looping through the last attribute. So still keep the following implementation.
         else if (is_eq(attr.name(), "structOffset")) {
             string so_str = attr.value();
             // decompose the string.
             vector<string> so_str_vec = BESUtil::split(so_str, ' ');
             vector<unsigned int> struct_offsets;
             struct_offsets.reserve(so_str_vec.size());
             for (const auto &s_off: so_str_vec)
                 struct_offsets.push_back(stoul(s_off));
             dc(btp)->set_struct_offsets(struct_offsets);
         }
         // The following only applies to rare cases when handling HDF4, most cases won't even come here.
         else if (is_eq(attr.name(), "LBChunk")) {
             string is_lbchunk_value = attr.value();
             if (is_lbchunk_value == "true") {
                 is_multi_lb_chunks = true;
                 dc(btp)->set_multi_linked_blocks_chunk(true);
             }
         }
     }

     // reset one_chunk_fillvalue to false if has_fill_value = false
     if (has_fill_value == false && dc(btp)->get_one_chunk_fill_value() == true) // reset fillvalue
         dc(btp)->set_one_chunk_fill_value(false);

     // Look for the chunksDimensionSizes element - it will not be present for contiguous data
     process_cds_node(dc(btp), chunks);

     // If child node "dmrpp:chunk" is found, the child node "dmrpp:block" will be not present.
     // They are mutual exclusive.

     bool is_chunked_storage = false;
     for (auto chunk = chunks.child("dmrpp:chunk"); chunk; chunk = chunk.next_sibling()) {
         if (is_eq(chunk.name(), "dmrpp:chunk")) {
             is_chunked_storage = true;
             break;
         }
     }

     if (is_chunked_storage && is_multi_lb_chunks == false) {
         // Chunks for this node will be held in the var_node siblings.
         for (auto chunk = chunks.child("dmrpp:chunk"); chunk; chunk = chunk.next_sibling()) {
             if (is_eq(chunk.name(), "dmrpp:chunk")) {
                 process_chunk(dc(btp), chunk);
             }
         }
     }
     else {
         // Blocks for this node, we need to first check if there is only one block. If this is the case,
         // we should issue an error.
         for (auto chunk = chunks.child("dmrpp:block"); chunk; chunk = chunk.next_sibling()) {
             if (is_eq(chunk.name(), "dmrpp:block")) {
                 block_count++;
             }
             if (block_count > 1)
                 break;
         }
     }
     if (block_count > 0) {
         if (block_count == 1)
             throw BESInternalError(" The number of linked block is 1, but it should be > 1.", __FILE__, __LINE__);
         if (block_count > 1) {
             // set using linked block
             dc(btp)->set_using_linked_block();
             // reset the count to 0 to process the blocks.
             block_count = 0;
             for (auto chunk = chunks.child("dmrpp:block"); chunk; chunk = chunk.next_sibling()) {
                 if (is_eq(chunk.name(), "dmrpp:block")) {
                     process_block(dc(btp), chunk, block_count);
                     BESDEBUG(PARSER,
                              prolog << "This count of linked block of this variable is: " << block_count << endl);
                     block_count++;
                 }
             }
             dc(btp)->set_total_linked_blocks(block_count);
         }
     }
     else if (is_multi_lb_chunks) {
         queue<vector<pair<unsigned long long, unsigned long long> > > mb_index_queue;
         vector<pair<unsigned long long, unsigned long long> > offset_length_pair;

         // Loop through all the chunks.
         for (auto chunk = chunks.child("dmrpp:chunk"); chunk; chunk = chunk.next_sibling()) {
             // Check the block offset and length for this chunk.
             if (is_eq(chunk.name(), "dmrpp:chunk"))
                 add_mblock_index(chunk, mb_index_queue, offset_length_pair);
         }
         // This is the last one.
         mb_index_queue.push(offset_length_pair);

         // Now we get all the blocks and we will process them.
         for (auto chunk = chunks.child("dmrpp:chunk"); chunk; chunk = chunk.next_sibling()) {
             if (is_eq(chunk.name(), "dmrpp:chunk"))
                 process_multi_blocks_chunk(dc(btp), chunk, mb_index_queue);
         }
         dc(btp)->set_multi_linked_blocks_chunk(true);
     }
     return true;
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
        array_dim_sizes.push_back(array->dimension_size_ll(i));
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
    const auto & chunk_dim_sizes = dc->get_chunk_dimension_sizes();
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
set<vector<unsigned long long> > DMZ::get_chunk_map(const vector<shared_ptr<Chunk> > &chunks) {
    set<vector<unsigned long long> > chunk_map;
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
void DMZ::process_fill_value_chunks(BaseType *btp, const set<shape> &chunk_map, const shape &chunk_shape,
                                    const shape &array_shape, unsigned long long chunk_size, unsigned int struct_size) {
    auto dcp = dc(btp);
    // Use an Odometer to walk over each potential chunk
    DmrppChunkOdometer odometer(array_shape, chunk_shape);
    do {
        const auto &s = odometer.indices();
        if (chunk_map.find(s) == chunk_map.end()) {
            // Fill Value chunk
            // what we need byte order, pia, fill value
            // We also need to check the user-defined fill value case.
            vector<pair<Type, int> > structure_type_element;
            const bool ret_value = is_simple_dap_structure_scalar_array(btp, structure_type_element);
            if (ret_value) {
                if (struct_size != 0)
                    dcp->add_chunk(dcp->get_byte_order(), dcp->get_fill_value(), dcp->get_fill_value_type(), chunk_size,
                                   s, struct_size);
                else
                    dcp->add_chunk(dcp->get_byte_order(), dcp->get_fill_value(), dcp->get_fill_value_type(), chunk_size,
                                   s, structure_type_element);
            }
            else
                dcp->add_chunk(dcp->get_byte_order(), dcp->get_fill_value(), dcp->get_fill_value_type(), chunk_size, s);
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
void DMZ::load_chunks(BaseType *btp) {
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
    int vlsa_found = 0;
    int missing_data_found = 0;
    int special_structure_data_found = 0;

    // Chunked data
    if (process_chunks(btp, var_node)) {
        chunks_found = 1;
        BESDEBUG(PARSER,
                 prolog << "This variable's chunks storage size is: " << dc(btp)->get_var_chunks_storage_size() << endl)
        ;
        auto array = dynamic_cast<Array *>(btp);
        // It's possible to have a chunk, but not have a chunk dimension sizes element
        // when there is only one chunk (e.g., with HDF5 Contiguous storage). jhrg 5/5/22
        if (array && !dc(btp)->get_chunk_dimension_sizes().empty()) {
            const auto &array_shape = get_array_dims(array);
            size_t num_logical_chunks = logical_chunks(array_shape, dc(btp));
            // do we need to run this code?
            if (num_logical_chunks != dc(btp)->get_chunk_count()) {
                const auto &chunk_map = get_chunk_map(dc(btp)->get_immutable_chunks());
                // Since the variable has some chunks that hold only fill values, add those chunks
                // to the vector of chunks.
                const auto &chunk_shape = dc(btp)->get_chunk_dimension_sizes();
                unsigned long long chunk_size_bytes = array->var()->width(); // start with the element size in bytes
                vector<unsigned int> s_off = dc(btp)->get_struct_offsets();
                if (!s_off.empty())
                    chunk_size_bytes = s_off.back();

                for (auto dim_size: chunk_shape)
                    chunk_size_bytes *= dim_size;
                unsigned int struct_size = (s_off.empty()) ? 0 : s_off.back();
                process_fill_value_chunks(btp, chunk_map, dc(btp)->get_chunk_dimension_sizes(),
                                          array_shape, chunk_size_bytes, struct_size);
                // Now we need to check if this var only contains one chunk.
                // If yes, we will go ahead to set one_chunk_fill_value be true. 
                // While later in process_chunks(), we will check if fillValue is defined and adjust the value.
                if (num_logical_chunks == 1)
                    dc(btp)->set_one_chunk_fill_value(true);
                dc(btp)->set_processing_fv_chunks();
            }
        }
        // If both chunks and chunk_dimension_sizes are empty, this is contiguous storage
        // with nothing but fill values. Make a single chunk that can hold the fill values.
        else if (array && dc(btp)->get_immutable_chunks().empty()) {
            const auto &array_shape = get_array_dims(array);

            // Position in array is 0, 0, ..., 0 were the number of zeros is the number of array dimensions
            shape pia(0, array_shape.size());
            auto dcp = dc(btp);

            // Since there is one chunk, the chunk size and array size are one and the same.
            unsigned long long array_size_bytes = 1;
            for (auto dim_size: array_shape)
                array_size_bytes *= dim_size;

            if (array->var()->type() == dods_str_c) {
                size_t str_size = dcp->get_fill_value().size();
                string fvalue = dcp->get_fill_value();

                // array size above is in _elements_, multiply by the element width to get bytes
                // We encounter a special case here. In one NASA file, the fillvalue='\0', so
                // when converting to string fillvalue becomes "" and the string size is 0.
                // This won't correctly pass the fillvalue buffer downstream. So here we
                // change the fillvalue to ' ' so that it can successfully generate netCDF file via fileout netcdf.
                // Also, for this special case, the string length is 1.
                // KY 2022-12-22
                if (dcp->get_fill_value() == "") {
                    fvalue = " ";
                }
                else
                    array_size_bytes *= str_size;
                dcp->add_chunk(dcp->get_byte_order(), fvalue, dcp->get_fill_value_type(), array_size_bytes, pia);
            }
            else {
                array_size_bytes *= array->var()->width();

                // We also need to check the user-defined fill value case.
                vector<pair<Type, int> > structure_type_element;
                bool ret_value = is_simple_dap_structure_scalar_array(btp, structure_type_element);
                if (ret_value)
                    dcp->add_chunk(dcp->get_byte_order(), dcp->get_fill_value(), dcp->get_fill_value_type(),
                                   array_size_bytes, pia, structure_type_element);
                else
                    dcp->add_chunk(dcp->get_byte_order(), dcp->get_fill_value(), dcp->get_fill_value_type(),
                                   array_size_bytes, pia);
            }
        }
        // This is the case when the scalar variable that holds the fill value with the contiguous storage comes. 
        // Note we only support numeric datatype now. KY 2022-07-12
        else if (btp->type() != dods_array_c && dc(btp)->get_immutable_chunks().empty()) {
            if (btp->type() == dods_grid_c || btp->type() == dods_sequence_c || btp->type() == dods_url_c) {
                ostringstream oss;
                oss << " For scalar variable with the contiguous storage that holds the fillvalue, only numeric"
                        << " types are supported.";
                throw BESInternalError(oss.str(), __FILE__, __LINE__);
            }
            shape pia;
            auto dcp = dc(btp);
            if (btp->type() == dods_str_c) {
                size_t array_size = dcp->get_fill_value().size();
                string fvalue = dcp->get_fill_value();

                // We encounter a special case here. In one NASA file, the fillvalue='\0', so
                // when converting to string fillvalue becomes "" and the string size is 0. 
                // This won't correctly pass the fillvalue buffer downstream. So here we 
                // change the fillvalue to ' ' so that it can successfully generate netCDF file via fileout netcdf.
                // KY 2022-12-22
                if (dcp->get_fill_value().empty()) {
                    fvalue = " ";
                    array_size = 1;
                }
                dcp->add_chunk(dcp->get_byte_order(), fvalue, dcp->get_fill_value_type(), array_size, pia);
            }
            else {
                vector<pair<Type, int> > structure_type_element;
                bool ret_value = is_simple_dap_structure_scalar_array(btp, structure_type_element);
                if (ret_value)
                    dcp->add_chunk(dcp->get_byte_order(), dcp->get_fill_value(), dcp->get_fill_value_type(),
                                   btp->width(), pia, structure_type_element);
                else
                    dcp->add_chunk(dcp->get_byte_order(), dcp->get_fill_value(), dcp->get_fill_value_type(),
                                   btp->width(), pia);
            }
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

    auto missing_data = var_node.child("dmrpp:missingdata");
    if (missing_data) {
        missing_data_found = 1;
        process_missing_data(btp, missing_data);
    }

    auto special_structure_data = var_node.child("dmrpp:specialstructuredata");
    if (special_structure_data) {
        special_structure_data_found = 1;
        process_special_structure_data(btp, special_structure_data);
    }

    auto vlsa_element = var_node.child(DMRPP_VLSA_ELEMENT);
    if (vlsa_element) {
        vlsa_found = 1;
        process_vlsa(btp, vlsa_element);
    }

    // Here we (optionally) check that exactly one of the supported types of node was found
    if (DmrppRequestHandler::d_require_chunks) {
        int elements_found = chunks_found + chunk_found + compact_found + vlsa_found + missing_data_found +
                             special_structure_data_found;
        if (elements_found != 1) {
            ostringstream oss;
            oss <<
                    "Expected chunk, chunks or compact or variable length string or missing data or special structure data information in the DMR++ data. Found "
                    << elements_found
                    << " types of nodes.";
            throw BESInternalError(oss.str(), __FILE__, __LINE__);
        }
    }

    dc(btp)->set_chunks_loaded(true);
}

bool DMZ::is_simple_dap_structure_scalar_array(BaseType *btp, vector<pair<Type, int> > &structure_type_element) {
    bool ret_value = false;

    if (btp->type() == dods_array_c) {
        auto t_a = dynamic_cast<Array *>(btp);
        Type t_array_var = t_a->var()->type();
        if (t_array_var == dods_structure_c) {
            auto t_s = dynamic_cast<Structure *>(t_a->var());
            ret_value = is_simple_dap_structure_internal(t_s, structure_type_element);
        }
    }
    else if (btp->type() == dods_structure_c) {
        auto t_s = dynamic_cast<Structure *>(btp);
        ret_value = is_simple_dap_structure_internal(t_s, structure_type_element);
    }

    return ret_value;
}

bool DMZ::is_simple_dap_structure_internal(const Structure *ds, vector<pair<Type, int> > &structure_type_element) {
    bool ret_value = true;
    for (const auto &bt: ds->variables()) {
        Type t_bt = bt->type();

        // Only support array or scalar of float/int.
        if (t_bt == dods_array_c) {
            auto t_a = dynamic_cast<Array *>(bt);
            Type t_array_var = t_a->var()->type();

            if (libdap::is_simple_type(t_array_var) == true && t_array_var != dods_str_c) {
                pair<Type, int> temp_pair;
                int64_t num_eles = t_a->length_ll();
                temp_pair.first = t_array_var;
                temp_pair.second = (int) (num_eles);
                structure_type_element.push_back(temp_pair);
            }
            else {
                ret_value = false;
                break;
            }
        }
        else if (libdap::is_simple_type(t_bt) == true && t_bt != dods_str_c) {
            pair<Type, int> temp_pair;
            temp_pair.first = t_bt;
            temp_pair.second = 1;
            structure_type_element.push_back(temp_pair);
        }
        else {
            ret_value = false;
            break;
        }
    }

    return ret_value;
}

void DMZ::handle_subset(DmrppArray *da, libdap::Array::Dim_iter dim_iter, unsigned long &subset_index,
                        vector<unsigned long long> &subset_pos,
                        vector<unsigned char> &subset_buf, vector<unsigned char> &whole_buf) {
    // Obtain the number of elements in each dimension 
    vector<unsigned long long> da_dims = da->get_shape(false);

    // Obtain the number of bytes of each element
    unsigned int bytes_per_elem = da->prototype()->width();

    // Obtain the start, stop and stride for this each dimension
    uint64_t start = da->dimension_start_ll(dim_iter, true);
    uint64_t stop = da->dimension_stop_ll(dim_iter, true);
    uint64_t stride = da->dimension_stride_ll(dim_iter, true);

    dim_iter++;

    // The end case for the recursion is dimIter == dim_end(); stride == 1 is an optimization
    // See the else clause for the general case.
    if (dim_iter == da->dim_end() && stride == 1) {
        // For the start and stop indexes of the subset, get the matching indexes in the whole array.
        subset_pos.push_back(start);
        unsigned long long start_index = INDEX_nD_TO_1D(da_dims, subset_pos);
        subset_pos.pop_back();

        subset_pos.push_back(stop);
        unsigned long long stop_index = INDEX_nD_TO_1D(da_dims, subset_pos);
        subset_pos.pop_back();

        // Copy data block from start_index to stop_index
        unsigned char *temp_subset_buf = subset_buf.data() + subset_index * bytes_per_elem;
        unsigned char *temp_whole_buf = whole_buf.data() + start_index * bytes_per_elem;
        size_t num_bytes_to_copy = (stop_index - start_index + 1) * bytes_per_elem;

        memcpy(temp_subset_buf, temp_whole_buf, num_bytes_to_copy);

        // Move the subset_index to the next location.
        subset_index = subset_index + (stop_index - start_index + 1);
    }
    else {
        for (uint64_t myDimIndex = start; myDimIndex <= stop; myDimIndex += stride) {
            // Is it the last dimension?
            if (dim_iter != da->dim_end()) {
                // Nope! Then we recurse to the last dimension to read stuff
                subset_pos.push_back(myDimIndex);

                // The recursive function will fill in the subset_pos until the dim_end().
                handle_subset(da, dim_iter, subset_index, subset_pos, subset_buf, whole_buf);
                subset_pos.pop_back();
            }
            else {
                // We are at the last (innermost) dimension, so it's time to copy values.
                subset_pos.push_back(myDimIndex);
                unsigned int sourceIndex = INDEX_nD_TO_1D(da_dims, subset_pos);
                subset_pos.pop_back();

                unsigned char *temp_subset_buf = subset_buf.data() + subset_index * bytes_per_elem;
                unsigned char *temp_whole_buf = whole_buf.data() + sourceIndex * bytes_per_elem;
                memcpy(temp_subset_buf, temp_whole_buf, bytes_per_elem);

                subset_index++;
            }
        }
    }
}

void DMZ::add_mblock_index(const xml_node &chunk,
                           queue<vector<pair<unsigned long long, unsigned long long> > > &mb_index_queue,
                           vector<pair<unsigned long long, unsigned long long> > &offset_length_pair) const {
    string LBIndex_value;
    for (xml_attribute attr = chunk.first_attribute(); attr; attr = attr.next_attribute()) {
        if (is_eq(attr.name(), "LinkedBlockIndex")) {
            LBIndex_value = attr.value();
            break;
        }
    }

    // We find the linked blocks in this chunk
    if (LBIndex_value.empty() == false) {
        pair<unsigned long long, unsigned long long> temp_offset_length;

        // We need to loop through the chunk attributes again to find the offset and length.
        bool found_offset = false;
        bool found_length = false;
        for (xml_attribute attr = chunk.first_attribute(); attr; attr = attr.next_attribute()) {
            if (is_eq(attr.name(), "offset")) {
                string offset = attr.value();
                temp_offset_length.first = stoull(offset);
                found_offset = true;
            }
            else if (is_eq(attr.name(), "nBytes")) {
                string size = attr.value();
                temp_offset_length.second = stoull(size);
                found_length = true;
            }
            if (found_offset && found_length)
                break;
        }

        // We make this a new chunk that stores the multiple blocks.
        if (LBIndex_value == "0") {
            if (offset_length_pair.empty() == false) {
                mb_index_queue.push(offset_length_pair);

                // Here offset_length_pair will be reused, so clear it.
                offset_length_pair.clear();
                offset_length_pair.push_back(temp_offset_length);
            }
            else
                offset_length_pair.push_back(temp_offset_length);
        }
        else
            offset_length_pair.push_back(temp_offset_length);
    }
}

void DMZ::process_multi_blocks_chunk(dmrpp::DmrppCommon *dc, const pugi::xml_node &chunk,
                                     std::queue<std::vector<std::pair<unsigned long long, unsigned long long> > > &
                                     mb_index_queue) const {
    // Follow process_chunk
    string href;
    string trust;
    string offset;
    string size;
    string chunk_position_in_array;
    string filter_mask;
    bool href_trusted = false;

    // We will only check if the last attribute is the "LinkedBlockIndex". 
    // If yes, we will check the "LinkedBlockIndex" value, mark it if it is the first index(0).
    //    If the "LinkedBlockIndex" is not 0, we simply return. The information of this linked block is retrieved from the mb_index_queue already.
    bool multi_lbs_chunk = false;
    auto LBI_attr = chunk.last_attribute();
    if (is_eq(LBI_attr.name(), "LinkedBlockIndex")) {
        string LBI_attr_value = LBI_attr.value();
        if (LBI_attr_value == "0")
            multi_lbs_chunk = true;
        else
            return;
    }
    else {
        // This should happen really rarely, still we try to cover the corner case. We loop through all the attributes and search if Linked BlockIndex is present.
        for (xml_attribute attr = chunk.first_attribute(); attr; attr = attr.next_attribute()) {
            if (is_eq(LBI_attr.name(), "LinkedBlockIndex")) {
                string LBI_attr_value = LBI_attr.value();
                if (LBI_attr_value == "0")
                    multi_lbs_chunk = true;
                else
                    return;
            }
        }
    }

    // For linked block cases, as far as we know, we don't need to load fill values as the HDF5 case. So we ignore checking and filling the fillvalue to save performance.
    for (xml_attribute attr = chunk.first_attribute(); attr; attr = attr.next_attribute()) {
        if (is_eq(attr.name(), "offset")) {
            offset = attr.value();
        }
        else if (is_eq(attr.name(), "nBytes")) {
            size = attr.value();
        }
        else if (is_eq(attr.name(), "chunkPositionInArray")) {
            chunk_position_in_array = attr.value();
        }
        else if (is_eq(attr.name(), "fm")) {
            filter_mask = attr.value();
        }
        else if (is_eq(attr.name(), "href")) {
            href = attr.value();
        }
        else if (is_eq(attr.name(), "trust") || is_eq(attr.name(), "dmrpp:trust")) {
            href_trusted = is_eq(attr.value(), "true");
        }
    }

    if (offset.empty() || size.empty())
        throw BESInternalError("Both size and offset are required for a chunk node.", __FILE__, __LINE__);

    if (multi_lbs_chunk) {
        //The chunk that has linked blocks

        vector<pair<unsigned long long, unsigned long long> > temp_pair;
        if (!mb_index_queue.empty())
            temp_pair = mb_index_queue.front();

        if (!href.empty()) {
            shared_ptr<http::url> data_url(new http::url(href, href_trusted));
            dc->add_chunk(data_url, dc->get_byte_order(), chunk_position_in_array, temp_pair);
        }
        else {
            dc->add_chunk(d_dataset_elem_href, dc->get_byte_order(), chunk_position_in_array, temp_pair);
        }
        mb_index_queue.pop(); // Remove the processed element
    }
    else {
        //General Chunk, not the linked block.
        if (!href.empty()) {
            shared_ptr<http::url> data_url(new http::url(href, href_trusted));
            dc->add_chunk(data_url, dc->get_byte_order(), stoull(size), stoull(offset), chunk_position_in_array);
        }
        else {
            dc->add_chunk(d_dataset_elem_href, dc->get_byte_order(), stoull(size), stoull(offset),
                          chunk_position_in_array);
        }
    }


    dc->accumlate_storage_size(stoull(size));
}

// Return the index of the pos in nD array to the equivalent pos in 1D array
size_t DMZ::INDEX_nD_TO_1D(const std::vector<unsigned long long> &dims,
                           const std::vector<unsigned long long> &pos) {
    //
    //  "int a[10][20][30]  // & a[1][2][3] == a + (20*30+1 + 30*2 + 1 *3)"
    //  "int b[10][2] // &b[1][1] == b + (2*1 + 1)"
    //
    if (dims.size() != pos.size())
        throw InternalErr(__FILE__,__LINE__, "dimension error in INDEX_nD_TO_1D routine.");
    size_t sum = 0;
    size_t start = 1;

    for (const auto &one_pos: pos) {
        size_t m = 1;
        for (size_t j = start; j < dims.size(); j++)
            m *= dims[j];
        sum += m * one_pos;
        start++;
    }
    return sum;
}

/// @}

} // namespace dmrpp
