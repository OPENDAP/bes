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


#ifndef cache_marshaller_h
#define cache_marshaller_h 1

#include <iostream>

#include <libdap/Marshaller.h>     // from libdap

// namespace bes {

/**
 * @brief Marshaller that knows how serialize dap data objects to a disk cache
 * This class can be used with libdap::BaseType::serialize() to write out
 * data values. Unlike the XDR-based code used with DAP2, this does not
 * translate data to network byte order and thus eliminates copy and memory
 * allocation operations.
 */
class CacheMarshaller: public libdap::Marshaller {
private:
    ostream &d_out;

    CacheMarshaller();
    CacheMarshaller(const CacheMarshaller &m);
    CacheMarshaller &operator=(const CacheMarshaller &);

    void put_vector(char *val, unsigned int num, int width, libdap::Type);

public:
    CacheMarshaller(ostream &out): Marshaller(), d_out(out) { }
    virtual ~CacheMarshaller() { }

    virtual void put_byte(libdap::dods_byte val);

    virtual void put_int16(libdap::dods_int16 val);
    virtual void put_int32(libdap::dods_int32 val);

    virtual void put_float32(libdap::dods_float32 val);
    virtual void put_float64(libdap::dods_float64 val);

    virtual void put_uint16(libdap::dods_uint16 val);
    virtual void put_uint32(libdap::dods_uint32 val);

    virtual void put_str(const string &val);
    virtual void put_url(const string &val);

    virtual void put_opaque(char *val, unsigned int len);
    virtual void put_int(int val);

    virtual void put_vector(char *val, int num, libdap::Vector &);
    virtual void put_vector(char *val, int num, int width, libdap::Vector &);

    virtual void put_vector_start(int num);
    virtual void put_vector_part(char *val, unsigned int num, int width, libdap::Type);
    virtual void put_vector_end();

    void dump(ostream &strm) const override;
};

// } // namespace bes

#endif // cache_marshaller_h

