// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2022 OPeNDAP
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
// Created by James Gallagher on 4/6/22.
//

#ifndef BES_DAPUTILS_H
#define BES_DAPUTILS_H

#include <vector>
#include <string>
#include <memory>

#include "AttrTable.h"
#include "DDS.h"
#include "DAS.h"

namespace libdap {
class DDS;
class DMR;
}

namespace dap_utils {

void log_request_and_memory_size(libdap::DDS *const *dds);

void log_request_and_memory_size(/*const*/ libdap::DMR &dmr);

void throw_for_dap4_typed_attrs(libdap::DAS *das, const std::string &file, unsigned int line);
void throw_for_dap4_typed_vars_or_attrs(libdap::DDS *dds, const std::string &file, unsigned int line);

void throw_if_dap2_response_too_big(libdap::DDS *dds, const std::string &file, unsigned int line);
void throw_if_dap4_response_too_big(libdap::DMR &dmr, const std::string &file, unsigned int line);

}
#endif //BES_DAPUTILS_H
