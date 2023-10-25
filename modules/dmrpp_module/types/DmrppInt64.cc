
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

#include "byteswap_compat.h"
#include "DmrppInt64.h"

using namespace libdap;
using namespace std;

namespace dmrpp {

DmrppInt64 &
DmrppInt64::operator=(const DmrppInt64 &rhs)
{
    if (this == &rhs)
    return *this;

    Int64::operator=(rhs);
    DmrppCommon::operator=(rhs);

    return *this;
}

bool
DmrppInt64::read()
{
    BESDEBUG("dmrpp", "Entering " <<__PRETTY_FUNCTION__ << " for '" << name() << "'" << endl);

    if (!get_chunks_loaded())
        load_chunks(this);

    if (read_p())
        return true;

    set_value(*reinterpret_cast<dods_int64*>(read_atomic(name())));

    if ( this->twiddle_bytes() ) {
        d_buf = bswap_64(d_buf);
    }
    set_read_p(true);

    return true;

}

void
DmrppInt64::set_send_p(bool state)
{
    if (!get_attributes_loaded())
        load_attributes(this);

    Int64::set_send_p(state);
}

void DmrppInt64::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "DmrppInt64::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    DmrppCommon::dump(strm);
    Int64::dump(strm);
    strm << BESIndent::LMarg << "value:    " << d_buf << endl;
    BESIndent::UnIndent();
}

} // namespace dmrpp

