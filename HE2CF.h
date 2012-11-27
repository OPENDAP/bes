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
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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

        // SDStart return value
        int32 sd_id;
        // Hopen return value
        int32 file_id;
        int32 num_global_attributes;

        string metadata;
        string gname;

        // Store all metadata names.
        // Not an ideal approach, need to re-visit later. KY 2012-6-11
        vector<string>  eosmetadata_namelist;
    
        map < string, int32 > vg_sd_map;
        map < string, int32 > vg_vd_map;

        void set_eosmetadata_namelist(const string &metadata_name)
        {
            eosmetadata_namelist.push_back(metadata_name);
        }

        bool is_eosmetadata(const string& metadata_name) {
            return (find(eosmetadata_namelist.begin(),eosmetadata_namelist.end(),metadata_name) !=eosmetadata_namelist.end());
        }
         
        bool get_vgroup_field_refids(const string&  _gname, int32* _ref_df, int32* _ref_gf);
        bool open_sd(const string& filename);
        bool open_vgroup(const string& filename);
        bool set_metadata(const string& metadataname,vector<string>&non_num_names, vector<string>&non_num_data);
        void arrange_list(list<string> & sl1, list<string>&sl2,vector<string>&v1,string name,int& flag);
        void obtain_SD_attr_value(const string &,string&);

        bool set_vgroup_map(int32 refid);
        bool write_attr_long_name(const string& long_name, 
            const string& varname,
            int fieldtype);
        bool write_attr_long_name(const string& group_name, 
            const string& long_name, 
            const string& varname,
            int fieldtype);
        bool write_attr_sd(int32 sds_id, const string& newfname);
        bool write_attr_vdata(int32 vd_id, const string& newfname);
        void write_error(string _error);
    
    public:
        HE2CF();
        virtual ~HE2CF();

        /// closes the opened file.
        bool   close();

        /// retrieves the merged metadata.
        string get_metadata(const string& metadataname,bool&suffix_is_num,vector<string>&non_num_names, vector<string>&non_num_data);
        bool set_non_ecsmetadata_attrs();

        /// openes \afilename  HDF4 file.
        bool   open(const string& filename);

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
