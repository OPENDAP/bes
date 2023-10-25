
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

#include "BESDebug.h"
#include "BESIndent.h"

#include "byteswap_compat.h"
#include "DmrppInt32.h"

using namespace libdap;
using namespace std;

namespace dmrpp {

DmrppInt32 &
DmrppInt32::operator=(const DmrppInt32 &rhs) {
    if (this == &rhs)
        return *this;

    Int32::operator=(rhs);
    DmrppCommon::operator=(rhs);

    return *this;
}

bool
DmrppInt32::read() {
    BESDEBUG("dmrpp", "Entering " << __PRETTY_FUNCTION__ << " for '" << name() << "'" << endl);

    if (!get_chunks_loaded())
        load_chunks(this);

    if (read_p())
        return true;

    set_value(*reinterpret_cast<dods_int32 *>(read_atomic(name())));

    if (this->twiddle_bytes()) {
        d_buf = bswap_32(d_buf);
    }
    set_read_p(true);

    return true;
}

void
DmrppInt32::set_send_p(bool state) {
    if (!get_attributes_loaded())
        load_attributes(this);

    Int32::set_send_p(state);
}

void DmrppInt32::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "DmrppInt32::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    DmrppCommon::dump(strm);
    Int32::dump(strm);
    strm << BESIndent::LMarg << "value:    " << d_buf << endl;
    BESIndent::UnIndent();
}

} // namespace dmrpp

