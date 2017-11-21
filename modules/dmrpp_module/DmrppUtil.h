
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

// Copyright (c) 2016 OPeNDAP, Inc.
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

#ifndef MODULES_DMRPP_MODULE_DMRPPUTIL_H_
#define MODULES_DMRPP_MODULE_DMRPPUTIL_H_

#include <string>

namespace dmrpp {

size_t h4bytestream_write_data(void *buffer, size_t size, size_t nmemb, void *data);

void curl_read_byte_stream(const std::string &url, const std::string& range, void *user_data);

void inflate(char *dest, unsigned int dest_len, char *src, unsigned int src_len);

void unshuffle(char *dest, const char *src, unsigned int src_size, unsigned int width);

} // namespace dmrpp

#endif /* MODULES_DMRPP_MODULE_DMRPPUTIL_H_ */
