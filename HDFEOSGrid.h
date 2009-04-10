// -*- C++ -*-
// 
// This file is part of the hdf4 data handler for the OPeNDAP data server.

// Copyright (c) 2008 The HDF Group
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

#ifndef _HDFEOSGRID_H
#define _HDFEOSGRID_H

// STL includes
#include <string>

// DODS includes
#include <Grid.h>

#include <hdfclass.h>

#include "ReadTagRef.h"

using namespace libdap;

class HDFEOSGrid:public Grid, public ReadTagRef {
  public:
    HDFEOSGrid(const string &n, const string &d);
    virtual ~HDFEOSGrid();
    virtual BaseType *ptr_duplicate();
    virtual bool read();
    virtual vector < array_ce > get_map_constraints();
    virtual bool read_tagref(int32 tag, int32 ref, int &error);
    dods_float32 *get_dimension_data(dods_float32 * buf, int start,
                                     int stride, int stop, int count); // <hyokyung 2008.11.13. 11:58:45>
    void read_dimension(Array * a); // <hyokyung 2008.11.13. 11:58:44>
  
};

#endif                          // _HDFGRID_H
