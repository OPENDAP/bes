// -*- C++ -*-

// This file is part of the hdf4 data handler for the OPeNDAP data server.

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
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
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

#ifndef _HDFARRAY_H
#define _HDFARRAY_H

// STL includes
#include <string>
#include <vector>

// DODS includes
#include <libdap/Array.h>

#include "ReadTagRef.h"


class HDFArray:public libdap::Array, public ReadTagRef {
  public:
    HDFArray(const std::string & n, const std::string &d, libdap::BaseType * v);
    ~ HDFArray() override;
    libdap::BaseType *ptr_duplicate() override;
    bool read() override;
    bool read_tagref(int32 tag, int32 ref, int &error) override;
    bool GetSlabConstraint(std::vector < int >&start_array,
                           std::vector < int >&edge_array,
                           std::vector < int >&stride_array);

    void transfer_attributes(libdap::AttrTable *at_container) override;
    virtual void transfer_dimension_attribute(libdap::AttrTable *dim);
};

#endif                          // _HDFARRAY_H

