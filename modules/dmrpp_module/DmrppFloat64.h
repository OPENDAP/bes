
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

#ifndef _dmrpp_float64_h
#define _dmrpp_float64_h 1

#include <string>

#include <Float64.h>
#include "DmrppCommon.h"

namespace libdap {
class XMLWriter;
}

namespace dmrpp {

class DmrppFloat64: public libdap::Float64, public DmrppCommon {
    void _duplicate(const DmrppFloat64 &ts);

public:
    DmrppFloat64(const std::string &n);
    DmrppFloat64(const std::string &n, const std::string &d);
    DmrppFloat64(const DmrppFloat64 &rhs);

    virtual ~DmrppFloat64() {}

    DmrppFloat64 &operator=(const DmrppFloat64 &rhs);

    virtual libdap::BaseType *ptr_duplicate();

    virtual bool read();

    virtual void print_dap4(libdap::XMLWriter &writer, bool constrained = false)
    {
        DmrppCommon::print_dap4(writer, constrained);
    }

    virtual void dump(ostream & strm) const;
};

} // namespace dmrpp

#endif // _dmrpp_float64_h
