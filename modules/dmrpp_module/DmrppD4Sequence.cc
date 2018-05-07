
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
#
#include <BESError.h>
#include <BESDebug.h>

#include "DmrppD4Sequence.h"

using namespace libdap;
using namespace std;

namespace dmrpp {

void
DmrppD4Sequence::_duplicate(const DmrppD4Sequence &)
{
}

DmrppD4Sequence::DmrppD4Sequence(const string &n) : D4Sequence(n), DmrppCommon()
{
}

DmrppD4Sequence::DmrppD4Sequence(const string &n, const string &d) : D4Sequence(n, d), DmrppCommon()
{
}

BaseType *
DmrppD4Sequence::ptr_duplicate()
{
    return new DmrppD4Sequence(*this);
}

DmrppD4Sequence::DmrppD4Sequence(const DmrppD4Sequence &rhs) : D4Sequence(rhs), DmrppCommon(rhs)
{
    _duplicate(rhs);
}

DmrppD4Sequence &
DmrppD4Sequence::operator=(const DmrppD4Sequence &rhs)
{
    if (this == &rhs)
    return *this;

    dynamic_cast<D4Sequence &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);
    DmrppCommon::_duplicate(rhs);

    return *this;
}

bool
DmrppD4Sequence::read()
{
    BESDEBUG("dmrpp", "Entering " <<__PRETTY_FUNCTION__ << " for '" << name() << "'" << endl);

    throw BESError("Unsupported type libdap::D4Sequence (dmrpp::DmrppSequence)",BES_INTERNAL_ERROR, __FILE__, __LINE__);
}

void DmrppD4Sequence::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "DmrppD4Sequence::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    DmrppCommon::dump(strm);
    D4Sequence::dump(strm);
    strm << BESIndent::LMarg << "value:    " << "----" << /*d_buf <<*/ endl;
    BESIndent::UnIndent();
}

} // namespace dmrpp
