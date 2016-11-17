
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

#include "DmrppFloat32.h"

using namespace libdap;
using namespace std;

void
DmrppFloat32::_duplicate(const DmrppFloat32 &)
{
}

DmrppFloat32::DmrppFloat32(const string &n) : Float32(n), DmrppCommon()
{
}

DmrppFloat32::DmrppFloat32(const string &n, const string &d) : Float32(n, d), DmrppCommon()
{
}

BaseType *
DmrppFloat32::ptr_duplicate()
{
    return new DmrppFloat32(*this);
}

DmrppFloat32::DmrppFloat32(const DmrppFloat32 &rhs) : Float32(rhs), DmrppCommon(rhs)
{
    _duplicate(rhs);
}

DmrppFloat32 &
DmrppFloat32::operator=(const DmrppFloat32 &rhs)
{
    if (this == &rhs)
    return *this;

    dynamic_cast<Float32 &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);
    DmrppCommon::_duplicate(rhs);

    return *this;
}

bool
DmrppFloat32::read()
{
    BESDEBUG("dmrpp", "Entering DmrppFloat32::read for " << name() << endl);

    if (read_p())
        return true;

    // FIXME

    set_read_p(true);

    return true;
}
