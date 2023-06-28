
// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c) 2007-2023 The HDF Group, Inc. and OPeNDAP, Inc.
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
/// \author Keng Yang <myang6@hdfgroup.org>
///
///////////////////////////////////////////////////////////////////////////////
#ifndef _h5das_H
#define _h5das_H

#include <hdf5.h>
#include <libdap/DAS.h>
#include <libdap/Str.h>

void add_group_structure_info(libdap::DAS & das, const char *gname, const char *oname,
                              bool is_group);
void depth_first(hid_t, const char *, libdap::DAS &);
void find_gloattr(hid_t file, libdap::DAS & das);
string get_hardlink(hid_t, const std::string &);
void get_softlink(libdap::DAS &, hid_t, const char*, const std::string &, int,size_t);
void read_comments(libdap::DAS & das, const std::string & varname, hid_t oid);
void read_objects(libdap::DAS & das, const std::string & varname, hid_t dset,
                  int num_attr);
#endif
