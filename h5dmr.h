
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
/// \file h5dds.h
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


#include <H5Gpublic.h>
#include <H5Fpublic.h>
#include <H5Ipublic.h>
#include <H5Tpublic.h>
#include <H5Spublic.h>
#include <H5Apublic.h>

#include <H5public.h>

#include <DDS.h>

using namespace libdap;

#include <D4Group.h>
#include <D4Attributes.h>

//bool depth_first(hid_t, char *,  D4Group* par_grp, const char *);

//bool breadth_first(hid_t, char *, DMR &, D4Group* par_grp, const char *,bool);
bool breadth_first(hid_t, char *, D4Group* par_grp, const char *,bool);

//void read_objects(DMR & dmr, D4Group* d4_grp,const string & varname, const string & filename,const hid_t);
void read_objects(D4Group* d4_grp,const string & varname, const string & filename,const hid_t);
void read_objects_base_type(D4Group* d4_grp,const string & varname, const string & filename,const hid_t);
void read_objects_structure(D4Group* d4_grp,const string & varname, const string & filename,const hid_t);


string get_hardlink_dmr(hid_t, const string &);
void get_softlink(D4Group* par_grp, hid_t,  const string &, int,size_t);
void map_h5_dset_hardlink_to_d4(hid_t h5_objid,const string & full_path, BaseType* d4b,Structure * d4s,int flag);

