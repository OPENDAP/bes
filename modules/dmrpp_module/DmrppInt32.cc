
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

#include "DmrppInt32.h"
#include "DmrppUtil.h"

using namespace libdap;
using namespace std;

void
DmrppInt32::_duplicate(const DmrppInt32 &)
{
}

DmrppInt32::DmrppInt32(const string &n) : Int32(n), DmrppCommon()
{
}

DmrppInt32::DmrppInt32(const string &n, const string &d) : Int32(n, d), DmrppCommon()
{
}

BaseType *
DmrppInt32::ptr_duplicate()
{
    return new DmrppInt32(*this);
}

DmrppInt32::DmrppInt32(const DmrppInt32 &rhs) : Int32(rhs), DmrppCommon(rhs)
{
    _duplicate(rhs);
}

DmrppInt32 &
DmrppInt32::operator=(const DmrppInt32 &rhs)
{
    if (this == &rhs)
    return *this;

    dynamic_cast<Int32 &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);
    DmrppCommon::_duplicate(rhs);

    return *this;
}

/**
 * @brief Callback passed to libcurl to handle reading a single byte.
 *
 * This callback assumes that the size of the data is small enough
 * that all of the bytes will be either read at once or that a local
 * temporary buffer can be used to build up the values.
 *
 * @param buffer Data from libcurl
 * @param size Number of bytes
 * @param nmemb Total size of data in this call is 'size * nmemb'
 * @param data Pointer to this
 * @return The number of bytes read
 */
static size_t int32_write_data(void *buffer, size_t size, size_t nmemb, void *data)
{
    size_t nbytes = size * nmemb;

    //void *memmove(void *dst, const void *src, size_t len);
    if (sizeof(dods_int32) == nbytes) {
        DmrppInt32 *di32 = reinterpret_cast<DmrppInt32*>(data);
        di32->set_value(*reinterpret_cast<dods_int32*>(buffer));
    }
    else {
        // FIXME It could be that only one byte is read at a time... jhrg 11/22/16
        throw BESDapError("DmrppInt32: Could not read data.", /*fatal*/ true, unknown_error, __FILE__, __LINE__);
    }

    return nbytes;
}

bool
DmrppInt32::read()
{
    BESDEBUG("dmrpp", "Entering DmrppInt32::read for " << name() << endl);

    if (read_p())
        return true;

    ostringstream range;   // range-get needs a string arg for the range
    range << get_offset() << "-" << get_offset() + get_size();

    BESDEBUG("dmrpp", "Reading  " << get_data_url() << ": " << range.str() << endl);
    curl_read_bytes(get_data_url().c_str(), range.str(), int32_write_data, this);

    set_read_p(true);

    return true;
}

void DmrppInt32::dump(ostream & strm) const
{
    strm << DapIndent::LMarg << "DmrppInt32::dump - (" << (void *) this << ")" << endl;
    DapIndent::Indent();
    strm << DapIndent::LMarg << "offset: " << get_offset() << endl;
    strm << DapIndent::LMarg << "size:   " << get_size() << endl;
    strm << DapIndent::LMarg << "md5:    " << get_md5() << endl;
    strm << DapIndent::LMarg << "uuid:   " << get_uuid() << endl;
    Int32::dump(strm);
    strm << DapIndent::LMarg << "value: " << d_buf << endl;
    DapIndent::UnIndent();
}
