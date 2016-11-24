
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
#include <BESDEBUG.h>

#include "DmrppUtil.h"
#include "DmrppUInt32.h"

using namespace libdap;
using namespace std;

void
DmrppUInt32::_duplicate(const DmrppUInt32 &)
{
}

DmrppUInt32::DmrppUInt32(const string &n) : UInt32(n), DmrppCommon()
{
}

DmrppUInt32::DmrppUInt32(const string &n, const string &d) : UInt32(n, d), DmrppCommon()
{
}

BaseType *
DmrppUInt32::ptr_duplicate()
{
    return new DmrppUInt32(*this);
}

DmrppUInt32::DmrppUInt32(const DmrppUInt32 &rhs) : UInt32(rhs), DmrppCommon(rhs)
{
    _duplicate(rhs);
}

DmrppUInt32 &
DmrppUInt32::operator=(const DmrppUInt32 &rhs)
{
    if (this == &rhs)
    return *this;

    dynamic_cast<UInt32 &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);
    DmrppCommon::_duplicate(rhs);

    return *this;
}

bool
DmrppUInt32::read()
{
#if 0
    BESDEBUG("dmrpp", "Entering DmrppUInt32::read for " << name() << endl);

    if (read_p())
        return true;

    // FIXME

    set_read_p(true);

    return true;
#endif
    BESDEBUG("dmrpp", "Entering " <<__PRETTY_FUNCTION__ << " for '" << name() << "'" << endl);

    if (read_p())
        return true;

    rbuf_size(sizeof(dods_uint32));

    ostringstream range;   // range-get needs a string arg for the range
    range << get_offset() << "-" << get_offset() + get_size() - 1;

    BESDEBUG("dmrpp", "Reading  " << get_data_url() << ": " << range.str() << endl);

    // Slice 'this' to just the DmrppCommon parts. Needed because the generic
    // version of the 'write_data' callback only knows about DmrppCommon. Passing
    // in a whole object like DmrppInt32 and then using reinterpret_cast<>()
    // will leave the code using garbage memory. jhrg 11/23/16
    curl_read_bytes(get_data_url(), range.str(), dynamic_cast<DmrppCommon*>(this));

    // Could use get_rbuf_size() in place of sizeof() for a more generic version.
    if (sizeof(dods_uint32) != get_bytes_read()) {
        ostringstream oss;
        oss << "DmrppInt32: Wrong number of bytes read for '" << name() << "'; expected " << sizeof(dods_uint32)
            << " but found " << get_bytes_read() << endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }

    set_value(*reinterpret_cast<dods_uint32*>(get_rbuf()));

    set_read_p(true);

    return true;

}


void DmrppUInt32::dump(ostream & strm) const
{
    strm << DapIndent::LMarg << "DmrppUInt32::dump - (" << (void *) this << ")" << endl;
    DapIndent::Indent();
    strm << DapIndent::LMarg << "offset:   " << get_offset() << endl;
    strm << DapIndent::LMarg << "size:     " << get_size() << endl;
    strm << DapIndent::LMarg << "md5:      " << get_md5() << endl;
    strm << DapIndent::LMarg << "uuid:     " << get_uuid() << endl;
    strm << DapIndent::LMarg << "data_url: " << get_data_url() << endl;
    UInt32::dump(strm);
    strm << DapIndent::LMarg << "value:    " << d_buf << endl;
    DapIndent::UnIndent();
}
