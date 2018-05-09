
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

#include "config.h"

#include <string>

#include <BESError.h>
#include <BESDebug.h>

#include "DmrppUInt64.h"

using namespace libdap;
using namespace std;

namespace dmrpp {

void
DmrppUInt64::_duplicate(const DmrppUInt64 &)
{
}

DmrppUInt64::DmrppUInt64(const string &n) : UInt64(n), DmrppCommon()
{
}

DmrppUInt64::DmrppUInt64(const string &n, const string &d) : UInt64(n, d), DmrppCommon()
{
}

BaseType *
DmrppUInt64::ptr_duplicate()
{
    return new DmrppUInt64(*this);
}

DmrppUInt64::DmrppUInt64(const DmrppUInt64 &rhs) : UInt64(rhs), DmrppCommon(rhs)
{
    _duplicate(rhs);
}

DmrppUInt64 &
DmrppUInt64::operator=(const DmrppUInt64 &rhs)
{
    if (this == &rhs)
    return *this;

    dynamic_cast<UInt64 &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);
    DmrppCommon::m_duplicate_common(rhs);

    return *this;
}

bool
DmrppUInt64::read()
{
    BESDEBUG("dmrpp", "Entering " <<__PRETTY_FUNCTION__ << " for '" << name() << "'" << endl);

    if (read_p())
        return true;

    set_value(*reinterpret_cast<dods_uint64*>(read_atomic(name())));

    set_read_p(true);

    return true;

}

void DmrppUInt64::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "DmrppUInt64::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    DmrppCommon::dump(strm);
    UInt64::dump(strm);
    strm << BESIndent::LMarg << "value:    " << d_buf << endl;
    BESIndent::UnIndent();
}

} // namespace dmrpp
