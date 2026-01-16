// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of hdf5_handler, a data handler for the OPeNDAP data
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

// HDF5RequestHandler.h

#ifndef I_HDF5RequestHandler_H
#define I_HDF5RequestHandler_H 

#include <string>
#include <map>
#include <vector>
#include "BESRequestHandler.h"
#include "HDF5_DataMemCache.h"
#include <BESDDSResponse.h>
#include <BESDataDDSResponse.h>
#include <BESDMRResponse.h>
#include <hdf5.h>

class ObjMemCache; // in bes/dap

namespace libdap {

    class DAS;
    class DDS;
    class DMR;
    class DataDDS;

}

/// \file HDF5RequestHandler.h
/// \brief include the entry functions to execute the handlers
///
/// It includes build_das, build_dds and build_data etc.
/// \author James Gallagher <jgallagher@opendap.org>
class HDF5RequestHandler:public BESRequestHandler {
  public:
    explicit HDF5RequestHandler(const std::string & name);
    ~HDF5RequestHandler(void) override;

    static bool hdf5_build_das(BESDataHandlerInterface & dhi);
    static bool hdf5_build_dds(BESDataHandlerInterface & dhi);
    static bool hdf5_build_data(BESDataHandlerInterface & dhi);
    static bool hdf5_build_dmr(BESDataHandlerInterface & dhi);
    static bool hdf5_build_help(BESDataHandlerInterface & dhi);
    static bool hdf5_build_version(BESDataHandlerInterface & dhi);

    static bool get_usecf()       { return _usecf;}
    static bool get_pass_fileid() { return _pass_fileid;}
    static bool get_disable_structmeta() { return _disable_structmeta;}
    static bool get_disable_ecsmeta() { return _disable_ecsmeta;}
    static bool get_keep_var_leading_underscore() { return _keep_var_leading_underscore;}
    static bool get_check_name_clashing() { return _check_name_clashing;}
    static bool get_add_path_attrs() { return _add_path_attrs;}
    static bool get_drop_long_string() { return _drop_long_string;}
    static bool get_fillvalue_check() { return _fillvalue_check;}
    static bool get_check_ignore_obj() { return _check_ignore_obj;}
    static bool get_force_flatten_coor_attr() { return _flatten_coor_attr;}
    static bool get_eos5_rm_convention_attr_path() { return _eos5_rm_convention_attr_path;}
    static bool get_dmr_long_int() { return _dmr_long_int;}
    static bool get_no_zero_size_fullnameattr() { return _no_zero_size_fullnameattr;}
    static bool get_enable_coord_attr_add_path() { return _enable_coord_attr_add_path;}

    static bool get_usecfdmr() { return _usecfdmr;}
    static bool get_add_dap4_coverage() { return _add_dap4_coverage;}

    // Default Handle dimension key, only valid when the default option is on.
    static bool get_default_handle_dimension() { return _default_handle_dimension;}
    static bool get_default_add_unlimited_dimension_dap4() { return _add_unlimited_dimension_dap4;}
    static bool set_default_add_unlimited_dimension_dap4(bool flag) { _add_unlimited_dimension_dap4 = flag;}

    static void set_dmr_64bit_int(libdap::DMR *dmr) { dmr_int64 = dmr;}
    static libdap::DMR* get_dmr_64bit_int() {return dmr_int64;}

    // Handling Memory Cache
    static string get_stp_east_filename() {return _stp_east_filename;}
    static string get_stp_north_filename() {return _stp_north_filename;}

    // Handling Cache
    static unsigned int get_mdcache_entries() { return _mdcache_entries;}
    static unsigned int get_lrdcache_entries() { return _lrdcache_entries;}
    static unsigned int get_srdcache_entries() { return _srdcache_entries;}
    static float get_cache_purge_level() { return _cache_purge_level;}

    
    static ObjMemCache* get_lrdata_mem_cache() {return lrdata_mem_cache;}
    void set_lrdata_mem_cache(ObjMemCache* my_lrdata_mem_cache) const 
                                             {lrdata_mem_cache=my_lrdata_mem_cache;}

    static ObjMemCache* get_srdata_mem_cache() {return srdata_mem_cache;}
    void set_srdata_mem_cache(ObjMemCache* my_srdata_mem_cache) const
                                             {srdata_mem_cache=my_srdata_mem_cache;}

    static bool get_common_cache_dirs() { return _common_cache_dirs;}
    static void get_lrd_cache_dir_list(std::vector<string>& cur_lrd_cache_dir_list) 
                                              { cur_lrd_cache_dir_list = lrd_cache_dir_list;}

    static void get_lrd_non_cache_dir_list(std::vector<string>& cur_lrd_non_cache_dir_list) 
                                              { cur_lrd_non_cache_dir_list = lrd_non_cache_dir_list;}

    static void get_lrd_var_cache_file_list(std::vector<string>& cur_lrd_var_cache_file_list) 
                                              { cur_lrd_var_cache_file_list = lrd_var_cache_file_list;}

    // Handling Disk Cache
    static bool get_use_disk_cache() {return _use_disk_cache;}
    static string get_disk_cache_dir() { return _disk_cache_dir;}
    static string get_disk_cachefile_prefix() { return _disk_cachefile_prefix;}
    static unsigned long long get_disk_cache_size() {return _disk_cache_size;}
    static bool get_disk_cache_comp_data() { return _disk_cache_comp_data;}
    static bool get_disk_cache_float_only_comp(){return _disk_cache_float_only_comp_data;}
    static float get_disk_comp_threshold() {return _disk_cache_comp_threshold;}
    static unsigned long get_disk_var_size() {return _disk_cache_var_size; }

    static bool get_use_eosgeo_cachefile() {return _use_latlon_disk_cache; }
    static string get_latlon_disk_cache_dir() { return _latlon_disk_cache_dir;}
    static string get_latlon_disk_cachefile_prefix() { return _latlon_disk_cachefile_prefix;}
    static long get_latlon_disk_cache_size() {return _latlon_disk_cache_size;}

    // jhrg 3/9/22
    static bool get_escape_utf8_attr() {return _escape_utf8_attr;}

    // This handler supports the "not including attributes" in
    // the data access feature. Attributes are generated only
    // if necessary. KY 10/30/19
    void add_attributes(BESDataHandlerInterface &dhi) override;

  private:
    // Loads configuration state from TheBESKeys
    void static load_config();
    void static load_config_disk_cache();
    void static load_config_cf_cache();

    //cache variables.

    static unsigned int _mdcache_entries;
    static unsigned int _lrdcache_entries;
    static unsigned int _srdcache_entries;
    static float _cache_purge_level;

    static ObjMemCache *das_cache;
    static ObjMemCache *dds_cache;
    static ObjMemCache *datadds_cache;
    static ObjMemCache *dmr_cache;
    static ObjMemCache *lrdata_mem_cache;
    static ObjMemCache *srdata_mem_cache;

    // BES keys
    static bool _usecf;
    static bool _pass_fileid;
    static bool _disable_structmeta;
    static bool _disable_ecsmeta;
    static bool _keep_var_leading_underscore;
    static bool _check_name_clashing;
    static bool _add_path_attrs;
    static bool _drop_long_string;
    static bool _fillvalue_check;
    static bool _check_ignore_obj;
    static bool _flatten_coor_attr;
    static bool _eos5_rm_convention_attr_path;
    static bool _dmr_long_int;
    static bool _no_zero_size_fullnameattr;
    static bool _enable_coord_attr_add_path;

    static bool _usecfdmr;
    static bool _add_dap4_coverage;

    // For the default option
    static bool _default_handle_dimension;
    static bool _add_unlimited_dimension_dap4;
    static std::string _stp_east_filename;
    static std::string _stp_north_filename;

    static libdap::DMR *dmr_int64;
    static bool _use_disk_cache;
    static std::string _disk_cache_dir;
    static std::string _disk_cachefile_prefix;
    static unsigned long long _disk_cache_size;
    static bool _disk_cache_comp_data;
    static bool _disk_cache_float_only_comp_data;
    static float _disk_cache_comp_threshold;
    static unsigned long _disk_cache_var_size;


    static bool _use_disk_meta_cache;
    static bool _use_disk_dds_cache;
    static std::string _disk_meta_cache_path;

    static bool _use_latlon_disk_cache;
    static long _latlon_disk_cache_size;
    static std::string _latlon_disk_cache_dir;
    static std::string _latlon_disk_cachefile_prefix;

    static bool _common_cache_dirs;

    static bool _escape_utf8_attr;   // jhrg 3/9/22

    static std::vector<std::string> lrd_cache_dir_list;
    static std::vector<string> lrd_non_cache_dir_list;
    static std::vector<string> lrd_var_cache_file_list;
    static bool obtain_lrd_common_cache_dirs();
    static void obtain_lrd_common_cache_dirs_data_vars(std::vector<std::string> &temp_name_list,
                                                    const std::string &subline, char sep);

    static bool hdf5_build_data_with_IDs(BESDataHandlerInterface &dhi);
    static bool hdf5_build_dmr_with_IDs(BESDataHandlerInterface &dhi);

    static bool hdf5_build_dmr_from_file(BESDataHandlerInterface & dhi, BESDMRResponse &bes_dmr_response,
                                         libdap::DMR *dmr,const std::string &filename, hid_t &cf_fileid, hid_t &fileid);

    static void hdf5_build_das_internal_no_mem_cache(const std::string& filename, libdap::DAS *das, hid_t &cf_fileid);

    static void get_dds_with_attributes(BESDDSResponse*bdds,BESDataDDSResponse*data_bdds,
                                         const std::string &container_name,const std::string &filename,
                                         const std::string &dds_cache_fname, const std::string &das_cache_fname,
                                         bool dds_from_dc,bool das_from_dc, bool build_data);

    static void get_dds_without_attributes_datadds(BESDataDDSResponse*data_bdds, const std::string &filename);
    static void read_datadds_from_file(libdap::DDS *dds, const std::string &filename, hid_t &cf_fileid, hid_t &fileid);

    static void read_dds_from_disk_cache(BESDDSResponse* bdds, BESDataDDSResponse* data_bdds,bool build_data,
                                         const std::string & container_name,const std::string & h5_fname,
                                        const std::string & dds_cache_fname,const std::string &das_cache_fname,
                                        hid_t h5_fd, bool das_from_dc);

    static void read_dds_from_file(libdap::DDS *dds, const std::string &filename, hid_t &cf_fileid, hid_t &fileid,
                                     const std::string &dds_cache_fname, bool dds_from_dc);

    static void add_das_to_dds(libdap::DDS *dds,const std::string &container_name, const std::string &filename,
                               const std::string &das_cache_fname,hid_t h5_fd, bool das_from_dc);
    static void add_das_to_dds_wrapper(libdap::DDS *dds, const std::string &filename, hid_t cf_fileid, hid_t fileid,
                                            const std::string &container_name, const std::string &das_cache_fname,
                                            bool das_from_dc);

    static bool write_dds_to_disk_cache(const std::string& dds_cache_fname,libdap::DDS *dds_ptr);

    static bool write_das_to_disk_cache(const std::string & das_cache_fname, libdap::DAS *das_ptr);

    static bool read_das_from_disk_cache(const std::string & cache_filename,libdap::DAS *das_ptr);

    static void read_das_from_file(libdap::DAS *das, const std::string &filename, const std::string &das_cache_fname,
                                        hid_t h5_fd, bool das_from_dc);

    static void close_h5_files(hid_t cf_fileid, hid_t file_id);
};

#endif
