
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

#ifndef _dmrpp_array_h
#define _dmrpp_array_h 1

#include <string>

#include <Array.h>
#include "DmrppCommon.h"
#include "Odometer.h"

namespace dmrpp {

class DmrppArray: public libdap::Array, public DmrppCommon {
    void _duplicate(const DmrppArray &ts);

    bool is_projected();

private:
    void read_constrained_no_chunks(
			Dim_iter p,
			unsigned long *target_index,
			vector<unsigned int> &subsetAddress,
			const vector<unsigned int> &array_shape,
			H4ByteStream *h4bytestream);

    virtual bool read_no_chunks();
    virtual bool read_chunked();


public:
    DmrppArray(const std::string &n, libdap::BaseType *v);
    DmrppArray(const std::string &n, const std::string &d, libdap::BaseType *v);
    DmrppArray(const DmrppArray &rhs);

    virtual ~DmrppArray() {}

    DmrppArray &operator=(const DmrppArray &rhs);

    virtual libdap::BaseType *ptr_duplicate();

    virtual bool read();

    virtual void dump(ostream & strm) const;

    virtual unsigned long long get_size(bool constrained=false);
    virtual vector<unsigned int> get_shape(bool constrained);

};

} // namespace dmrpp

#endif // _dmrpp_array_h

