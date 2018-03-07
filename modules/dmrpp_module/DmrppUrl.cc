
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
#include <sstream>
#include <cassert>

#include <BESError.h>
#include <BESDebug.h>

#include "DmrppUrl.h"
#include "DmrppUtil.h"

using namespace libdap;
using namespace std;

namespace dmrpp {

void
DmrppUrl::_duplicate(const DmrppUrl &)
{
}

DmrppUrl::DmrppUrl(const string &n) : Url(n), DmrppCommon()
{
}

DmrppUrl::DmrppUrl(const string &n, const string &d) : Url(n, d), DmrppCommon()
{
}

BaseType *
DmrppUrl::ptr_duplicate()
{
    return new DmrppUrl(*this);
}

DmrppUrl::DmrppUrl(const DmrppUrl &rhs) : Url(rhs), DmrppCommon(rhs)
{
    _duplicate(rhs);
}

DmrppUrl &
DmrppUrl::operator=(const DmrppUrl &rhs)
{
    if (this == &rhs)
    return *this;

    dynamic_cast<Url &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);
    DmrppCommon::_duplicate(rhs);

    return *this;
}

bool
DmrppUrl::read()
{
#if 0
    BESDEBUG("dmrpp", "Entering DmrppUrl::read for " << name() << endl);

    if (read_p())
        return true;

    // FIXME

    set_read_p(true);

    return true;
#endif
    BESDEBUG("dmrpp", "Entering " <<__PRETTY_FUNCTION__ << " for '" << name() << "'" << endl);

    throw BESError("Unsupported type libdap::D4Structure (dmrpp::DmrppStructure)",BES_INTERNAL_ERROR, __FILE__, __LINE__);


}


void DmrppUrl::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "DmrppUrl::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    DmrppCommon::dump(strm);
    Url::dump(strm);
    strm << BESIndent::LMarg << "value:    " << d_buf << endl;
    BESIndent::UnIndent();
}

} // namespace dmrpp

