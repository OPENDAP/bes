
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

namespace dmrpp {
/**
 * Interface for the size and offset information of data described by
 * DMR++ files.
 */
class DmrppCommon {

	friend class DmrppTypeReadTest;

private:
	std::vector<H4ByteStream> d_chunk_refs;

#if 0
    // These are used only during the libcurl callback;
    // they are not duplicated by the copy ctor or assignment
    // operator.
    unsigned long long d_bytes_read;
    char *d_read_buffer;
    unsigned long long d_read_buffer_size;
#endif

protected:
    void _duplicate(const DmrppCommon &dc) {

#if 0
        // See above
    	d_bytes_read = 0;
    	d_read_buffer = 0;
    	d_read_buffer_size = 0;
#endif
    	d_chunk_refs =  dc.d_chunk_refs;
    }

    /**
     * @brief Returns a pointer to the internal vector of
     * H4ByteStream objects so that they can be manipulated.
     */
    virtual std::vector<H4ByteStream> *get_chunk_vec() {
    	return &d_chunk_refs;
    }

public:
    DmrppCommon() { }

    DmrppCommon(const DmrppCommon &dc) { _duplicate(dc); }

    virtual ~DmrppCommon() {
    	// delete[] d_read_buffer;
    }

    /**
     * @brief Add a new chunk as defined by an h4:byteStream element
     * @return The number of chunk refs (byteStreams) held.
     */
    virtual unsigned long add_chunk(std::string data_url,
    		unsigned long long size,
			unsigned long long offset,
			std::string md5,
			std::string uuid,
			std::string position_in_array = ""){

    	d_chunk_refs.push_back(
    			H4ByteStream(data_url,size,offset,md5,uuid,position_in_array)
				);
    	return d_chunk_refs.size();
    }

    virtual std::vector<H4ByteStream> get_immutable_chunks() const {
    	return d_chunk_refs;
    }

#if 0
    /**
     * @brief Get the size of this variable's data block
     */
    virtual unsigned long long get_size() const { return d_size; }

    /**
     * @brief Set the size of this variable's data block
     * @param size Size of the data in bytes
     */
    virtual void set_size(unsigned long long size) { d_size = size; }

    /**
     * @brief Get the offset to this variable's data block
     */
    virtual unsigned long long get_offset() const { return d_offset; }

    /**
     * @brief Set the offset to this variable's data block.
     * @param offset The offset to this variable's data block
     */
    virtual void set_offset(unsigned long long offset) { d_offset = offset; }


    /**
     * @brief Get the md5 string for this variable's data block
     */
    virtual std::string get_md5() const { return d_md5; }

    /**
     * @brief Set the md5 for this variable's data block.
     * @param offset The md5 of this variable's data block
     */
    virtual void set_md5(const std::string md5) { d_md5 = md5; }


    /**
     * @brief Get the uuid string for this variable's data block
     */
    // virtual std::string get_uuid() const { return d_uuid; }

    /**
     * @brief Set the uuid for this variable's data block.
     * @param offset The uuid of this variable's data block
     */
    virtual void set_uuid(const std::string uuid) { d_uuid = uuid; }


    /**
     * @brief Get the size of this variable's data block
     */
    virtual unsigned long long get_bytes_read() const { return d_bytes_read; }

    /**
     * @brief Set the size of this variable's data block
     * @param size Size of the data in bytes
     */
    virtual void set_bytes_read(unsigned long long bytes_read) { d_bytes_read = bytes_read; }

    /**
     * @brief Sets the size of the internal read buffer.
     *
     * The memory management of the read buffer is managed internal to this
     * class. This means that calling this method will release any previously
     * allocated read buffer memory and then allocates a new memory block. Since
     * this method always dumps the exiting read buffer the bytes_read counter is
     * set to zero.
     *
     * @param size Size of the internal read buffer.
     */
    virtual void rbuf_size(unsigned long long size) {

    	// Calling delete on a null pointer is fine, so we don't need to check
    	// to see if this is the first call.
    	delete[] d_read_buffer;

    	d_read_buffer = new char[size];
    	d_read_buffer_size = size;
    	set_bytes_read(0);
    }

    virtual char *get_rbuf() {
    	return d_read_buffer;
    }

    virtual unsigned long long get_rbuf_size() {
    	return d_read_buffer_size;
    }
#endif



};

} // namepsace dmrpp

#endif // _dmrpp_common_h

