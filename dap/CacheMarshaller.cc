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

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#include <cassert>

#include <iostream>
#include <sstream>
#include <iomanip>

#include <Vector.h>

#include "CacheMarshaller.h"

using namespace std;
using namespace libdap;

// Build this code so it does not use pthreads to write some kinds of
// data (see the put_vector() and put_vector_part() methods) in a child thread.
// #undef USE_POSIX_THREADS

// namespace bes {

void CacheMarshaller::put_byte(dods_byte val)
{
    d_out.write(reinterpret_cast<char*>(&val), sizeof(val));
}

void CacheMarshaller::put_int16(dods_int16 val)
{
    d_out.write(reinterpret_cast<char*>(&val), sizeof(val));
}

void CacheMarshaller::put_int32(dods_int32 val)
{
    d_out.write(reinterpret_cast<char*>(&val), sizeof(val));
}

void CacheMarshaller::put_float32(dods_float32 val)
{
    d_out.write(reinterpret_cast<char*>(&val), sizeof(val));
}

void CacheMarshaller::put_float64(dods_float64 val)
{
    d_out.write(reinterpret_cast<char*>(&val), sizeof(val));
}

void CacheMarshaller::put_uint16(dods_uint16 val)
{
    d_out.write(reinterpret_cast<char*>(&val), sizeof(val));
}

void CacheMarshaller::put_uint32(dods_uint32 val)
{
    d_out.write(reinterpret_cast<char*>(&val), sizeof(val));
}

void CacheMarshaller::put_str(const string &val)
{
    size_t len = val.length();
    d_out.write(reinterpret_cast<const char*>(&len), sizeof(size_t));
    d_out.write(val.data(), val.length());
}

void CacheMarshaller::put_url(const string &val)
{
    put_str(val);
}

void CacheMarshaller::put_opaque(char *val, unsigned int len)
{
    d_out.write(val, len);
}

void CacheMarshaller::put_int(int val)
{
   d_out.write(reinterpret_cast<char*>(&val), sizeof(val));
}

void CacheMarshaller::put_vector(char *val, int num, int width, Vector &vec)
{
    put_vector(val, num, width, vec.var()->type());
}


/**
 * Prepare to send a single array/vector using a series of 'put' calls.
 *
 * @param num The number of elements in the Array/Vector
 * @see put_vector_part()
 * @see put_vector_end()
 */
void CacheMarshaller::put_vector_start(int num)
{
    put_int(num);
}

/**
 * Close a vector when its values are written using put_vector_part().
 *
 * @see put_vector_start()
 * @see put_vector_part()
 */
void CacheMarshaller::put_vector_end()
{
}

// Start of parallel I/O support. jhrg 8/19/15
void CacheMarshaller::put_vector(char *val, int num, Vector &)
{
    assert(val || num == 0);

    // write the number of array members being written, then set the position back to 0
    put_int(num);

    if (num == 0)
        return;

    d_out.write(val, num);
}

// private
/**
 * Write elements of a Vector (i.e. an Array) to the stream using XDR encoding.
 * Encoding is performed on 'num' values that use 'width' bytes. The parameter
 * 'type' is used to choose the XDR encoding function.
 *
 * @param val Pointer to the values to write
 * @param num The number of elements in the memory referenced by 'val'
 * @param width The number of bytes in each element
 * @param type The DAP type of the elements
 */
void CacheMarshaller::put_vector(char *val, unsigned int num, int width, Type)
{
    assert(val || num == 0);

    // write the number of array members being written, then set the position back to 0
    put_int(num);

    if (num == 0)
        return;

    d_out.write(val, num * width);
}

/**
 * Write num values for an Array/Vector.
 *
 * @param val The values to write
 * @param num the number of values to write
 * @param width The width of the values
 * @param type The DAP2 type of the values.
 *
 * @see put_vector_start()
 * @see put_vector_end()
 */
void CacheMarshaller::put_vector_part(char *val, unsigned int num, int width, Type )
{
    d_out.write(val, num * width);
}

void CacheMarshaller::dump(ostream &strm) const
{
    strm << DapIndent::LMarg << "CacheMarshaller::dump - (" << (void *) this << ")" << endl;
}

//} // namespace bes

