
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

#ifndef MODULES_DMRPP_MODULE_DMRPP_H_
#define MODULES_DMRPP_MODULE_DMRPP_H_

#include <string>
#include <DMR.h>

namespace libdap {
class XMLWriter;
}

namespace dmrpp {

/**
 * @brief Provide a way to print the DMR++ response
 */
class DMRpp : public libdap::DMR {
public:
    DMRpp() { }
    virtual ~DMRpp() { }

    virtual void print_dmrpp(libdap::XMLWriter &xml, const std::string &href ="", bool constrained = false, bool print_chunks = true);
};

} /* namespace dmrpp */

#endif /* MODULES_DMRPP_MODULE_DMRPP_H_ */
