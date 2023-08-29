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
////////////////////////////////////////////////////////////////////////////////
/// \file h5common.h
/// Common helper functions to access HDF5 data for both the CF and the default options.
///
///
/// 


#ifndef _H5COMMON_H
#define _H5COMMON_H
#include <hdf5.h>
#include <vector>
#include <string>

void get_data(hid_t dset, void *buf);

int get_slabdata(hid_t dset, const int64_t *, const int64_t *, const int64_t *, const int num_dim, void *);

void get_strdata(int64_t, char *, char *, int);

bool read_vlen_string(hid_t d_dset_id, const int64_t nelms, const hsize_t *offset, const hsize_t *step,
                      const hsize_t *count,std::vector<std::string> &finstrval);
void read_vlen_string_value(const int64_t nelms, std::vector<char> &strval,std::vector<std::string>&finstrval,
                            size_t ty_size);
void claim_vlen_string_memory(hid_t memtype, hid_t dspace, hid_t dtypeid, hid_t mspace, std::vector<char> &strval,
                              bool is_scalar) ;
void get_vlen_str_data(const char*src,std::string &finalstrval);


bool promote_char_to_short(H5T_class_t type_cls, hid_t type_id);



#endif                          //_H5COMMON_H
