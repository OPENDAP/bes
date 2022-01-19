
// This file is part of hdf5_handler: an HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c) 2011-2016 The HDF Group, Inc. and OPeNDAP, Inc.
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
/// \file h5gmcfdap.h
/// \brief Map and generate DDS and DAS for the CF option for generic HDF5 products 
///
/// 
/// \author Muqun Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////


#ifndef _H5GMCFDAP_H
#define _H5GMCFDAP_H
#include "h5commoncfdap.h"
#include "HDF5GCFProduct.h"

void map_gmh5_cfdds(libdap::DDS &, hid_t, const std::string &);
void map_gmh5_cfdas(libdap::DAS &, hid_t, const std::string &);

void map_gmh5_cfdmr(libdap::D4Group*, hid_t, const std::string &);
void gen_gmh5_cfdds(libdap::DDS &, HDF5CF::GMFile *);
void gen_gmh5_cfdas(libdap::DAS &, HDF5CF::GMFile *);
void gen_gmh5_cfdmr(libdap::D4Group* d4_root, HDF5CF::GMFile *);
void gen_gmh5_cf_ignored_obj_info(libdap::DAS &, HDF5CF::GMFile *);
void gen_dap_onegmcvar_dds(libdap::DDS &,const HDF5CF::GMCVar*,const hid_t, const std::string &);
void gen_dap_onegmspvar_dds(libdap::DDS &dds,const HDF5CF::GMSPVar* spvar, const hid_t, const std::string & filename);
void update_GPM_special_attrs(libdap::DAS &, const HDF5CF::Var* var,bool );

void gen_dap_onegmcvar_dmr(libdap::D4Group* d4_root,const HDF5CF::GMCVar*,const hid_t, const std::string &);
void gen_dap_onegmspvar_dmr(libdap::D4Group* d4_root,const HDF5CF::GMSPVar* spvar, const hid_t, const std::string & filename);

#endif
