
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of hdf4_handler, a data handler for the OPeNDAP data
// server. 

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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

// CDFRequestHandler.h

#ifndef I_HDF4RequestHandler_H
#define I_HDF4RequestHandler_H 1

#include <string>

#include "BESRequestHandler.h"

class HDF4RequestHandler:public BESRequestHandler {

  private:
    static std::string _cachedir;
    static bool hdf4_build_data_with_IDs(BESDataHandlerInterface & dhi);
    static bool hdf4_build_dds_cf_sds(BESDataHandlerInterface & dhi);
    static bool hdf4_build_das_cf_sds(BESDataHandlerInterface & dhi);
    static bool hdf4_build_data_cf_sds(BESDataHandlerInterface & dhi);
    static bool hdf4_build_data_cf_sds_with_IDs(BESDataHandlerInterface & dhi);
    static bool hdf4_build_dmr_with_IDs(BESDataHandlerInterface & dhi);
    static bool hdf4_build_direct_dmr(BESDataHandlerInterface & dhi);

    //BES keys - check the file h4.conf.in for descriptions
    //Key to turn on the CF option
    static bool _usecf;

    // Keys to tune the performance -general
    static bool _pass_fileid;
    static bool _disable_structmeta;
    static bool _enable_special_eos;
    static bool _disable_scaleoffset_comp;
    static bool _disable_ecsmetadata_min;
    static bool _disable_ecsmetadata_all;
    
    // Keys to tune the performance - cache
    static bool _enable_eosgeo_cachefile;
    static bool _enable_data_cachefile;
    static bool _enable_metadata_cachefile;

   // Keys to handle vdata and vgroups
   static bool _enable_hybrid_vdata;
   static bool _enable_ceres_vdata;
   static bool _enable_vdata_attr;
   static bool _enable_vdata_desc_attr;
   static bool _disable_vdata_nameclashing_check;
   static bool _enable_vgroup_attr;

  // Misc. keys
  static bool _enable_check_modis_geo_file;
  static bool _enable_swath_grid_attr;
  static bool _enable_ceres_merra_short_name;
  static bool _enable_check_scale_offset_type;
  static bool _disable_swath_dim_map;

  static bool _cache_latlon_path_exist;
  static std::string _cache_latlon_path;
  static bool _cache_latlon_prefix_exist;
  static std::string _cache_latlon_prefix;
  static bool _cache_latlon_size_exist;
  static long _cache_latlon_size;

  static bool _cache_metadata_path_exist;
  static std::string _cache_metadata_path;
   
  static bool _direct_dmr;

  public:
    explicit HDF4RequestHandler(const std::string & name);
    ~HDF4RequestHandler(void) override = default;

    static bool hdf4_build_das(BESDataHandlerInterface & dhi);
    static bool hdf4_build_dds(BESDataHandlerInterface & dhi);
    static bool hdf4_build_data(BESDataHandlerInterface & dhi);
    static bool hdf4_build_dmr(BESDataHandlerInterface & dhi);
    static bool hdf4_build_help(BESDataHandlerInterface & dhi);
    static bool hdf4_build_version(BESDataHandlerInterface & dhi);

    static bool get_direct_dmr() { return _direct_dmr; }

    // CF key
    static bool get_usecf() { return _usecf; }

    // Keys to tune the performance -general
    static bool get_pass_fileid() { return _pass_fileid; }
    static bool get_disable_structmeta() { return _disable_structmeta; }
    static bool get_enable_special_eos() { return _enable_special_eos; }
    static bool get_disable_scaleoffset_comp() { return _disable_scaleoffset_comp; }
    static bool get_disable_ecsmetadata_min() { return _disable_ecsmetadata_min; }
    static bool get_disable_ecsmetadata_all() { return _disable_ecsmetadata_all; }

    // Keys to tune the performance - cache
    static bool get_enable_eosgeo_cachefile() { return _enable_eosgeo_cachefile;}
    static bool get_enable_data_cachefile() { return _enable_data_cachefile;}
    static bool get_enable_metadata_cachefile() { return _enable_metadata_cachefile;}

    // Keys to handle vdata and vgroups
    static bool get_enable_hybrid_vdata() { return _enable_hybrid_vdata; }
    static bool get_enable_ceres_vdata() { return _enable_ceres_vdata; }
    static bool get_enable_vdata_attr() { return _enable_vdata_attr; }
    static bool get_enable_vdata_desc_attr() { return _enable_vdata_desc_attr; }
    static bool get_disable_vdata_nameclashing_check() { return _disable_vdata_nameclashing_check;}
    static bool get_enable_vgroup_attr() {return _enable_vgroup_attr;}

    // Misc. keys
    static bool get_enable_check_modis_geo_file() { return _enable_check_modis_geo_file; }
    static bool get_enable_swath_grid_attr() { return _enable_swath_grid_attr;}
    static bool get_enable_ceres_merra_short_name() { return _enable_ceres_merra_short_name;}
    static bool get_enable_check_scale_offset_type() { return _enable_check_scale_offset_type;}
    static bool get_disable_swath_dim_map() { return _disable_swath_dim_map;}

    static bool get_cache_latlon_path_exist() { return _cache_latlon_path_exist; }
    static std::string get_cache_latlon_path() {return _cache_latlon_path; }

    static bool get_cache_latlon_prefix_exist() { return _cache_latlon_prefix_exist; }
    static std::string get_cache_latlon_prefix() {return _cache_latlon_prefix;}

    static bool get_cache_latlon_size_exist() { return _cache_latlon_size_exist; }
    static long get_cache_latlon_size() { return _cache_latlon_size; }

    static bool get_cache_metadata_path_exist() { return _cache_metadata_path_exist; }
    static std::string get_cache_metadata_path() { return _cache_metadata_path;}

};


#endif
