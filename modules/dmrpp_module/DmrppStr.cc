
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

#include "BESError.h"
#include "BESInternalError.h"
#include "BESDebug.h"

#include "Chunk.h"
#include "DmrppStr.h"

using namespace libdap;
using namespace std;

namespace dmrpp {

DmrppStr &
DmrppStr::operator=(const DmrppStr &rhs)
{
    if (this == &rhs)
    return *this;

    dynamic_cast<Str &>(*this) = rhs; // run Constructor=

    dynamic_cast<DmrppCommon &>(*this) = rhs;
    //DmrppCommon::m_duplicate_common(rhs);

    return *this;
}

bool
DmrppStr::read() {
    if (!get_chunks_loaded())
        load_chunks(this);

    if (read_p())
        return true;

    // The following replaces a call to DmrppCommon::read_atomic() because the
    // string code requires special processing of the data array and requires information
    // about the chunk size to produce correct answers.

    if (get_chunks_size() != 1)
        throw BESInternalError(string("Expected only a single chunk for variable ") + name(), __FILE__, __LINE__);

    auto chunk = get_immutable_chunks()[0];
    chunk->read_chunk();
    auto chunk_size = chunk->get_size();
    const char *data = chunk->get_rbuf();

    // It is possible that the string data is not null terminated and/or
    // Does not span the full width of the chunk.
    // This should correct those issues.
    unsigned long long str_len = 0;
    // Find the length of the string. jhrg 2/7/24
#if 0
    bool done = false;
    while (!done) {
        done = (data[str_len] == 0) || (str_len >= chunk_size);
        if (!done) str_len++;
    }
#endif
    while (str_len < chunk_size && data[str_len] != 0) {
        str_len++;
    }
#if 0
    string value(data, str_len);
    set_value(value);   // sets read_p too
#endif
    set_value(string{data, str_len});   // sets read_p too
    return true;
}

void
DmrppStr::set_send_p(bool state)
{
    if (!get_attributes_loaded())
        load_attributes(this);

    Str::set_send_p(state);
}

void DmrppStr::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "DmrppStr::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    DmrppCommon::dump(strm);
    Str::dump(strm);
    strm << BESIndent::LMarg << "value:    " << d_buf << endl;
    BESIndent::UnIndent();
}

} // namespace dmrpp
