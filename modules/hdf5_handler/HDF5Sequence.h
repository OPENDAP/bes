// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org> and Kent Yang
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

#ifndef _HDF5Sequence_h
#define _HDF5Sequence_h 

#include <libdap/Sequence.h>
#include "h5get.h"


/// \file HDF5Sequence.h
/// \brief A HDF5Sequence class.
/// This class is not used in the current hdf5 handler and
/// is provided to support DAP Sequence data type if necessary.
///
/// \author James Gallagher
///
/// @see Sequence 
class HDF5Sequence:public libdap::Sequence {


  public:

    HDF5Sequence(const std::string &n, const std::string &d);
    ~ HDF5Sequence() override = default;

    libdap::BaseType *ptr_duplicate() override;


};

#endif
