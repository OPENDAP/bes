
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

/**
 * Interface for the size and offset information of data described by
 * DMR++ files.
 */
class DmrppCommon {
    unsigned long long d_size;
    unsigned long long d_offset;

protected:
    void _duplicate(const DmrppCommon &dc) {
        d_size = dc.d_size;
        d_offset = dc.d_offset;
    }

public:
    DmrppCommon() : d_size(0), d_offset(0) { }
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
};

#endif // _dmrpp_common_h

