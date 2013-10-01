// This file is part of the hdf5_handler implementing for the CF-compliant
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
/// \file HDF5GCFProduct.h
/// \brief This file includes functions to identify different NASA HDF5 products.
/// Current supported products include MEaSUREs SeaWiFS, OZone, Aquarius level 3
/// Decadal survey SMAP level 2 and ACOS level 2S.
///
/// \author Muqun Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////

#ifndef _H5GCFProduct_H
#define _H5GCFProduct_H

#include <string>
#include <iostream>
#include <vector>
#include "hdf5.h"

using namespace std;


enum H5GCFProduct
{ General_Product,Mea_SeaWiFS_L2, Mea_SeaWiFS_L3,Mea_Ozone,Aqu_L3,ACOS_L2S,SMAP };

// Currently we only need to support four NASA generic HDF5 products for
// the access of NASA data via CF-compliant vis. tools(IDV and Panoply) 
// via OPeNDAP.
// MEaSUREs SeaWiFS level 2 and level 3
// MEaSUREs Ozone zonal average
// Aquarius level 3
// Decadal survey SMAP level 2
// ACOS_L2S needs special handling of 64-bit integer mapping but there 
// is no way to support the access of data via the CF-compliant tools.

// For all products 
static const char ROOT_NAME[] ="/";

// MEaSUREs SeaWiFS level 2 and 3  
static const char SeaWiFS_ATTR1_NAME[] ="instrument_short_name";
static const char SeaWiFS_ATTR2_NAME[] ="long_name";
static const char SeaWiFS_ATTR3_NAME[] ="short_name";
static const string SeaWiFS_ATTR1_VALUE ="SeaWiFS";

// FPVALUE means Part of the attribute VALUE starting from the First value. 
// PVALUE means Part VALUE.  
static const string SeaWiFS_ATTR2_FPVALUE ="SeaWiFS";
static const string SeaWiFS_ATTR2_L2PVALUE ="Level 2";
static const string SeaWiFS_ATTR2_L3PVALUE ="Level 3";
static const string SeaWiFS_ATTR3_L2FPVALUE ="SWDB_L2";
static const string SeaWiFS_ATTR3_L3FPVALUE ="SWDB_L3";

// Aquarius level 3
static const char Aquarius_ATTR1_NAME[] ="Sensor";
static const char Aquarius_ATTR2_NAME[] ="Title";
static const string Aquarius_ATTR1_VALUE ="Aquarius";
static const string Aquarius_ATTR2_PVALUE ="Level-3";

// SMAP simulation and ACOS L2S 
static const char SMAC2S_META_GROUP_NAME[] ="Metadata";
static const char SMAP_ATTR_NAME[] ="ProjectID";
static const string SMAP_ATTR_VALUE ="SMAP";

static const char ACOS_L2S_DSET_NAME[] ="ProjectId";
static const string ACOS_L2S_ATTR_VALUE ="ACOS";

// MEaSURES Ozone level 2 and level 3
static const char Ozone_ATTR1_NAME[] ="ProductType";
static const string Ozone_ATTR1_VALUE1 ="L3 Monthly Zonal Means";
static const string Ozone_ATTR1_VALUE2 ="L2 Daily Trajectory";

static const char Ozone_ATTR2_NAME[] ="ParameterName";
static const string Ozone_ATTR2_VALUE ="Nadir Profile and Total Column Ozone";

// Function to check the product type
H5GCFProduct check_product(hid_t fileid);

// Function to check if the product is MeaSure seaWiFS 
// The returned integer reference of level will tell the level
// of the SeaWiFS product.
bool check_measure_seawifs(hid_t root_id,int& level);

// Function to check if the product is Aquarius
// The returned integer reference of level will tell the level
// of the Aquarius product.
bool check_aquarius(hid_t root_id,int & level);

// Function to check if the product is ACOS Level 2 or SMAP.
//  which_product tells if the product is SMAP or ACOSL2S. 
//  For example, if which_product is SMAP, it will just check
//  if the attribute name and value are SMAP attribute and value.
//  Then return true or false. Similar case is applied to ACOSL2S.
bool check_smap_acosl2s(hid_t root_id, int which_product);

// Function to check if the product is MEaSURES Ozone zonal average or level 2.
bool check_measure_ozone(hid_t root_id);

// Function to check if the product is NETCDF4_GENERAL.
bool check_netcdf4_general(hid_t root_id);

// Obtain the attribute value of the HDF5 general attribute.
void obtain_gm_attr_value(hid_t group_id, const char* attr_name, string & attr_value);


#endif
