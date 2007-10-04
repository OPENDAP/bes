////////////////////////////////////////////////////////////////////////////////
/// \file h5das.h
/// \brief Data attributes processing header
///
/// This file is part of h5_dap_handler, A C++ implementation of the DAP handler
/// for HDF5 data.
///    
/// It defines functions that generate data attributes from HDF5 files.
/// 
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
/// \author Muqun Yang <ymuqun@hdfgroup.org>
///
/// Copyright (C) 2007	HDF Group, Inc.
///
/// Copyright (C) 1999 National Center for Supercomputing Applications.
///
/// All rights reserved.
////////////////////////////////////////////////////////////////////////////////
#include <string>
#include <hdf5.h>


using std::string;

#include "DAS.h"
void add_group_structure_info(DAS & das, char* gname, char* oname, bool is_group);
bool depth_first( hid_t , char *, DAS &, const char * );
bool find_gloattr( hid_t file, DAS &das );
int  get_hardlink( hid_t, const string &);
bool get_softlink( DAS &, hid_t, const string &, int );
void read_comments( DAS & das, const string & varname, hid_t oid );
void read_objects( DAS &das, const string &varname, hid_t dset, int num_attr );
#ifdef CF
void add_dimension_attributes(DAS & das);
#endif
