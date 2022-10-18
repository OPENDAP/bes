
// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c) 2007-2016 The HDF Group, Inc. and OPeNDAP, Inc.
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
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  

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
/// \author Muqun Yang <ymuqun@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////


#ifndef _h5dmr_H
#define _h5dmr_H
#include <unordered_map>
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
#include <HE5Var.h>

// This struct stores the link object address and the shortest path link. 
// Note: if it is necessary to retrieve all the link paths, uncomment
// vector<string>link_paths and change corresponding code.

#if (H5_VERS_MAJOR == 1 && ((H5_VERS_MINOR == 12) || (H5_VERS_MINOR == 13)))
typedef struct {
    H5O_token_t  link_addr;
    string slink_path;
    //vector<string> link_paths;
} link_info_t;
#else 
typedef struct {
    haddr_t  link_addr;
    string slink_path;
    //vector<string> link_paths;
} link_info_t;
#endif

enum class HE5_TYPE {SW,GD,ZA};

typedef struct {
    std::unordered_map<std::string,std::vector<std::string>> varpath_to_dims;
    std::unordered_map<std::string,std::vector<std::string>> grppath_to_dims;
} eos5_dim_info_t;
#if 0
typedef struct {
    int varpath_to_dims;
    float grppath_to_dims;
} eos5_dim_info_t;
#endif

bool breadth_first(const hid_t, hid_t, const char *, libdap::D4Group* par_grp, const char *,bool,bool,std::vector<link_info_t>&, std::unordered_map<std::string, std::vector<std::string>>& );

void read_objects(libdap::D4Group* d4_grp,const std::string & varname, const std::string & filename,const hid_t, bool, bool,std::unordered_map<std::string, std::vector<std::string>>&);
void read_objects_base_type(libdap::D4Group* d4_grp,const std::string & varname, const std::string & filename,const hid_t, bool, bool, std::unordered_map<std::string, std::vector<std::string>>&);
void read_objects_structure(libdap::D4Group* d4_grp,const std::string & varname, const std::string & filename,const hid_t, bool, bool, std::unordered_map<std::string, std::vector<std::string>>&);

string get_hardlink_dmr(hid_t, const std::string &);
void get_softlink(libdap::D4Group* par_grp, hid_t,  const std::string &, int,size_t);
void map_h5_dset_hardlink_to_d4(hid_t h5_objid,const std::string & full_path, libdap::BaseType* d4b,libdap::Structure * d4s,int flag);

/// A function that maps HDF5 attributes to DAP4
void map_h5_attrs_to_dap4(hid_t oid,libdap::D4Group* d4g, libdap::BaseType* d4b, libdap::Structure * d4s,int flag);

/// A function that maps HDF5 object full path as an attribute to DAP4
void map_h5_varpath_to_dap4_attr(libdap::D4Group* d4g,libdap::BaseType* d4b,libdap::Structure * d4s,const std::string &,short flag);

/// EOS5 handling 
string read_struct_metadata(hid_t s_file_id);
int get_strmetadata_num(const string & meta_str);
void obtain_eos5_var_dims(hid_t fileid, eos5_dim_info_t &);
//void obtain_eos5_var_dims(hid_t fileid, std::unordered_map<std::string, std::vector<std::string>>&);
void build_var_dim_path(const std::string & eos5_obj_name, std::vector<HE5Var> var_list, std::unordered_map<std::string, std::vector<std::string>>& varpath_to_dims, HE5_TYPE eos5_type, bool is_geo);
void build_grp_dim_path(const std::string & eos5_obj_name, std::vector<HE5Dim> dim_list, std::unordered_map<std::string, std::vector<std::string>>& grppath_to_dims, HE5_TYPE eos5_type);
bool obtain_eos5_dim(const std::string & varname, const std::unordered_map<std::string, vector<std::string>>& varpath_to_dims, vector<std::string> & dimnames);
#endif
