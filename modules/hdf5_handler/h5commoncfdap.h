// This file is part of hdf5_handler: an HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c) 2011-2023 The HDF Group, Inc. and OPeNDAP, Inc.
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
/// \file h5commoncfdap.h
/// \brief Functions to generate DDS and DAS for one object(variable). 
///
/// 
/// \author Kent Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////

#ifndef _H5COMMONCFDAP_H
#define _H5COMMONCFDAP_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>

#include <libdap/DMR.h>
#include <libdap/DDS.h>
#include <libdap/DAS.h>
#include <libdap/D4Attributes.h>
#include <libdap/D4Maps.h>
#include "hdf5.h"

#include "HDF5CF.h"


void gen_dap_onevar_dds(libdap::DDS &dds, const HDF5CF::Var*, hid_t, const std::string &);
void gen_dap_onevar_dds_sca_64bit_int(const HDF5CF::Var *var, const std::string &filename);
void gen_dap_onevar_dds_sca_atomic(libdap::DDS &dds, const HDF5CF::Var *var, const std::string &filename);
void gen_dap_onevar_dds_array(libdap::DDS &dds, const HDF5CF::Var *var, hid_t file_id, const std::string &filename,
                              const std::vector<HDF5CF::Dimension *>& dims);
void gen_dap_oneobj_das(libdap::AttrTable*, const HDF5CF::Attribute*, const HDF5CF::Var*);

void gen_dap_onevar_dmr(libdap::D4Group*, const HDF5CF::Var*, hid_t, const std::string &);
void gen_dap_onevar_dmr_sca(libdap::D4Group* d4_grp, const HDF5CF::Var* var, const std::string & filename);
void gen_dap_onevar_dmr_array(libdap::D4Group* d4_grp, const HDF5CF::Var* var, hid_t file_id,
                              const std::string &filename, const std::vector<HDF5CF::Dimension *>& dims);

void map_cfh5_var_attrs_to_dap4(const HDF5CF::Var*var,libdap::BaseType*new_var);
void map_cfh5_grp_attr_to_dap4(libdap::D4Group*, const HDF5CF::Attribute*);
void map_cfh5_attr_container_to_dap4(libdap::D4Attribute *, const HDF5CF::Attribute*);


void add_cf_grid_mapping_attr(libdap::DAS &das, const std::vector<HDF5CF::Var*>& vars, const std::string& cf_projection,
    const std::string & dim0name, hsize_t dim0size, const std::string &dim1name, hsize_t dim1size);
#if 0
void add_cf_grid_cv_attrs(DAS & das, const vector<HDF5CF::Var*>& vars, EOS5GridPCType cv_proj_code,
    float cv_point_lower, float cv_point_upper, float cv_point_left, float cv_point_right,
    const vector<HDF5CF::Dimension*>& dims,const vector<double>&  params,const unsigned short);
#endif
void add_cf_grid_cv_attrs(libdap::DAS & das, const std::vector<HDF5CF::Var*>& vars, EOS5GridPCType cv_proj_code,
    const std::vector<HDF5CF::Dimension*>& dims,const std::vector<double>&  params, unsigned short);

void add_cf_projection_attrs(libdap::DAS &,EOS5GridPCType ,const std::vector<double> &,const std::string&);
void add_cf_grid_cvs(libdap::DDS & dds, EOS5GridPCType cv_proj_code, float cv_point_lower, float cv_point_upper,
    float cv_point_left, float cv_point_right, const std::vector<HDF5CF::Dimension*>& dims);

void add_cf_grid_mapinfo_var(libdap::DDS &dds, EOS5GridPCType, unsigned short);
bool need_special_attribute_handling(const HDF5CF::Attribute*, const HDF5CF::Var*);
void gen_dap_special_oneobj_das(libdap::AttrTable*, const HDF5CF::Attribute*, const HDF5CF::Var*);
bool is_fvalue_valid(H5DataType, const HDF5CF::Attribute*);
void gen_dap_str_attr(libdap::AttrTable*, const HDF5CF::Attribute *);
void add_ll_valid_range(libdap::AttrTable*, bool is_lat);
void map_cfh5_var_attrs_to_dap4_int64(const HDF5CF::Var*var,libdap::BaseType*new_var);
bool need_attr_values_for_dap4(const HDF5CF::Var*var);
void check_update_int64_attr(const std::string &, const HDF5CF::Attribute *);
void handle_coor_attr_for_int64_var(const HDF5CF::Attribute *, const std::string &,std::string&,bool);
libdap::D4Attribute *gen_dap4_attr(const HDF5CF::Attribute *);
std::string get_cf_string(std::string & s);
std::string get_cf_string_helper(std::string & s);

void add_gm_spcvs(libdap::D4Group *d4_root, EOS5GridPCType cv_proj_code, float cv_point_lower, float cv_point_upper,
    float cv_point_left, float cv_point_right, const std::vector<HDF5CF::Dimension*>& dims);
void add_gm_spcvs_attrs(libdap::BaseType *d4_var, bool is_dim0);

void add_cf_grid_cv_dap4_attrs(libdap::D4Group *d4_root, const std::string& cf_projection,
                               const std::vector<HDF5CF::Dimension*>&dims, const std::vector<std::string> &);
void add_cf_grid_cv_dap4_attrs_helper(libdap::Array *t_a, const std::string &dim0name, hsize_t dim0size, bool &has_dim0,
                                      const std::string &dim1name, hsize_t dim1size, bool &has_dim1);

void add_gm_oneproj_var_dap4_attrs(libdap::BaseType *d4_var,EOS5GridPCType cv_proj_code,
                                   const std::vector<double> &eos5_proj_params);

void add_var_dap4_attr(libdap::BaseType *d4_var,const std::string& attr_name, D4AttributeType attr_type,
                       const std::string& attr_value);

void add_grp_dap4_attr(libdap::D4Group *d4_grp,const std::string& attr_name, D4AttributeType attr_type,
                       const std::string& attr_value);

void add_dap4_coverage(libdap::D4Group* d4_grp, const std::vector<std::string>& map_array, bool is_coard);
void add_dap4_coverage_set_up(unordered_map<std::string, libdap::Array*> &d4map_array_maps,
                              std::vector<libdap::Array *> &has_map_arrays, libdap::Array *t_a,
                              const std::vector<std::string>& coord_var_names, const std::string & vname);
void add_dap4_coverage_grid(unordered_map<std::string, libdap::Array*> &d4map_array_maps,
                            std::vector<libdap::Array *> &has_map_arrays);
void add_dap4_coverage_swath(unordered_map<std::string, libdap::Array*> &d4map_array_maps,
                            const std::vector<libdap::Array *> &has_map_arrays);
void add_dap4_coverage_swath_coords(unordered_map<std::string, libdap::Array*> &d4map_array_maps, libdap::Array *has_map_array,
                                    const std::vector<std::string> &coord_names, unordered_set<std::string> &coord_dim_names);
static inline libdap::AttrTable * obtain_new_attr_table() {
    auto new_attr_table_unique = make_unique<libdap::AttrTable>();
    return new_attr_table_unique.release();
}
#endif
