
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
#include "DmrppD4Enum.h"

using namespace libdap;
using namespace std;

namespace dmrpp {

void
DmrppD4Enum::_duplicate(const DmrppD4Enum &)
{
}

DmrppD4Enum::DmrppD4Enum(const string &n, const string &enum_type) : D4Enum(n, enum_type), DmrppCommon()
{

}

DmrppD4Enum::DmrppD4Enum(const string &n, Type type) : D4Enum(n, type), DmrppCommon()
{
}

DmrppD4Enum::DmrppD4Enum(const string &n, const string &d, Type type) : D4Enum(n, d, type), DmrppCommon()
{
}

BaseType *
DmrppD4Enum::ptr_duplicate()
{
    return new DmrppD4Enum(*this);
}

DmrppD4Enum::DmrppD4Enum(const DmrppD4Enum &rhs) : D4Enum(rhs), DmrppCommon(rhs)
{
    _duplicate(rhs);
}

DmrppD4Enum &
DmrppD4Enum::operator=(const DmrppD4Enum &rhs)
{
    if (this == &rhs)
    return *this;

    dynamic_cast<D4Enum &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);
    DmrppCommon::_duplicate(rhs);

    return *this;
}

bool
DmrppD4Enum::read()
{
    BESDEBUG("dmrpp", "Entering " <<__PRETTY_FUNCTION__ << " for '" << name() << "'" << endl);

    if (read_p())
        return true;

    vector<H4ByteStream> *chunk_refs = get_chunk_vec();
    if((*chunk_refs).size() == 0){
        ostringstream oss;
        oss << "DmrppD4Enum::read() - Unable to obtain a byteStream object for DmrppD4Enum " << name()
        		<< " Without a byteStream we cannot read! "<< endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }
    else {
		BESDEBUG("dmrpp", "DmrppD4Enum::read() - Found H4ByteStream (chunks): " << endl);
    	for(unsigned long i=0; i<(*chunk_refs).size(); i++){
    		BESDEBUG("dmrpp", "DmrppD4Enum::read() - chunk[" << i << "]: " << (*chunk_refs)[i].to_string() << endl);
    	}
    }
    // For now we only handle the one chunk case.
    H4ByteStream h4bs = (*chunk_refs)[0];
    h4bs.set_rbuf_to_size();

    // Do a range get with libcurl
    // Slice 'this' to just the DmrppCommon parts. Needed because the generic
    // version of the 'write_data' callback only knows about DmrppCommon. Passing
    // in a whole object like DmrppInt32 and then using reinterpret_cast<>()
    // will leave the code using garbage memory. jhrg 11/23/16
    BESDEBUG("dmrpp", "DmrppD4Enum::read() - Reading  " << h4bs.get_data_url() << ": " << h4bs.get_curl_range_arg_string() << endl);
    curl_read_byteStream(h4bs.get_data_url(), h4bs.get_curl_range_arg_string(), dynamic_cast<H4ByteStream*>(&h4bs));

    // Could use get_rbuf_size() in place of sizeof() for a more generic version.
    if (sizeof(dods_enum) != h4bs.get_bytes_read()) {
        ostringstream oss;
        oss << "DmrppD4Enum: Wrong number of bytes read for '" << name() << "'; expected " << sizeof(dods_enum)
            << " but found " << h4bs.get_bytes_read() << endl;
        throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }

    set_value(*reinterpret_cast<dods_enum*>(h4bs.get_rbuf()));

    set_read_p(true);

    return true;

}

void DmrppD4Enum::dump(ostream & strm) const
{
    strm << DapIndent::LMarg << "DmrppD4Enum::dump - (" << (void *) this << ")" << endl;
    DapIndent::Indent();
    vector<H4ByteStream> chunk_refs = get_immutable_chunks();
    strm << DapIndent::LMarg << "H4ByteStreams (aka chunks): "
    		<< (chunk_refs.size()?"":"None Found.") << endl;
    DapIndent::Indent();
    for(unsigned int i=0; i<chunk_refs.size() ;i++){
        strm << DapIndent::LMarg << chunk_refs[i].to_string() << endl;
    }
    DapIndent::UnIndent();
    D4Enum::dump(strm);
    strm << DapIndent::LMarg << "value:    " << d_buf << endl;
    DapIndent::UnIndent();
}

} // namespace dmrpp
