
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

#ifndef _dmz_h
#define _dmz_h 1

#include "config.h"

#include <string>
#include <vector>
#include <set>

#include "rapidxml/rapidxml.hpp"

#include <libdap/Type.h>

namespace libdap {
class DMR;
class BaseType;
class Array;
class D4Group;
}

namespace http {
class url;
}

namespace dmrpp {

/**
 * @brief Interface to hide the DMR++ information storage format.
 *
 */
class DMZ {

private:
    std::vector<char> d_xml_text;  // Holds XML text
    rapidxml::xml_document<> d_xml_doc{};    // character type defaults to char

    http::url *d_dataset_elem_href;

    void m_duplicate_common(const DMZ &) {
    }

    void process_dataset(libdap::DMR *dmr, rapidxml::xml_node<> *xml_root);
    void process_dim(libdap::DMR *dmr, libdap::D4Group *grp, libdap::Array *arry, rapidxml::xml_node<> *dim_node);
    void process_variable(libdap::DMR *dmr, libdap::D4Group *grp, rapidxml::xml_node<> *var_node);
    libdap::BaseType *build_scalar_variable(libdap::DMR *dmr, libdap::D4Group *grp, libdap::Type t, rapidxml::xml_node<> *var_node);
    void add_scalar_variable(libdap::DMR *dmr, libdap::D4Group *grp, libdap::Type t, rapidxml::xml_node<> *var_node);
    void add_array_variable(libdap::DMR *dmr, libdap::D4Group *grp, libdap::Type t, rapidxml::xml_node<> *var_node);
    void process_group(libdap::DMR *dmr, libdap::D4Group *parent, rapidxml::xml_node<> *var_node);
    void  process_dimension(libdap::D4Group *grp, rapidxml::xml_node<> *dimension_node);

    friend class DMZTest;

public:

    DMZ() = default;
    explicit DMZ(std::string xml_file_name);

    DMZ(const DMZ &dmz) : d_xml_doc() {
        m_duplicate_common(dmz);
    }

    virtual ~DMZ()= default;

    void build_thin_dmr(libdap::DMR *dmr);

    // Make these take a Variable/DmrppCommon and not a DMR
    void load_attributes(libdap::DMR &dmr, std::string path);
    void load_chunks(libdap::DMR &dmr, std::string path);
    void load_compact_data();

    std::string get_attribute_xml(std::string path);
    std::string get_variable_xml(std::string path);
};

} // namepsace dmrpp

#endif // _dmz_h

