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

#include <libdap/DMR.h>

#include "DMZ.h" // this includes the rapidxml header
#include "BESInternalError.h"

using namespace rapidxml;
using namespace std;
using namespace libdap;

#define prolog std::string("DMZ::").append(__func__).append("() - ")

namespace dmrpp {

#if 0
void build_thin_dmr(libdap::DMR &dmr);
void load_attributes(libdap::DMR &dmr, std::string path);
void load_chunks(libdap::DMR &dmr, std::string path);

std::string get_attribute_xml(std::string path);
std::string get_variable_xml(std::string path);
#endif

DMZ::DMZ(string file_name)
{
    ifstream ifs(file_name, ios::in | ios::binary | ios::ate);
    if (!ifs)
        throw BESInternalError(string("Could not open file: ").append(file_name), __FILE__, __LINE__);

    ifstream::pos_type file_size = ifs.tellg();
    ifs.seekg(0, ios::beg);

    d_xml_text.resize(file_size + 1LL);   // Add space for text and null termination
    ifs.read(d_xml_text.data(), file_size);
    d_xml_text[file_size] = '\0';

    d_xml_doc.parse<0>(d_xml_text.data());    // 0 means default parse flags
}

void DMZ::build_thin_dmr(DMR &dmr)
{
}

} // namespace dmrpp
