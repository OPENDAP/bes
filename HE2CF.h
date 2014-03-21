//////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// Copyright (c) 2010-2012 The HDF Group
//
// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org>
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
// You should have received a copy of the GNU Lesser General Public License
// along with this software; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
//////////////////////////////////////////////////////////////////////////////
#ifndef _HE2CF_H_
#define _HE2CF_H_

#include <mfhdf.h>
#include <hdf.h>
//#include <HdfEosDef.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include "debug.h"
#include "DAS.h"
#include "HDFCFUtil.h"


using namespace std;
using namespace libdap;


/// A class for writing attributes from an HDF-EOS2 file.
///
/// This class contains functions that generates SDS attributes
/// and Vdata attributes. Since HDF-EOS2 library API don't
/// have access to field attributes, we need to use generic
/// HDF4 API.
///
/// For most HDF-EOS2 files, Grid doesn't use Vdata.
/// Swath has Vdata but fields with Vdata normally contain
/// attributes. However, we still check and provide ways to
/// generate attributes.
///
///
/// @author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
class HE2CF
{

    private:

        DAS* das;

        // SDStart ID
        int32 sd_id;

        // Hopen ID 
        int32 file_id;

        // Number of global attributes
        int32 num_global_attributes;

        // ECS metadata 
        string metadata;

        // group name
        string gname;

        // Store all metadata names.
        // Not an ideal approach, need to re-visit later. KY 2012-6-11
        vector<string>  eosmetadata_namelist;
    
        // SDS name to SDS id 
        map < string, int32 > vg_sd_map;

        // vdata name to vdata id
        map < string, int32 > vg_vd_map;

        // Add metadata_name to the metadata name list.
        void set_eosmetadata_namelist(const string &metadata_name)
        {
            eosmetadata_namelist.push_back(metadata_name);
        }

        // Is this EOS metadata
        bool is_eosmetadata(const string& metadata_name) {
            return (find(eosmetadata_namelist.begin(),eosmetadata_namelist.end(),metadata_name) !=eosmetadata_namelist.end());
        }
         
        // Get HDF-EOS2 "Data Fields" and "Geolocation Fields" group object tag and object reference numbers.
        bool get_vgroup_field_refids(const string&  _gname, int32* _ref_df, int32* _ref_gf);

        // Open SD 
        bool open_sd(const string& filename,const int sd_id);

        // Open vgroup
        bool open_vgroup(const string& filename,const int fileid);

        // Combine ECS metadata coremetadata.0, coremetadata.1 etc. into one string.
        bool set_metadata(const string& metadataname,vector<string>&non_num_names, vector<string>&non_num_data);

        // This routine will generate three ECS metadata lists. Note in theory list sl1 and sl2 should be sorted.
        // Since the ECS metadata is always written(sorted) in increasing numeric order, we don't perform this now.
        // Should watch if there are any outliers. 
        void arrange_list(list<string> & sl1, list<string>&sl2,vector<string>&v1,string name,int& flag);

        // Obtain SD attribute value
        void obtain_SD_attr_value(const string &,string&);

        // Create SDS name to SDS ID and Vdata name to vdata ID maps.
        bool set_vgroup_map(int32 refid);

        // Write the long_name attribute.
        bool write_attr_long_name(const string& long_name, 
            const string& varname,
            int fieldtype);
        bool write_attr_long_name(const string& group_name, 
            const string& long_name, 
            const string& varname,
            int fieldtype);

        // Write the SD attribute.
        bool write_attr_sd(int32 sds_id, const string& newfname);

        // Write the Vdata attribute.
        bool write_attr_vdata(int32 vd_id, const string& newfname);
        void throw_error(string _error);
    
    public:
        HE2CF();
        virtual ~HE2CF();

        /// closes the opened file.
        bool   close();

        /// retrieves the merged metadata.
        string get_metadata(const string& metadataname,bool&suffix_is_num,vector<string>&non_num_names, vector<string>&non_num_data);
        bool set_non_ecsmetadata_attrs();

        /// openes \afilename  HDF4 file.
        bool   open(const string& filename,const int sd_id, const int file_id);

        /// sets DAS pointer so that we can bulid attribute tables.
        void   set_DAS(DAS* das);

        /// writes attribute table into DAS given grid/swath name and
        /// its field name.
        bool   write_attribute(const string& gname, 
            const string& fname, 
            const string& newfname,
            int n_groups,
            int fieldtype);

        /// writes _FillValue attribute into \a varname attribute table.
        /// 
        /// This attribute plays an essential role for two dimensional
        /// coordinate system like Swath.
        bool write_attribute_FillValue(const string& varname, int type, float val);

        /// writes coordinates attribute into \a varname attribute table.
        /// 
        /// This attribute plays an essential role for two dimensional
        /// coordinate system like Swath.
        bool write_attribute_coordinates(const string& varname, string coord);

        /// writes units attribute into \a varname attribute table.
        /// 
        /// Any existing units attribute will be overwritten by this function.
        bool write_attribute_units(const string& varname, string units);

};


#endif
