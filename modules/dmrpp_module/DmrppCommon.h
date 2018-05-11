
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

#ifndef _dmrpp_common_h
#define _dmrpp_common_h 1

#include <string>
#include <vector>

#include "Chunk.h"
#include "CurlHandlePool.h"

namespace libdap {
    class BaseType;
}

namespace dmrpp {

/**
 * @brief Size and offset information of data included in DMR++ files.
 *
 * A mixin class the provides common behavior for the libdap types
 * when they are used with teh DMR++ handler. This includes instances
 * of the Chunk object, code to help the parser break apart the info
 * in the DMR++ XML documents, and other stuff.
 *
 * Included in this class is the read_atomic() method that reads the
 * atomic types like Byte, Int32, ... Str.
 *
 */
class DmrppCommon {

	friend class DmrppTypeReadTest;
	friend class DmrppChunkedReadTest;

private:
	bool d_deflate;
	bool d_shuffle;
	std::vector<unsigned int> d_chunk_dimension_sizes;
	std::vector<Chunk> d_chunks;

protected:
    void _duplicate(const DmrppCommon &dc) {
    	d_deflate = dc.d_deflate;
    	d_shuffle = dc.d_shuffle;
    	d_chunk_dimension_sizes = dc.d_chunk_dimension_sizes;
    	d_chunks =  dc.d_chunks;
    }

    /**
     * @brief Returns a reference to the internal Chunk vector.
     */
    virtual std::vector<Chunk> &get_chunk_vec() {
    	return d_chunks;
    }

    virtual char *read_atomic(const std::string &name);

public:
    DmrppCommon() : d_deflate(false), d_shuffle(false)
    {
    }

    DmrppCommon(const DmrppCommon &dc)
    {
        _duplicate(dc);
    }

    virtual ~DmrppCommon()
    {
    }

    /**
     * @brief Returns true if this object utilizes deflate compression.
     */
    virtual bool is_deflate_compression() const {
        return d_deflate;
    }

    /**
     * @brief Returns true if this object utilizes shuffle compression.
     */
    virtual bool is_shuffle_compression() const {
        return d_shuffle;
    }

    virtual unsigned long add_chunk(std::string data_url,
    		unsigned long long size,
			unsigned long long offset,
			std::string position_in_array = "");

    virtual const std::vector<Chunk> &get_immutable_chunks() const {
    	return d_chunks;
    }

    virtual const std::vector<unsigned int> &get_chunk_dimension_sizes() const {
    	return d_chunk_dimension_sizes;
    }

    /**
     * @brief Get the number of elements in this chunk
     *
     * @return The number of elements; multiply by element size to get the number of bytes.
     */
    virtual unsigned int get_chunk_size_in_elements() const {
        unsigned int elements = 1;
        for (std::vector<unsigned int>::const_iterator i = d_chunk_dimension_sizes.begin(),
                e = d_chunk_dimension_sizes.end(); i != e; ++i) {
            elements *= *i;
        }

        return elements;
    }

    /**
     * @brief Parses the text content of the XML element h4:chunkDimensionSizes
     * into the internal vector<unsigned int> representation.
     */
    virtual void ingest_chunk_dimension_sizes(std::string chunk_dim_sizes_string);

    /**
     * @brief Parses the text content of the XML element h4:chunkDimensionSizes
     * into the internal vector<unsigned int> representation.
     */
    virtual void ingest_compression_type(std::string compression_type_string);

    virtual void dump(std::ostream & strm) const;
};

} // namepsace dmrpp

#endif // _dmrpp_common_h

