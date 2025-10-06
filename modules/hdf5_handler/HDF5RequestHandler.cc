// This file is part of hdf5_handler, a data handler for the OPeNDAP data
// server. 

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Copyright (c) 2007-2023 The HDF Group, Inc. and OPeNDAP, Inc.
// Authors:
// Kent Yang <myang6@hdfgroup.org>
// James Gallagher <jgallagher@opendap.org>
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
// You can contact The HDF Group, Inc. at 410 E University Ave,
// Suite 200, Champaign, IL 61820

// HDF5RequestHandler.cc
/// \file HDF5RequestHandler.cc
/// \brief The implementation of the entry functions to execute the handlers
///
/// It includes build_das, build_dds and build_data etc.
/// \author James Gallagher <jgallagher@opendap.org>

#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <libdap/DMR.h>
#include <libdap/D4BaseTypeFactory.h>
#include <ObjMemCache.h>
#include "HDF5_DMR.h"

#include <libdap/mime_util.h>
#include "hdf5_handler.h"
#include "HDF5RequestHandler.h"
#include "HDF5CFModule.h"
#include "HDF5_DDS.h"

#include <BESDASResponse.h>
#include <libdap/Ancillary.h>
#include <BESInfo.h>
#include <BESDapNames.h>
#include <BESResponseNames.h>
#include <BESVersionInfo.h>
#include <BESServiceRegistry.h>
#include <BESUtil.h>
#include <BESDapError.h>
#include <BESInternalFatalError.h>
#include <BESSyntaxUserError.h>
#include <TheBESKeys.h>
#include <BESDebug.h>
#include <BESStopWatch.h>
#include "h5get.h"

#define HDF5_NAME "h5"
#include "h5cfdaputil.h"

using namespace std;
using namespace libdap;


#define prolog std::string("HDF5RequestHandler::").append(__func__).append("() - ")

// The debug function to dump all the contents of a DAS table.
void get_attr_contents(AttrTable* temp_table);

// Five functions to generate a DAS cache file.
// 1. The wrapper function to call write_das_table_to_file to generate the cache.
void write_das_to_file(DAS*das_ptr,FILE* das_file);

// 2. The main function to generate the DAS cache. 
void write_das_table_to_file(AttrTable*temp_table,FILE* das_file);

// 3. The function to write the DAS container/table name to the cache
void write_container_name_to_file(const string&,FILE* das_file);

// 4. The function to write the DAS attribute name,type and values to the cache
void write_das_attr_info(AttrTable* dtp,const string&,const string&,FILE * das_file);

// 5. The function to copy a string to a memory buffer. 
char* copy_str(char *temp_ptr,const string & str);

// These two functions are for reading the DAS from the DAS cache file
// 1. Obtain the string from a memory buffer.
char* obtain_str(char*temp_ptr,string & str);

// 2. The main function to obtain DAS info from the cache.
char* get_attr_info_from_dc(char*temp_pointer,DAS *das,AttrTable *at);


// Obtain the BES key as an integer
static unsigned int get_uint_key(const string &key,unsigned int def_val);
static unsigned long get_ulong_key(const string &key,unsigned long def_val);

// Obtain the BES key as a floating-pointer number.
static float get_float_key(const string &key, float def_val);

// Obtain the BES key as a string.
static string get_beskeys(const string&);

// Obtain the BES key info.
bool obtain_beskeys_info(const string &, bool &);

// For the CF option to read DAS, DDS and DMR.
extern void read_cfdas(DAS &das, const string & filename,hid_t fileid);
extern void read_cfdds(DDS &dds, const string & filename,hid_t fileid);
extern void read_cfdmr(DMR *dmr, const string & filename,hid_t fileid);

// Check the description of cache_entries and cache_purge_level at h5.conf.in.
unsigned int HDF5RequestHandler::_mdcache_entries = 500;
unsigned int HDF5RequestHandler::_lrdcache_entries = 0;
unsigned int HDF5RequestHandler::_srdcache_entries = 0;
float HDF5RequestHandler::_cache_purge_level = 0.2F;

// Metadata object cache at DAS,DDS and DMR.
ObjMemCache *HDF5RequestHandler::das_cache = nullptr;
ObjMemCache *HDF5RequestHandler::dds_cache = nullptr;
ObjMemCache *HDF5RequestHandler::datadds_cache = nullptr;
ObjMemCache *HDF5RequestHandler::dmr_cache = nullptr;

ObjMemCache *HDF5RequestHandler::lrdata_mem_cache = nullptr;
ObjMemCache *HDF5RequestHandler::srdata_mem_cache = nullptr;

// Set default values of all BES keys according to h5.conf.in.
// This will help the dmrpp module. No need to
// set multiple keys if the user's setting is the same as
// the h5.conf.in. KY 2021-08-23

// Note the CF option is still true by default. This may change if more users move to DAP4. KY 2023-07-11
bool HDF5RequestHandler::_usecf                       = true;
bool HDF5RequestHandler::_pass_fileid                 = false;
bool HDF5RequestHandler::_disable_structmeta          = true;
bool HDF5RequestHandler::_disable_ecsmeta             = false;
bool HDF5RequestHandler::_keep_var_leading_underscore = false;
bool HDF5RequestHandler::_check_name_clashing         = false;
bool HDF5RequestHandler::_add_path_attrs              = true;
bool HDF5RequestHandler::_drop_long_string            = true;
bool HDF5RequestHandler::_fillvalue_check             = true;
bool HDF5RequestHandler::_check_ignore_obj            = false;
bool HDF5RequestHandler::_flatten_coor_attr           = true;
bool HDF5RequestHandler::_default_handle_dimension    = true; //Ignored when _usecf=true.
bool HDF5RequestHandler::_add_unlimited_dimension_dap4   = false; //Ignored when _usecf=true.
bool HDF5RequestHandler::_eos5_rm_convention_attr_path = true;
bool HDF5RequestHandler::_dmr_long_int                = true;
bool HDF5RequestHandler::_no_zero_size_fullnameattr   = false;
bool HDF5RequestHandler::_enable_coord_attr_add_path  = true;

bool HDF5RequestHandler::_usecfdmr                    = true;
bool HDF5RequestHandler::_add_dap4_coverage           = true;

bool HDF5RequestHandler::_common_cache_dirs           = false;

bool HDF5RequestHandler::_use_disk_cache              = false;
bool HDF5RequestHandler::_use_disk_dds_cache          = false;
string HDF5RequestHandler::_disk_cache_dir;
string HDF5RequestHandler::_disk_cachefile_prefix;
unsigned long long HDF5RequestHandler::_disk_cache_size = 0;

bool HDF5RequestHandler::_disk_cache_comp_data        = false;
bool HDF5RequestHandler::_disk_cache_float_only_comp_data  = false;
float HDF5RequestHandler::_disk_cache_comp_threshold       = 1.0;
unsigned long HDF5RequestHandler::_disk_cache_var_size    = 0;

bool HDF5RequestHandler::_use_disk_meta_cache        = false;
string HDF5RequestHandler::_disk_meta_cache_path;

bool HDF5RequestHandler::_use_latlon_disk_cache        = false;
long HDF5RequestHandler::_latlon_disk_cache_size       = 0;
string HDF5RequestHandler::_latlon_disk_cache_dir;
string HDF5RequestHandler::_latlon_disk_cachefile_prefix;

bool HDF5RequestHandler::_escape_utf8_attr = true;

DMR* HDF5RequestHandler::dmr_int64 = nullptr;

string HDF5RequestHandler::_stp_east_filename;
string HDF5RequestHandler::_stp_north_filename;
vector<string> HDF5RequestHandler::lrd_cache_dir_list;
vector<string> HDF5RequestHandler::lrd_non_cache_dir_list;
vector<string> HDF5RequestHandler::lrd_var_cache_file_list;


HDF5RequestHandler::HDF5RequestHandler(const string & name)
    :BESRequestHandler(name)
{
    BESDEBUG(HDF5_NAME, prolog << "BEGIN" << endl);

    add_handler(DAS_RESPONSE, HDF5RequestHandler::hdf5_build_das);
    add_handler(DDS_RESPONSE, HDF5RequestHandler::hdf5_build_dds);
    add_handler(DATA_RESPONSE, HDF5RequestHandler::hdf5_build_data);
    add_handler(DMR_RESPONSE, HDF5RequestHandler::hdf5_build_dmr);
    add_handler(DAP4DATA_RESPONSE, HDF5RequestHandler::hdf5_build_dmr);

    add_handler(HELP_RESPONSE, HDF5RequestHandler::hdf5_build_help);
    add_handler(VERS_RESPONSE, HDF5RequestHandler::hdf5_build_version);

    // DYNAMIC_CONFIG_ENABLED is defined by TheBESKeys.h. As of 3/11/22, this
    // is 0 so the dynamic keys feature is not used. jhrg 3/8/22
#if !(DYNAMIC_CONFIG_ENABLED)
    load_config();
#endif

    BESDEBUG(HDF5_NAME, prolog << "END" << endl);
}

HDF5RequestHandler::~HDF5RequestHandler()
{
    // delete the cache.
    delete das_cache;
    delete dds_cache;
    delete datadds_cache;
    delete dmr_cache;
    delete lrdata_mem_cache;
    delete srdata_mem_cache;
     
}

/**
 * Loads configuration state from TheBESKeys
 */
void HDF5RequestHandler::load_config()
{
    BESDEBUG(HDF5_NAME, prolog << "BEGIN" << endl);
    BES_STOPWATCH_START(HDF5_NAME, prolog + "ClockTheBESKeys");

    // Obtain the metadata cache entries and purge level.
    HDF5RequestHandler::_mdcache_entries   = get_uint_key("H5.MetaDataMemCacheEntries", 0);
    HDF5RequestHandler::_lrdcache_entries  = get_uint_key("H5.LargeDataMemCacheEntries", 0);
    HDF5RequestHandler::_srdcache_entries  = get_uint_key("H5.SmallDataMemCacheEntries", 0);
    HDF5RequestHandler::_cache_purge_level = get_float_key("H5.CachePurgeLevel", 0.2F);

    if (get_mdcache_entries()) {  // else it stays at its default of null
        das_cache = new ObjMemCache(get_mdcache_entries(), get_cache_purge_level());
        dds_cache = new ObjMemCache(get_mdcache_entries(), get_cache_purge_level());
        datadds_cache = new ObjMemCache(get_mdcache_entries(), get_cache_purge_level());
        dmr_cache = new ObjMemCache(get_mdcache_entries(), get_cache_purge_level());
    }

    // Starting from hyrax 1.16.5, users don't need to explicitly set the BES keys if
    // they are happy with the default settings in the h5.conf.in.
    // In the previous releases, we required users to set BES key values. Otherwise,
    // the BES key values of true/false will always be false if the key is not found.
    // Hopefully this change will make it convenient for the general users.
    // In the meantime, the explicit BES key settings will not be affected.
    // KY 2021-10-13
    //
    // Check if the EnableCF key is set.
    bool has_key = false;
    bool key_value = obtain_beskeys_info("H5.EnableCF",has_key);
    if (has_key)
        _usecf = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.EnableCF: " << (_usecf?"true":"false") << endl);

    // The DefaultHandleDimension is effective only when EnableCF=false 
    key_value = obtain_beskeys_info("H5.DefaultHandleDimension",has_key);
    if (has_key)
        _default_handle_dimension  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.DefaultHandleDimension: " << (_default_handle_dimension?"true":"false") << endl);

    key_value = obtain_beskeys_info("H5.DefaultDAP4AddUnlimitedDimension",has_key);
    if (has_key)
        _add_unlimited_dimension_dap4  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.DefaultDAP4AddUnlimitedDimension: " << (_add_unlimited_dimension_dap4?"true":"false") << endl);


    // The following keys are only effective when EnableCF is true or unset(EnableCF is true if users don't set the key).
    key_value = obtain_beskeys_info("H5.EnablePassFileID",has_key);
    if (has_key)
        _pass_fileid  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.EnablePassFileID: " << (_pass_fileid?"true":"false") << endl);

    key_value = obtain_beskeys_info("H5.DisableStructMetaAttr",has_key);
    if (has_key)
        _disable_structmeta  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.DisableStructMetaAttr: " << (_disable_structmeta?"true":"false") << endl);

    key_value = obtain_beskeys_info("H5.DisableECSMetaAttr",has_key);
    if (has_key)
        _disable_ecsmeta = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.DisableECSMetaAttr: " << (_disable_ecsmeta?"true":"false") << endl);

    key_value = obtain_beskeys_info("H5.KeepVarLeadingUnderscore",has_key);
    if (has_key)
        _keep_var_leading_underscore  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.KeepVarLeadingUnderscore: " << (_keep_var_leading_underscore?"true":"false") << endl);

    key_value = obtain_beskeys_info("H5.EnableCheckNameClashing",has_key);
    if (has_key)
        _check_name_clashing  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.EnableCheckNameClashing: " << (_check_name_clashing?"true":"false") << endl);

    key_value = obtain_beskeys_info("H5.EnableAddPathAttrs",has_key);
    if (has_key)
        _add_path_attrs  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.EnableAddPathAttrs: " << (_add_path_attrs?"true":"false") << endl);

    key_value = obtain_beskeys_info("H5.EnableDropLongString",has_key);
    if (has_key)
        _drop_long_string  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.EnableDropLongString: " << (_drop_long_string?"true":"false") << endl);

    key_value = obtain_beskeys_info("H5.EnableFillValueCheck",has_key);
    if (has_key)
        _fillvalue_check  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.EnableFillValueCheck: " << (_fillvalue_check?"true":"false") << endl);


    key_value = obtain_beskeys_info("H5.CheckIgnoreObj",has_key);
    if (has_key)
        _check_ignore_obj  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.CheckIgnoreObj: " << (_check_ignore_obj?"true":"false") << endl);

    key_value = obtain_beskeys_info("H5.ForceFlattenNDCoorAttr",has_key);
    if (has_key)
        _flatten_coor_attr  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.ForceFlattenNDCoorAttr: " << (_flatten_coor_attr?"true":"false") << endl);

    key_value = obtain_beskeys_info("H5.RmConventionAttrPath",has_key);
    if (has_key)
        _eos5_rm_convention_attr_path  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.RmConventionAttrPath: " << (_eos5_rm_convention_attr_path?"true":"false") << endl);
   
    key_value = obtain_beskeys_info("H5.EnableDMR64bitInt",has_key);
    if (has_key)
        _dmr_long_int  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.EnableDMR64bitInt: " << (_dmr_long_int?"true":"false") << endl);
 
    key_value = obtain_beskeys_info("H5.NoZeroSizeFullnameAttr",has_key);
    if (has_key)
        _no_zero_size_fullnameattr  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.NoZeroSizeFullnameAttr: " << (_no_zero_size_fullnameattr?"true":"false") << endl);
 
    key_value = obtain_beskeys_info("H5.EnableCoorattrAddPath",has_key);
    if (has_key)
        _enable_coord_attr_add_path  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.EnableCoorattrAddPath: " << (_enable_coord_attr_add_path?"true":"false") << endl);

    // This is a critical key to ask the handler to generate the DMR directly for the CF option.
    // If this key is not true, the DMR will be generated via DDS and DAS. 64-bit integer mapping may be ignored or
    // incomplete. KY 2023-07-11
    key_value = obtain_beskeys_info("H5.EnableCFDMR",has_key);
    if (has_key)
        _usecfdmr  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.EnableCFDMR: " << (_usecfdmr?"true":"false") << endl);

    key_value = obtain_beskeys_info("H5.EnableDAP4Coverage",has_key);
    if (has_key)
        _add_dap4_coverage  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.EnableDAP4Coverage: " << (_add_dap4_coverage?"true":"false") << endl);

    // Load a bunch of keys to set the disk cache. Currently it only applies to CF but in the future it may
    // apply to the default option. KY 2023-07-11
    load_config_disk_cache();

    // The UTF8 escaping key applies to both CF and default options.
    key_value = obtain_beskeys_info("H5.EscapeUTF8Attr", has_key);
    if (has_key)
        _escape_utf8_attr = key_value;

    // load the cache keys when CF option is set.
    if (get_usecf())
        load_config_cf_cache();

    _stp_east_filename = get_beskeys("H5.STPEastFileName");
    _stp_north_filename = get_beskeys("H5.STPNorthFileName");
    BESDEBUG(HDF5_NAME, prolog << "END" << endl);
}

void HDF5RequestHandler::load_config_disk_cache() {

    bool has_key = false;
    bool key_value = obtain_beskeys_info("H5.EnableDiskDataCache",has_key);
    if (has_key)
        _use_disk_cache  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.EnableDiskDataCache: " << (_use_disk_cache?"true":"false") << endl);

    _disk_cache_dir              = get_beskeys("H5.DiskCacheDataPath");
    _disk_cachefile_prefix       = get_beskeys("H5.DiskCacheFilePrefix");
    _disk_cache_size             = get_ulong_key("H5.DiskCacheSize",0);

    key_value = obtain_beskeys_info("H5.DiskCacheComp",has_key);
    if (has_key)
        _disk_cache_comp_data  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.DiskCacheComp: " << (_disk_cache_comp_data?"true":"false") << endl);

    key_value = obtain_beskeys_info("H5.DiskCacheFloatOnlyComp",has_key);
    if (has_key)
        _disk_cache_float_only_comp_data  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.DiskCacheFloatOnlyComp: " << (_disk_cache_float_only_comp_data?"true":"false") << endl);
    _disk_cache_comp_threshold   = get_float_key("H5.DiskCacheCompThreshold",1.0);
    _disk_cache_var_size         = 1024*get_uint_key("H5.DiskCacheCompVarSize",0);

    key_value = obtain_beskeys_info("H5.EnableDiskMetaDataCache",has_key);
    if (has_key)
        _use_disk_meta_cache  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.EnableDiskMetaDataCache: " << (_use_disk_meta_cache?"true":"false") << endl);

    key_value = obtain_beskeys_info("H5.EnableDiskDDSCache",has_key);
    if (has_key)
        _use_disk_dds_cache  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.EnableDiskDDSCache: " << (_use_disk_dds_cache?"true":"false") << endl);
    _disk_meta_cache_path        = get_beskeys("H5.DiskMetaDataCachePath");

    key_value = obtain_beskeys_info("H5.EnableEOSGeoCacheFile",has_key);
    if (has_key)
        _use_latlon_disk_cache  = key_value;
    BESDEBUG(HDF5_NAME, prolog << "H5.EnableEOSGeoCacheFile: " << (_use_latlon_disk_cache?"true":"false") << endl);
    _latlon_disk_cache_size      = get_uint_key("H5.Cache.latlon.size",0);
    _latlon_disk_cache_dir       = get_beskeys("H5.Cache.latlon.path");
    _latlon_disk_cachefile_prefix= get_beskeys("H5.Cache.latlon.prefix");

    if (_disk_cache_comp_data == true && _use_disk_cache == true) {
        if (_disk_cache_comp_threshold < 1.0) {
            ostringstream ss;
            ss<< _disk_cache_comp_threshold;
            string _comp_threshold_str(ss.str());
            string invalid_comp_threshold ="The Compression Threshold is the total size of the variable array";
            invalid_comp_threshold+=" divided by the storage size of compressed array. It should always be >1";
            invalid_comp_threshold+=" The current threshold set at h5.conf is ";
            invalid_comp_threshold+=_comp_threshold_str;
            invalid_comp_threshold+=" . Go back to h5.conf and change the H5.DiskCacheCompThreshold to a >1.0 number.";
            throw BESInternalError(invalid_comp_threshold,__FILE__,__LINE__);
        }
    }
}

void HDF5RequestHandler::load_config_cf_cache() {

    bool has_key = false;
    if (get_lrdcache_entries()) {

        lrdata_mem_cache = new ObjMemCache(get_lrdcache_entries(), get_cache_purge_level());
        bool has_LFMC_config = false;
        bool key_value = obtain_beskeys_info("H5.LargeDataMemCacheConfig",has_key);
        if (has_key)
            has_LFMC_config  = key_value;
        BESDEBUG(HDF5_NAME, prolog << "H5.LargeDataMemCacheConfig: " << (has_LFMC_config?"true":"false") << endl);
        if (true == has_LFMC_config)
            _common_cache_dirs =obtain_lrd_common_cache_dirs();
    }
    if (get_srdcache_entries()) {

        BESDEBUG(HDF5_NAME, prolog << "Generate memory cache for smaller coordinate variables" << endl);
        srdata_mem_cache = new ObjMemCache(get_srdcache_entries(),get_cache_purge_level());

    }


}

// Build DAS
bool HDF5RequestHandler::hdf5_build_das(BESDataHandlerInterface & dhi)
{
    BESDEBUG(HDF5_NAME, prolog << "BEGIN" << endl);
#if DYNAMIC_CONFIG_ENABLED
    load_config();
#endif

    // For the time being, separate CF file ID from the default file ID(mainly for debugging)
    hid_t cf_fileid = -1;

    // Obtain the HDF5 file name.
    string filename = dhi.container->access();

    // Obtain the BES object from the client
    BESResponseObject *response = dhi.response_handler->get_response_object() ;

    // Convert to the BES DAS response
    auto bdas = dynamic_cast < BESDASResponse * >(response) ;
    if( !bdas )
        throw BESInternalError( "Cannot cast the BESResponse Object to a BESDASResponse object", __FILE__, __LINE__ ) ;

    try {
        bdas->set_container( dhi.container->get_symbolic_name() ) ;
        DAS *das = bdas->get_das();

        // Look inside the memory cache to see if it's initialized
        DAS *cached_das_ptr = nullptr;
        bool use_das_cache = false;
        if (das_cache) 
            cached_das_ptr = dynamic_cast<DAS*>(das_cache->get(filename));
        if (cached_das_ptr) 
            use_das_cache = true;
        
        if (true == use_das_cache) {

            // copy the cached DAS into the BES response object
            BESDEBUG(HDF5_NAME, prolog << "DAS Cached hit for : " << filename << endl);
            *das = *cached_das_ptr;
        }
        else
            hdf5_build_das_internal_no_mem_cache(filename, das, cf_fileid);
        bdas->clear_container() ;
    }

    catch(const BESSyntaxUserError & e) {
        if (cf_fileid !=-1)
            H5Fclose(cf_fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught BESSyntaxUserError! Message: " << e.get_message() << endl);
        throw;
    }

    catch(const BESInternalError & e) {
        if (cf_fileid !=-1)
            H5Fclose(cf_fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught BESInternalError! Message: " << e.get_message() << endl);
        throw;
    }

    catch(const BESDapError & e) {
        if (cf_fileid !=-1)
            H5Fclose(cf_fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught BESDapError! Message: " << e.get_message() << endl);
        throw;
    }

    catch(const BESError & e) {
        if (cf_fileid !=-1)
            H5Fclose(cf_fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught BESError! Message: " << e.get_message() << endl);
        throw;
    }
    catch (InternalErr & e) {
        if (cf_fileid !=-1)
            H5Fclose(cf_fileid);
        string libdap_error="libdap4: "+ e.get_file();
        BESDapError ex(e.get_error_message(), true, e.get_error_code(), libdap_error, e.get_line());
        throw ex;
    }
    catch (Error & e) {
        if (cf_fileid !=-1)
            H5Fclose(cf_fileid);
        string libdap_error="libdap4: "+ e.get_file();
        BESDapError ex(e.get_error_message(), false, e.get_error_code(), libdap_error, e.get_line());
        throw ex;
    }

    catch (std::exception &e) {
        if (cf_fileid !=-1)
            H5Fclose(cf_fileid);
        string s = string("C++ Exception: ") + e.what();
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }
    catch(...) {
        if (cf_fileid !=-1)
            H5Fclose(cf_fileid);
        string s = "unknown exception caught building HDF5 DAS";
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }

    BESDEBUG(HDF5_NAME, prolog << "END" << endl);
    return true;
}

void HDF5RequestHandler::hdf5_build_das_internal_no_mem_cache(const string& filename, DAS *das, hid_t &cf_fileid)
{
    bool das_from_dc = false;
    string das_cache_fname;

    // If the use_disk_meta_cache is set, check if the cache file exists and sets the flag.
    if (_use_disk_meta_cache == true) {

        string base_filename   =  HDF5CFUtil::obtain_string_after_lastslash(filename);
        das_cache_fname = _disk_meta_cache_path+"/" +base_filename+"_das";

        if (access(das_cache_fname.c_str(),F_OK) !=-1)
           das_from_dc = true;

    }

    // If reading DAS from the disk cache, call the corresponding function
    if (true == das_from_dc) {
        read_das_from_disk_cache(das_cache_fname,das);

        // If the memory cache is set, adding the DAS copy to the memory cache
        if (das_cache) {
            // add a copy
            BESDEBUG(HDF5_NAME, prolog << "HDF5 DAS reading DAS from the disk cache. For memory cache, DAS added to the cache for : " << filename << endl);
            das_cache->add(new DAS(*das), filename);
        }
    }

    else {// Need to build from the HDF5 file
        H5Eset_auto2(H5E_DEFAULT,nullptr,nullptr);
        if (true == _usecf) {//CF option

            cf_fileid = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
            if (cf_fileid < 0){
                string invalid_file_msg="Could not open this HDF5 file ";
                invalid_file_msg +=filename;
                invalid_file_msg +=". It is very possible that this file is not an HDF5 file ";
                invalid_file_msg +=" but with the .h5/.HDF5 suffix. Please check with the data";
                invalid_file_msg +=" distributor.";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
            }
            // Need to check if DAP4 DMR CF 64-bit integer mapping is on.
            if (HDF5RequestHandler::get_dmr_64bit_int()!=nullptr)
                HDF5RequestHandler::set_dmr_64bit_int(nullptr);
            read_cfdas( *das,filename,cf_fileid);
            H5Fclose(cf_fileid);
        }
        else {// Default option
            hid_t fileid = get_fileid(filename.c_str());
            if (fileid < 0) {
                string invalid_file_msg="Could not open this HDF5 file ";
                invalid_file_msg +=filename;
                invalid_file_msg +=". It is very possible that this file is not an HDF5 file ";
                invalid_file_msg +=" but with the .h5/.HDF5 suffix. Please check with the data";
                invalid_file_msg +=" distributor.";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
            }

            find_gloattr(fileid, *das);
            depth_first(fileid, "/", *das);
            close_fileid(fileid);
        }
        Ancillary::read_ancillary_das( *das, filename ) ;


#if 0
// Dump all attribute contents
AttrTable* top_table = das->get_top_level_attributes();
get_attr_contents(top_table);

// Dump all variable contents
AttrTable::Attr_iter start_aiter=das->var_begin();
AttrTable::Attr_iter it = start_aiter;
AttrTable::Attr_iter end_aiter = das->var_end();
while(it != end_aiter) {
AttrTable* temp_table = das->get_table(it);
if(temp_table!=0){
cerr<<"var_begin"<<endl;
temp_table->print(cerr);
}
++it;
}
#endif
        // If the memory cache is turned on
        if (das_cache) {
            // add a copy
            BESDEBUG(HDF5_NAME, prolog << "DAS added to the cache for : " << filename << endl);
            das_cache->add(new DAS(*das), filename);
        }

        // DAS disk cache fname will be set only when the metadata disk cache is turned on
        // So if it comes here, the das cache should be generated.
        if (das_cache_fname.empty() == false) {
            BESDEBUG(HDF5_NAME, prolog << "HDF5 Build DAS: Write DAS to disk cache " << das_cache_fname << endl);
            write_das_to_disk_cache(das_cache_fname,das);
        }
    }
}
// Convenient function that helps  build DDS and Data
// Since this function will be used by both the DDS and Data services, we need to pass both BESDDSResponse
// and BESDataDDSResponse. These two parameters are necessary for the future DDS disk cache feature.
void HDF5RequestHandler::get_dds_with_attributes( BESDDSResponse*bdds,BESDataDDSResponse*data_bdds,
                                                  const string &container_name, const string& filename,
                                                  const string &dds_cache_fname, const string &das_cache_fname,
                                                  bool dds_from_dc,bool das_from_dc, bool build_data)
{
    DDS *dds;
    if (true == build_data)
        dds = data_bdds->get_dds();
    else
        dds = bdds->get_dds();

    // For the time being, separate CF file ID from the default file ID(mainly for debugging)
    hid_t fileid    = -1;
    hid_t cf_fileid = -1;

    try {

        // Look in memory cache to see if it's initialized
        const DDS* cached_dds_ptr = nullptr;
        bool use_dds_cache = false;
        if (dds_cache) 
            cached_dds_ptr = dynamic_cast<DDS*>(dds_cache->get(filename));
        if (cached_dds_ptr) 
            use_dds_cache = true;
        if (true == use_dds_cache) {
            // copy the cached DDS into the BES response object. Assume that any cached DDS
            // includes the DAS information.
            BESDEBUG(HDF5_NAME, prolog << "DDS Metadata Cached hit for : " << filename << endl);
            *dds = *cached_dds_ptr; // Copy the referenced object
        }
        else if (true ==dds_from_dc) {//Currently the dds_from_dc is always false by default. 
            read_dds_from_disk_cache(bdds,data_bdds,build_data,container_name,filename,dds_cache_fname,
                                     das_cache_fname,-1,das_from_dc);
        }
        else {
            read_dds_from_file(dds, filename, cf_fileid, fileid,  dds_cache_fname, dds_from_dc);

            // Add attributes
            add_das_to_dds_wrapper(dds,filename,cf_fileid,fileid,container_name,das_cache_fname,das_from_dc);

            // Add memory cache if possible
            if (dds_cache) {
            	// add a copy
                BESDEBUG(HDF5_NAME, prolog << "DDS added to the cache for : " << filename << endl);
                dds_cache->add(new DDS(*dds), filename);
            }

            H5Fclose(cf_fileid);
            H5Fclose(fileid);
 
        }
    
    }
    
    catch(const BESSyntaxUserError & e) {
        close_h5_files(cf_fileid, fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught BESSyntaxUserError! Message: " << e.get_message() << endl);
        throw;
    }

    catch(const BESInternalError & e) {
        close_h5_files(cf_fileid, fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught BESInternalError! Message: " << e.get_message() << endl);
        throw;
    }

    catch(const BESDapError & e) {
        close_h5_files(cf_fileid, fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught BESDapError! Message: " << e.get_message() << endl);
        throw;
    }

    catch(const BESError & e) {
        close_h5_files(cf_fileid, fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught BESError! Message: " << e.get_message() << endl);
        throw;
    }
    catch(const InternalErr & e) {
        close_h5_files(cf_fileid, fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught InternalErr! Message: " << e.get_error_message() << endl);
        throw; 
    }
    catch(const Error & e) {
        close_h5_files(cf_fileid, fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught Err! Message: " << e.get_error_message() << endl);
        throw; 
    }
    catch (std::exception &e) {
        close_h5_files(cf_fileid, fileid);
        string s = string("C++ Exception: ") + e.what();
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }
    catch(...) {
        close_h5_files(cf_fileid, fileid);
        string s = "unknown exception caught building HDF5 DDS";
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }
}

void HDF5RequestHandler::read_dds_from_file(DDS *dds, const string &filename, hid_t &cf_fileid, hid_t &fileid,
                                            const string &dds_cache_fname, bool dds_from_dc)
{
    BESDEBUG(HDF5_NAME, prolog << "Build DDS from the HDF5 file. " << filename << endl);
    H5Eset_auto2(H5E_DEFAULT,nullptr,nullptr);
    dds->filename(filename);

    // For the time being, not mess up CF's fileID with Default's fileID
    if (true == _usecf) {

        cf_fileid = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
        if (cf_fileid < 0){
            string invalid_file_msg="Could not open this HDF5 file ";
            invalid_file_msg +=filename;
            invalid_file_msg +=". It is very possible that this file is not an HDF5 file ";
            invalid_file_msg +=" but with the .h5/.HDF5 suffix. Please check with the data";
            invalid_file_msg +=" distributor.";
            throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
        }

        // The following is for DAP4 CF(DMR) 64-bit mapping, we need to set the flag
        // to let the handler map the 64-bit integer.
        if(HDF5RequestHandler::get_dmr_64bit_int() != nullptr)
            HDF5RequestHandler::set_dmr_64bit_int(nullptr);
        read_cfdds(*dds,filename,cf_fileid);
    }
    else {

        fileid = get_fileid(filename.c_str());
        if (fileid < 0) {
            string invalid_file_msg="Could not open this HDF5 file ";
            invalid_file_msg +=filename;
            invalid_file_msg +=". It is very possible that this file is not an HDF5 file ";
            invalid_file_msg +=" but with the .h5/.HDF5 suffix. Please check with the data";
            invalid_file_msg +=" distributor.";
            throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
        }

        depth_first(fileid, (char*)"/", *dds, filename.c_str());

    }
    // Check semantics
    if (!dds->check_semantics()) {   // DDS didn't comply with the semantics
        dds->print(cerr);
        throw InternalErr(__FILE__, __LINE__,
                          "DDS check_semantics() failed. This can happen when duplicate variable names are defined. ");
    }

    Ancillary::read_ancillary_dds( *dds, filename ) ;

    // Generate the DDS cached file if needed,currently this is always false by default
    if(dds_cache_fname.empty() == false && dds_from_dc == false)
        write_dds_to_disk_cache(dds_cache_fname,dds);

}

void HDF5RequestHandler::add_das_to_dds_wrapper(DDS *dds, const string &filename, hid_t cf_fileid, hid_t fileid,
                                                const string &container_name, const string &das_cache_fname,
                                                bool das_from_dc) {
    hid_t h5_fd = -1;
    if (_usecf == true)
        h5_fd = cf_fileid;
    else
        h5_fd = fileid;

    add_das_to_dds(dds,container_name,filename,das_cache_fname,h5_fd,das_from_dc);
}

void HDF5RequestHandler::get_dds_without_attributes_datadds(BESDataDDSResponse*data_bdds, const string& filename)
{
    DDS *dds = data_bdds->get_dds();

    // For the time being, separate CF file ID from the default file ID(mainly for debugging)
    hid_t fileid    = -1;
    hid_t cf_fileid = -1;

    try {

        // Look in memory cache to see if it's initialized
        const DDS* cached_dds_ptr = nullptr;
        bool use_datadds_cache = false;

        if (datadds_cache) 
            cached_dds_ptr = dynamic_cast<DDS*>(datadds_cache->get(filename));

        if (cached_dds_ptr) 
            use_datadds_cache = true;
        if (true == use_datadds_cache) {
            // copy the cached DDS into the BES response object. 
            // The DAS information is not included.
            BESDEBUG(HDF5_NAME, prolog << "DataDDS Metadata Cached hit for : " << filename << endl);
            *dds = *cached_dds_ptr; // Copy the referenced object
        }
        else
             read_datadds_from_file(dds, filename, cf_fileid, fileid);

        BESDEBUG(HDF5_NAME, prolog << "Data ACCESS build_data(): set the including attribute flag to false: "<<filename << endl);
        data_bdds->set_ia_flag(false);
    
    }
    catch(const BESSyntaxUserError & e) {
        BESDEBUG(HDF5_NAME, prolog << "Caught BESSyntaxUserError! Message: " << e.get_message() << endl);
        close_h5_files(cf_fileid, fileid);
        throw;
    }
    catch(const BESInternalError & e) {
        close_h5_files(cf_fileid, fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught BESInternalError! Message: " << e.get_message() << endl);
        throw;
    }

    catch(const BESDapError & e) {
        close_h5_files(cf_fileid, fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught BESDapError! Message: " << e.get_message() << endl);
        throw;
    }

    catch(const BESError & e) {
        close_h5_files(cf_fileid, fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught BESError! Message: " << e.get_message() << endl);
        throw;
    }
 
    catch(const InternalErr & e) {
        close_h5_files(cf_fileid, fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught libdap InternalError! Message: " << e.get_error_message() << endl);
        throw;
    }
    catch(const Error & e) {
        close_h5_files(cf_fileid, fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught libdap Error! Message: " << e.get_error_message() << endl);
        throw;
    }
    catch (std::exception &e) {
        close_h5_files(cf_fileid, fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught standard exception Error! Message: " << e.what() << endl);
        string s = string("C++ Exception: ") + e.what();
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }
    catch(...) {
       close_h5_files(cf_fileid, fileid);
       BESDEBUG(HDF5_NAME, prolog << "Caught unknown exception Error! in get_dds_without_attributes_datadds() when building HDF5 DDS" << endl);
       string s = "unknown exception caught in get_dds_without_attributes_datadds() when building HDF5 DDS";
       throw BESInternalFatalError(s, __FILE__, __LINE__);
    }
}

void HDF5RequestHandler::read_datadds_from_file(DDS *dds, const string &filename, hid_t &cf_fileid, hid_t &fileid)
{
    BESDEBUG(HDF5_NAME, prolog << "Build DDS from the HDF5 file. " << filename << endl);
    H5Eset_auto2(H5E_DEFAULT,nullptr,nullptr);
    dds->filename(filename);

    // For the time being, not mess up CF's fileID with Default's fileID
    if (true == _usecf) {

        cf_fileid = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
        if (cf_fileid < 0){
            string invalid_file_msg="Could not open this HDF5 file ";
            invalid_file_msg +=filename;
            invalid_file_msg +=". It is very possible that this file is not an HDF5 file ";
            invalid_file_msg +=" but with the .h5/.HDF5 suffix. Please check with the data";
            invalid_file_msg +=" distributor.";
            throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
        }
        // The following is for DAP4 CF(DMR) 64-bit mapping, we need to set the flag
        // to let the handler map the 64-bit integer.
        if (HDF5RequestHandler::get_dmr_64bit_int() != nullptr)
            HDF5RequestHandler::set_dmr_64bit_int(nullptr);
        read_cfdds(*dds,filename,cf_fileid);
    }
    else {
        fileid = get_fileid(filename.c_str());
        if (fileid < 0) {
            string invalid_file_msg="Could not open this HDF5 file ";
            invalid_file_msg +=filename;
            invalid_file_msg +=". It is very possible that this file is not an HDF5 file ";
            invalid_file_msg +=" but with the .h5/.HDF5 suffix. Please check with the data";
            invalid_file_msg +=" distributor.";
            throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
        }

        depth_first(fileid, (char*)"/", *dds, filename.c_str());
    }
    // Check semantics
    if (!dds->check_semantics()) {   // DDS didn't comply with the semantics
        dds->print(cerr);
        throw InternalErr(__FILE__, __LINE__,
                          "DDS check_semantics() failed. This can happen when duplicate variable names are defined. ");
    }
    Ancillary::read_ancillary_dds( *dds, filename ) ;

    // Add memory cache if possible
    if (datadds_cache) {
        // add a copy
        BESDEBUG(HDF5_NAME, prolog << "DataDDS added to the cache for : " << filename << endl);
        datadds_cache->add(new DDS(*dds), filename);
    }

    close_h5_files(cf_fileid, fileid);

}

// Build DDS
bool HDF5RequestHandler::hdf5_build_dds(BESDataHandlerInterface & dhi)
{
    BESDEBUG(HDF5_NAME, prolog << "BEGIN" << endl);
#if DYNAMIC_CONFIG_ENABLED
    load_config();
#endif

    // Obtain the HDF5 file name.
    string filename = dhi.container->access();
 
    string container_name = dhi.container->get_symbolic_name();
    BESResponseObject *response = dhi.response_handler->get_response_object();
    auto bdds = dynamic_cast < BESDDSResponse * >(response);
    if( !bdds )
        throw BESInternalError( "Cannot cast the BESResponse Object to a BESDDSResponse object", __FILE__, __LINE__ ) ;
    bdds->set_container(container_name);

    try {

        bool dds_from_dc = false;
        bool das_from_dc = false;
        bool build_data  = false;
        string dds_cache_fname;
        string das_cache_fname;

        if (_use_disk_meta_cache) {

            string base_filename   =  HDF5CFUtil::obtain_string_after_lastslash(filename);

            // The _use_disk_dds_cache is always set to false by default
            if (_use_disk_dds_cache) {
                dds_cache_fname = _disk_meta_cache_path+"/" +base_filename+"_dds";
                if (access(dds_cache_fname.c_str(),F_OK) !=-1)
                    dds_from_dc = true;
            }

            das_cache_fname = _disk_meta_cache_path+"/" +base_filename+"_das";
            // Check if das files exist
            if (access(das_cache_fname.c_str(),F_OK) !=-1)
               das_from_dc = true;
        }

        get_dds_with_attributes(bdds, nullptr,container_name,filename, dds_cache_fname,das_cache_fname,
                                dds_from_dc,das_from_dc,build_data);

        bdds->set_constraint( dhi ) ;
        bdds->clear_container() ;
    
    }

    catch(const BESSyntaxUserError & e) {
        BESDEBUG(HDF5_NAME, prolog << "Caught BESSyntaxUserError! Message: " << e.get_message() << endl);
        throw;
    }
    catch(const BESInternalError & e) {
        BESDEBUG(HDF5_NAME, prolog << "Caught BESInternalError! Message: " << e.get_message() << endl);
        throw;
    }
    catch(const BESDapError & e) {
        BESDEBUG(HDF5_NAME, prolog << "Caught BESDapError! Message: " << e.get_message() << endl);
        throw;
    }
    catch(const BESError & e) {
        BESDEBUG(HDF5_NAME, prolog << "Caught BESError! Message: " << e.get_message() << endl);
        throw;
    }
    catch(const InternalErr & e) {
        BESDEBUG(HDF5_NAME, prolog << "Caught libdap InternalError! Message: " << e.get_error_message() << endl);
        string libdap_error="libdap4: "+ e.get_file();
        BESDapError ex(e.get_error_message(), true, e.get_error_code(), libdap_error, e.get_line());
        throw ex;

    }
    catch(const Error & e) {
        BESDEBUG(HDF5_NAME, prolog << "Caught libdap Error! Message: " << e.get_error_message() << endl);
        string libdap_error="libdap4: "+ e.get_file();
        BESDapError ex(e.get_error_message(), false, e.get_error_code(), libdap_error, e.get_line());
        throw ex;
    }
    catch (std::exception &e) {
        BESDEBUG(HDF5_NAME, prolog << "Caught standard Exception! Message: " << e.what() << endl);
        string s = string("C++ Exception: ") + e.what();
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }
    catch(...) {
        BESDEBUG(HDF5_NAME, prolog << "Caught unknown Exception! Message: "  << endl);
        string s = "unknown exception caught building HDF5 DDS";
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }

    BESDEBUG(HDF5_NAME, prolog << "END" << endl);
    return true;
}

bool HDF5RequestHandler::hdf5_build_data(BESDataHandlerInterface & dhi)
{
    BESDEBUG(HDF5_NAME, prolog << "BEGIN" << endl);
#if DYNAMIC_CONFIG_ENABLED
    load_config();
#endif

    if (_usecf && _pass_fileid) 
            return hdf5_build_data_with_IDs(dhi);

    string filename = dhi.container->access();

    string container_name = dhi.container->get_symbolic_name();
    BESResponseObject *response = dhi.response_handler->get_response_object();
    auto bdds = dynamic_cast < BESDataDDSResponse * >(response);
    if( !bdds )
        throw BESInternalError( "Cannot cast the BESResponse Object to a BESDataDDSResponse object", __FILE__, __LINE__ ) ;
    bdds->set_container(container_name);

    try {

        // DDS from disk cache is not currently supported. It may be supported in the future.
        // We will leave the code as commented. 
#if 0
        bool dds_from_dc = false;
        bool build_data  = true;
#endif

        // The code inside the following #if 0 #endif is not used anymore. So comment out. KY 2022-12-11 
#if 0
        bool das_from_dc = false;
        string dds_cache_fname;
        string das_cache_fname;


        // Only DAS is read from the cache. dds_from_dc is always false.
        if(_use_disk_meta_cache == true) {

            string base_filename   =  HDF5CFUtil::obtain_string_after_lastslash(filename);
            das_cache_fname = _disk_meta_cache_path+"/" +base_filename+"_das";

            if(access(das_cache_fname.c_str(),F_OK) !=-1)
               das_from_dc = true;

        }
#endif

        get_dds_without_attributes_datadds(bdds, filename);

        bdds->set_constraint( dhi ) ;
        bdds->clear_container() ;
    }

    catch(const BESSyntaxUserError & e) {
        BESDEBUG(HDF5_NAME, prolog << "Caught BESSyntaxUserError! Message: " << e.get_message() << endl);
        throw;
    }

    catch(const BESInternalError & e) {
        BESDEBUG(HDF5_NAME, prolog << "Caught BESInternalError! Message: " << e.get_message() << endl);
        throw;
    }

    catch(const BESDapError & e) {
        BESDEBUG(HDF5_NAME, prolog << "Caught BESDapError! Message: " << e.get_message() << endl);
        throw;
    }

    catch(const BESError & e) {
        BESDEBUG(HDF5_NAME, prolog << "Caught BESError! Message: " << e.get_message() << endl);
        throw;
    }

    catch(const InternalErr & e) {
        string libdap_error="libdap4: "+ e.get_file();
        BESDapError ex(e.get_error_message(), true, e.get_error_code(), libdap_error, e.get_line());
        throw ex;
    }
    catch(const Error & e) {
        string libdap_error="libdap4: "+ e.get_file();
        BESDapError ex(e.get_error_message(), false, e.get_error_code(), libdap_error, e.get_line());
        throw ex;
    }
    catch (std::exception &e) {
        string s = string("C++ Exception: ") + e.what();
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }
    catch(...) {
       string s = "unknown exception caught building HDF5 DDS";
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }

    BESDEBUG(HDF5_NAME, prolog << "END" << endl);
    return true;
}

// Obtain data when turning on the pass fileID key.The memory cache is not used.
bool HDF5RequestHandler::hdf5_build_data_with_IDs(BESDataHandlerInterface & dhi)
{
    BESDEBUG(HDF5_NAME, prolog << "BEGIN" << endl);
#if DYNAMIC_CONFIG_ENABLED
    load_config();
#endif

    BESDEBUG(HDF5_NAME,prolog << "Building DataDDS by passing file IDs. "<<endl);
    hid_t cf_fileid = -1;

    string filename = dhi.container->access();
    
    H5Eset_auto2(H5E_DEFAULT,nullptr,nullptr);
    cf_fileid = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (cf_fileid < 0){
        string invalid_file_msg="Could not open this HDF5 file ";
        invalid_file_msg +=filename;
        invalid_file_msg +=". It is very possible that this file is not an HDF5 file ";
        invalid_file_msg +=" but with the .h5/.HDF5 suffix. Please check with the data";
        invalid_file_msg +=" distributor.";
        throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
    }

    BESResponseObject *response = dhi.response_handler->get_response_object();
    auto bdds = dynamic_cast < BESDataDDSResponse * >(response);
    if( !bdds ) {
        H5Fclose(cf_fileid);
        throw BESInternalError( "Cannot cast the BESResponse Object to a BESDataDDSResponse object", __FILE__, __LINE__ ) ;
    }

    try {

        bdds->set_container( dhi.container->get_symbolic_name() ) ;

        auto hdds_unique = make_unique<HDF5DDS>(bdds->get_dds());
        delete bdds->get_dds();

        auto hdds = hdds_unique.release();
        bdds->set_dds(hdds);
        hdds->setHDF5Dataset(cf_fileid);

        read_cfdds( *hdds,filename,cf_fileid);

        if (!hdds->check_semantics()) {   // DDS didn't comply with the DAP semantics 
            hdds->print(cerr);
            string s = "DDS check_semantics() failed. This can happen when duplicate variable names are defined.";
            throw InternalErr(__FILE__,__LINE__,s);
        }
        
        Ancillary::read_ancillary_dds( *hdds, filename ) ;

        auto das_unique = make_unique<DAS>();
        auto das = das_unique.release();
        BESDASResponse bdas( das ) ;
        bdas.set_container( dhi.container->get_symbolic_name() ) ;
        read_cfdas( *das,filename,cf_fileid);

        Ancillary::read_ancillary_das( *das, filename ) ;

        hdds->transfer_attributes(das);
        bdds->set_constraint( dhi ) ;
        bdds->clear_container() ;

    }

    catch(const BESSyntaxUserError & e) {
        BESDEBUG(HDF5_NAME, prolog << "Caught BESSyntaxUserError! Message: " << e.get_message() << endl);
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        throw;
    }
    catch(const BESInternalError & e) {
        BESDEBUG(HDF5_NAME, prolog << "Caught BESInternalError! Message: " << e.get_message() << endl);
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        throw;
    }

    catch(const BESDapError & e) {
        BESDEBUG(HDF5_NAME, prolog << "Caught BESDapError! Message: " << e.get_message() << endl);
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        throw;
    }

    catch(const BESError & e) {
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught BESError! Message: " << e.get_message() << endl);
        throw;
    }
    catch(const InternalErr & e) {
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        throw BESDapError(e.get_error_message(), true, e.get_error_code(),
                       __FILE__, __LINE__);
    }
    catch(const Error & e) {
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        throw BESDapError(e.get_error_message(), false, e.get_error_code(),
                       __FILE__, __LINE__);
    }
    catch (std::exception &e) {
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        string s = string("C++ Exception: ") + e.what();
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }
    catch(...) {
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        string s = "unknown exception caught building HDF5 DataDDS";
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }

    BESDEBUG(HDF5_NAME, prolog << "END" << endl);
    return true;
}

bool HDF5RequestHandler::hdf5_build_dmr(BESDataHandlerInterface & dhi)
{
    BESDEBUG(HDF5_NAME, prolog << "BEGIN" << endl);
#if DYNAMIC_CONFIG_ENABLED
    load_config();
#endif

    // Extract the DMR Response object - this holds the DMR used by the
    // other parts of the framework.
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDMRResponse &bes_dmr_response = dynamic_cast<BESDMRResponse &>(*response);

    string filename = dhi.container->access();

    DMR *dmr = bes_dmr_response.get_dmr();

    // For the time being, separate CF file ID from the default file ID(mainly for debugging)
    hid_t fileid = -1;
    hid_t cf_fileid = -1;
 
    try {

        const DMR* cached_dmr_ptr = nullptr;
        if (dmr_cache){
            BESDEBUG(HDF5_NAME, prolog << "Checking DMR cache for : " << filename << endl);
            cached_dmr_ptr = dynamic_cast<DMR*>(dmr_cache->get(filename));
        }

        if (cached_dmr_ptr) {
            // copy the cached DMR into the BES response object
            BESDEBUG(HDF5_NAME, prolog << "DMR cache hit for : " << filename << endl);
            *dmr = *cached_dmr_ptr; // Copy the referenced object
            dmr->set_request_xml_base(bes_dmr_response.get_request_xml_base());
        }
        else {// No cache
            if (true == hdf5_build_dmr_from_file(dhi,bes_dmr_response,dmr, filename, cf_fileid, fileid))
                return true;
        }// else no cache
    }// try

    catch(const BESSyntaxUserError & e) {
        close_h5_files(cf_fileid, fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught BESSyntaxUserError! Message: " << e.get_message() << endl);
        throw;
    }
    catch(const BESInternalError & e) {
        close_h5_files(cf_fileid, fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught BESInternalError! Message: " << e.get_message() << endl);
        throw;
    }

    catch(const BESDapError & e) {
        close_h5_files(cf_fileid, fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught BESDapError! Message: " << e.get_message() << endl);
        throw;
    }
    catch(const BESError & e) {
        close_h5_files(cf_fileid, fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught BESError! Message: " << e.get_message() << endl);
        throw;
    }
    catch(const InternalErr & e) {
        close_h5_files(cf_fileid, fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught libdap InternalError! Message: " << e.get_error_message() << endl);
        string libdap_error="libdap4: "+ e.get_file();
        BESDapError ex(e.get_error_message(), true, e.get_error_code(), libdap_error, e.get_line());
        throw ex;
    }
    catch(const Error & e) {
        close_h5_files(cf_fileid, fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught libdap Error! Message: " << e.get_error_message() << endl);
        string libdap_error="libdap4: "+ e.get_file();
        BESDapError ex(e.get_error_message(), false, e.get_error_code(), libdap_error, e.get_line());
        throw ex;
    }
    catch (std::exception &e) {
        close_h5_files(cf_fileid, fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught standard Exception! Message: " << e.what() << endl);
        string s = string("C++ Exception: ") + e.what();
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }
    catch(...) {
        close_h5_files(cf_fileid, fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught unknown Exception! Message: "  << endl);
        string s = "unknown exception caught building HDF5 DMR";
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }

    // Instead of fiddling with the internal storage of the DHI object,
    // (by setting dhi.data[DAP4_CONSTRAINT], etc., directly) use these
    // methods to set the constraints. But, why? Ans: from Patrick is that
    // in the 'container' mode of BES each container can have a different
    // CE.
    bes_dmr_response.set_dap4_constraint(dhi);
    bes_dmr_response.set_dap4_function(dhi);
    dmr->set_factory(nullptr);

    BESDEBUG(HDF5_NAME, prolog << "END" << endl);
    return true;
}

bool HDF5RequestHandler::hdf5_build_dmr_from_file(BESDataHandlerInterface & dhi, BESDMRResponse &bes_dmr_response,
                                                  DMR *dmr, const string &filename,
                                                  hid_t &cf_fileid, hid_t &fileid)
{
    H5Eset_auto2(H5E_DEFAULT,nullptr,nullptr);
    D4BaseTypeFactory MyD4TypeFactory;
    dmr->set_factory(&MyD4TypeFactory);
    if(_escape_utf8_attr == false)
        dmr->set_utf8_xml_encoding();

    if(true ==_usecf) {// CF option

        if(true == _usecfdmr) {

            cf_fileid = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
            if (cf_fileid < 0) {
                string invalid_file_msg="Could not open this HDF5 file ";
                invalid_file_msg +=filename;
                invalid_file_msg +=". It is very possible that this file is not an HDF5 file ";
                invalid_file_msg +=" but with the .h5/.HDF5 suffix. Please check with the data";
                invalid_file_msg +=" distributor.";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
            }
            read_cfdmr(dmr,filename,cf_fileid);
            H5Fclose(cf_fileid);
            bes_dmr_response.set_dap4_constraint(dhi);
            bes_dmr_response.set_dap4_function(dhi);
            dmr->set_factory(nullptr);

            BESDEBUG(HDF5_NAME, prolog << "END" << endl);

            return true;
        }

        if(true == _pass_fileid)
            return hdf5_build_dmr_with_IDs(dhi);

        cf_fileid = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
        if (cf_fileid < 0){
            string invalid_file_msg="Could not open this HDF5 file ";
            invalid_file_msg +=filename;
            invalid_file_msg +=". It is very possible that this file is not an HDF5 file ";
            invalid_file_msg +=" but with the .h5/.HDF5 suffix. Please check with the data";
            invalid_file_msg +=" distributor.";
            throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
        }

        BaseTypeFactory factory;
        DDS dds(&factory, name_path(filename), "3.2");
        dds.filename(filename);

        DAS das;

        // For the CF option dmr response, we need to map 64-bit integer separately
        // So set the flag to map 64-bit integer.
        HDF5RequestHandler::set_dmr_64bit_int(dmr);
        read_cfdds( dds,filename,cf_fileid);
        if (!dds.check_semantics()) {   // DDS didn't comply with the DAP semantics
            dds.print(cerr);
            throw InternalErr(__FILE__, __LINE__,
                      "DDS check_semantics() failed. This can happen when duplicate variable names are defined.");
        }

        read_cfdas(das,filename,cf_fileid);
        Ancillary::read_ancillary_das( das, filename ) ;

        dds.transfer_attributes(&das);

        ////close the file ID.
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);

        dmr->build_using_dds(dds);


    }// "if(true == _usecf)"
    else {// default option

        // Obtain the HDF5 file ID.
        fileid = get_fileid(filename.c_str());
        if (fileid < 0) {
            string invalid_file_msg="Could not open this HDF5 file ";
            invalid_file_msg +=filename;
            invalid_file_msg +=". It is very possible that this file is not an HDF5 file ";
            invalid_file_msg +=" but with the .h5/.HDF5 suffix. Please check with the data";
            invalid_file_msg +=" distributor.";
            throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
        }

        // Check if this is an HDF-EOS5 file
        bool is_eos5 = check_eos5(fileid);

        // Check if dim scale is used.
        bool use_dimscale = false;
        if (true == _default_handle_dimension)
            use_dimscale = check_dimscale(fileid);

        // If it is an HDF-EOS5 file but not having dimension scale,
        // treat it as an HDF-EOS5 file, retrieve all the HDF-EOS5 info.
        eos5_dim_info_t eos5_dim_info;
        if (is_eos5 && !use_dimscale)
            obtain_eos5_dims(fileid,eos5_dim_info);

        // However, we find some HDF-EOS5 products that use dimension scales but miss some dimensions. This will cause
        // the handler not able to retrieve the dimension names based on the dimension scales. To make the service continue,
        // we have to find a way to obtain those dimension names and avoid the failure of dmr generation.

        unordered_set<string> eos5_missing_dim_names;
        if (is_eos5 && use_dimscale) {

            bool has_var_null_dim_name = check_var_null_dim_name(fileid);
            // If we find the null dimension scale case, we need to obtain all the EOS5 information via parser.
            if (has_var_null_dim_name) {
                obtain_eos5_dims(fileid,eos5_dim_info);
                obtain_eos5_missing_dims(fileid,eos5_dim_info,eos5_missing_dim_names);
            }
#if 0
            // We may need to use the code to handle the non-eos5-null-dim case. TODO later.
            eos5_use_dimscale_null_dims = check_eos5_dimscale_null_dims(fileid);
#endif
        }

        dmr->set_name(name_path(filename));
        dmr->set_filename(name_path(filename));

        // The breadth_first() function builds the variables and attributes and
        // loads them into the root group (building child groups as needed).
        // jhrg 4/30/20
        D4Group* root_grp = dmr->root();
        BESDEBUG("h5", "use_dimscale is "<< use_dimscale <<endl);

        // It is possible that a dimension variable has hardlinks. To make it
        // right for the netCDF-4 data model and the current DAP4 implementation,
        // we need to choose the shortest path of all hardlinks as the dimension path.
        // So to avoid iterating all HDF5 objects multiple times, save the found
        // hardlinks and search them when necessary.  Note we have to search hardlinks from the root.
        // KY 2021-11-15
        vector<link_info_t> hdf5_hls;
        vector<string> handled_coord_names;

        breadth_first(fileid, fileid,(const char*)"/",root_grp,filename.c_str(),use_dimscale,is_eos5,hdf5_hls,
                      eos5_dim_info,handled_coord_names, eos5_missing_dim_names);

        if (is_eos5 == false)
            add_dap4_coverage_default(root_grp,handled_coord_names);

        // Leave the following block until the HDF-EOS5 is fully supported.
#if 0
        BESDEBUG("h5", "build_dmr - before obtain dimensions"<< endl);
        D4Dimensions *root_dims = root_grp->dims();
for(D4Dimensions::D4DimensionsIter di = root_dims->dim_begin(), de = root_dims->dim_end(); di != de; ++di) {
BESDEBUG("fonc", "transform_dap4() - check dimensions"<< endl);
BESDEBUG("fonc", "transform_dap4() - dim name is: "<<(*di)->name()<<endl);
BESDEBUG("fonc", "transform_dap4() - dim size is: "<<(*di)->size()<<endl);
BESDEBUG("fonc", "transform_dap4() - fully_qualfied_dim name is: "<<(*di)->fully_qualified_name()<<endl);
//cout <<"dim size is: "<<(*di)->size()<<endl;
//cout <<"dim fully_qualified_name is: "<<(*di)->fully_qualified_name()<<endl;
}
        BESDEBUG("h5", "build_dmr - after obtain dimensions"<< endl);
#endif

        close_fileid(fileid);

    }// else (default option)

    // If the cache is turned on, add the memory cache.
    if (dmr_cache) {
        // add a copy
        BESDEBUG(HDF5_NAME, prolog << "DMR added to the cache for : " << filename << endl);
        dmr_cache->add(new DMR(*dmr), filename);
    }
    return false;
}
// This function is only used when EnableCF is true.
bool HDF5RequestHandler::hdf5_build_dmr_with_IDs(BESDataHandlerInterface & dhi)
{
    BESDEBUG(HDF5_NAME, prolog << "BEGIN" << endl);
#if DYNAMIC_CONFIG_ENABLED
    load_config();
#endif

    string filename = dhi.container->access();
    hid_t cf_fileid = -1;

    H5Eset_auto2(H5E_DEFAULT,nullptr,nullptr);
    cf_fileid = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (cf_fileid < 0){
        string invalid_file_msg="Could not open this HDF5 file ";
        invalid_file_msg +=filename;
        invalid_file_msg +=". It is very possible that this file is not an HDF5 file ";
        invalid_file_msg +=" but with the .h5/.HDF5 suffix. Please check with the data";
        invalid_file_msg +=" distributor.";
        throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
    }

    BaseTypeFactory factory;
    DDS dds(&factory, name_path(filename), "3.2");
    dds.filename(filename);

    DAS das;

    try {
        // This is the CF option
        read_cfdds( dds,filename,cf_fileid);

        if (!dds.check_semantics()) {   // DDS didn't comply with the DAP semantics 
            dds.print(cerr);
            throw InternalErr(__FILE__, __LINE__,
                              "DDS check_semantics() failed. This can happen when duplicate variable names are defined.");
        }
        
        Ancillary::read_ancillary_dds( dds, filename ) ;

        read_cfdas(das,filename,cf_fileid);

        Ancillary::read_ancillary_das( das, filename ) ;

        dds.transfer_attributes(&das);

        ////Don't close the file ID,it will be closed by the derived class.

    }

    catch(const BESSyntaxUserError & e) {
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught BESSyntaxUserError! Message: " << e.get_message() << endl);
        throw;
    }
    catch(const BESInternalError & e) {
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught BESInternalError! Message: " << e.get_message() << endl);
        throw;
    }
    catch(const BESDapError & e) {
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught BESDapError! Message: " << e.get_message() << endl);
        throw;
    }
    catch(const BESError & e) {
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught BESError! Message: " << e.get_message() << endl);
        throw;
    }
    catch(const InternalErr & e) {
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught libdap InternalError! Message: " << e.get_error_message() << endl);
        throw;
    }
    catch(const Error & e) {
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught libdap Error! Message: " << e.get_error_message() << endl);
        throw;
    }
    catch (std::exception &e) {
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        BESDEBUG(HDF5_NAME, prolog << "Caught standard exception Error! Message: " << e.what() << endl);
        string s = string("C++ Exception: ") + e.what();
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }
    catch(...) {
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        string s = "unknown exception caught building HDF5 DataDDS";
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }

    // Extract the DMR Response object - this holds the DMR used by the
    // other parts of the framework.
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDMRResponse &bes_dmr = dynamic_cast<BESDMRResponse &>(*response);

    // In this handler we use a different pattern since the handler specializes the DDS/DMR.
    // First, build the DMR adding the open handle to the HDF4 dataset, then free the DMR
    // the BES built and add this one. The HDF4DMR object will close the open dataset when
    // the BES runs the DMR's destructor.

    DMR *dmr = bes_dmr.get_dmr();
    D4BaseTypeFactory MyD4TypeFactory;
    dmr->set_factory(&MyD4TypeFactory);
    dmr->build_using_dds(dds);

    auto hdf5_dmr_unique = make_unique<HDF5DMR>(dmr);
    auto hdf5_dmr = hdf5_dmr_unique.release();
    hdf5_dmr->setHDF5Dataset(cf_fileid);
    delete dmr;     // The call below will make 'dmr' unreachable; delete it now to avoid a leak.
    bes_dmr.set_dmr(hdf5_dmr); // BESDMRResponse will delete hdf5_dmr

    // Instead of fiddling with the internal storage of the DHI object,
    // (by setting dhi.data[DAP4_CONSTRAINT], etc., directly) use these
    // methods to set the constraints. But, why? Ans: from Patrick is that
    // in the 'container' mode of BES each container can have a different
    // CE.
    bes_dmr.set_dap4_constraint(dhi);
    bes_dmr.set_dap4_function(dhi);
    hdf5_dmr->set_factory(nullptr);

    BESDEBUG(HDF5_NAME, prolog << "END" << endl);
    return true;
}

bool HDF5RequestHandler::hdf5_build_help(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    auto info = dynamic_cast<BESInfo *>(response);
    if( !info )
        throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    string add_info="Just for Test";

    map<string, string, std::less<>> attrs ;
    attrs["name"] = MODULE_NAME ;
    attrs["version"] = MODULE_VERSION ;
    list<string> services ;
    BESServiceRegistry::TheRegistry()->services_handled( HDF5_NAME, services );
    if ( services.empty()==false ) {
        string handles = BESUtil::implode( services, ',' ) ;
        attrs["handles"] = handles ;
    }
    info->begin_tag( "module", &attrs ) ;
    info->end_tag( "module" ) ;
    info->add_data(add_info);

    return true;
}

bool HDF5RequestHandler::hdf5_build_version(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    auto info = dynamic_cast < BESVersionInfo * >(response);
    if( !info )
        throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;
  
    info->add_module( MODULE_NAME, MODULE_VERSION ) ;

    return true;
}


bool HDF5RequestHandler::obtain_lrd_common_cache_dirs() 
{
    string lrd_config_fpath;
    string lrd_config_fname;

    // Obtain DataCache path
    lrd_config_fpath = get_beskeys("H5.DataCachePath");

    // Obtain the configure file name that specifics the large file configuration
    lrd_config_fname = get_beskeys("H5.LargeDataMemCacheFileName");
 
    // If either the configure file path or fname is missing, won't add specific mem. cache dirs. 
    if (lrd_config_fpath.empty() || lrd_config_fname.empty())
        return false;

    // temp_line for storing info of one line in the config. file 
    string temp_line;

    // The full path of the configure file
    string mcache_config_fname = lrd_config_fpath+"/"+lrd_config_fname;

    // Open the configure file
    ifstream mcache_config_file(mcache_config_fname.c_str());

    // If the configuration file is not open, return false.
    if (mcache_config_file.is_open()==false) {
        BESDEBUG(HDF5_NAME, prolog << "The large data memory cache configure file "<<mcache_config_fname );
        BESDEBUG(HDF5_NAME, prolog << " cannot be opened."<<endl);
        return false;
    }

    // Read the configuration file line by line
    while(getline(mcache_config_file,temp_line)) {

        // Only consider lines that is no less than 2 characters and the 2nd character is space.
        if (temp_line.size() >1 && temp_line.at(1)==' ') {
            char sep=' ';
            string subline = temp_line.substr(2);
            vector<string> temp_name_list;

            // Include directories to store common latitude and longitude values
            if(temp_line.at(0)=='1') {
                HDF5CFUtil::Split_helper(temp_name_list,subline,sep);
                lrd_cache_dir_list.insert(lrd_cache_dir_list.end(),temp_name_list.begin(),temp_name_list.end());
            }
            // Include directories not to store common latitude and longitude values
            else if(temp_line.at(0)=='0'){
                HDF5CFUtil::Split_helper(temp_name_list,subline,sep);
                lrd_non_cache_dir_list.insert(lrd_non_cache_dir_list.end(),temp_name_list.begin(),temp_name_list.end());
            }
            // Include variable names that the server would like to store in the memory cache
            else if(temp_line.at(0)=='2')
                obtain_lrd_common_cache_dirs_data_vars(temp_name_list, subline, sep);
        }
    }


    mcache_config_file.close();
    if(lrd_cache_dir_list.empty() && lrd_non_cache_dir_list.empty() && lrd_var_cache_file_list.empty())
        return false;
    else 
        return true;
}

void HDF5RequestHandler::obtain_lrd_common_cache_dirs_data_vars(vector<string> &temp_name_list, const string &subline,
                                                                char sep)
{
    // We need to handle the space case inside a variable path
    // either "" or '' needs to be used to identify a var path
    vector<unsigned int>dq_pos;
    vector<unsigned int>sq_pos;
    for (unsigned int i = 0; i<subline.size();i++) {
        if (subline[i]=='"') {
            dq_pos.push_back(i);
        }
        else if(subline[i]=='\'')
            sq_pos.push_back(i);
    }
    if (dq_pos.empty() && sq_pos.empty())
        HDF5CFUtil::Split_helper(temp_name_list,subline,sep);
    else if((dq_pos.empty()==false) && (dq_pos.size()%2==0) && sq_pos.empty()==true) {
        unsigned int dq_index= 0;
        while(dq_index < dq_pos.size()){
            if(dq_pos[dq_index+1]>(dq_pos[dq_index]+1)) {
                temp_name_list.push_back
                (subline.substr(dq_pos[dq_index]+1,dq_pos[dq_index+1]-dq_pos[dq_index]-1));
            }
            dq_index = dq_index + 2;
        }
    }
    else if((sq_pos.empty()==false) &&(sq_pos.size()%2==0) && dq_pos.empty()==true) {
        unsigned int sq_index= 0;
        while(sq_index < sq_pos.size()){
            if (sq_pos[sq_index+1]>(sq_pos[sq_index]+1)) {
                temp_name_list.push_back
                (subline.substr(sq_pos[sq_index]+1,sq_pos[sq_index+1]-sq_pos[sq_index]-1));
            }
            sq_index = sq_index+2;
        }
    }

    lrd_var_cache_file_list.insert(lrd_var_cache_file_list.end(),temp_name_list.begin(),temp_name_list.end());

}
bool HDF5RequestHandler::read_das_from_disk_cache(const string & cache_filename,DAS *das_ptr) {

    BESDEBUG(HDF5_NAME, prolog << "Coming to read_das_from_disk_cache() " << cache_filename << endl);
    bool ret_value = true;
    FILE *md_file = nullptr;
    md_file = fopen(cache_filename.c_str(),"rb");

    if(nullptr == md_file) {
        string bes_error = "An error occurred trying to open a metadata cache file  " + cache_filename;
        throw BESInternalError( bes_error, __FILE__, __LINE__);
    }
    else {
        
        int fd_md = fileno(md_file);
        struct flock *l_md;
        l_md = lock(F_RDLCK);

        // hold a read(shared) lock to read metadata from a file.
        if (fcntl(fd_md,F_SETLKW,l_md) == -1) {
            fclose(md_file);
            ostringstream oss;
            oss << "cache process: " << l_md->l_pid << " triggered a locking error: " << get_errno();
            throw BESInternalError( oss.str(), __FILE__, __LINE__);
        }

        try {

            struct stat sb{};
            if (stat(cache_filename.c_str(),&sb) != 0) {
                string bes_error = "An error occurred trying to stat a metadata cache file size " + cache_filename;
                throw BESInternalError( bes_error, __FILE__, __LINE__);

            }

            auto bytes_expected_read=(size_t)sb.st_size;
            BESDEBUG(HDF5_NAME, prolog << "DAS Disk cache file size is " << bytes_expected_read << endl);

            vector<char> buf;
            buf.resize(bytes_expected_read);
            size_t bytes_to_read =fread((void*)buf.data(),1,bytes_expected_read,md_file);
            if (bytes_to_read != bytes_expected_read)
                throw BESInternalError("Fail to read the data from the das cache file.",__FILE__,__LINE__);

            char* temp_pointer =buf.data();

            AttrTable*at = nullptr;
            // recursively build DAS, the folloing code is necessary. KY 2022-12-11
            temp_pointer = get_attr_info_from_dc(temp_pointer,das_ptr,at);
        }
        catch(...) {
            if (fcntl(fd_md,F_SETLK,lock(F_UNLCK)) == -1) {
                fclose(md_file);
                throw BESInternalError( "An error occurred trying to unlock the file" + get_errno(), __FILE__, __LINE__);
            }

            fclose(md_file);
            throw BESInternalError("Fail to parse a das cache file.",__FILE__,__LINE__);
        }

        // Unlock the cache file
        if(fcntl(fd_md,F_SETLK,lock(F_UNLCK)) == -1) { 
            fclose(md_file);
            throw BESInternalError( "An error occurred trying to unlock the file" + get_errno(), __FILE__, __LINE__);
        }
        fclose(md_file);
    }
    return ret_value;

}

// This fucntion will NOT be used by default. Leave here for future improvement.
bool HDF5RequestHandler::write_dds_to_disk_cache(const string& dds_cache_fname,DDS *dds_ptr) {

    BESDEBUG(HDF5_NAME, prolog << "Write DDS to disk cache " << dds_cache_fname << endl);
    FILE *dds_file = fopen(dds_cache_fname.c_str(),"w");

    if(nullptr == dds_file) {
        string bes_error = "An error occurred trying to open a metadata cache file  " + dds_cache_fname;
        throw BESInternalError( bes_error, __FILE__, __LINE__);
    }
    else {
        
        int fd_md = fileno(dds_file);
        struct flock *l_md;
        l_md = lock(F_WRLCK);

        // hold a read(shared) lock to read metadata from a file.
        if (fcntl(fd_md,F_SETLKW,l_md) == -1) {
            fclose(dds_file);
            ostringstream oss;
            oss << "cache process: " << l_md->l_pid << " triggered a locking error: " << get_errno();
            throw BESInternalError( oss.str(), __FILE__, __LINE__);
        }

        try {
            dds_ptr->print(dds_file);
        }
        catch(...) {
            if (fcntl(fd_md,F_SETLK,lock(F_UNLCK)) == -1) {
                fclose(dds_file);
                throw BESInternalError( "An error occurred trying to unlock the file" + get_errno(), __FILE__, __LINE__);
            }

            fclose(dds_file);
            throw BESInternalError("Fail to parse a dds cache file.",__FILE__,__LINE__);
        }

        if (fcntl(fd_md,F_SETLK,lock(F_UNLCK)) == -1) {
            fclose(dds_file);
            throw BESInternalError( "An error occurred trying to unlock the file" + get_errno(), __FILE__, __LINE__);
        }

        fclose(dds_file);
    }
    return true;

}

// Write DAS to a binary cached file on disk.
bool HDF5RequestHandler::write_das_to_disk_cache(const string & das_cache_fname, DAS *das_ptr) {

    BESDEBUG(HDF5_NAME, prolog << "Write DAS to disk cache " << das_cache_fname << endl);
    FILE *das_file = fopen(das_cache_fname.c_str(),"wb");
    if(nullptr == das_file) {
        string bes_error = "An error occurred trying to open a metadata cache file  " + das_cache_fname;
        throw BESInternalError( bes_error, __FILE__, __LINE__);
    }
    else {
        int fd_md = fileno(das_file);
        struct flock *l_md;
        l_md = lock(F_WRLCK);

        // hold a write-lock(exclusive lock) to write metadata to a file.
        if(fcntl(fd_md,F_SETLKW,l_md) == -1) {
            fclose(das_file);
            ostringstream oss;
            oss << "cache process: " << l_md->l_pid << " triggered a locking error: " << get_errno();
            throw BESInternalError( oss.str(), __FILE__, __LINE__);
        }

        try {
            write_das_to_file(das_ptr,das_file);
        }
        catch(...) {
            if(fcntl(fd_md,F_SETLK,lock(F_UNLCK)) == -1) { 
                fclose(das_file);
                throw BESInternalError( "An error occurred trying to unlock the file" + get_errno(), __FILE__, __LINE__);
            }

            fclose(das_file);
            throw BESInternalError("Fail to parse a dds cache file.",__FILE__,__LINE__);
        }

        if(fcntl(fd_md,F_SETLK,lock(F_UNLCK)) == -1) { 
            fclose(das_file);
            throw BESInternalError( "An error occurred trying to unlock the file" + get_errno(), __FILE__, __LINE__);
        }

        fclose(das_file);

    }
    
    return true;

}

// The wrapper function to call write_das_table_to_file to generate the cache.
void write_das_to_file(DAS*das_ptr,FILE* das_file) {

    // When category_flag is 2, it marks the end of the file.
    uint8_t category_flag = 2;
    AttrTable* top_table = das_ptr->get_top_level_attributes();
    write_das_table_to_file(top_table,das_file);

    // Add the final ending flag for retrieving the info.
    fwrite((const void*)&category_flag,1,1,das_file);

}

// The main function to write DAS to a file
void write_das_table_to_file(AttrTable*temp_table,FILE* das_file) {

    if(temp_table !=nullptr) {

        // 2 is the end mark of an attribute table
        uint8_t category_flag = 2;

        // Loop through the whole DAS top table
        AttrTable::Attr_iter top_startit = temp_table->attr_begin();
        AttrTable::Attr_iter top_endit = temp_table->attr_end();
        AttrTable::Attr_iter top_it = top_startit;
        while(top_it !=top_endit) {
            AttrType atype = temp_table->get_attr_type(top_it);
            if (atype == Attr_unknown)
                throw InternalErr(__FILE__,__LINE__,"Unsupported DAS Attribute type");
            else if (atype!=Attr_container) {
                BESDEBUG(HDF5_NAME, prolog << "DAS to the disk cache, attr name is: "
                                       << temp_table->get_name(top_it) << endl);
                BESDEBUG(HDF5_NAME, prolog << "DAS to the disk cache, attr type is: "
                                       << temp_table->get_type(top_it) << endl);
                // For the debugging purpose
#if 0
                //unsigned int num_attrs = temp_table->get_attr_num(temp_table->get_name(top_it));
                //cerr<<"Attribute values are "<<endl;
                //for (int i = 0; i <num_attrs;i++) 
                //    cerr<<(*(temp_table->get_attr_vector(temp_table->get_name(top_it))))[i]<<" ";
                //cerr<<endl;
                //write_das_attr_info(temp_table,top_it,das_file);
#endif
                // Write DAS attribute info to the file
                write_das_attr_info(temp_table,temp_table->get_name(top_it),
                                    temp_table->get_type(top_it),das_file);
            }
            else {
                BESDEBUG(HDF5_NAME, prolog << "DAS to the disk cache, attr container name is: "
                                       << (*top_it)->name << endl);
                // Write the container and then write the info. in this container
                AttrTable* sub_table = temp_table->get_attr_table(top_it);
                write_container_name_to_file(sub_table->get_name(),das_file);
                write_das_table_to_file(sub_table,das_file);

                // Write the end flag
                fwrite((const void*)&category_flag,1,1,das_file);
                
            }
            ++top_it;
        }

    }
}

// Write container name to the disk file
void write_container_name_to_file(const string& cont_name,FILE *das_file) {

    // 1 marks the starting of a container
    uint8_t category_flag = 1;
    vector<char> buf;
    size_t bytes_to_write = cont_name.size()+sizeof(size_t)+1;
    buf.resize(bytes_to_write);
    char*temp_pointer =buf.data();
    memcpy((void*)temp_pointer,(void*)&category_flag,1);
    temp_pointer++;
    temp_pointer=copy_str(temp_pointer,cont_name);

    size_t bytes_to_be_written = fwrite((const void*)buf.data(),1,bytes_to_write,das_file);
    if (bytes_to_be_written != bytes_to_write)
        throw BESInternalError("Failed to write a DAS container name to a cache", __FILE__, __LINE__);

}


// Write DAS attribute info. to the disk cache file
void write_das_attr_info(AttrTable* dtp,const string& attr_name, const string & attr_type,FILE * das_file) {

    // 0 marks the starting of a DAS attribute
    uint8_t category_flag = 0;

    unsigned int num_attr_elems = dtp->get_attr_num(attr_name);
    vector<string> attr_values;
    size_t total_attr_values_size = 0;
    for (unsigned int i = 0; i <num_attr_elems;i++){
        attr_values.push_back((*(dtp->get_attr_vector(attr_name)))[i]);
        total_attr_values_size += attr_values[i].size();
    }
    // Need to add a flag, value as 0 to indicate the attribute.
    // DAS: category flag, sizeof attribute name, attribute name, size of attribute type, attribute type
    size_t bytes_to_write_attr = 1 + attr_name.size() + attr_type.size() + 2* sizeof(size_t);

    // One unsigned int to store the number of element elements i
    // + sizeof(size_t) * number of elements to store the number of characters for each attribute
    // (in DAP, every attribute is in string format)
    // +total size of all attribute values 
    bytes_to_write_attr += sizeof(unsigned int) + num_attr_elems*sizeof(size_t)+total_attr_values_size;

    vector<char>attr_buf;
    attr_buf.resize(bytes_to_write_attr);
    char* temp_attrp =attr_buf.data();

    // The attribute flag
    memcpy((void*)temp_attrp,(void*)&category_flag,1);
    temp_attrp++;

    // The attribute name and type
    temp_attrp=copy_str(temp_attrp,attr_name);
    temp_attrp=copy_str(temp_attrp,attr_type);

    // Number of elements
    memcpy((void*)temp_attrp,(void*)&num_attr_elems,sizeof(unsigned int));
    temp_attrp+=sizeof(unsigned int);

    // All attributes 
    for (unsigned int i = 0; i <num_attr_elems;i++)
        temp_attrp=copy_str(temp_attrp,(*(dtp->get_attr_vector(attr_name)))[i]);

    size_t bytes_to_be_written = fwrite((const void*)attr_buf.data(),1,bytes_to_write_attr,das_file);
    if (bytes_to_be_written != bytes_to_write_attr)
        throw BESInternalError("Failed to write a DAS attribute to a cache",__FILE__, __LINE__);

}

// Read DDS from a disk cache, this function is not used by default.
void HDF5RequestHandler::read_dds_from_disk_cache(BESDDSResponse* bdds, BESDataDDSResponse* data_bdds,
                                bool build_data,const string & container_name,const string & h5_fname,
                              const string & dds_cache_fname,const string &das_cache_fname, hid_t h5_fd, 
                              bool das_from_dc) {

     
     BESDEBUG(HDF5_NAME, prolog << "BEGIN dds_cache_fname: " << dds_cache_fname << endl);

     DDS *dds;
     if(true == build_data) 
        dds = data_bdds->get_dds();
     else
        dds = bdds->get_dds();
  
     // write a function to pass the following with the lock.
     BaseTypeFactory tf;
     DDS tdds(&tf,name_path(h5_fname),"3.2");
     tdds.filename(h5_fname);

     FILE *dds_file = fopen(dds_cache_fname.c_str(),"r");
     tdds.parse(dds_file);

     auto cache_dds_unique = make_unique<DDS>(tdds);
     delete dds;

     auto cache_dds = cache_dds_unique.release();
     Ancillary::read_ancillary_dds( *cache_dds, h5_fname ) ;

     add_das_to_dds(cache_dds,container_name,h5_fname,das_cache_fname,h5_fd,das_from_dc);
     if(true == build_data)
        data_bdds->set_dds(cache_dds);
     else 
        bdds->set_dds(cache_dds);
     if(dds_file !=nullptr)
        fclose(dds_file);

    if (dds_cache) {
        // add a copy
        BESDEBUG(HDF5_NAME, prolog << "For memory cache, DDS added to the cache for : " << h5_fname << endl);
        dds_cache->add(new DDS(*cache_dds), h5_fname);
    }

}

// Add DAS to DDS. 
void HDF5RequestHandler::add_das_to_dds(DDS *dds, const string &/*container_name*/, const string &filename,
    const string &das_cache_fname, hid_t h5_fd, bool das_from_dc) {

    BESDEBUG(HDF5_NAME, prolog << "BEGIN"  << endl);

    // Check DAS memory cache
    DAS *das = nullptr ;
    bool use_das_cache = false;
    if (das_cache) 
        das = dynamic_cast<DAS*>(das_cache->get(filename));
    if (das) 
        use_das_cache = true;
 
    if (true == use_das_cache) {
        BESDEBUG(HDF5_NAME, prolog << "DAS Cached hit for : " << filename << endl);
        dds->transfer_attributes(das); // no need to copy the cached DAS
    }

    else {

        auto das_unique = make_unique<DAS>();
        das = das_unique.get();
        // The following block is commented out because the attribute containers in DDX disappear
	    // when the container_name of DAS is added.  Without adding the container_name of DAS,
        // the attribute containers show up in DDX. This information is re-discovered while working on
        // https://bugs.earthdata.nasa.gov/browse/HYRAX-714 although the following code was commented
        // out long time ago. KY 2022-05-27
#if 0
        if (!container_name.empty())
            das->container_name(container_name);
#endif
        if (das_from_dc == true)
            read_das_from_disk_cache(das_cache_fname,das);       
        else
            read_das_from_file(das, filename, das_cache_fname, h5_fd, das_from_dc);

        dds->transfer_attributes(das);

        if (das_cache) {
                    // add a copy
            BESDEBUG(HDF5_NAME, prolog << "For memory cache, DAS added to the cache for : " << filename << endl);
            das_cache->add(new DAS(*das), filename);
        }

    }
    
}

void HDF5RequestHandler::read_das_from_file(DAS *das, const string &filename, const string &das_cache_fname,
                                            hid_t h5_fd, bool das_from_dc)
{
    // This bool is for the case, when DDS is read from a cache then we need to open the HDF5 file.
    bool h5_file_open = true;
    if (h5_fd == -1)
        h5_file_open = false;
    if (true == _usecf) {
        // go to the CF option
        if (h5_file_open == false)
            h5_fd = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

        read_cfdas( *das,filename,h5_fd);
        if (h5_file_open == false)
            H5Fclose(h5_fd);
    }
    else {
        if(h5_file_open == false)
            h5_fd = get_fileid(filename.c_str());
        find_gloattr(h5_fd, *das);
        depth_first(h5_fd, "/", *das);
        if(h5_file_open == false)
            close_fileid(h5_fd);
    }

    Ancillary::read_ancillary_das( *das, filename ) ;

    if (das_cache_fname.empty() == false && das_from_dc == false)
        write_das_to_disk_cache(das_cache_fname,das);

}
bool obtain_beskeys_info(const string& key, bool & has_key) {

    bool ret_value = false;
    string doset;
    TheBESKeys::TheKeys()->get_value( key, doset, has_key ) ;
    if (has_key) {
        const string dosettrue ="true";
        const string dosetyes = "yes";
        doset = BESUtil::lowercase(doset) ;
        ret_value = (dosettrue == doset  || dosetyes == doset);
    }
    return ret_value;
}


// get_uint_key and get_float_key are copied from the netCDF handler.
static unsigned int get_uint_key(const string &key, unsigned int def_val)
{
    bool found = false;
    string doset;

    TheBESKeys::TheKeys()->get_value(key, doset, found);
    if (true == found) {
        // In C++11, stoi is better.
        return stoi(doset);
    }
    else {
        return def_val;
    }
}

static unsigned long get_ulong_key(const string &key, unsigned long def_val)
{
    bool found = false;
    string doset;

    TheBESKeys::TheKeys()->get_value(key, doset, found);
    if (true == found) {
        // In C++11, stoul is better.
        return stoul(doset);
    }
    else {
        return def_val;
    }
}
static float get_float_key(const string &key, float def_val)
{
    bool found = false;
    string doset;

    TheBESKeys::TheKeys()->get_value(key, doset, found);
    if (true == found)
        return stof(doset);
    else
        return def_val;
}

static string get_beskeys(const string &key) {

    bool found = false;
    string ret_value;

    TheBESKeys::TheKeys()->get_value( key, ret_value, found ) ;
    return ret_value;
}

// The function to copy a string to a memory buffer. 
char* copy_str(char*temp_ptr,const string & str) {

    size_t str_size=str.size();
    memcpy((void*)temp_ptr,(void*)&str_size,sizeof(size_t));
    temp_ptr+=sizeof(size_t);
    vector<char>temp_vc2(str.begin(),str.end());
    memcpy((void*)temp_ptr,(void*)temp_vc2.data(),str.size());
    temp_ptr+=str.size();
    return temp_ptr;

}


//  Obtain the string from a memory buffer.
//  Note: both char* and string(as a reference) will be returned
//  The attribute binary first stores the size of the string, then the string itself
char* obtain_str(char*temp_ptr,string & str) {

    size_t oname_size = *((size_t *)temp_ptr);
    temp_ptr = temp_ptr + sizeof(size_t);
    string oname;
    for(unsigned int i =0; i<oname_size; i++){
        oname.push_back(*temp_ptr);
        ++temp_ptr;
    }
    str = oname;
    return temp_ptr;

}

// For our case, there are no global attributes for DAS. 
// The global attributes are always under HDF_GLOBAL.
// The main function to obtain the DAS info. from the cache.
char* get_attr_info_from_dc(char*temp_pointer,DAS *das,AttrTable *at_par) {

    // 3 is only for the code  to come into the loop.
    uint8_t flag =3;
    while(flag !=2) {
        flag = *((uint8_t*)(temp_pointer));
        BESDEBUG(HDF5_NAME, prolog << "Build DAS from the disk cache file flag: "
                 <<" flag = 0, attribute; flag = 1, container; flag =2; end of container;"
                 <<" flag = 3; the initial value to get the attribute retrieval process started." 
                 <<" The flag value is "
                 << (int)flag <<endl);
        temp_pointer++;

        if (flag ==1) {
            string container_name;
            temp_pointer = obtain_str(temp_pointer,container_name);
            BESDEBUG(HDF5_NAME, prolog << "DAS from the disk cache, container name is " << container_name << endl);

            // Remember the current Attribute table state
            AttrTable*temp_at_par = at_par;
            if(at_par == nullptr) {
                auto new_attr_table_unique = make_unique<libdap::AttrTable>();
                auto new_attr_table = new_attr_table_unique.release();
                at_par = das->add_table(container_name, new_attr_table);
            }
            else 
                at_par = at_par->append_container(container_name);
                
            temp_pointer = get_attr_info_from_dc(temp_pointer,das,at_par);
            // MUST resume the original state
            at_par = temp_at_par;
 
        }
        else if(flag == 0) {
            // The attribute must have a table.
            if(at_par ==nullptr) 
                throw BESInternalError( "The AttrTable  must exist for DAS attributes", __FILE__, __LINE__ ) ;
            
            // Attribute name
            string attr_name;
            temp_pointer = obtain_str(temp_pointer,attr_name);
            BESDEBUG(HDF5_NAME, prolog << "DAS from the disk cache, attr name is: " << attr_name << endl);

            // Attribute type
            string attr_type;
            temp_pointer = obtain_str(temp_pointer,attr_type);
            BESDEBUG(HDF5_NAME, prolog << "DAS from the disk cache, attr type is: " << attr_type << endl);

            // Attribute values
            unsigned int num_values = *((unsigned int*)(temp_pointer));
            BESDEBUG(HDF5_NAME, prolog << "DAS from the disk cache, number of attribute values is: " << num_values << endl);
            temp_pointer+=sizeof(unsigned int);

            vector <string> attr_values;

            for(unsigned int i = 0; i<num_values; i++) {
                string attr_value;
                temp_pointer = obtain_str(temp_pointer,attr_value);
                attr_values.push_back(attr_value);
                BESDEBUG(HDF5_NAME, prolog << "DAS from the disk cache,  attribute value is: " << attr_value << endl);
            }

            at_par->append_attr(attr_name,attr_type,&attr_values);
        }

    }
    return temp_pointer;
}

// The debugging function to get attribute info.
void get_attr_contents(AttrTable*temp_table) {
    if(temp_table !=nullptr) {
        AttrTable::Attr_iter top_startit = temp_table->attr_begin();
        AttrTable::Attr_iter top_endit = temp_table->attr_end();
        AttrTable::Attr_iter top_it = top_startit;
        while(top_it !=top_endit) {
            AttrType atype = temp_table->get_attr_type(top_it);
            if(atype == Attr_unknown) 
                cerr<<"unsupported DAS attributes" <<endl;
            else if(atype!=Attr_container) {
           
                   cerr<<"Attribute name is "<<temp_table->get_name(top_it)<<endl;
                   cerr<<"Attribute type is "<<temp_table->get_type(top_it)<<endl;
                   unsigned int num_attrs = temp_table->get_attr_num(temp_table->get_name(top_it));
                   cerr<<"Attribute values are "<<endl;
                   for (unsigned int i = 0; i <num_attrs;i++)
                        cerr<<(*(temp_table->get_attr_vector(temp_table->get_name(top_it))))[i]<<" ";
                   cerr<<endl;
            }
            else {
                cerr<<"Coming to the attribute container.  "<<endl;
                cerr<<"container  name is "<<(*top_it)->name <<endl;
                AttrTable* sub_table = temp_table->get_attr_table(top_it);
                cerr<<"container table name is "<<sub_table->get_name() <<endl;
                get_attr_contents(sub_table);
            }
            ++top_it;
        }

    }
}


void HDF5RequestHandler::add_attributes(BESDataHandlerInterface &dhi) {

    BESResponseObject *response = dhi.response_handler->get_response_object();
    auto bdds = dynamic_cast<BESDataDDSResponse *>(response);
    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);
    DDS *dds = bdds->get_dds();
    string container_name = bdds->get_explicit_containers() ? dhi.container->get_symbolic_name(): "";
    string filename = dhi.container->access();
    DAS* das = nullptr;
    bool das_from_mcache = false;
    if(das_cache) {
        das = dynamic_cast<DAS*>(das_cache->get(filename));
        if (das) {
            BESDEBUG(HDF5_NAME, prolog << "DAS Cached hit for : " << filename << endl);
            dds->transfer_attributes(das); // no need to copy the cached DAS
            das_from_mcache = true;
        }
    }

    if(false == das_from_mcache) {
        auto das_unique = make_unique<DAS>();
        das = das_unique.release();
        // This looks at the 'use explicit containers' prop, and if true
        // sets the current container for the DAS.
        if (!container_name.empty()) das->container_name(container_name);

        hid_t h5_fd =-1;
        if (true == _usecf) {
            // go to the CF option
            h5_fd = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
            if (h5_fd < 0){
                string invalid_file_msg="Could not open this HDF5 file ";
                invalid_file_msg +=filename;
                invalid_file_msg +=". It is very possible that this file is not an HDF5 file ";
                invalid_file_msg +=" but with the .h5/.HDF5 suffix. Please check with the data";
                invalid_file_msg +=" distributor.";
                throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
            }
 
            read_cfdas( *das,filename,h5_fd);
            
            H5Fclose(h5_fd);
        }
        else { 
            h5_fd = get_fileid(filename.c_str());
            find_gloattr(h5_fd, *das);
            depth_first(h5_fd, "/", *das);
            close_fileid(h5_fd);
        }

        Ancillary::read_ancillary_das(*das, filename);

        dds->transfer_attributes(das);

        // Only free the DAS if it's not added to the cache
        if (das_cache) {
            // add a copy
            BESDEBUG(HDF5_NAME, prolog << "DAS added to the cache for : " << filename << endl);
            das_cache->add(das, filename);
        }
        else {
            delete das;
        }
    }
    BESDEBUG(HDF5_NAME, prolog << "Data ACCESS in add_attributes(): set the including attribute flag to true: "<<filename << endl);
    bdds->set_ia_flag(true);

}

void HDF5RequestHandler::close_h5_files(hid_t cf_fileid, hid_t fileid) {

    if (cf_fileid != -1)
        H5Fclose(cf_fileid);
    if (fileid != -1)
        H5Fclose(fileid);

}
