
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

#include "DmrppInt8.h"

using namespace libdap;
using namespace std;

namespace dmrpp {

void
DmrppInt8::_duplicate(const DmrppInt8 &)
{
}

DmrppInt8::DmrppInt8(const string &n) : Int8(n), DmrppCommon()
{
}

DmrppInt8::DmrppInt8(const string &n, const string &d) : Int8(n, d), DmrppCommon()
{
}

BaseType *
DmrppInt8::ptr_duplicate()
{
    return new DmrppInt8(*this);
}

DmrppInt8::DmrppInt8(const DmrppInt8 &rhs) : Int8(rhs), DmrppCommon(rhs)
{
    _duplicate(rhs);
}

DmrppInt8 &
DmrppInt8::operator=(const DmrppInt8 &rhs)
{
    if (this == &rhs)
    return *this;

    dynamic_cast<Int8 &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);
    DmrppCommon::_duplicate(rhs);

    return *this;
}

bool
DmrppInt8::read()
{
    BESDEBUG("dmrpp", "Entering " <<__PRETTY_FUNCTION__ << " for '" << name() << "'" << endl);

    if (read_p())
        return true;

#if 0
    vector<Chunk> *chunk_refs = get_chunk_vec();
    if((*chunk_refs).size() == 0) {
        ostringstream oss;
        oss << "DmrppInt8::read() - Unable to obtain a byteStream object for DmrppInt8 " << name()
        << " Without a byteStream we cannot read! "<< endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }
    else {
        BESDEBUG("dmrpp", "DmrppInt8::read() - Found Chunk (chunks): " << endl);
        for(unsigned long i=0; i<(*chunk_refs).size(); i++) {
            BESDEBUG("dmrpp", "DmrppInt8::read() - chunk[" << i << "]: " << (*chunk_refs)[i].to_string() << endl);
        }
    }
    // For now we only handle the one chunk case.
    Chunk h4bs = (*chunk_refs)[0];
    h4bs.set_rbuf_to_size();

    // Do a range get with libcurl
    // Slice 'this' to just the DmrppCommon parts. Needed because the generic
    // version of the 'write_data' callback only knows about DmrppCommon. Passing
    // in a whole object like DmrppInt32 and then using reinterpret_cast<>()
    // will leave the code using garbage memory. jhrg 11/23/16
    BESDEBUG("dmrpp", "DmrppInt8::read() - Reading  " << h4bs.get_data_url() << ": " << h4bs.get_curl_range_arg_string() << endl);
    curl_read_byte_stream(h4bs.get_data_url(), h4bs.get_curl_range_arg_string(), dynamic_cast<Chunk*>(&h4bs));

    // Could use get_rbuf_size() in place of sizeof() for a more generic version.
    if (sizeof(dods_int8) != h4bs.get_bytes_read()) {
        ostringstream oss;
        oss << "DmrppInt8: Wrong number of bytes read for '" << name() << "'; expected " << sizeof(dods_int8)
        << " but found " << h4bs.get_bytes_read() << endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }

    set_value(*reinterpret_cast<dods_int8*>(h4bs.get_rbuf()));
#endif

    set_value(*reinterpret_cast<dods_int8*>(read_atomic(name())));

    set_read_p(true);

    return true;

}

void DmrppInt8::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "DmrppInt8::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    DmrppCommon::dump(strm);
    Int8::dump(strm);
    strm << BESIndent::LMarg << "value:    " << d_buf << endl;
    BESIndent::UnIndent();
}

} // namespace dmrpp


