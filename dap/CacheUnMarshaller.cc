// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of Hyrax, A C++ implementation of the OPeNDAP Data
// Access Protocol.

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

#include <InternalErr.h>

#include "CacheUnMarshaller.h"

using namespace libdap;

// namespace bes {

void CacheUnMarshaller::get_byte(dods_byte &val)
{
    d_in.read(reinterpret_cast<char*>(&val), sizeof(val));
}

void CacheUnMarshaller::get_int16(dods_int16 &val)
{
    d_in.read(reinterpret_cast<char*>(&val), sizeof(val));
}

void CacheUnMarshaller::get_int32(dods_int32 &val)
{
    d_in.read(reinterpret_cast<char*>(&val), sizeof(val));
}

void CacheUnMarshaller::get_float32(dods_float32 &val)
{
    d_in.read(reinterpret_cast<char*>(&val), sizeof(val));
}

void CacheUnMarshaller::get_float64(dods_float64 &val)
{
    d_in.read(reinterpret_cast<char*>(&val), sizeof(val));
}

void CacheUnMarshaller::get_uint16(dods_uint16 &val)
{
    d_in.read(reinterpret_cast<char*>(&val), sizeof(val));
}

void CacheUnMarshaller::get_uint32(dods_uint32 &val)
{
    d_in.read(reinterpret_cast<char*>(&val), sizeof(val));
}

void CacheUnMarshaller::get_str(string &val)
{
    size_t len;
    d_in.read(reinterpret_cast<char*>(&len), sizeof(size_t));
    val.resize(len);
    d_in.read(&val[0], len);
}

void CacheUnMarshaller::get_url(string &val)
{
    get_str(val);
}

/**
 * @brief Get bytes; assume the caller knows what they are doing
 * The get_opaque() and put_opaque() methods of UnMarshaller and
 * Marshaller are use by teh DAP2 Sequence class to write the positional
 * markers for start of sequence rows and the end of a sequence.
 * @param val
 * @param bytes
 */
void CacheUnMarshaller::get_opaque(char *val, unsigned int bytes)
{
    d_in.read(val, bytes);
    //throw InternalErr(__FILE__, __LINE__, "CacheUnMarshaller::get_opaque() not implemented");
}

void CacheUnMarshaller::get_int(int &val)
{
    d_in.read(reinterpret_cast<char*>(&val), sizeof(val));
}

/**
 * @brief Read a vector of bytes; assume storage is allocated by the caller
 * @param val
 * @param bytes
 * @param
 */
void CacheUnMarshaller::get_vector(char **val, unsigned int &bytes, Vector &)
{
    d_in.read(*val, bytes);
}

void CacheUnMarshaller::get_vector(char **val, unsigned int &num, int width, Vector &)
{
    d_in.read(*val, num * width);
}

void CacheUnMarshaller::dump(ostream &strm) const
{
    strm << DapIndent::LMarg << "CacheUnMarshaller::dump - (" << (void *) this << ")" << endl;
}

//} // namespace bes

