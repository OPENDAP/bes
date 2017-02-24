
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

#ifndef _dmrpp_array_h
#define _dmrpp_array_h 1

#include <string>
#include <map>

#include <curl/curl.h>

#include <Array.h>
#include "DmrppCommon.h"
#include "Odometer.h"

namespace dmrpp {

/**
 * @brief Extend libdap::Array so that a handler can read data using a DMR++ file.
 *
 * @note A key feature of HDF5 is that is can 'chunk' data, breaking up an
 * array into a number of smaller pieces, each of which can be compressed.
 * This code will read both array data that are chunked (and possibly compressed)
 * as well as code that is not (essentially the entire array is written in a
 * single 'chunk'). Because the two cases are different and susceptible to
 * different kinds of optimizations, we have implemented two different read()
 * methods, one for the 'no chunks' case and one for arrays 'with chunks.'
 */
class DmrppArray: public libdap::Array, public DmrppCommon {
    void _duplicate(const DmrppArray &ts);

    bool is_projected();

private:
    DmrppArray::dimension get_dimension(unsigned int dim_num);

    virtual bool read_no_chunks();
    virtual bool read_chunks();

    void insert_constrained_no_chunk(
			Dim_iter p,
			unsigned long *target_index,
			vector<unsigned int> &subsetAddress,
			const vector<unsigned int> &array_shape,
			H4ByteStream *h4bytestream);

    virtual bool insert_constrained_chunk(
    		unsigned int dim,
			vector<unsigned int> *target_address,
			vector<unsigned int> *chunk_source_address,
    		H4ByteStream *chunk,
    		CURLM *multi_handle);

    void multi_finish(CURLM *curl_multi_handle, vector<H4ByteStream> *chunk_refs);

public:
    DmrppArray(const std::string &n, libdap::BaseType *v);
    DmrppArray(const std::string &n, const std::string &d, libdap::BaseType *v);
    DmrppArray(const DmrppArray &rhs);

    virtual ~DmrppArray() {}

    DmrppArray &operator=(const DmrppArray &rhs);

    virtual libdap::BaseType *ptr_duplicate();

    virtual bool read();

    virtual void dump(ostream & strm) const;

    virtual unsigned long long get_size(bool constrained=false);
    virtual vector<unsigned int> get_shape(bool constrained);
};

} // namespace dmrpp

#endif // _dmrpp_array_h

