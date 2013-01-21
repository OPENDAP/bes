// This file is part of hdf5_handler: an HDF5 file handler for the OPeNDAP
// data server.

// Copyright (c)  2011-2012 The HDF Group, Inc. and OPeNDAP, Inc.
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
/// \file heos5cfdap.h
/// \brief Map and generate DDS and DAS for the CF option for HDF-EOS5 products 
///
/// This file also includes a function to retrieve ECS metadata in C++ string forms.
/// \author Muqun Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////


#ifndef _HEOS5CFDAP_H
#define _HEOS5CFDAP_H
#include <DDS.h>
#include <DAS.h>
#include "hdf5.h"
#include "HDF5CF.h"
#include "h5commoncfdap.h"
using namespace std;
using namespace libdap;

enum EOS5Metadata
{StructMeta,CoreMeta,ArchivedMeta,SubsetMeta,ProductMeta,XMLMeta,OtherMeta};
void map_eos5_cfdds(DDS &, hid_t, const string &);
void map_eos5_cfdas(DAS &, hid_t, const string &);
void gen_eos5_cfdds(DDS &, HDF5CF::EOS5File*);
void gen_eos5_cfdas(DAS &, hid_t, HDF5CF::EOS5File*);
void gen_dap_oneeos5cvar_dds(DDS &,const HDF5CF::EOS5CVar*,const string &);
void read_ecs_metadata(hid_t file_id, string & st_str, string & core_str, string & arch_str,string &xml_str, string& subset_str, string & product_str,string &other_str,bool st_only);
int get_metadata_num(const string &);
#endif
