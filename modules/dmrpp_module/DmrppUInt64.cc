
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
#include <BESDebug.h>

#include "DmrppUtil.h"
#include "DmrppUInt64.h"

using namespace libdap;
using namespace std;

namespace dmrpp {

void
DmrppUInt64::_duplicate(const DmrppUInt64 &)
{
}

DmrppUInt64::DmrppUInt64(const string &n) : UInt64(n), DmrppCommon()
{
}

DmrppUInt64::DmrppUInt64(const string &n, const string &d) : UInt64(n, d), DmrppCommon()
{
}

BaseType *
DmrppUInt64::ptr_duplicate()
{
    return new DmrppUInt64(*this);
}

DmrppUInt64::DmrppUInt64(const DmrppUInt64 &rhs) : UInt64(rhs), DmrppCommon(rhs)
{
    _duplicate(rhs);
}

DmrppUInt64 &
DmrppUInt64::operator=(const DmrppUInt64 &rhs)
{
    if (this == &rhs)
    return *this;

    dynamic_cast<UInt64 &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);
    DmrppCommon::_duplicate(rhs);

    return *this;
}

bool
DmrppUInt64::read()
{
    BESDEBUG("dmrpp", "Entering " <<__PRETTY_FUNCTION__ << " for '" << name() << "'" << endl);

    if (read_p())
        return true;

    rbuf_size(sizeof(dods_uint64));

    vector<H4ByteStream> chunk_refs = get_immutable_chunks();
    if(chunk_refs.size() == 0){
        ostringstream oss;
        oss << "DmrppUInt64::read() - Unable to obtain a byteStream object for DmrppUInt64 " << name()
        		<< " Without a byteStream we cannot read! "<< endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }
    else {
		BESDEBUG("dmrpp", "DmrppUInt64::read() - Found H4ByteStream (chunks): " << endl);
    	for(unsigned long i=0; i<chunk_refs.size(); i++){
    		BESDEBUG("dmrpp", "DmrppUInt64::read() - chunk[" << i << "]: " << chunk_refs[i].to_string() << endl);
    	}
    }
    // For now we only handle the one chunk case.
    H4ByteStream h4bs = chunk_refs[0];
    // Do a range get with libcurl
    // Slice 'this' to just the DmrppCommon parts. Needed because the generic
    // version of the 'write_data' callback only knows about DmrppCommon. Passing
    // in a whole object like DmrppInt32 and then using reinterpret_cast<>()
    // will leave the code using garbage memory. jhrg 11/23/16
    BESDEBUG("dmrpp", "DmrppUInt64::read() - Reading  " << h4bs.get_data_url() << ": " << h4bs.get_curl_range_arg_string() << endl);
    curl_read_bytes(h4bs.get_data_url(), h4bs.get_curl_range_arg_string(), dynamic_cast<DmrppCommon*>(this));

    // Could use get_rbuf_size() in place of sizeof() for a more generic version.
    if (sizeof(dods_uint64) != get_bytes_read()) {
        ostringstream oss;
        oss << "DmrppUInt64: Wrong number of bytes read for '" << name() << "'; expected " << sizeof(dods_uint64)
            << " but found " << get_bytes_read() << endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }

    set_value(*reinterpret_cast<dods_uint64*>(get_rbuf()));

    set_read_p(true);

    return true;

}

void DmrppUInt64::dump(ostream & strm) const
{
    strm << DapIndent::LMarg << "DmrppUInt64::dump - (" << (void *) this << ")" << endl;
    DapIndent::Indent();
#if 0
    strm << DapIndent::LMarg << "offset:   " << get_offset() << endl;
    strm << DapIndent::LMarg << "size:     " << get_size() << endl;
    strm << DapIndent::LMarg << "md5:      " << get_md5() << endl;
    strm << DapIndent::LMarg << "uuid:     " << get_uuid() << endl;
    strm << DapIndent::LMarg << "data_url: " << get_data_url() << endl;
#endif
    vector<H4ByteStream> chunk_refs = get_immutable_chunks();
    strm << DapIndent::LMarg << "H4ByteStreams (aka chunks):"
    		<< (chunk_refs.size()?"":"None Found.") << endl;
    DapIndent::Indent();
    for(unsigned int i=0; i<chunk_refs.size() ;i++){
        strm << DapIndent::LMarg << chunk_refs[i].to_string() << endl;
    }
    DapIndent::UnIndent();
    UInt64::dump(strm);
    strm << DapIndent::LMarg << "value:    " << d_buf << endl;
    DapIndent::UnIndent();
}

} // namespace dmrpp
