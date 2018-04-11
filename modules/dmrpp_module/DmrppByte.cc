
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
#if 0
    vector<H4ByteStream> *chunk_refs = get_chunk_vec();
    if((*chunk_refs).size() == 0){
        ostringstream oss;
        oss << "DmrppByte::read() - Unable to obtain byteStream objects for array " << name()
        		<< " Without a byteStream we cannot read! "<< endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }
    else {
		BESDEBUG("dmrpp", "DmrppByte::read() - Found H4ByteStream (chunks): " << endl);
    	for(unsigned long i=0; i<(*chunk_refs).size(); i++){
    		BESDEBUG("dmrpp", "DmrppByte::read() - chunk[" << i << "]: " << (*chunk_refs)[i].to_string() << endl);
    	}
    }

    // For now we only handle the one chunk case.
    H4ByteStream h4_byte_stream = (*chunk_refs)[0];
    h4_byte_stream.set_rbuf_to_size();
    // First cut at subsetting; read the whole thing and then subset that.
    BESDEBUG("dmrpp", "DmrppArray::read() - Reading  " << h4_byte_stream.get_size() << " bytes from "<< h4_byte_stream.get_data_url() << ": " << h4_byte_stream.get_curl_range_arg_string() << endl);

    curl_read_byte_stream(h4_byte_stream.get_data_url(), h4_byte_stream.get_curl_range_arg_string(), dynamic_cast<H4ByteStream*>(&h4_byte_stream));

    // If the expected byte count was not read, it's an error.
    if (h4_byte_stream.get_size() != h4_byte_stream.get_bytes_read()) {
        ostringstream oss;
        oss << "DmrppArray: Wrong number of bytes read for '" << name() << "'; expected " << h4_byte_stream.get_size()
            << " but found " << h4_byte_stream.get_bytes_read() << endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }

    set_value(*reinterpret_cast<dods_byte*>(h4_byte_stream.get_rbuf()));
#endif

    set_value(*reinterpret_cast<dods_byte*>(read_atomic(name())));

    set_read_p(true);

    return true;
}

void DmrppByte::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "DmrppByte::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    DmrppCommon::dump(strm);
    Byte::dump(strm);
    strm << BESIndent::LMarg << "value:    " << d_buf << endl;
    BESIndent::UnIndent();
}

} // namespace dmrpp
