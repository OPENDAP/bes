
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

#include <BESDapError.h>
#include <BESDEBUG.h>

#include "DmrppByte.h"
#include "DmrppUtil.h"

using namespace libdap;
using namespace std;

void
DmrppByte::_duplicate(const DmrppByte &)
{
}

DmrppByte::DmrppByte(const string &n) : Byte(n), DmrppCommon()
{
}

DmrppByte::DmrppByte(const string &n, const string &d) : Byte(n, d), DmrppCommon()
{
}

BaseType *
DmrppByte::ptr_duplicate()
{
    return new DmrppByte(*this);
}

DmrppByte::DmrppByte(const DmrppByte &rhs) : Byte(rhs), DmrppCommon(rhs)
{
    _duplicate(rhs);
}

DmrppByte &
DmrppByte::operator=(const DmrppByte &rhs)
{
    if (this == &rhs)
	return *this;

    dynamic_cast<Byte &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);
    DmrppCommon::_duplicate(rhs);

    return *this;
}

bool DmrppByte::read()
{
    BESDEBUG("dmrpp", "Entering " <<__PRETTY_FUNCTION__ << " for " << name() << endl);

    if (read_p())
        return true;

    rbuf_size(sizeof(dods_byte));
    set_bytes_read(0);

    ostringstream range;   // range-get needs a string arg for the range
    range << get_offset() << "-" << get_offset() + get_size();

    BESDEBUG("dmrpp", "Reading  " << get_data_url() << ": " << range.str() << endl);

    curl_read_bytes(get_data_url(), range.str(), this);

    // Could use get_rbuf_size() in place of sizeof() for a more generic version.
    if (sizeof(dods_byte) != get_bytes_read()) {
        ostringstream oss;
        oss << "DmrppInt32: Wrong number of bytes read for '" << name() << "'; expected " << sizeof(dods_byte)
            << " but found " << get_bytes_read() << endl;
        throw BESDapError(oss.str(), /*fatal*/ true, unknown_error, __FILE__, __LINE__);
    }

    set_value(*reinterpret_cast<dods_byte*>(get_rbuf()));

    set_read_p(true);

    return true;
}

void DmrppByte::dump(ostream & strm) const
{
    strm << DapIndent::LMarg << "DmrppByte::dump - (" << (void *) this << ")" << endl;
    DapIndent::Indent();
    strm << DapIndent::LMarg << "offset: " << get_offset() << endl;
    strm << DapIndent::LMarg << "size:   " << get_size() << endl;
    strm << DapIndent::LMarg << "md5:    " << get_md5() << endl;
    strm << DapIndent::LMarg << "uuid:   " << get_uuid() << endl;
    Byte::dump(strm);
    strm << DapIndent::LMarg << "value: " << d_buf << endl;
    DapIndent::UnIndent();
}
