
// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c) 2007-2012 The HDF Group, Inc. and OPeNDAP, Inc.
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

///////////////////////////////////////////////////////////////////////////////
/// \file h5das.h
/// \brief Data attributes processing header for the default option.
///
/// This file is part of h5_dap_handler, A C++ implementation of the DAP 
/// handler for HDF5 data.
///    
/// It defines functions that generate data attributes from HDF5 files.
/// 
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
/// \author Muqun Yang <ymuqun@hdfgroup.org>
///
///////////////////////////////////////////////////////////////////////////////
#ifndef _h5das_H
#define _h5das_H

#include <hdf5.h>
#include <DAS.h>
#include <Str.h>
using namespace libdap;

void add_group_structure_info(DAS & das, const char *gname, char *oname,
                              bool is_group);
void depth_first(hid_t, const char *, DAS &);
void find_gloattr(hid_t file, DAS & das);
string get_hardlink(hid_t, const string &);
void get_softlink(DAS &, hid_t, const char*, const string &, int,size_t);
void read_comments(DAS & das, const string & varname, hid_t oid);
void read_objects(DAS & das, const string & varname, hid_t dset,
                  int num_attr);
#endif
