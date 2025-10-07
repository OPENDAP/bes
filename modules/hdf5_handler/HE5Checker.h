// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.
//
// Authors: 
// Hyo-Kyung Lee <hyoklee@hdfgroup.org> and Kent Yang <myang6@hdfgroup.org> 
//
// Copyright (c) 2009-2023 The HDF Group, Inc. and OPeNDAP, Inc.
// All rights reserved.
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

#ifndef _HE5Checker_H
#define _HE5Checker_H

#include "HE5Parser.h"
#include <hdf5.h>

/// \file HE5Checker.h
/// \brief A class for parsing NASA HDF-EOS5 StructMetadata.
///
/// This class contains functions that parse NASA HDF-EOS5 StructMetadata
/// and prepares the Vector structure that other functions reference.
///
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
/// \author Kent Yang <myang6@hdfgroup.org>
class HE5Checker {
 public:

    HE5Checker() = default;
    ~ HE5Checker() = default;

    // Check if it has multiple grids that have different dimension sizes
    // of coordinate variables.
    bool check_grids_multi_latlon_coord_vars(HE5Parser* p) const;
    bool check_grids_missing_projcode(const HE5Parser*p) const;
    bool check_grids_unknown_parameters(const HE5Parser* p) const;
    bool check_grids_support_projcode(const HE5Parser*p) const;
    void update_unlimited_dim_sizes(HE5Parser* p,hid_t file_id);
    void update_unlimited_dim_sizes_internal(hid_t file_id, const std::string& eos5_name, std::vector<HE5Dim>&dim_list, const std::vector<HE5Var>&data_var_list, const std::vector<HE5Var>&geo_var_list);
    int obtain_correct_dim_size(hid_t file_id, const std::string &var_path, int var_dim_index);
    void set_grids_missing_pixreg_orig(HE5Parser* p) const;

};
#endif
