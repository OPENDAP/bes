
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

#include "H4ByteStream.h"

namespace libdap {
    class BaseType;
}

namespace dmrpp {

/**
 * Interface for the size and offset information of data described by
 * DMR++ files.
 */
class DmrppCommon {

	friend class DmrppTypeReadTest;
	friend class DmrppChunkedReadTest;

private:
#if 0
	unsigned int d_deflate_level;
#endif
	bool d_compression_type_deflate;
	bool d_compression_type_shuffle;
	std::vector<unsigned int> d_chunk_dimension_sizes;
	std::vector<H4ByteStream> d_chunk_refs;

protected:
    void _duplicate(const DmrppCommon &dc) {
#if 0
        d_deflate_level = dc.d_deflate_level;
#endif

    	d_compression_type_deflate = dc.d_compression_type_deflate;
    	d_compression_type_shuffle = dc.d_compression_type_shuffle;
    	d_chunk_dimension_sizes = dc.d_chunk_dimension_sizes;
    	d_chunk_refs =  dc.d_chunk_refs;
    }

    /**
     * @brief Returns a reference to the internal H4ByteStream vector.
     */
    virtual std::vector<H4ByteStream> &get_chunk_vec() {
    	return d_chunk_refs;
    }

    virtual char *read_atomic(const std::string &name);

public:
    DmrppCommon() :
        /*d_deflate_level(0),*/ d_compression_type_deflate(false), d_compression_type_shuffle(false)
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
        return d_compression_type_deflate;
    }

    /**
     * @brief Returns true if this object utilizes shuffle compression.
     */
    virtual bool is_shuffle_compression() const {
        return d_compression_type_shuffle;
    }

#if 0
    // TODO These next two methods are not actually needed since the deflate level
    // is not used when deflating stuff. jhrg
    /**
     * @brief Sets the deflate level for this object to deflate_level.
     * @return The previous value of deflate level.
     * @deprecated Not needed to deflate data
     */
    virtual unsigned int set_deflate_level(unsigned int deflate_level){
    	unsigned int old_level = deflate_level;
    	d_deflate_level = deflate_level;
    	return old_level;
    }

    /**
     * @brief Returns the current value of this objects deflate level.
     * @deprecated Not needed to deflate data
     */
    virtual unsigned int get_deflate_level() const {
    	return d_deflate_level;
    }
#endif

    virtual unsigned long add_chunk(std::string data_url,
    		unsigned long long size,
			unsigned long long offset,
#if 0
            std::string md5,
            std::string uuid,
#endif

			std::string position_in_array = "");

    virtual const std::vector<H4ByteStream> &get_immutable_chunks() const {
    	return d_chunk_refs;
    }

    virtual std::vector<unsigned int> get_chunk_dimension_sizes() const {
    	return d_chunk_dimension_sizes;
    }

    /**
     * @brief Get the byte number of elements in this chunk
     * One use for this is set the size of a destination buffer when decompressing
     * a chunk. Because this is in Common, we don't know the size of the elements,
     * so the caller will have to get that information and use the element count
     * to determine the number of bytes to allocate for the dest buffer.
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

#if 0
    bool readAtomic()
    {
        // BESDEBUG("dmrpp", "Entering " <<__PRETTY_FUNCTION__ << " for " << name() << endl);

        if (read_p())
            return true;

        vector<H4ByteStream> *chunk_refs = get_chunk_vec();
        if((*chunk_refs).size() == 0){
            ostringstream oss;
            oss << "DmrppByte::read() - Unable to obtain byteStream objects for " << name()
            		<< " Without a byteStream we cannot read! "<< endl;
            throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
        }
        else {
    		//BESDEBUG("dmrpp", "DmrppByte::read() - Found H4ByteStream (chunks): " << endl);
        	for(unsigned long i=0; i<(*chunk_refs).size(); i++){
        		//BESDEBUG("dmrpp", "DmrppByte::read() - chunk[" << i << "]: " << (*chunk_refs)[i].to_string() << endl);
        	;
        	}
        }

        // For now we only handle the one chunk case.
        H4ByteStream h4_byte_stream = (*chunk_refs)[0];
        h4_byte_stream.set_rbuf_to_size();
        // First cut at subsetting; read the whole thing and then subset that.
       // BESDEBUG("dmrpp", "DmrppArray::read() - Reading  " << h4_byte_stream.get_size() << " bytes from "<< h4_byte_stream.get_data_url() << ": " << h4_byte_stream.get_curl_range_arg_string() << endl);

        curl_read_byte_stream(h4_byte_stream.get_data_url(), h4_byte_stream.get_curl_range_arg_string(), dynamic_cast<H4ByteStream*>(&h4_byte_stream));

        // If the expected byte count was not read, it's an error.
        if (h4_byte_stream.get_size() != h4_byte_stream.get_bytes_read()) {
            ostringstream oss;
            oss << "DmrppArray: Wrong number of bytes read for '" << name() << "'; expected " << h4_byte_stream.get_size()
                << " but found " << h4_byte_stream.get_bytes_read() << endl;
            throw BESError(oss.str(), BES_INTERNAL_ERROR, __FILE__, __LINE__);
        }

        set_value(*reinterpret_cast<dods_byte*>(h4_byte_stream.get_rbuf()));

        set_read_p(true);

        return true;
    }
#endif
};

} // namepsace dmrpp

#endif // _dmrpp_common_h

