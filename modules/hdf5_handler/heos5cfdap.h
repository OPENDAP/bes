// This file is part of hdf5_handler: an HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c)  2011-2023 The HDF Group, Inc. and OPeNDAP, Inc.
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
/// \file heos5cfdap.h
/// \brief Map and generate DDS and DAS for the CF option for HDF-EOS5 products 
///
/// This file also includes a function to retrieve ECS metadata in C++ string forms.
/// \author Keng Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////


#ifndef _HEOS5CFDAP_H
#define _HEOS5CFDAP_H
#include <libdap/DDS.h>
#include <libdap/DAS.h>
#include "hdf5.h"
#include "HDF5CF.h"
#include "h5commoncfdap.h"

enum EOS5Metadata
{StructMeta,CoreMeta,ArchivedMeta,SubsetMeta,ProductMeta,XMLMeta,OtherMeta};
void map_eos5_cfdds(libdap::DDS &, hid_t, const std::string &);
void map_eos5_cfdas(libdap::DAS &, hid_t, const std::string &);
void gen_eos5_cfdds(libdap::DDS &, const HDF5CF::EOS5File*);
void gen_eos5_cfdas(libdap::DAS &, hid_t, HDF5CF::EOS5File*);

void gen_eos5_cf_ignored_obj_info(libdap::DAS &,const HDF5CF::EOS5File*);
void gen_dap_oneeos5cvar_dds(libdap::DDS &,const HDF5CF::EOS5CVar*,const hid_t, const std::string &);

#if 0
//void gen_dap_oneeos5cf_dmr(libdap::D4Group*,const HDF5CF::EOS5CVar*);
#endif

void gen_dap_oneeos5cf_dds(libdap::DDS &,const HDF5CF::EOS5CVar* );
void gen_dap_oneeos5cf_das(libdap::DAS &,const std::vector<HDF5CF::Var*>&,const HDF5CF::EOS5CVar* ,const unsigned short);

// DMR
void map_eos5_cfdmr(libdap::D4Group*,hid_t, const std::string &);
void gen_eos5_cfdmr(libdap::D4Group*, const HDF5CF::EOS5File*);
void gen_dap_oneeos5cvar_dmr(libdap::D4Group*,const HDF5CF::EOS5CVar*,const hid_t, const std::string &);
void gen_dap_eos5cf_gm_dmr(libdap::D4Group*,const HDF5CF::EOS5File*);
void gen_gm_proj_spvar_info(libdap::D4Group* d4_root,const HDF5CF::EOS5File* f);
void gen_gm_oneproj_var(libdap::D4Group*, const HDF5CF::EOS5CVar*, const unsigned short, const HDF5CF::EOS5File *f); 
void gen_gm_oneproj_spvar(libdap::D4Group*, const HDF5CF::EOS5CVar*); 
void gen_gm_proj_var_info(libdap::D4Group* ,const HDF5CF::EOS5File* );
void add_var_sp_attrs_to_dap4(libdap::BaseType *d4_var,const HDF5CF::EOS5CVar* cvar);

void read_ecs_metadata(hid_t file_id, std::string & st_str, std::string & core_str, std::string & arch_str,std::string &xml_str, std::string& subset_str, std::string & product_str,std::string &other_str,bool st_only);
int get_metadata_num(const std::string &);
#endif
