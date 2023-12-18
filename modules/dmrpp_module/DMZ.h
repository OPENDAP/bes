
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

#ifndef h_dmz_h
#define h_dmz_h 1

#include <string>
#include <vector>
#include <set>
#include <stack>
#include <memory>

#define PUGIXML_NO_XPATH
#define PUGIXML_HEADER_ONLY
#include <pugixml.hpp>

#include <libdap/Type.h>

namespace libdap {
class DMR;
class BaseType;
class Array;
class D4Group;
class D4Attributes;
class Constructor;
}

namespace http {
class url;
}

namespace dmrpp {

using shape = std::vector<unsigned long long>;

class Chunk;
class DmrppCommon;

/**
 * @brief Interface to hide the DMR++ information storage format.
 *
 * This class uses an XML library that uses the document text for all the
 * string values (tag/element names, attribute names and values, etc.)
 * so the text of the XML document must persist for as long as the xml_document
 * object itself. For files, the class uses the pugixml load function.
 *
 * @note This class holds a pugi::xml_document and a shared_ptr<http::url>
 * but does not define its own copy ctor or assignment operator, so copies
 * of an instance of DMZ will share those objects
 */
class DMZ {

private:
    pugi::xml_document d_xml_doc;
    std::shared_ptr<http::url> d_dataset_elem_href;

    // Controls if teh parser will drop variables that have been flagged
    // with a dmrpp:chunks/@fillValue attribute value of "unsupported-*"
    // This is set from TheBESKeys in the DMZ's constructor.
    static bool d_elide_unsupported;

    /// Holds names of the XML elements that define variables (e.g., Byte)
    static const std::set<std::string> variable_elements;

    static void load_config_from_keys();
    void process_dataset(libdap::DMR *dmr, const pugi::xml_node &xml_root);
    static pugi::xml_node get_variable_xml_node(libdap::BaseType *btp);
    void process_chunk(dmrpp::DmrppCommon *dc, const pugi::xml_node &chunk) const;
    bool process_chunks(libdap::BaseType *btp, const pugi::xml_node &chunks) const;

    static void process_fill_value_chunks(dmrpp::DmrppCommon *dc, const std::set<shape> &chunk_map, const shape &chunk_shape,
                                   const shape &array_shape, unsigned long long chunk_size);

    static std::vector<unsigned long long int> get_array_dims(libdap::Array *array);
    static size_t logical_chunks(const std::vector<unsigned long long> &array_dim_sizes, const dmrpp::DmrppCommon *dc);
    static std::set< std::vector<unsigned long long> > get_chunk_map(const std::vector<std::shared_ptr<Chunk>> &chunks);

    static void process_compact(libdap::BaseType *btp, const pugi::xml_node &compact);
    static void process_vlsa(libdap::BaseType *btp, const pugi::xml_node &vlsa_element);

    static pugi::xml_node get_variable_xml_node_helper(const pugi::xml_node &var_node, std::stack<libdap::BaseType*> &bt);
    static void build_basetype_chain(libdap::BaseType *btp, std::stack<libdap::BaseType*> &bt);

    static void process_group(libdap::DMR *dmr, libdap::D4Group *parent, const pugi::xml_node &var_node);
    static void process_dimension(libdap::D4Group *grp, const pugi::xml_node &dimension_node);
    static void process_variable(libdap::DMR *dmr, libdap::D4Group *grp, libdap::Constructor *parent, const pugi::xml_node &var_node);
    static void process_dim(libdap::DMR *dmr, libdap::D4Group *grp, libdap::Array *array, const pugi::xml_node &dim_node);
    static void process_map(libdap::DMR *dmr, libdap::D4Group *grp, libdap::Array *array, const pugi::xml_node &map_node);
    static libdap::BaseType *build_variable(libdap::DMR *dmr, libdap::D4Group *group, libdap::Type t, const pugi::xml_node &var_node);
    static libdap::BaseType *add_scalar_variable(libdap::DMR *dmr, libdap::D4Group *group, libdap::Constructor *parent, libdap::Type t, const pugi::xml_node &var_node);
    static libdap::BaseType *add_array_variable(libdap::DMR *dmr, libdap::D4Group *grp, libdap::Constructor *parent, libdap::Type t, const pugi::xml_node &var_node);
    static void process_attribute(libdap::D4Attributes *attributes, const pugi::xml_node &dap_attr_node);

    static void process_cds_node(dmrpp::DmrppCommon *dc, const pugi::xml_node &chunks);

    void load_attributes(libdap::BaseType *btp, pugi::xml_node var_node) const;

    friend class DMZTest;

public:

    /// @brief Build a DMZ without simultaneously parsing an XML document
    DMZ() = default;

    explicit DMZ(const std::string &file_name);

    virtual ~DMZ() = default;

    // This is not virtual because we call it from a ctor
    void parse_xml_doc(const std::string &filename);

    void parse_xml_string(const std::string &contents);

    virtual void build_thin_dmr(libdap::DMR *dmr);

    virtual bool set_up_all_direct_io_flags_phase_1(libdap::DMR *dmr);
    virtual bool set_up_direct_io_flag_phase_1(libdap::D4Group *group);
    virtual bool set_up_direct_io_flag_phase_1(libdap::BaseType *btp);

    virtual void set_up_all_direct_io_flags_phase_2(libdap::DMR *dmr);
    virtual void set_up_direct_io_flag_phase_2(libdap::D4Group *group);
    virtual void set_up_direct_io_flag_phase_2(libdap::BaseType *btp);


    virtual void load_attributes(libdap::BaseType *btp);
    virtual void load_attributes(libdap::Constructor *constructor);
    virtual void load_attributes(libdap::D4Group *group);

    virtual void load_chunks(libdap::BaseType *btp);

    virtual void load_all_attributes(libdap::DMR *dmr);
};

} // namespace dmrpp

#endif // h_dmz_h

