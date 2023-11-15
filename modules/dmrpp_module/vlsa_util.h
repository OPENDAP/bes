// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2023 OPeNDAP
// Author: Nathan Potter <ndp@opendap.org>
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
// Created by ndp on 11/11/23.
//

#ifndef BES_VLSA_UTIL_H
#define BES_VLSA_UTIL_H

#include <string>
#include "DmrppArray.h"

namespace vlsa {

std::string zlib_msg(int retval);
std::string encode(const std::string &source_string);
std::string decode(const std::string &encoded, uint64_t expected_size);

void write_value(libdap::XMLWriter &xml, std::string &value, uint64_t dup_count);
void write(libdap::XMLWriter &xml, dmrpp::DmrppArray &a);

string read_value(const pugi::xml_node &v);
void read(const pugi::xml_node &vlsa_element, std::vector<std::string> &entries);
}
#endif //BES_VLSA_UTIL_H
