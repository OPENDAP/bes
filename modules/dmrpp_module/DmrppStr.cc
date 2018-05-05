
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

#include "DmrppStr.h"

using namespace libdap;
using namespace std;

namespace dmrpp {

void
DmrppStr::_duplicate(const DmrppStr &)
{
}

DmrppStr::DmrppStr(const string &n) : Str(n), DmrppCommon()
{
}

DmrppStr::DmrppStr(const string &n, const string &d) : Str(n, d), DmrppCommon()
{
}

BaseType *
DmrppStr::ptr_duplicate()
{
    return new DmrppStr(*this);
}

DmrppStr::DmrppStr(const DmrppStr &rhs) : Str(rhs), DmrppCommon(rhs)
{
    _duplicate(rhs);
}

DmrppStr &
DmrppStr::operator=(const DmrppStr &rhs)
{
    if (this == &rhs)
    return *this;

    dynamic_cast<Str &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);
    DmrppCommon::_duplicate(rhs);

    return *this;
}

bool
DmrppStr::read()
{
    BESDEBUG("dmrpp", "Entering " <<__PRETTY_FUNCTION__ << " for '" << name() << "'" << endl);

    throw BESError("Unsupported type libdap::Str (dmrpp::DmrppStr)",BES_INTERNAL_ERROR, __FILE__, __LINE__);
}

void DmrppStr::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "DmrppStr::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    DmrppCommon::dump(strm);
    Str::dump(strm);
    strm << BESIndent::LMarg << "value:    " << d_buf << endl;
    BESIndent::UnIndent();
}

} // namespace dmrpp
