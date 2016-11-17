
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

#include "DmrppInt8.h"

using namespace libdap;
using namespace std;

void
DmrppInt8::_duplicate(const DmrppInt8 &)
{
}

DmrppInt8::DmrppInt8(const string &n) : Int8(n), DmrppCommon()
{
}

DmrppInt8::DmrppInt8(const string &n, const string &d) : Int8(n, d), DmrppCommon()
{
}

BaseType *
DmrppInt8::ptr_duplicate()
{
    return new DmrppInt8(*this);
}

DmrppInt8::DmrppInt8(const DmrppInt8 &rhs) : Int8(rhs), DmrppCommon(rhs)
{
    _duplicate(rhs);
}

DmrppInt8 &
DmrppInt8::operator=(const DmrppInt8 &rhs)
{
    if (this == &rhs)
    return *this;

    dynamic_cast<Int8 &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);
    DmrppCommon::_duplicate(rhs);

    return *this;
}

bool
DmrppInt8::read()
{
    BESDEBUG("dmrpp", "Entering DmrppInt8::read for " << name() << endl);

    if (read_p())
        return true;

    // FIXME

    set_read_p(true);

    return true;
}
