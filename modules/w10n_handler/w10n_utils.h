// -*- mode: c++; c-basic-offset:4 -*-
//
// w10n_utils.h
//
// This file is part of BES w10n handler
//
// Copyright (c) 2015v OPeNDAP, Inc.
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
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#ifndef E_w10n_util_h
#define E_w10n_util_h 1

#include <string>
#include <list>
#include <iostream>

#include <DDS.h>
#include <Array.h>
#include <Constructor.h>

namespace w10n {
/** Check if the specified path is valid **/
void eval_resource_path(const string &w10nResourceId, const string &catalogRoot, const bool follow_sym_links,
    string &validPath, bool &isFile, bool &isDir, string &remainder);

std::string escape_for_json(const std::string& input);

// std::string backslash_escape(std::string source, char char_to_escape);

long computeConstrainedShape(libdap::Array *a, std::vector<unsigned int> *shape);
void checkConstructorForW10nDataCompatibility(libdap::Constructor *constructor);
void checkConstrainedDDSForW10nDataCompatibility(libdap::DDS *dds);
bool allVariablesMarkedToSend(libdap::DDS *dds);
bool allVariablesMarkedToSend(libdap::Constructor *ctor);

} // namespace w10n

#endif // E_w10n_util_h

