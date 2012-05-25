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

#ifndef _HDF5Structure_h
#define _HDF5Structure_h 1

#include <string>
#include <H5Ipublic.h>
#include "Structure.h"
#include "h5get.h"

using namespace libdap;

/// \file HDF5Structure.h
/// \brief This class converts HDF5 compound type into DAP structure for the default option.
///
/// \author James Gallagher
/// \author Hyo-Kyung Lee
///
/// \see Structure 
class HDF5Structure:public Structure {
  private:
    hid_t dset_id;
    hid_t ty_id;
    int array_index;
    int array_size;             // size constrained by constraint expression
    int array_entire_size;      // entire size in case of array of structure

  public:
    /// Constructor
    HDF5Structure(const string &n, const string &d);
    virtual ~ HDF5Structure();

    /// Assignment operator for dynamic cast into generic Structure.
    HDF5Structure & operator=(const HDF5Structure & rhs);

    /// Clone this instance.
    /// 
    /// Allocate a new instance and copy *this into it. This method must perform a deep copy.
    /// \return A newly allocated copy of this class  
    virtual BaseType *ptr_duplicate();

    /// Reads HDF5 structure data by calling each member's read method in this structure.
    virtual bool read();

    /// See return_type function defined in h5dds.cc.  
    friend string return_type(hid_t datatype);

    /// returns HDF5 dataset id.  
    hid_t get_did();
    /// returns HDF5 datatype id.
    hid_t get_tid();

    /// returns the array index of this Structure if it's a part of array of structures.
    int get_array_index();
    /// returns the array size for subsetting if it's a part of array of structures.
    int get_array_size();
    /// returns the entire array size of this Structure if it's a part of array of structures.
    int get_entire_array_size();

    /// remembers HDF5 datatype id.  
    void set_did(hid_t dset);
    /// remembers HDF5 datatype id.
    void set_tid(hid_t type);

    /// remembers the array index of this Structure if it's a part of array of structures.
    void set_array_index(int i);
    /// remembers the array size for subsetting if it's a part of array of structures.
    void set_array_size(int i);
    /// returns the entire array size of this Structure if it's a part of array of structures.  
    void set_entire_array_size(int i);


};

#endif
