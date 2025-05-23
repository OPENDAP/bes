
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

#include "DmrppD4Enum.h"

using namespace libdap;
using namespace std;

namespace dmrpp {

DmrppD4Enum &
DmrppD4Enum::operator=(const DmrppD4Enum &rhs)
{
    if (this == &rhs)
    return *this;

    dynamic_cast<D4Enum &>(*this) = rhs; // run Constructor=

    dynamic_cast<DmrppCommon &>(*this) = rhs;
    //DmrppCommon::m_duplicate_common(rhs);

    return *this;
}

bool
DmrppD4Enum::read()
{
    BESDEBUG("dmrpp", "Entering " <<__PRETTY_FUNCTION__ << " for '" << name() << "'" << endl);

    if (!get_chunks_loaded())
        load_chunks(this);

    if (read_p())
        return true;

    char *enum_buf = read_atomic(name());
    val2buf((void*)enum_buf,true);
    set_read_p(true);
     

    return true;

}

void
DmrppD4Enum::set_send_p(bool state)
{
    if (!get_attributes_loaded())
        load_attributes(this);

    D4Enum::set_send_p(state);
}

void DmrppD4Enum::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "DmrppD4Enum::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    DmrppCommon::dump(strm);
    D4Enum::dump(strm);
    strm << BESIndent::LMarg << "value:    " << d_buf << endl;
    BESIndent::UnIndent();
}

} // namespace dmrpp
