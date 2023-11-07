// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2020 OPeNDAP, Inc.
// Author: Nathan Potter<ndp@opendap.org>
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
// Created by ndp on 12/11/19.
//

#ifndef HYRAX_KVP_UTILS_H
#define HYRAX_KVP_UTILS_H
#include <fstream>
#include <unordered_map>
#include <set>
#include <vector>


namespace kvp {


void load_keys(
        std::ifstream *keys_file,
        std::unordered_map<std::string,
        std::vector<std::string> > &keystore);

void load_keys(
        const std::string &keys_file_name,
        std::set<std::string> &loaded_kvp_files,
        std::unordered_map<std::string, std::vector<std::string> > &keystore);

void load_keys(
        const std::string &config_file,
        std::unordered_map <std::string, std::vector<std::string>>  &keystore);

bool break_pair(const char* b, std::string& key, std::string &value, bool &addto);

} // namespace kvp

#endif //HYRAX_KVP_UTILS_H
