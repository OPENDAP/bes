// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org> and Muqun Yang
// <myang6@hdfgroup.org> 

// Copyright (c) 2009 The HDF Group, Inc. and OPeNDAP, Inc.
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  

#ifndef _HDF5Int32_h
#define _HDF5Int32_h 1

#include <string>

#include <H5Ipublic.h>

#include "Int32.h"
#include "h5get.h"

using namespace libdap;

/// \file HDF5Int32.h
/// 
/// \brief This class provides a way to map HDF5 32 bit integer to DAP Int32 for the default option.
///
/// \author Hyo-Kyung Lee   (hyoklee@hdfgroup.org)
/// \author Kent Yang       (myang6@hdfgroup.org)
/// \author James Gallagher (jgallagher@opendap.org)
///
///

class HDF5Int32:public Int32 {

  private:
    hid_t dset_id;
    hid_t ty_id;

  public:
    /// Constructor
    HDF5Int32(const string &n, const string &d);
    virtual ~ HDF5Int32() { }

    /// Clone this instance.
    ///
    /// Allocate a new instance and copy *this into it. This method must perform a deep copy.
    /// \return A newly allocated copy of this class  
    virtual BaseType *ptr_duplicate();

    /// Reads HDF5 32-bit integer data into local buffer
    virtual bool read();

    /// See return_type function defined in h5dds.cc.  
    friend string return_type(hid_t datatype);

    /// returns HDF5 dataset id.      
    hid_t get_did();

    /// returns HDF5 datatype id.
    hid_t get_tid();

    /// remembers HDF5 dataset id.      
    void set_did(hid_t dset);

    /// remembers HDF5 datatype id.
    void set_tid(hid_t type);


};

#endif
