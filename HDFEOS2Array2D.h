// -*- C++ -*-
//
// This file is part of the hdf4 data handler for the OPeNDAP data server.
//
// Copyright (c) 2009 The HDF Group
// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org>
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
// You should have received a copy of the GNU Lesser General Public License
// along with this software; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
/////////////////////////////////////////////////////////////////////////////

#ifndef _HDFEOSARRAY2D_H
#define _HDFEOSARRAY2D_H

// STL includes
#include <string>
#include <vector>

// DAP includes
#include <Array.h>

using namespace libdap;

/// A class for generating 2-D shared dimension for HDF-EOS2 Grid files.
///
/// This class is for generating 2-D shared dimension variables among
/// the HDF-EOS2 Grid variables that use 2-D grid projection. Unlike the 1-D
/// geographic projection, 2-D projections like polar or sinusoidal do not
/// require DAP Grid generation from HDF-EOS2 Grid datasets. The normal DAP
/// Arrays generation will be sufficient. However, it is still necessary to
/// have the shared dimension DAP Arrays according to CF-1.x convention.
///
/// The parser cannot handle 2-D grids at all. However, for some HDF-EOS2
/// 2-D projection Grid files, the HDF-EOS2 library can  generate the
/// right 2-D dimension map data and they will be captured by this class.
///
/// \see HDFEOSArray
/// \see HDFEOS2
class HDFEOS2Array2D:public HDFArray {
  
private:
    int d_num_dim;
    int format_constraint(int *cor, int *step, int *edg);
    int linearize_multi_dimensions(int *start, int *stride, int *count,
                                   int *picks);
public:
    
    HDFEOS2Array2D(const string & n, BaseType * v);
    virtual ~ HDFEOS2Array2D();
    virtual BaseType *ptr_duplicate();
    
    /// Reads the content of 2-D lat/lon information via HDF-EOS2 library.
    virtual bool read();
};

#endif                          // _HDFEOSARRAY2D_H

