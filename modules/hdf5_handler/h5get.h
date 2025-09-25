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
// You can contact The HDF Group, Inc. at 410 E University Avenue, Suite 200,
// Champaign, IL 61820
////////////////////////////////////////////////////////////////////////////////
/// \file h5get.h
/// Helper functions to generate DDS/DAS/DODS/DMR for the default option.
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
/// \author Kent Yang <myang6@hdfgroup.org>


#ifndef _H5GET_H
#define _H5GET_H
#include "hdf5_handler.h"
#include "h5common.h"
#include "h5apicompatible.h"

union attr_data_ptr_t {
        unsigned char* ucp;
        char *tcp;
        short *tsp;
        unsigned short *tusp;
        int *tip;
        unsigned int*tuip;
        long *tlp;
        unsigned long*tulp;
        float *tfp;
        double *tdp;
};

bool check_h5str(hid_t);

void close_fileid(hid_t fid);

hid_t get_attr_info(hid_t dset, int index, bool, DSattr_t * attr_inst, bool*);
bool check_ignored_attrs(hid_t attrid, hid_t ty_id, const vector <char>& attr_name, bool is_dap4);

std::string get_dap_type(hid_t type, bool);
std::string get_dap_integer_type(hid_t dtype, bool);

void get_dataset_dmr(hid_t file_id, hid_t pid, const std::string &dname, DS_t * dt_inst_ptr, bool has_dimscale,
                     bool is_eos5, bool &is_pure_dims, std::vector<link_info_t> &, std::vector<std::string> &, const eos5_dim_info_t &);
void get_dataset(hid_t pid, const std::string &dname, DS_t * dt_inst_ptr);

hid_t get_fileid(const char *filename);

std::string print_attr(hid_t type, int loc, void *sm_buf);
void print_integer_type_attr(hid_t atype, int loc, void*sm_buf, union attr_data_ptr_t gp, std::vector<char> &rep ) ;
void print_float_type_attr(hid_t atype, int loc, void*sm_buf, union attr_data_ptr_t gp, vector<char> &rep );

D4AttributeType daptype_strrep_to_dap4_attrtype(const std::string & s);

libdap::BaseType *Get_bt_enhanced(libdap::D4Group *d4_grp, hid_t pid, const std::string &vname, const std::string &var_path,
                         const std::string &dataset, hid_t datatype);


libdap::BaseType *Get_bt(const std::string &vname, const std::string &var_path,
                         const std::string &dataset, hid_t datatype, bool is_dap4);

libdap::BaseType *Get_integer_bt(const std::string &vname, const std::string &vpath,
                                const std::string &dataset, hid_t datatype, bool is_dap4);
libdap::BaseType *Get_byte_bt(const std::string &vname, const std::string &vpath, const std::string &dataset,
                        H5T_sign_t sign, bool is_dap4);
libdap::BaseType *Get_float_bt(const std::string &vname, const std::string &vpath,
                                const std::string &dataset, hid_t datatype);

libdap::Structure *Get_structure(const std::string &varname, const std::string &var_path,
                                const std::string &dataset, hid_t datatype,bool is_dap4);
void Get_structure_array_type(libdap::Structure *structure_ptr, hid_t memb_type, const std::string &memb_name,
                              const std::string &dataset, bool is_dap4 );

void handle_vlen_int_float(libdap::D4Group *d4_grp, hid_t pid, const std::string &vname, const std::string &var_path,
                           const std::string &filename, hid_t dset_id);

bool check_dimscale(hid_t fid);
bool has_dimscale_attr(hid_t dataset);
void obtain_dimnames(hid_t file_id, hid_t dset, int ndim, DS_t*dt_inst_ptr, std::vector<link_info_t>&, bool is_eos5, const eos5_dim_info_t &);
void obtain_dimnames_internal(hid_t file_id,hid_t dset,int ndims, DS_t *dt_inst_ptr,std::vector<link_info_t> & hdf5_hls,
                              bool is_eos5, const string &dimlist_name, const eos5_dim_info_t &);
std::string obtain_dimname_deref(hid_t ref_dset, const DS_t *dt_inst_ptr);
void obtain_dimname_hardlinks(hid_t file_id, hid_t ref_dset, vector<link_info_t>& hdf5_hls, std::string & trim_objname);
bool handle_dimscale_dmr(hid_t file_id, hid_t dset, hid_t dspace,  bool is_eos5,
                         DS_t * dt_inst_ptr,std::vector<link_info_t> &hdf5_hls,std::vector<std::string> &handled_cv_names,
                         const eos5_dim_info_t &);

bool check_var_null_dim_name(hid_t);
bool has_null_dim_name(hid_t);
void write_vlen_str_attrs(hid_t attr_id, hid_t ty_id, const DSattr_t *, libdap::D4Attribute *d4_attr,
                          libdap::AttrTable* d2_attr, bool is_dap4);

libdap::D4EnumDef* map_hdf5_enum_to_dap4(libdap::D4Group *d4_grp, hid_t pid, hid_t datatype);
void obtain_enum_def_name_value(hid_t base_datatype, hid_t datatype, vector<string>& labels, vector<int64_t> &label_values);

bool check_if_utf8_str(hid_t ty_id);
bool check_str_attr_value(hid_t attr_id, hid_t atype_id, const string & value_to_compare, bool is_substr);
hsize_t obtain_number_elements(hid_t space_id);
std::string obtain_vlstr_values(std::vector<char> & temp_buf, hid_t atype_id, size_t ty_size,
                           hsize_t nelmts, hid_t aspace_id);

std::string obtain_assigned_obj_name(const vector<string>& obj_names, const string &obj_name_mark);
std::string obtain_shortest_ancestor_path(const std::vector<std::string> &);

std::string handle_string_special_characters(std::string &s);
std::string handle_string_special_characters_in_path(const string &instr);

std::string invalid_type_error_msg(const std::string &s);
#endif                          //_H5GET_H
