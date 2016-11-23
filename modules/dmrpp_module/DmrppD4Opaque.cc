
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


#include "DmrppD4Opaque.h"
#include "DmrppUtil.h"

using namespace libdap;
using namespace std;

void
DmrppD4Opaque::_duplicate(const DmrppD4Opaque &)
{
}

DmrppD4Opaque::DmrppD4Opaque(const string &n) : D4Opaque(n), DmrppCommon()
{
}

DmrppD4Opaque::DmrppD4Opaque(const string &n, const string &d) : D4Opaque(n, d), DmrppCommon()
{
}

BaseType *
DmrppD4Opaque::ptr_duplicate()
{
    return new DmrppD4Opaque(*this);
}

DmrppD4Opaque::DmrppD4Opaque(const DmrppD4Opaque &rhs) : D4Opaque(rhs), DmrppCommon(rhs)
{
    _duplicate(rhs);
}

DmrppD4Opaque &
DmrppD4Opaque::operator=(const DmrppD4Opaque &rhs)
{
    if (this == &rhs)
    return *this;

    dynamic_cast<D4Opaque &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);
    DmrppCommon::_duplicate(rhs);

    return *this;
}

bool
DmrppD4Opaque::read()
{
#if 0
    BESDEBUG("dmrpp", "Entering DmrppD4Opaque::read for " << name() << endl);

    if (read_p())
        return true;

    // FIXME

    set_read_p(true);

    return true;
#endif
    BESDEBUG("dmrpp", "Entering " <<__PRETTY_FUNCTION__ << " for '" << name() << "'" << endl);

    throw BESError("Unsupported type libdap::D4Opaque (dmrpp::DmrppOpaque)",BES_INTERNAL_ERROR, __FILE__, __LINE__);

}


void DmrppD4Opaque::dump(ostream & strm) const
{
    strm << DapIndent::LMarg << "DmrppD4Opaque::dump - (" << (void *) this << ")" << endl;
    DapIndent::Indent();
    strm << DapIndent::LMarg << "offset: " << get_offset() << endl;
    strm << DapIndent::LMarg << "size:   " << get_size() << endl;
    strm << DapIndent::LMarg << "md5:    " << get_md5() << endl;
    strm << DapIndent::LMarg << "uuid:   " << get_uuid() << endl;
    D4Opaque::dump(strm);
    strm << DapIndent::LMarg << "value: " << "----" << /*d_buf <<*/ endl;
    DapIndent::UnIndent();
}
