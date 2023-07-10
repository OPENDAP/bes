// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org> and Keng Yang
// <myang6@hdfgroup.org> 

// Copyright (c) 2009-2023 The HDF Group, Inc. and OPeNDAP, Inc.
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

#ifndef _HDF5Structure_h
#define _HDF5Structure_h 

#include <string>
#include <H5Ipublic.h>
#include <libdap/Structure.h>
#include "HDF5Array.h"
#include "h5get.h"


/// \file HDF5Structure.h
/// \brief This class converts HDF5 compound type into DAP structure for the default option.
///
///\author  Kent Yang
///
/// \see Structure 
class HDF5Structure:public libdap::Structure {
  private:

    std::string var_path;

    void m_duplicate(const HDF5Structure & src) {
        var_path = src.var_path;
    }

    void do_structure_read(hid_t dsetid, hid_t dtypeid,std::vector <char> &values,bool has_values, size_t values_offset);
    void do_structure_read_string(hid_t memb_id, char *memb_name, const std::vector<char> &values,
                                                 size_t values_offset, size_t memb_offset);
    friend class HDF5Array;
  public:

    /// Constructor
    HDF5Structure(const std::string &n, const std::string &vpath, const std::string &d);
    HDF5Structure(const HDF5Structure &rhs);
    ~ HDF5Structure() override = default;

    /// Assignment operator for dynamic cast into generic Structure.
    HDF5Structure & operator=(const HDF5Structure & rhs);

    /// Clone this instance.
    /// 
    /// Allocate a new instance and copy *this into it. This method must perform a deep copy.
    /// \return A newly allocated copy of this class  
    libdap::BaseType *ptr_duplicate() override;

    /// Reads HDF5 structure data by calling each member's read method in this structure.
    bool read() override;


};

#endif
