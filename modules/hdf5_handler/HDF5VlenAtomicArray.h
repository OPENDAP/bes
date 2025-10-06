// This file handles the variable-length int and float array.
// Author: Kent Yang
// <myang6@hdfgroup.org> 

// Copyright (c)  The HDF Group, Inc. and OPeNDAP, Inc.
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
//
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 410 E University Ave,
// Suite 200, Champaign, IL 61820  

#ifndef _HDF5VLENATOMICARRAY_H
#define _HDF5VLENATOMICARRAY_H 

#include <H5Ipublic.h>
#include <H5Rpublic.h>

#include <libdap/Array.h>
#include "HDF5Array.h"

#include "h5get.h"


///////////////////////////////////////////////////////////////////////////////
/// \file HDFVlenAtomic5Array.h
/// \brief A class for handling variable-length int and float array in HDF5 for the default option.
/// 
/// \author Kent Yang       (myang6@hdfgroup.org)
///
////////////////////////////////////////////////////////////////////////////////
class HDF5VlenAtomicArray:public HDF5Array {

  private:
    bool is_vlen_index = false;
    void read_vlen_internal(bool);

  public:

    /// Constructor
    HDF5VlenAtomicArray(const std::string & n, const std::string &d, libdap::BaseType * v, bool is_vlen_index);
    ~ HDF5VlenAtomicArray() override = default;

    /// Clone this instance.
    /// 
    /// Allocate a new instance and copy *this into it. This method must 
    /// perform a deep copy.
    /// \return A newly allocated copy of this class
    libdap::BaseType *ptr_duplicate() override;

    /// Reads HDF5 array data into local buffer
    bool read() override;

};

#endif
