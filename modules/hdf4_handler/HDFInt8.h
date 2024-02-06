// This file is part of the hdf4 data handler for the OPeNDAP data server.
// Copyright (c) The HDF Group.
// Copyright (c) 2005 OPeNDAP, Inc.
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
// You can contact The HDF Group, Inc. at 410 E University Ave,
// Suite 200, Champaign, IL 61820

/////////////////////////////////////////////////////////////////////////////

#ifndef _HDFINT8_H
#define _HDFINT8_H

// STL includes
#include <string>

// DODS includes
#include <libdap/Int8.h>


class HDFInt8:public libdap::Int8 {

  public:

    HDFInt8(const std::string & n, const std::string &d);
    ~ HDFInt8() override;
    libdap::BaseType *ptr_duplicate() override;
    bool read() override;
};

#endif                          // _HDFINT8_H

