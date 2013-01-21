// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org> and Muqun Yang
// <myang6@hdfgroup.org> 

// Copyright (c) 2009-2012 The HDF Group, Inc. and OPeNDAP, Inc.
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  

#ifndef _HDF5Sequence_h
#define _HDF5Sequence_h 1

#include "Sequence.h"
#include "h5get.h"

using namespace libdap;

/// \file HDF5Sequence.h
/// \brief A HDF5Sequence class.
/// This class is not used in the current hdf5 handler and
/// is provided to support DAP Sequence data type if necessary.
///
/// \author James Gallagher
///
/// @see Sequence 
class HDF5Sequence:public Sequence {

  private:
    hid_t dset_id;
    hid_t ty_id;

  public:

    HDF5Sequence(const string &n, const string &d);
    virtual ~ HDF5Sequence();

    virtual BaseType *ptr_duplicate();
    virtual bool read();

    friend string return_type(hid_t datatype);

    hid_t get_did();
    hid_t get_tid();
    void set_did(hid_t dset);
    void set_tid(hid_t type);


};

#endif
