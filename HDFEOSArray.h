// -*- C++ -*-

// This file is part of the hdf4 data handler for the OPeNDAP data server.

// Copyright (c) 2008 The HDF Group
// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org>

// Copyright (c) 2005 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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
// Copyright 1996, by the California Institute of Technology.
// ALL RIGHTS RESERVED. United States Government Sponsorship
// acknowledged. Any commercial use must be negotiated with the
// Office of Technology Transfer at the California Institute of
// Technology. This software may be subject to U.S. export control
// laws and regulations. By accepting this software, the user
// agrees to comply with all applicable U.S. export laws and
// regulations. User has the responsibility to obtain export
// licenses, or other export authority as may be required before
// exporting such information to foreign countries or providing
// access to foreign persons.

// Author: Todd Karakashian, NASA/Jet Propulsion Laboratory
//         Todd.K.Karakashian@jpl.nasa.gov
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _HDFEOSARRAY_H
#define _HDFEOSARRAY_H

// STL includes
#include <string>
#include <vector>

// DAP includes
#include <Array.h>

using namespace libdap;

/// A class for generating 1-D shared dimension for HDF-EOS2 Grid files.
///
/// According to CF-1.x convention, all shared dimension variables should be
/// available outside of DAP Grids. All dimension variables inside any DAP Grid
/// variable will be identified by either the parser or the HDF-EOS2 library
/// it will be extracted exactly once without duplicates.
/// If there are conflicts among the shared dimension variables because of
/// the different dimension sizes, an error will be thrown by HDFEOS class.
///
/// This class is a mere place holder for those shared dimension variables and
/// it actually reads dimension data from HDFEOS class. It is the HDFEOS
/// class that builds the contents for the read() operation of this class. 
///
/// \see HDFEOS
class HDFEOSArray:public Array {
    dods_float32 *get_dimension_data(dods_float32 * buf, int start,
                                     int stride, int stop, int count);
public:
    HDFEOSArray(const string & n, const string &d, BaseType * v);
    virtual ~ HDFEOSArray();
    virtual BaseType *ptr_duplicate();
    
    /// Read the (subsetted) contents of dimension_data in HDFEOS class.
    virtual bool read();
};

#endif                          // _HDFEOSARRAY_H

