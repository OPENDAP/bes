// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the Hyrax data server.

// Copyright (c) 2022 OPeNDAP, Inc.
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

#ifndef BES_BUILD_DMRPP_UTIL_H
#define BES_BUILD_DMRPP_UTIL_H

#include <string>


namespace dmrpp {
class DMRpp;
}

namespace build_dmrpp_util {

void add_chunk_information(const std::string &h5_file_name, dmrpp::DMRpp *dmrpp, bool disable_dio);

void build_dmrpp_from_dmr_file(const string &dmrpp_href_value, const string &dmr_filename, const string &h5_file_fqn,
                               bool add_production_metadata, const string &bes_conf_file_used_to_create_dmr, bool disable_dio, int argc, char *argv[]);

void qc_input_file(const std::string &file_name);


void inject_build_dmrpp_metadata(int argc, char **argv, const string &bes_conf_file_used_to_create_dmr, dmrpp::DMRpp *dmrpp);

void inject_build_dmrpp_metadata(dmrpp::DMRpp *dmrpp);

extern bool verbose;
}

#endif //BES_BUILD_DMRPP_UTIL_H
