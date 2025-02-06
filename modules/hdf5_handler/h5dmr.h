
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
/// \file h5dmr.h
/// \brief Data structure and retrieval processing header for the default option
///
/// This file is part of h5_dap_handler, A C++ implementation of the DAP handler
/// for HDF5 data.
///    
/// It defines functions that describe and retrieve group/dataset from
/// HDF5 files. 
/// 
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
/// \author Kent Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////


#ifndef _h5dmr_H
#define _h5dmr_H
#include <unordered_map>
#include <unordered_set>
#include <map>

#include <H5Gpublic.h>
#include <H5Fpublic.h>
#include <H5Ipublic.h>
#include <H5Tpublic.h>
#include <H5Spublic.h>
#include <H5Apublic.h>
#include <H5public.h>

#include <libdap/DDS.h>
#include <libdap/D4Group.h>
#include <libdap/D4Attributes.h>
#include <HE5Grid.h>
#include <HE5Var.h>
#include <HE5GridPara.h>

// This struct stores the link object address and the shortest path link. 
// Note: if it is necessary to retrieve all the link paths, uncomment
// vector<string>link_paths and change corresponding code.

#if (H5_VERS_MAJOR == 1 && ((H5_VERS_MINOR == 12) || (H5_VERS_MINOR == 13)) || (H5_VERS_MINOR == 14))
typedef struct {
    H5O_token_t  link_addr;
    string slink_path;
    //vector<string> link_paths;
} link_info_t;
#else 
typedef struct {
    haddr_t  link_addr;
    string slink_path;
} link_info_t;
#endif

enum class HE5_TYPE {SW,GD,ZA};

typedef struct {

    std::string xdim_fqn;
    std::string ydim_fqn;
    int xdim_size;
    int ydim_size;

    /// The bottom coordinate value of a Grid
    double point_lower;
    /// The top coordinate value of a Grid
    double point_upper;
    /// The leftmost coordinate value of a Grid
    double point_left;
    /// The rightmost coordinate value of a Grid
    double point_right;

    // The following pixel registration, grid origin, and projection code
    // are defined in include/HE5_HdfEosDef.h that can be found in the
    // HDF-EOS5 library distribution.

    // PixelRegistration
    // These are actually EOS5 constants, but we define these
    // since we do not depend on the HDF-EOS5 library.
    EOS5GridPRType pixelregistration; // either _HE5_HDFE_(CENTER|CORNER)

    // GridOrigin
    EOS5GridOriginType gridorigin; // one of HE5_HDFE_GD_(U|L)(L|R)

    // ProjectionCode
    EOS5GridPCType projection;

    // Projection parameters
    double param[13];

    // zone (may only be applied to UTM)
    int zone;

    // sphere
    int sphere;
 
} eos5_grid_info_t;

typedef struct {
    std::string dpath0;
    std::string dpath1;
} eos5_dname_info_t;

typedef struct {
    std::string vpath0;
    std::string vpath1;
    std::string cf_gmap_path;
} eos5_cname_info_t;

typedef struct {
    std::unordered_map<std::string,std::vector<std::string>> varpath_to_dims;
    std::unordered_map<std::string,std::vector<HE5Dim>> grppath_to_dims;
    std::unordered_map<std::string,eos5_grid_info_t> gridname_to_info;
    std::vector<std::pair<eos5_dname_info_t,eos5_cname_info_t>> dimpath_to_cvpath;
} eos5_dim_info_t;


bool breadth_first(hid_t, hid_t, const char *, libdap::D4Group* par_grp, const char *,bool,bool,
                   std::vector<link_info_t>&, eos5_dim_info_t & ,std::vector<std::string> &);
void obtain_hdf5_object_name(hid_t pid, hsize_t obj_index, const char *gname, std::vector<char> &oname);
bool check_soft_external_links(libdap::D4Group *par_grp, hid_t pid, int & slinkindex, const char *gname,
                               const std::vector<char> &oname, bool handle_softlink);
void handle_actual_dataset(libdap::D4Group *par_grp, hid_t pid, const string &full_path_name, const string &fname,
                           bool use_dimscale, bool is_eos5, eos5_dim_info_t &eos5_dim_info);
void handle_pure_dimension(libdap::D4Group *par_grp, hid_t pid, const std::vector<char>& oname, bool is_eos5,
                           const std::string &full_path_name);
void handle_eos5_datasets(libdap::D4Group* par_grp, const char *gname, eos5_dim_info_t &eos5_dim_info);
void handle_child_grp(hid_t file_id, hid_t pid, const char *gname, libdap::D4Group* par_grp, const char *fname,
                      bool use_dimscale, bool is_eos5,std::vector<link_info_t> & hdf5_hls,
                      eos5_dim_info_t & eos5_dim_info, std::vector<std::string> & handled_cv_names,
                      const std::vector<char>& oname);

void read_objects(libdap::D4Group* d4_grp,hid_t, const std::string & varname, const std::string & filename, hid_t, bool, bool,
                  eos5_dim_info_t &);
void read_objects_base_type(libdap::D4Group* d4_grp,hid_t, const std::string & varname, const std::string & filename, hid_t,
                            bool, bool, eos5_dim_info_t &);
void read_objects_basetype_attr_hl(const std::string &varname, libdap::BaseType *bt, hid_t dset_id,  bool is_eos5);

void read_objects_structure(libdap::D4Group* d4_grp,const std::string & varname, const std::string & filename,
                            hid_t, bool, bool);
void read_objects_structure_arrays(libdap::D4Group *d4_grp, libdap::Structure *structure, const std::string & varname,
                                   const std::string &newvarname, const std::string &filename, hid_t dset_id,
                                   bool is_eos5);
void read_objects_structure_scalar(libdap::D4Group *d4_grp, libdap::Structure *structure, const std::string & varname,
                                   hid_t dset_id, bool is_eos5);

std::string obtain_new_varname(const std::string &varname, bool use_dimscale, bool is_eos5);

std::string get_hardlink_dmr(hid_t, const std::string &);
void get_softlink(libdap::D4Group* par_grp, hid_t,  const std::string &, int,size_t);
void map_h5_dset_hardlink_to_d4(hid_t h5_objid,const std::string & full_path, libdap::BaseType* d4b,
                                libdap::Structure * d4s,int flag);

/// A function that maps HDF5 attributes to DAP4
void map_h5_attrs_to_dap4(hid_t oid,libdap::D4Group* d4g, libdap::BaseType* d4b, libdap::Structure * d4s,int flag);
/// A function that maps HDF5 object full path as an attribute to DAP4
void map_h5_varpath_to_dap4_attr(libdap::D4Group* d4g,libdap::BaseType* d4b,libdap::Structure * d4s,
                                 const std::string &,short flag);

/// Add DAP4 coverage 
void add_dap4_coverage_default(libdap::D4Group* d4_grp,const std::vector<std::string>& handled_coord_names);
void add_dap4_coverage_default_internal(libdap::D4Group* d4_grp, std::unordered_map<std::string,libdap::Array*> &,
                                        std::unordered_map<std::string,libdap::Array*> &);
void obtain_ds_name_array_maps(libdap::D4Group*, std::unordered_map<std::string,libdap::Array*> &,
                                const std::vector<std::string>& handled_coord_names);
void obtain_ds_name_array_maps_internal(libdap::BaseType *v, std::unordered_map<string,libdap::Array*>&dsn_array_maps,
                               const std::vector<std::string>& handled_all_cv_names);
void obtain_coord_names(libdap::Array*, std::vector<std::string>& coord_names);
void obtain_multi_string_coord_names(libdap::D4Attribute *d4_attr, std::vector<std::string> & coord_names);
void make_coord_names_fpath(libdap::D4Group*, std::vector<std::string>& coord_names);
bool obtain_no_path_cv(libdap::D4Group*, std::string &coord_name);
void handle_absolute_path_cv(const libdap::D4Group*, std::string &coord_name);
void handle_relative_path_cv(const libdap::D4Group*, std::string &coord_name);
void handle_relative_path_cvname_internal(const libdap::D4Group *d4_grp, std::string &coord_name,
                                          unsigned short sep_count);

void remove_empty_coord_names(std::vector<std::string>&);
void obtain_handled_dim_names(libdap::Array*, std::unordered_set<std::string> & handled_dim_names);
void add_coord_maps(libdap::D4Group*, libdap::Array*, std::vector<std::string> &coord_name,
                    std::unordered_map<std::string,libdap::Array*> & coname_array_maps, std::unordered_set<std::string>&);
void add_dimscale_maps(libdap::Array*, std::unordered_map<std::string,libdap::Array*> & dc_array_maps,
                       const std::unordered_set<std::string> & handled_dim_names);
void reorder_vars(libdap::D4Group*, const std::map<std::string,libdap::Array*> &coname_array_maps,
                  const std::map<std::string,libdap::Array*> & dc_array_maps);
void reorder_vars_internal(libdap::D4Group* d4_grp, const std::vector<int> &cv_pos,
                           const std::vector<libdap::BaseType *> &cv_obj_ptr,int stop_index);
void reorder_vars_internal_final_phase(libdap::D4Group* d4_grp, const std::vector<int> &mov_cv_pos,
                                       const std::vector<int> &mov_front_pos,
                                       const std::vector<libdap::BaseType *> &mov_front_vptr,
                                       const std::vector<libdap::BaseType *> &mov_cv_ptr);
#if 0
bool is_cvar(const libdap::BaseType*, const std::unordered_map<std::string,libdap::Array*> &coname_array_maps, const std::unordered_map<std::string,libdap::Array*> & dc_array_maps);
#endif

/// EOS5 handling 
string read_struct_metadata(hid_t s_file_id);
void obtain_struct_metadata_info(hid_t ecs_grp_id, std::vector<std::string> &s_oname, std::vector<bool> &smetatype,
                                 int &strmeta_num_total, bool &strmeta_no_suffix, hsize_t nelems) ;
int obtain_struct_metadata_value(hid_t ecs_grp_id, const std::vector<std::string> &s_oname,
                                  const std::vector<bool> &smetatype, hsize_t nelems,
                                  std::vector<std::string> &strmeta_value, std::string &total_strmeta_value) ;
int obtain_struct_metadata_value_internal(hid_t ecs_grp_id, const vector<string> &s_oname,
                                           vector<string> &strmeta_value, string &total_strmeta_value,
                                           const string &finstr, hsize_t i);
int get_strmetadata_num(const string & meta_str);

void obtain_eos5_dims(hid_t fileid, eos5_dim_info_t &);
void build_var_dim_path(const std::string & eos5_obj_name, const std::vector<HE5Var>& var_list,
                        std::unordered_map<std::string, std::vector<std::string>>& varpath_to_dims,
                        HE5_TYPE eos5_type, bool is_geo);
void build_grp_dim_path(const std::string & eos5_obj_name, const std::vector<HE5Dim>& dim_list,
                        std::unordered_map<std::string, std::vector<HE5Dim>>& grppath_to_dims, HE5_TYPE eos5_type);
bool obtain_eos5_dim(const std::string & varname, const std::unordered_map<std::string,
                     vector<std::string>>& varpath_to_dims, vector<std::string> & dimnames);
bool obtain_eos5_grp_dim(const std::string & varname, const std::unordered_map<std::string,
                         vector<HE5Dim>>& grppath_to_dims, vector<std::string> & dimnames);

void add_possible_eos5_grid_vars(libdap::D4Group*,  eos5_dim_info_t &);
void add_eos5_grid_vars_geo(libdap::D4Group* d4_grp,  const eos5_grid_info_t & eg_info);
void add_eos5_grid_vars_non_geo(libdap::D4Group* d4_grp, eos5_dim_info_t &eos5_dim_info,  const eos5_grid_info_t & eg_info);
bool no_eos5_grid_vars_in_grp(libdap::D4Group *d4_group, const eos5_grid_info_t &eg_info);

void build_gd_info(const HE5Grid &gd,std::unordered_map<std::string,eos5_grid_info_t>& gridname_to_info);
bool is_eos5_grid_grp(libdap::D4Group *,const eos5_dim_info_t &eos5_dim_info, eos5_grid_info_t &);

// Pure dimensions etc. handling
hsize_t obtain_unlim_pure_dim_size(hid_t pid, const std::string &dname);
hsize_t obtain_unlim_pure_dim_size_internal(hid_t dset_id, const std::string &dname, const std::string &reference_name);
hsize_t obtain_unlim_pure_dim_size_internal_value(hid_t dset_id, hid_t attr_id, hid_t atype_id,
                                                  const std::string &reference_name, const std::string &dname);
void add_ps_cf_grid_mapping_attrs(libdap::BaseType *dummy_proj_cf, const eos5_grid_info_t &);
void add_lamaz_cf_grid_mapping_attrs(libdap::BaseType *dummy_proj_cf, const eos5_grid_info_t &);
void add_possible_var_cv_info(libdap::BaseType *, const eos5_dim_info_t &eos5_dim_info);
void make_attributes_to_cf(libdap::BaseType *, const eos5_dim_info_t &eos5_dim_info);

#endif
