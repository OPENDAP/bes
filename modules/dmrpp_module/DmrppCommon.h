
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

/**
 * Interface for the size and offset information of data described by
 * DMR++ files.
 */
class DmrppCommon {
    unsigned long long d_size;
    unsigned long long d_offset;
    std::string d_md5;
    std::string d_uuid;

    std::string d_data_url;

protected:
    void _duplicate(const DmrppCommon &dc) {
        d_size   = dc.d_size;
        d_offset = dc.d_offset;
        d_md5    = dc.d_md5;
        d_uuid   = dc.d_uuid;
    }

public:
    DmrppCommon() : d_size(0), d_offset(0), d_md5(""), d_uuid("") { }
    DmrppCommon(const DmrppCommon &dc) { _duplicate(dc); }
    virtual ~DmrppCommon() { }

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
    virtual void set_md5(string md5) { d_md5 = md5; }


    /**
     * @brief Get the uuid string for this variable's data block
     */
    virtual std::string get_uuid() const { return d_uuid; }

    /**
     * @brief Set the uuid for this variable's data block.
     * @param offset The uuid of this variable's data block
     */
    virtual void set_uuid(string uuid) { d_uuid = uuid; }

    /**
     * @brief Get the Data URL. Read data from this URL.
     */
    virtual std::string get_data_url() const { return d_data_url; }

    /**
     * @brief Set the Data URL
     * @param data_url Read data from this URL.
     */
    virtual void set_data_url(const std::string &data_url) { d_data_url = data_url; }
};

#endif // _dmrpp_common_h

