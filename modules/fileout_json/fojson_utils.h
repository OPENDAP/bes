// -*- mode: c++; c-basic-offset:4 -*-
//
// utils.h
//
// This file is part of BES JSON File Out Module
//
// Copyright (c) 2014 OPeNDAP, Inc.
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
// (c) COPYRIGHT URI/MIT 1995-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#ifndef FOJSON_UTILS_H_
#define FOJSON_UTILS_H_ 1

#include <string>
#include <vector>

#include <Array.h>

namespace fojson {

std::string escape_for_json(const std::string &source);

long computeConstrainedShape(libdap::Array *a, std::vector<unsigned int> *shape );

#if 0
std::string backslash_escape(std::string source, char char_to_escape);
#endif

} // namespace fojson



#endif /* FOJSON_UTILS_H_ */


