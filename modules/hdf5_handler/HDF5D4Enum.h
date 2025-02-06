// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.
// Copyright (c) The HDF Group, Inc. and OPeNDAP, Inc.
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
// You can contact The HDF Group, Inc. at 410 E University Ave,
// Suite 200, Champaign, IL 61820  

#ifndef _HDF5D4Enum_h
#define _HDF5D4Enum_h 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <H5Ipublic.h>
#include <libdap/D4Enum.h>
#include <libdap/D4EnumDefs.h>
#include "h5get.h"


/// \file HDF5D4Enum.h
/// \brief A class to map  HDF5 Enum type.
/// 
/// This class provides a way to map HDF5 signed 16 bit integer to DAP Int16.
///
///

class HDF5D4Enum:public libdap::D4Enum {
  private:
    std::string var_path;

  public:

    /// Constructor
    HDF5D4Enum(const std::string &n, const std::string &vpath, const std::string &dataset, libdap::Type type);
    ~ HDF5D4Enum() override = default;

    /// Clone this instance.
    ///
    /// Allocate a new instance and copy *this into it. This method must
    /// perform a deep copy. 
    /// \return A newly allocated copy of this class  
    libdap::BaseType *ptr_duplicate() override;

    /// Reads HDF5 16-bit integer data into local buffer
    bool read() override;
    void close_objids(hid_t mem_type, hid_t base_type, hid_t dtype, hid_t dset_id, hid_t file_id) const;

};
#endif
