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
// You can contact The HDF Group, Inc. at 1901 South First Street,
// Suite C-2, Champaign, IL 61820  

#ifndef _HDF5GridEOS_h
#define _HDF5GridEOS_h 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Grid.h"
#include "h5get.h"

using namespace libdap;

///////////////////////////////////////////////////////////////////////////////
/// A Grid subclass for handling HDF-EOS5 data.
/// 
/// This class synthesizes DAP Grids from NASA EOS HDF5 arrays that can be
/// mapped to Grid.
///
/// @author Hyo-Kyung Lee (hyoklee@hdfgroup.org)
///
/// Copyright (c) The 2007-2009 HDF Group
///////////////////////////////////////////////////////////////////////////////
class HDF5GridEOS:public Grid {

private:
    hid_t dset_id;
    hid_t ty_id;

public:

    /// Constructor
    HDF5GridEOS(const string &n, const string &d);
    virtual ~ HDF5GridEOS();

    /// Clone this instance.
    /// 
    /// Allocate a new instance and copy *this into it.
    /// This method must perform a deep copy.
    /// \return A newly allocated copy of this class    
    virtual BaseType *ptr_duplicate();

    /// Reads data array and map arrays in from this EOS Grid.
    ///
    /// This function is different from normal Grid read function
    /// since maps data are resulted from parsing the metadata.
    virtual bool read();

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
