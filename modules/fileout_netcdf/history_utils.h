// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the Hyrax data server.

// Copyright (c) 2021 OPeNDAP, Inc.
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#ifndef HYRAX_GIT_HISTORY_UTILS_H
#define HYRAX_GIT_HISTORY_UTILS_H

#include <DDS.h>
#include <DMR.h>
#include <D4Attributes.h>

// OLD
void updateHistoryAttributes(libdap::DDS *dds, const string &ce);

// NEW
void updateHistoryAttributes(libdap::DMR *dmr, const std::string &ce);

void update_cf_history_attr(libdap::D4Attribute *global_attribute, const std::string &request_url);
void update_history_json_attr(libdap::D4Attribute *global_attribute, const std::string &request_url);

std::string create_cf_history_txt(const std::string &request_url);
std::string get_cf_history_entry (const std::string &request_url);

std::string get_history_json_entry (const std::string &request_url);

std::string json_append_entry_to_array(const std::string &current_doc_str, const std::string &new_entry_str);
#endif //HYRAX_GIT_HISTORY_UTILS_H
