
// This file is part of hdf5_handler: an HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c) 2011-2013 The HDF Group, Inc. and OPeNDAP, Inc.
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

void map_gmh5_cfdds(DDS &, hid_t, const string &);
void map_gmh5_cfdas(DAS &, hid_t, const string &);
void gen_gmh5_cfdds(DDS &, HDF5CF::GMFile *);
void gen_gmh5_cfdas(DAS &, HDF5CF::GMFile *);
void gen_dap_onegmcvar_dds(DDS &,const HDF5CF::GMCVar*,const hid_t, const string &);
void gen_dap_onegmspvar_dds(DDS &dds,const HDF5CF::GMSPVar* spvar, const hid_t, const string & filename);

#endif
