// This file is part of hdf5_handler an HDF5 file handler for the OPeNDAP
// data server.

// Author: Muqun Yang <myang6@hdfgroup.org> 

// Copyright (c) 2011 The HDF Group, Inc. and OPeNDAP, Inc.
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

#ifndef _HDF5BASEARRAY_H
#define _HDF5BASEARRAY_H

// STL includes
#include <string>
#include <vector>

// DODS includes
#include <Array.h>

using namespace libdap;
///////////////////////////////////////////////////////////////////////////////
/// \file HDF5BaseArray.h
/// \brief A helper class that aims to reduce code redundence for different special CF derived array class
/// For example, format_constraint has been called by different CF derived array class,
/// and write_nature_number_buffer has also be used by missing variables on both 
/// HDF-EOS5 and generic HDF5 products. 
/// 
/// This class converts HDF5 array type into DAP array.
/// 
/// \author Kent Yang       (myang6@hdfgroup.org)
/// Copyright (c) 2011 The HDF Group
/// 
/// All rights reserved.
///
/// 
////////////////////////////////////////////////////////////////////////////////


class HDF5BaseArray:public Array {
  public:
    HDF5BaseArray(const string & n="",  BaseType * v = 0):
        Array(n,v)
        {
    }
        
    virtual ~ HDF5BaseArray() {
    }
    virtual BaseType *ptr_duplicate();
    virtual bool read();
    int format_constraint (int *cor, int *step, int *edg);
    void write_nature_number_buffer(int rank, int tnumelm);

};

#endif                          // _HDF5BASEARRAY_H

