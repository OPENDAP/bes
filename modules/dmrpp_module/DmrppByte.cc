
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

#include <BESError.h>
#include <BESDebug.h>

#include "DmrppByte.h"
#include "DmrppUtil.h"

using namespace libdap;
using namespace std;

namespace dmrpp {
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

    vector<H4ByteStream> chunk_refs = get_immutable_chunks();
    if(chunk_refs.size() == 0){
        ostringstream oss;
        oss << "DmrppByte::read() - Unable to obtain a byteStream objects for array " << name()
        		<< " Without a byteStream we cannot read! "<< endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }
    else {
		BESDEBUG("dmrpp", "DmrppByte::read() - Found H4ByteStream (chunks): " << endl);
    	for(unsigned long i=0; i<chunk_refs.size(); i++){
    		BESDEBUG("dmrpp", "DmrppByte::read() - chunk[" << i << "]: " << chunk_refs[i].to_string() << endl);
    	}
    }

    // For now we only handle the one chunk case.
    H4ByteStream h4bs = chunk_refs[0];

    // Do a range get with libcurl
    BESDEBUG("dmrpp", "DmrppByte::read() - Reading  " << h4bs.get_data_url() << ": " << h4bs.get_curl_range_arg_string() << endl);
    curl_read_bytes(h4bs.get_data_url(), h4bs.get_curl_range_arg_string(), dynamic_cast<DmrppCommon*>(this));

    // Could use get_rbuf_size() in place of sizeof() for a more generic version.
    if (sizeof(dods_byte) != get_bytes_read()) {
        ostringstream oss;
        oss << "DmrppByte: Wrong number of bytes read for '" << name() << "'; expected " << sizeof(dods_byte)
            << " but found " << get_bytes_read() << endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }

    set_value(*reinterpret_cast<dods_byte*>(get_rbuf()));

    set_read_p(true);

    return true;
}

void DmrppByte::dump(ostream & strm) const
{
    strm << DapIndent::LMarg << "DmrppByte::dump - (" << (void *) this << ")" << endl;
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
    Byte::dump(strm);
    strm << DapIndent::LMarg << "value:    " << d_buf << endl;
    DapIndent::UnIndent();
}

} // namespace dmrpp
