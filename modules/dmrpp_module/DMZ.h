
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

#include "config.h"

#include <string>
#include <vector>
#include <set>
#include <stack>
#include <memory>

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

class DmrppCommon;

/**
 * @brief Interface to hide the DMR++ information storage format.
 *
 * This class uses a XML library that uses the document text for all the
 * string values (tag/element names, attribute names and values, etc.,
 * so the text of the XML document must persist for as long as the xml_document
 * object itself. For files, the class uses the pugixml load function.
 * For strings, it uses a local vector<char>.
 */
class DMZ {

private:
#if 0
    // TODO add this back if needed
    std::vector<char> d_xml_text;       // Holds XML text if needed
#endif
    pugi::xml_document d_xml_doc;
    std::shared_ptr<http::url> d_dataset_elem_href;

    void m_duplicate_common(const DMZ &) {
    }

    void process_dataset(libdap::DMR *dmr, const pugi::xml_node &xml_root);
    pugi::xml_node get_variable_xml_node(libdap::BaseType *btp);
    void process_chunk(dmrpp::DmrppCommon *dc, const pugi::xml_node &chunk);
    void process_chunks(libdap::BaseType *btp, const pugi::xml_node &chunks);

    static pugi::xml_node get_variable_xml_node_helper(const pugi::xml_node &var_node, std::stack<libdap::BaseType*> &bt);
    static void build_basetype_chain(libdap::BaseType *btp, std::stack<libdap::BaseType*> &bt);

    static void process_group(libdap::DMR *dmr, libdap::D4Group *parent, const pugi::xml_node &var_node);
    static void  process_dimension(libdap::D4Group *grp, const pugi::xml_node &dimension_node);
    static void process_variable(libdap::DMR *dmr, libdap::D4Group *grp, libdap::Constructor *parent, const pugi::xml_node &var_node);
    static void process_dim(libdap::DMR *dmr, libdap::D4Group *grp, libdap::Array *array, const pugi::xml_node &dim_node);
    static void process_map(libdap::DMR *dmr, libdap::D4Group *grp, libdap::Array *array, const pugi::xml_node &map_node);
    static libdap::BaseType *build_variable(libdap::DMR *dmr, libdap::D4Group *group, libdap::Type t, const pugi::xml_node &var_node);
    static libdap::BaseType *add_scalar_variable(libdap::DMR *dmr, libdap::D4Group *group, libdap::Constructor *parent, libdap::Type t, const pugi::xml_node &var_node);
    static libdap::BaseType *add_array_variable(libdap::DMR *dmr, libdap::D4Group *grp, libdap::Constructor *parent, libdap::Type t, const pugi::xml_node &var_node);
    static void process_attribute(libdap::D4Attributes *attributes, const pugi::xml_node &dap_attr_node);

    static void process_cds_node(dmrpp::DmrppCommon *dc, const pugi::xml_node &chunks);

    void load_attributes(libdap::BaseType *btp, pugi::xml_node var_node);

    // This is for testing. jhrg 11/2/21
    void load_everything_constructor(libdap::Constructor *constructor);
    void load_everything_group(libdap::D4Group *group, bool is_root = false);

    friend class DMZTest;

public:

    /// @brief Build a DMZ without simultaneously parsing an XML document
    DMZ() = default;

    explicit DMZ(const std::string &xml_file_name);

    DMZ(const DMZ &dmz) : d_xml_doc() {
        m_duplicate_common(dmz);
    }

    virtual ~DMZ()= default;

    virtual void parse_xml_doc(const std::string &filename);
    virtual void build_thin_dmr(libdap::DMR *dmr);

    // Make these take a Variable/DmrppCommon and not a DMR
    virtual void load_attributes(libdap::BaseType *btp);

    virtual void load_chunks(libdap::BaseType *btp);
    void load_compact_data(libdap::BaseType *btp);

    std::string get_attribute_xml(std::string path);
    std::string get_variable_xml(std::string path);

    // These are for testing. jhrg 11/2/21
    void load_everything(libdap::DMR *dmr);
};

} // namespace dmrpp

#endif // h_dmz_h

