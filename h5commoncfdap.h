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
/// \file h5commoncfdap.h
/// \brief Functions to generate DDS and DAS for one object(variable). 
///
/// 
/// \author Muqun Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////

#ifndef _H5COMMONCFDAP_H
#define _H5COMMONCDDAP_H
#include <DDS.h>
#include <DAS.h>
#include "hdf5.h"
#include <string>
//#include <TheBESKeys.h>
//#include <BESUtil.h>
#include "HDF5CF.h"
using namespace std;
using namespace libdap;
void gen_dap_onevar_dds(DDS &dds,const HDF5CF::Var*,const string &);
void gen_dap_oneobj_das(AttrTable*,const HDF5CF::Attribute*,const HDF5CF::Var*);
bool need_special_attribute_handling(const HDF5CF::Attribute*,const HDF5CF::Var*);
void gen_dap_special_oneobj_das(AttrTable*,const HDF5CF::Attribute*,const HDF5CF::Var*);
void gen_dap_str_attr(AttrTable*,const HDF5CF::Attribute *);
#endif
