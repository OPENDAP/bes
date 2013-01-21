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
////////////////////////////////////////////////////////////////////////////////
/// \file h5get.h
/// Helper functions to generate DDS/DAS/DODS for the default option.
///

///
/// 
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
/// \author Muqun Yang <ymuqun@hdfgroup.org>


#ifndef _H5GET_H
#define _H5GET_H
#include "hdf5_handler.h"

bool
check_h5str(hid_t);

void 
close_fileid(hid_t fid);

char*
correct_name(char *);

hid_t 
get_attr_info(hid_t dset, int index, DSattr_t * attr_inst, bool*);

string
get_dap_type(hid_t type);

void
get_data(hid_t dset, void *buf);

void
get_dataset(hid_t pid, const string &dname, DS_t * dt_inst_ptr);

hid_t
get_fileid(const char *filename);

int
get_slabdata(hid_t dset, int *, int *, int *, int num_dim, void *);

void
get_strdata(int, char *, char *, int);


#endif                          //_H5GET_H
