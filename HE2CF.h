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

        libdap::DAS* das;

        // SDStart ID
        int32 sd_id;

        // Hopen ID 
        int32 file_id;

        // Number of global attributes
        int32 num_global_attributes;

        // ECS metadata 
        std::string metadata;

        // group name
        std::string gname;

        // Store all metadata names.
        // Not an ideal approach, need to re-visit later. KY 2012-6-11
        std::vector<std::string>  eosmetadata_namelist;
    
        // Data field SDS name to SDS id 
        std::map < std::string, int32 > vg_dsd_map;

        // Data field vdata name to vdata id
        std::map < std::string, int32 > vg_dvd_map;

        // Geolocation field SDS name to SDS id 
        std::map < std::string, int32 > vg_gsd_map;

        // Geolocation field vdata name to vdata id
        std::map < std::string, int32 > vg_gvd_map;

        // Add metadata_name to the metadata name list.
        void set_eosmetadata_namelist(const std::string &metadata_name)
        {
            eosmetadata_namelist.push_back(metadata_name);
        }

        // Is this EOS metadata
        bool is_eosmetadata(const std::string& metadata_name) {
            return (std::find(eosmetadata_namelist.begin(),eosmetadata_namelist.end(),metadata_name) !=eosmetadata_namelist.end());
        }
         
        // Get HDF-EOS2 "Data Fields" and "Geolocation Fields" group object tag and object reference numbers.
        bool get_vgroup_field_refids(const std::string&  _gname, int32* _ref_df, int32* _ref_gf);

        // Open SD 
        bool open_sd(const std::string& filename,const int sd_id);

        // Open vgroup
        bool open_vgroup(const std::string& filename,const int fileid);

        // Combine ECS metadata coremetadata.0, coremetadata.1 etc. into one string.
        bool set_metadata(const std::string& metadataname,std::vector<std::string>&non_num_names, std::vector<std::string>&non_num_data);

        // This routine will generate three ECS metadata lists. Note in theory list sl1 and sl2 should be sorted.
        // Since the ECS metadata is always written(sorted) in increasing numeric order, we don't perform this now.
        // Should watch if there are any outliers. 
        void arrange_list(std::list<std::string> & sl1, std::list<std::string>&sl2,std::vector<std::string>&v1,std::string name,int& flag);

        // Obtain SD attribute value
        void obtain_SD_attr_value(const std::string &,std::string&);

        // Create SDS name to SDS ID and Vdata name to vdata ID maps.
        bool set_vgroup_map(int32 refid,bool isgeo);

        // Write the long_name attribute.
        bool write_attr_long_name(const std::string& long_name, 
            const std::string& varname,
            int fieldtype);
        bool write_attr_long_name(const std::string& group_name, 
            const std::string& long_name, 
            const std::string& varname,
            int fieldtype);

        // Write the SD attribute.
        bool write_attr_sd(int32 sds_id, const std::string& newfname,int fieldtype);

        /// Check if we can ignore lat/lon scale/offset factors
        short check_scale_offset(int32 sds_id, bool is_scale);


        // Write the Vdata attribute.
        bool write_attr_vdata(int32 vd_id, const std::string& newfname,int fieldtype);
        void throw_error(std::string _error);
    
    public:
        HE2CF();
        virtual ~HE2CF();

        /// closes the opened file.
        bool   close();

        /// retrieves the merged metadata.
        string get_metadata(const std::string& metadataname,bool&suffix_is_num,std::vector<std::string>&non_num_names, std::vector<std::string>&non_num_data);
        bool set_non_ecsmetadata_attrs();

        /// openes \afilename  HDF4 file.
        bool   open(const std::string& filename,const int sd_id, const int file_id);

        /// sets DAS pointer so that we can bulid attribute tables.
        void   set_DAS(libdap::DAS* das);

        /// writes attribute table into DAS given grid/swath name and
        /// its field name.
        bool   write_attribute(const std::string& gname, 
            const std::string& fname, 
            const std::string& newfname,
            int n_groups,
            int fieldtype);

        /// writes _FillValue attribute into \a varname attribute table.
        /// 
        /// This attribute plays an essential role for two dimensional
        /// coordinate system like Swath.
        bool write_attribute_FillValue(const std::string& varname, int type, float val);

        /// writes coordinates attribute into \a varname attribute table.
        /// 
        /// This attribute plays an essential role for two dimensional
        /// coordinate system like Swath.
        bool write_attribute_coordinates(const std::string& varname, std::string coord);

        /// writes units attribute into \a varname attribute table.
        /// 
        /// Any existing units attribute will be overwritten by this function.
        bool write_attribute_units(const std::string& varname, std::string units);


};


#endif
