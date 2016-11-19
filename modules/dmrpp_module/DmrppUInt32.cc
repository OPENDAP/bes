
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

#include <BESDEBUG.h>

#include "DmrppUInt32.h"

using namespace libdap;
using namespace std;

void
DmrppUInt32::_duplicate(const DmrppUInt32 &)
{
}

DmrppUInt32::DmrppUInt32(const string &n) : UInt32(n), DmrppCommon()
{
}

DmrppUInt32::DmrppUInt32(const string &n, const string &d) : UInt32(n, d), DmrppCommon()
{
}

BaseType *
DmrppUInt32::ptr_duplicate()
{
    return new DmrppUInt32(*this);
}

DmrppUInt32::DmrppUInt32(const DmrppUInt32 &rhs) : UInt32(rhs), DmrppCommon(rhs)
{
    _duplicate(rhs);
}

DmrppUInt32 &
DmrppUInt32::operator=(const DmrppUInt32 &rhs)
{
    if (this == &rhs)
    return *this;

    dynamic_cast<UInt32 &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);
    DmrppCommon::_duplicate(rhs);

    return *this;
}

bool
DmrppUInt32::read()
{
    BESDEBUG("dmrpp", "Entering DmrppUInt32::read for " << name() << endl);

    if (read_p())
        return true;

    // FIXME

    set_read_p(true);

    return true;
}


void DmrppUInt32::dump(ostream & strm) const
{
    strm << DapIndent::LMarg << "DmrppUInt32::dump - (" << (void *) this << ")" << endl;
    DapIndent::Indent();
    strm << DapIndent::LMarg << "offset: " << get_offset() << endl;
    strm << DapIndent::LMarg << "size: " << get_size() << endl;
    UInt32::dump(strm);
    strm << DapIndent::LMarg << "value: " << d_buf << endl;
    DapIndent::UnIndent();
}
