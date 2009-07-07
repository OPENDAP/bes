// -*- C++ -*-

// This file is part of the hdf4 data handler for the OPeNDAP data server.

// Copyright (c) 2009 The HDF Group
// Author: Hyo-Kyung Lee <hyoklee@opendap.org>

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

#ifndef _HDFEOSARRAY2D_H
#define _HDFEOSARRAY2D_H

// STL includes
#include <string>
#include <vector>

// DODS includes
#include <Array.h>

using namespace libdap;

/// A class for shared dimension in 2-D.
class HDFEOSArray2D:public Array {
  
private:
  int d_num_dim;
  int format_constraint(int *cor, int *step, int *edg);
  int linearize_multi_dimensions(int *start, int *stride, int *count,
				 int *picks);
  public:
    HDFEOSArray2D(const string & n, BaseType * v); // <hyokyung 2008.12. 2. 10:10:48>
    virtual ~ HDFEOSArray2D();
    virtual BaseType *ptr_duplicate();
    virtual bool read();
};

#endif                          // _HDFEOSARRAY2D_H

