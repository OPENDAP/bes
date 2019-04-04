// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011-2016 The HDF Group, Inc. and OPeNDAP, Inc.
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
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  

/////////////////////////////////////////////////////////////////////////////
/// \file HDF5GMCFMissNonLLCVArray.h
/// \brief This class specifies the retrieval of the values of non-lat/lon coordinate variables for general HDF5 products
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2016 The HDF Group
///
/// All rights reserved.

#ifndef _HDF5GMCFMISSNONLLCVARRAY_H
#define _HDF5GMCFMISSNONLLCVARRAY_H

// STL includes
#include <string>

// DODS includes
#include "HDF5BaseArray.h"


class HDF5GMCFMissNonLLCVArray:public HDF5BaseArray {

  public:
    HDF5GMCFMissNonLLCVArray(int h5_rank, 
                              int h5_tnumelm, const std::string & n="",  libdap::BaseType * v = 0):
        HDF5BaseArray(n,v),
        rank(h5_rank),
        tnumelm(h5_tnumelm) {
    }
        
    virtual ~ HDF5GMCFMissNonLLCVArray() {
    }
    virtual libdap::BaseType *ptr_duplicate();
    virtual bool read();
    virtual void read_data_NOT_from_mem_cache(bool add_cache,void*buf);

  private:
        int rank;
        int tnumelm;

};

#endif // _HDF5GMCFMISSNONLLCVARRAY_H
