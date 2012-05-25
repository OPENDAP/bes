// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.
//
// Authors: 
// Hyo-Kyung Lee <hyoklee@hdfgroup.org> and Muqun Yang <myang6@hdfgroup.org> 
//
// Copyright (c) 2009-2011 The HDF Group, Inc. and OPeNDAP, Inc.
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

#ifndef _HE5Checker_H
#define _HE5Checker_H

#include "HE5Parser.h"
using namespace std;

/// \file HE5Checker.h
/// \brief A class for parsing NASA HDF-EOS5 StructMetadata.
///
/// This class contains functions that parse NASA HDF-EOS5 StructMetadata
/// and prepares the Vector structure that other functions reference.
///
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
/// \author MuQun Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011 The HDF Group
///
/// All rights reserved.

class HE5Checker {
 public:

    HE5Checker() {}
    ~ HE5Checker(){}

    // Check if it has multiple grids that have different dimension sizes
    // of coordinate variables.
    bool check_grids_multi_latlon_coord_vars(HE5Parser* p);
    bool check_grids_missing_projcode(HE5Parser*p);
    bool check_grids_unknown_parameters(HE5Parser* p);
    bool check_grids_support_projcode(HE5Parser*p);
    
    void set_grids_missing_pixreg_orig(HE5Parser* p);

};
#endif
