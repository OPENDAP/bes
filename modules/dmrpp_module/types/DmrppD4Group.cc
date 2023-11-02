
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

//#include "config.h"

#include <string>

#include <libdap/D4Attributes.h>
#include <libdap/D4Maps.h>
#include <libdap/D4Group.h>
#include <libdap/XMLWriter.h>

#include <BESError.h>

#include "DmrppD4Group.h"

using namespace libdap;
using namespace std;

namespace dmrpp {

DmrppD4Group &
DmrppD4Group::operator=(const DmrppD4Group &rhs)
{
    if (this == &rhs)
    return *this;

    D4Group::operator=(rhs);
    DmrppCommon::operator=(rhs);

    return *this;
}

void
DmrppD4Group::set_send_p(bool state)
{
    if (state && !get_attributes_loaded())
        load_attributes(this);

    D4Group::set_send_p(state);
}

void DmrppD4Group::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "DmrppD4Group::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    DmrppCommon::dump(strm);
    D4Group::dump(strm);
    strm << BESIndent::LMarg << "value:    " << "----" << /*d_buf <<*/ endl;
    BESIndent::UnIndent();
}

} // namespace dmrpp
