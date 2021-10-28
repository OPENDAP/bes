
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

#include <pugixml.hpp>

#include <libdap/Type.h>

namespace libdap {
class DMR;
class BaseType;
class Array;
class D4Group;
class D4Attributes;
}

namespace http {
class url;
}

namespace dmrpp {

class DmrppCommon;

/**
 * @brief Interface to hide the DMR++ information storage format.
 *
 */
class DMZ {

private:
    std::vector<char> d_xml_text;  // Holds XML text
    pugi::xml_document d_xml_doc;    // character type defaults to char
    shared_ptr<http::url> d_dataset_elem_href;

    void m_duplicate_common(const DMZ &) {
    }

    void process_dataset(libdap::DMR *dmr, pugi::xml_node *xml_root);
    pugi::xml_node *get_variable_xml_node(libdap::BaseType *btp);
    void process_chunk(dmrpp::DmrppCommon *dc, pugi::xml_node *chunk);
    void process_chunks(libdap::BaseType *btp, pugi::xml_node *chunks);

    static pugi::xml_node *get_variable_xml_node_helper(pugi::xml_node *var_node, std::stack<libdap::BaseType*> &bt);
    static void build_basetype_chain(libdap::BaseType *btp, std::stack<libdap::BaseType*> &bt);

    static void process_group(libdap::DMR *dmr, libdap::D4Group *parent, pugi::xml_node *var_node);
    static void  process_dimension(libdap::D4Group *grp, pugi::xml_node *dimension_node);
    static void process_variable(libdap::DMR *dmr, libdap::D4Group *grp, libdap::Constructor *parent, pugi::xml_node *var_node);
    static void process_dim(libdap::DMR *dmr, libdap::D4Group *grp, libdap::Array *array, pugi::xml_node *dim_node);
    static libdap::BaseType *build_variable(libdap::DMR *dmr, libdap::D4Group *group, libdap::Type t, pugi::xml_node *var_node);
    static libdap::BaseType *add_scalar_variable(libdap::DMR *dmr, libdap::D4Group *group, libdap::Constructor *parent, libdap::Type t, pugi::xml_node *var_node);
    static libdap::BaseType *add_array_variable(libdap::DMR *dmr, libdap::D4Group *grp, libdap::Constructor *parent, libdap::Type t, pugi::xml_node *var_node);
    static void process_attribute(libdap::D4Attributes *attributes, pugi::xml_node *dap_attr_node);

    static void process_cds_node(dmrpp::DmrppCommon *dc, pugi::xml_node *chunks);

    friend class DMZTest;

public:

    DMZ() = default;
    explicit DMZ(const std::string &xml_file_name);

    DMZ(const DMZ &dmz) : d_xml_doc() {
        m_duplicate_common(dmz);
    }

    virtual ~DMZ()= default;

    void build_thin_dmr(libdap::DMR *dmr);

    // Make these take a Variable/DmrppCommon and not a DMR
    void load_attributes(libdap::BaseType *btp);

    void load_chunks(libdap::BaseType *btp);
    void load_compact_data(libdap::BaseType *btp);

    std::string get_attribute_xml(std::string path);
    std::string get_variable_xml(std::string path);
};

} // namespace dmrpp

#endif // h_dmz_h

