// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org> and Muqun Yang
// <myang6@hdfgroup.org> 

// Copyright (c) 2009-2012 The HDF Group, Inc. and OPeNDAP, Inc.
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

#ifndef _hdf5str_h
#define _hdf5str_h 1

#include <string>
#include <H5Ipublic.h>

#include <limits.h>

#ifndef STR_FLAG
#define STR_FLAG 1
#endif

#ifndef STR_NOFLAG
#define STR_NOFLAG 0
#endif

#include "Str.h"
#include "h5get.h"

using namespace libdap;

////////////////////////////////////////////////////////////////////////////////
/// \file HDF5Str.h
///
/// \brief This class that translates HDF5 string into DAP string for the default option.
/// 
/// \author Hyo-Kyung Lee   (hyoklee@hdfgroup.org)
/// \author Kent Yang       (myang6@hdfgroup.org)
/// \author James Gallagher (jgallagher@opendap.org)
///
/// Copyright (c) 2007-2012 HDF Group
/// 
/// All rights reserved.
////////////////////////////////////////////////////////////////////////////////
class HDF5Str:public Str {
 private:
    hid_t dset_id;
    hid_t ty_id;
    int array_flag;

 public:

    /// Constructor
    HDF5Str(const string &n, const string &d);
    virtual ~ HDF5Str() { }

    /// Clone this instance.
    /// 
    /// Allocate a new instance and copy *this into it. This method must
    /// perform a deep copy.
    /// \return A newly allocated copy of this class    
    virtual BaseType *ptr_duplicate();

    /// Reads HDF5 string data into local buffer  
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
