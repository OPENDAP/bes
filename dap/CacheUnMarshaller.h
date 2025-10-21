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

#ifndef cache_unmarshaller_h
#define cache_unmarshaller_h 1

#include <cassert>
#include <istream>

#include <libdap/UnMarshaller.h>   // from libdap

// namespace bes {

/**
 *  @brief UnMarshaller that knows how to deserialize dap objects
 *
 *  Use this when reading cached DAP2 data responses from disk. Unlike
 *  the XDR-based Marshaller/UnMarshaller code, this does not translate
 *  the data to network byte order (and thus does not make a copy of
 *  the data or allocate temporary memory).
 */
class CacheUnMarshaller: public libdap::UnMarshaller {
private:
    std::istream &d_in;

    CacheUnMarshaller();
    CacheUnMarshaller(const CacheUnMarshaller &um);
    CacheUnMarshaller & operator=(const CacheUnMarshaller &);

public:
    CacheUnMarshaller(std::istream &in) : UnMarshaller(), d_in(in) {
        assert(sizeof(std::streamsize) >= sizeof(int64_t));
        // This will cause exceptions to be thrown on i/o errors. The exception
        // will be ostream::failure
        d_in.exceptions(std::istream::failbit | std::istream::badbit);
    }

    virtual ~CacheUnMarshaller() { }

    virtual void get_byte(libdap::dods_byte &val);

    virtual void get_int16(libdap::dods_int16 &val);
    virtual void get_int32(libdap::dods_int32 &val);

    virtual void get_float32(libdap::dods_float32 &val);
    virtual void get_float64(libdap::dods_float64 &val);

    virtual void get_uint16(libdap::dods_uint16 &val);
    virtual void get_uint32(libdap::dods_uint32 &val);

    virtual void get_str(std::string &val);
    virtual void get_url(std::string &val);

    virtual void get_opaque(char *val, unsigned int len);
    virtual void get_int(int &val);

    virtual void get_vector(char **val, unsigned int &num, libdap::Vector &);
    virtual void get_vector(char **val, unsigned int &num, int width, libdap::Vector &);

    void dump(ostream &strm) const override;
};

//} // namespace bes

#endif // cache_unmarshaller_h

