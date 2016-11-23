
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

#include "DmrppArray.h"
#include "DmrppUtil.h"

using namespace libdap;
using namespace std;

void
DmrppArray::_duplicate(const DmrppArray &)
{
}

DmrppArray::DmrppArray(const string &n, BaseType *v) : Array(n, v, true /*is dap4*/), DmrppCommon()
{
}

DmrppArray::DmrppArray(const string &n, const string &d, BaseType *v) : Array(n, d, v, true), DmrppCommon()
{
}

BaseType *
DmrppArray::ptr_duplicate()
{
    return new DmrppArray(*this);
}

DmrppArray::DmrppArray(const DmrppArray &rhs) : Array(rhs), DmrppCommon(rhs)
{
    _duplicate(rhs);
}

DmrppArray &
DmrppArray::operator=(const DmrppArray &rhs)
{
    if (this == &rhs)
    return *this;

    dynamic_cast<Array &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);
    DmrppCommon::_duplicate(rhs);

    return *this;
}

// FIXME This version of read() should work for unconstrained accesses where
// we don't have to think about chunking. jhrg 11/23/16
bool
DmrppArray::read()
{
    BESDEBUG("dmrpp", "Entering " <<__PRETTY_FUNCTION__ << " for " << name() << endl);

    if (read_p())
        return true;

    unsigned long long array_nbytes = width();

    rbuf_size(array_nbytes);

    ostringstream range;   // range-get needs a string arg for the range
    range << get_offset() << "-" << get_offset() + get_size() - 1;

    BESDEBUG("dmrpp", "Reading  " << get_data_url() << ": " << range.str() << endl);

    curl_read_bytes(get_data_url(), range.str(), dynamic_cast<DmrppCommon*>(this));

    // Could use get_rbuf_size() in place of sizeof() for a more generic version.
    if (array_nbytes != get_bytes_read()) {
        ostringstream oss;
        oss << "DmrppArray: Wrong number of bytes read for '" << name() << "'; expected " << array_nbytes
            << " but found " << get_bytes_read() << endl;
        throw BESDapError(oss.str(), /*fatal*/ true, unknown_error, __FILE__, __LINE__);
    }

    val2buf(get_rbuf());    // yes, it's not type-safe

    set_read_p(true);

    return true;
}

void DmrppArray::dump(ostream & strm) const
{
    strm << DapIndent::LMarg << "DmrppArray::dump - (" << (void *) this << ")" << endl;
    DapIndent::Indent();
    strm << DapIndent::LMarg << "offset: " << get_offset() << endl;
    strm << DapIndent::LMarg << "size:   " << get_size() << endl;
    strm << DapIndent::LMarg << "md5:    " << get_md5() << endl;
    strm << DapIndent::LMarg << "uuid:   " << get_uuid() << endl;
    Array::dump(strm);
    strm << DapIndent::LMarg << "value: " << "----" << /*d_buf <<*/ endl;
    DapIndent::UnIndent();
}
