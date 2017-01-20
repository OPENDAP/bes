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
#define I_HDF5RequestHandler_H 1

#include<string>
#include<map>
#include "BESRequestHandler.h"
#include "HDF5_DataMemCache.h"

class ObjMemCache; // in bes/dap

namespace libdap {

    class DAS;
    class DDS;
    class DataDDS;

}

/// \file HDF5RequestHandler.h
/// \brief include the entry functions to execute the handlers
///
/// It includes build_das, build_dds and build_data etc.
/// \author James Gallagher <jgallagher@opendap.org>
class HDF5RequestHandler:public BESRequestHandler {
  public:
    HDF5RequestHandler(const string & name);
    virtual ~HDF5RequestHandler(void);

    static bool hdf5_build_das(BESDataHandlerInterface & dhi);
    static bool hdf5_build_dds(BESDataHandlerInterface & dhi);
    static bool hdf5_build_data(BESDataHandlerInterface & dhi);
    static bool hdf5_build_dmr(BESDataHandlerInterface & dhi);
    static bool hdf5_build_help(BESDataHandlerInterface & dhi);
    static bool hdf5_build_version(BESDataHandlerInterface & dhi);

    static bool get_usecf()       { return _usecf;}
    static bool get_pass_fileid() { return _pass_fileid;}
    static bool get_disable_structmeta() { return _disable_structmeta;}
    static bool get_keep_var_leading_underscore() { return _keep_var_leading_underscore;}
    static bool get_check_name_clashing() { return _check_name_clashing;}
    static bool get_add_path_attrs() { return _add_path_attrs;}
    static bool get_drop_long_string() { return _drop_long_string;}
    static bool get_fillvalue_check() { return _fillvalue_check;}
    static bool get_check_ignore_obj() { return _check_ignore_obj;}

    static string get_stp_east_filename() {return _stp_east_filename;}
    static string get_stp_north_filename() {return _stp_north_filename;}
    // Handling Cache
    static unsigned int get_mdcache_entries() { return _mdcache_entries;}
    static unsigned int get_lrdcache_entries() { return _lrdcache_entries;}
    static unsigned int get_srdcache_entries() { return _srdcache_entries;}
    static float get_cache_purge_level() { return _cache_purge_level;}
    //static bool  check_dds_cache() {return (dds_cache?true:false);}
    
    static ObjMemCache* get_lrdata_mem_cache() {return lrdata_mem_cache;}
    void set_lrdata_mem_cache(ObjMemCache* my_lrdata_mem_cache) 
                                             {lrdata_mem_cache=my_lrdata_mem_cache;}

    static ObjMemCache* get_srdata_mem_cache() {return srdata_mem_cache;}
    void set_srdata_mem_cache(ObjMemCache* my_srdata_mem_cache) 
                                             {srdata_mem_cache=my_srdata_mem_cache;}

    static bool get_common_cache_dirs() { return _common_cache_dirs;}
    static void get_lrd_cache_dir_list(vector<string>& cur_lrd_cache_dir_list) 
                                              { cur_lrd_cache_dir_list = lrd_cache_dir_list;}

    static void get_lrd_non_cache_dir_list(vector<string>& cur_lrd_non_cache_dir_list) 
                                              { cur_lrd_non_cache_dir_list = lrd_non_cache_dir_list;}

    static void get_lrd_var_cache_file_list(vector<string>& cur_lrd_var_cache_file_list) 
                                              { cur_lrd_var_cache_file_list = lrd_var_cache_file_list;}

                                              

  private:
     //cache variables. 

     static unsigned int _mdcache_entries;
     static unsigned int _lrdcache_entries;
     static unsigned int _srdcache_entries;
     static float _cache_purge_level;

     static ObjMemCache *das_cache;
     static ObjMemCache *dds_cache;
     static ObjMemCache *dmr_cache;
     static ObjMemCache *lrdata_mem_cache;
     static ObjMemCache *srdata_mem_cache;


     // BES keys
     static bool _usecf;
     static bool _pass_fileid;
     static bool _disable_structmeta;
     static bool _keep_var_leading_underscore;
     static bool _check_name_clashing;
     static bool _add_path_attrs;
     static bool _drop_long_string;
     static bool _fillvalue_check;
     static bool _check_ignore_obj;
     //static bool _ld_mcache_config;
     //static bool _sd_mcache_config;
     static string _stp_east_filename;
     static string _stp_north_filename;
     
     static bool _common_cache_dirs;
     static vector<string> lrd_cache_dir_list;
     static vector<string> lrd_non_cache_dir_list;
     static vector<string> lrd_var_cache_file_list;
     static bool obtain_lrd_common_cache_dirs();

     static bool hdf5_build_data_with_IDs(BESDataHandlerInterface &dhi);
     static bool hdf5_build_dmr_with_IDs(BESDataHandlerInterface &dhi);
     static void get_dds_with_attributes(const string &filename, const string&container_name,libdap::DDS*dds);
};

#endif
