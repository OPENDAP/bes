
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of hdf5_handler, a data handler for the OPeNDAP data
// server. 

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Copyright (c) 2007-2016 The HDF Group, Inc. and OPeNDAP, Inc.
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

// HDF5RequestHandler.cc
/// \file HDF5RequestHandler.cc
/// \brief The implementation of the entry functions to execute the handlers
///
/// It includes build_das, build_dds and build_data etc.
/// \author James Gallagher <jgallagher@opendap.org>


#include<iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <DMR.h>
#include <D4BaseTypeFactory.h>
#include <BESDMRResponse.h>
#include <ObjMemCache.h>
#include "HDF5_DMR.h"

#include<mime_util.h>
#include "hdf5_handler.h"
#include "HDF5RequestHandler.h"
#include "HDF5_DDS.h"

#include <BESDASResponse.h>
#include <BESDDSResponse.h>
#include <BESDataDDSResponse.h>
#include <Ancillary.h>
#include <BESInfo.h>
#include <BESDapNames.h>
#include <BESResponseNames.h>
#include <BESContainer.h>
#include <BESResponseHandler.h>
#include <BESVersionInfo.h>
#include <BESServiceRegistry.h>
#include <BESUtil.h>
#include <BESDapError.h>
#include <BESInternalFatalError.h>
#include <TheBESKeys.h>
#include <BESDebug.h>
#include "h5get.h"
#include "config_hdf5.h"

#define HDF5_NAME "h5"
#include "h5cfdaputil.h"

using namespace std;
using namespace libdap;

void get_attr_contents(AttrTable* temp_table);
// Check if the BES key is set.
bool check_beskeys(const string);

// Obtain the BES key as an integer
static unsigned int get_uint_key(const string &key,unsigned int def_val);

// Obtain the BES key as a floating-pointer number.
static float get_float_key(const string &key, float def_val);

// Obtain the BES key as a string.
static string get_beskeys(const string&);

// For the CF option
extern void read_cfdas(DAS &das, const string & filename,hid_t fileid);
extern void read_cfdds(DDS &dds, const string & filename,hid_t fileid);


// Check the description of cache_entries and cache_purge_level at h5.conf.in.
unsigned int HDF5RequestHandler::_mdcache_entries = 200;
unsigned int HDF5RequestHandler::_lrdcache_entries = 40;
unsigned int HDF5RequestHandler::_srdcache_entries = 200;
float HDF5RequestHandler::_cache_purge_level = 0.2;

// Metadata object cache at DAS,DDS and DMR.
ObjMemCache *HDF5RequestHandler::das_cache = 0;
ObjMemCache *HDF5RequestHandler::dds_cache = 0;
ObjMemCache *HDF5RequestHandler::dmr_cache = 0;

ObjMemCache *HDF5RequestHandler::lrdata_mem_cache = 0;
ObjMemCache *HDF5RequestHandler::srdata_mem_cache = 0;

// Set default values of all BES keys be false.
bool HDF5RequestHandler::_usecf                       = false;
bool HDF5RequestHandler::_pass_fileid                 = false;
bool HDF5RequestHandler::_disable_structmeta          = false;
bool HDF5RequestHandler::_keep_var_leading_underscore = false;
bool HDF5RequestHandler::_check_name_clashing         = false;
bool HDF5RequestHandler::_add_path_attrs              = false;
bool HDF5RequestHandler::_drop_long_string            = false;
bool HDF5RequestHandler::_fillvalue_check             = false;
bool HDF5RequestHandler::_check_ignore_obj            = false;
bool HDF5RequestHandler::_common_cache_dirs            = false;


bool HDF5RequestHandler::_use_disk_cache              =false;
bool HDF5RequestHandler::_use_disk_dds_cache              =false;
string HDF5RequestHandler::_disk_cache_dir            ="";
string HDF5RequestHandler::_disk_cachefile_prefix     ="";
long HDF5RequestHandler::_disk_cache_size             =0;


bool HDF5RequestHandler::_disk_cache_comp_data        =false;
bool HDF5RequestHandler::_disk_cache_float_only_comp_data    =false;
float HDF5RequestHandler::_disk_cache_comp_threshold        =1.0;
long HDF5RequestHandler::_disk_cache_var_size        =0;

bool HDF5RequestHandler::_use_disk_meta_cache        = false;
string HDF5RequestHandler::_disk_meta_cache_path       ="";
//BaseTypeFactory factory;
//libdap::DDS HDF5RequestHandler::hd_dds(&factory,"");
vector<string> HDF5RequestHandler::lrd_cache_dir_list;
vector<string> HDF5RequestHandler::lrd_non_cache_dir_list;
vector<string> HDF5RequestHandler::lrd_var_cache_file_list;
//libdap::DDS*cache_dds;


HDF5RequestHandler::HDF5RequestHandler(const string & name)
    :BESRequestHandler(name)
{

    BESDEBUG(HDF5_NAME, "In HDF5RequestHandler::HDF5RequestHandler" << endl);

    add_handler(DAS_RESPONSE, HDF5RequestHandler::hdf5_build_das);
    add_handler(DDS_RESPONSE, HDF5RequestHandler::hdf5_build_dds);
    add_handler(DATA_RESPONSE, HDF5RequestHandler::hdf5_build_data);
    add_handler(DMR_RESPONSE, HDF5RequestHandler::hdf5_build_dmr);
    add_handler(DAP4DATA_RESPONSE, HDF5RequestHandler::hdf5_build_dmr);

    add_handler(HELP_RESPONSE, HDF5RequestHandler::hdf5_build_help);
    add_handler(VERS_RESPONSE, HDF5RequestHandler::hdf5_build_version);

    // Obtain the metadata cache entries and purge level.
    HDF5RequestHandler::_mdcache_entries     = get_uint_key("H5.MetaDataMemCacheEntries", 0);
    HDF5RequestHandler::_lrdcache_entries     = get_uint_key("H5.LargeDataMemCacheEntries", 0);
    HDF5RequestHandler::_srdcache_entries     = get_uint_key("H5.SmallDataMemCacheEntries", 0);
    HDF5RequestHandler::_cache_purge_level = get_float_key("H5.CachePurgeLevel", 0.2);

    if (get_mdcache_entries()) {  // else it stays at its default of null
        das_cache = new ObjMemCache(get_mdcache_entries(), get_cache_purge_level());
        dds_cache = new ObjMemCache(get_mdcache_entries(), get_cache_purge_level());
        dmr_cache = new ObjMemCache(get_mdcache_entries(), get_cache_purge_level());
    }

    // Check if the EnableCF key is set.
    _usecf                       = check_beskeys("H5.EnableCF");

    // The following keys are only effective when usecf is true.
    _pass_fileid                 = check_beskeys("H5.EnablePassFileID");
    _disable_structmeta          = check_beskeys("H5.DisableStructMetaAttr");
    _keep_var_leading_underscore = check_beskeys("H5.KeepVarLeadingUnderscore");
    _check_name_clashing         = check_beskeys("H5.EnableCheckNameClashing");
    _add_path_attrs              = check_beskeys("H5.EnableAddPathAttrs");
    _drop_long_string            = check_beskeys("H5.EnableDropLongString");
    _fillvalue_check             = check_beskeys("H5.EnableFillValueCheck");
    _check_ignore_obj            = check_beskeys("H5.CheckIgnoreObj");
    _use_disk_cache              = check_beskeys("H5.EnableDiskDataCache");
    _disk_cache_dir              = get_beskeys("H5.DiskCacheDataPath");
    _disk_cachefile_prefix       = get_beskeys("H5.DiskCacheFilePrefix");
    _disk_cache_size             = get_uint_key("H5.DiskCacheSize",0);

    _disk_cache_comp_data        = check_beskeys("H5.DiskCacheComp");
    _disk_cache_float_only_comp_data = check_beskeys("H5.DiskCacheFloatOnlyComp");
    _disk_cache_comp_threshold   = get_float_key("H5.DiskCacheCompThreshold",1.0);
    _disk_cache_var_size         = 1024*get_uint_key("H5.DiskCacheCompVarSize",0);

    _use_disk_meta_cache         = check_beskeys("H5.EnableDiskMetaDataCache");
    _use_disk_dds_cache         = check_beskeys("H5.EnableDiskDDSCache");
    _disk_meta_cache_path        = get_beskeys("H5.DiskMetaDataCachePath");

    if(get_usecf()) {
        if(get_lrdcache_entries()) {
            lrdata_mem_cache = new ObjMemCache(get_lrdcache_entries(), get_cache_purge_level());
            if(true == check_beskeys("H5.LargeDataMemCacheConfig")) {
                _common_cache_dirs =obtain_lrd_common_cache_dirs();
#if 0
if(false == _common_cache_dirs) 
cerr<<"No specific cache info"<<endl;
#endif
             
            }
        }
        if(get_srdcache_entries()) {
            srdata_mem_cache = new ObjMemCache(get_srdcache_entries(),get_cache_purge_level());
//cerr<<"small memory data cache "<<endl;

        }

        if(_disk_cache_comp_data == true && _use_disk_cache == true) {
            if(_disk_cache_comp_threshold < 1.0) {
                ostringstream ss;
                ss<< _disk_cache_comp_threshold;
                string _comp_threshold_str(ss.str());
                string invalid_comp_threshold ="The Compression Threshold is the total size of the variable array";
                invalid_comp_threshold+=" divided by the storage size of compressed array. It should always be >1";
                invalid_comp_threshold+=" The current threhold set at h5.conf is ";
                invalid_comp_threshold+=_comp_threshold_str;
                invalid_comp_threshold+=" . Go back to h5.conf and change the H5.DiskCacheCompThreshold to a >1.0 number.";
                throw BESInternalError(invalid_comp_threshold,__FILE__,__LINE__);
            }
        }
//        else 
//cerr<<"no small memory data cache "<<endl;
    }


    BESDEBUG(HDF5_NAME, "Exiting HDF5RequestHandler::HDF5RequestHandler" << endl);
}

HDF5RequestHandler::~HDF5RequestHandler()
{
    
    // delete the cache.
    delete das_cache;
    delete dds_cache;
    delete dmr_cache;
    delete lrdata_mem_cache;
    delete srdata_mem_cache;
     
}

// Build DAS
bool HDF5RequestHandler::hdf5_build_das(BESDataHandlerInterface & dhi)
{

    // For the time being, separate CF file ID from the default file ID(mainly for debugging)
    hid_t cf_fileid = -1;

    // Obtain the HDF5 file name.
    string filename = dhi.container->access();

    // Obtain the BES object from the client
    BESResponseObject *response = dhi.response_handler->get_response_object() ;

    // Convert to the BES DAS response
    BESDASResponse *bdas = dynamic_cast < BESDASResponse * >(response) ;
    if( !bdas )
        throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    try {
        bdas->set_container( dhi.container->get_symbolic_name() ) ;
        DAS *das = bdas->get_das();

        // Look inside the memory cache if it's initialized
        DAS *cached_das_ptr = 0;
        if (das_cache && (cached_das_ptr = static_cast<DAS*>(das_cache->get(filename)))) {

            // copy the cached DAS into the BES response object
            BESDEBUG(HDF5_NAME, "DAS Cached hit for : " << filename << endl);
            *das = *cached_das_ptr;
        }
        else {

            bool das_from_dc = false;
            string das_cache_fname;

            if(_use_disk_meta_cache == true) {

                string base_filename   =  HDF5CFUtil::obtain_string_after_lastslash(filename);
                das_cache_fname = _disk_meta_cache_path+"/" +base_filename+"_das";

                if(access(das_cache_fname.c_str(),F_OK) !=-1)
                   das_from_dc = true;

            }

            if(true == das_from_dc) {
                read_das_from_disk_cache(das_cache_fname,das);
                if (das_cache) {
                    // add a copy
                    BESDEBUG(HDF5_NAME, "HDF5 DAS reading DAS from the disk cache. For memory cache, DAS added to the cache for : " << filename << endl);
                    //das_cache->add(new DAS(*das), filename);
                    das_cache->add(das, filename);
                }
            }

           else {
            H5Eset_auto2(H5E_DEFAULT,NULL,NULL);
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

AttrTable* top_table = das->get_top_level_attributes();
get_attr_contents(top_table);
#if 0
AttrTable::Attr_iter top_startit = top_table->attr_begin();
AttrTable::Attr_iter top_endit = top_table->attr_end();
AttrTable::Attr_iter top_it = top_startit;
while(top_it != top_endit) {
cerr<<"attr it name is "<< (*top_it)->name <<endl;
AttrTable* temp_table = das->get_table(top_it);
if(temp_table!=0){
if(temp_table->get_attr_type(top_it)==Attr_container){
    //cerr<<"Adding the attribute container and the name is "<<temp_table->get_attr_type(top_it)<<endl;
    cerr<<"Adding the attribute container.  "<<endl;
    get_attr_contents(temp_table);
}
temp_table->print(cerr);
}
++top_it;
}
#endif

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

            // If the cache is turned on
            if(das_cache) {
                // add a copy
                BESDEBUG(HDF5_NAME, "DAS added to the cache for : " << filename << endl);
                das_cache->add(new DAS(*das), filename);
            }

            if(das_cache_fname!="") {
                BESDEBUG(HDF5_NAME, "HDF5 Build DAS: Write DAS to disk cache " << das_cache_fname << endl);
                write_das_to_disk_cache(das_cache_fname,das);
            }
               
          }
        }

        bdas->clear_container() ;
    }
    catch(InternalErr & e) {

        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        throw BESDapError(e.get_error_message(), true, e.get_error_code(),
                       __FILE__, __LINE__);
    }
    catch(Error & e) {
 
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        throw BESDapError(e.get_error_message(), false, e.get_error_code(),
                       __FILE__, __LINE__);
    }
    catch (BESError &e){
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        throw;
    }
    catch(...) {

        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        string s = "unknown exception caught building HDF5 DAS";
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }

    return true;
}

// Convenient function that helps  build DDS and Data
void HDF5RequestHandler::get_dds_with_attributes( BESDDSResponse*bdds,BESDataDDSResponse*data_bdds,const string &container_name, const string& filename,const string &dds_cache_fname, const string &das_cache_fname,bool dds_from_dc,bool das_from_dc, bool build_data)
{
    DDS *dds;
    if(true == build_data) 
        dds = data_bdds->get_dds();
    else  dds = bdds->get_dds();

    // For the time being, separate CF file ID from the default file ID(mainly for debugging)
    hid_t fileid    = -1;
    hid_t cf_fileid = -1;

//cerr<<"filename is "<<filename <<endl;
    try {

        // Look in memory cache to see if it's initialized
        DDS* cached_dds_ptr = 0;
        if (dds_cache && (cached_dds_ptr = static_cast<DDS*>(dds_cache->get(filename)))) {
            // copy the cached DDS into the BES response object. Assume that any cached DDS
            // includes the DAS information.
            BESDEBUG(HDF5_NAME, "DDS Metadata Cached hit for : " << filename << endl);
            *dds = *cached_dds_ptr; // Copy the referenced object
        }
        else if (true ==dds_from_dc) {
            read_dds_from_disk_cache(bdds,data_bdds,build_data,container_name,filename,dds_cache_fname,das_cache_fname,-1,das_from_dc);
        }
        else {

            BESDEBUG(HDF5_NAME, "Build DDS from the HDF5 file. " << filename << endl);
            H5Eset_auto2(H5E_DEFAULT,NULL,NULL);
            dds->filename(filename);

            // For the time being, not mess up CF's fileID with Default's fileID
            if(true == _usecf) {
 
                cf_fileid = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
                if (cf_fileid < 0){
                    string invalid_file_msg="Could not open this HDF5 file ";
                    invalid_file_msg +=filename;
                    invalid_file_msg +=". It is very possible that this file is not an HDF5 file ";
                    invalid_file_msg +=" but with the .h5/.HDF5 suffix. Please check with the data";
                    invalid_file_msg +=" distributor.";
                    throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
                }
                read_cfdds(*dds,filename,cf_fileid);
//cerr<<"DDS from file "<<endl;
//dds->dump(cerr);
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

            // Generate the DDS cached file if needed
            if(dds_cache_fname!="" && dds_from_dc == false) 
                write_dds_to_disk_cache(dds_cache_fname,dds);
            // Add attributes
            {
                hid_t h5_fd = -1;
                if(_usecf == true)
                    h5_fd = cf_fileid;
                else
                    h5_fd = fileid;
                add_das_to_dds(dds,container_name,filename,das_cache_fname,h5_fd,das_from_dc);
            }
        
            // Add memory cache if possible
            if (dds_cache) {
            	// add a copy
                BESDEBUG(HDF5_NAME, "DDS added to the cache for : " << filename << endl);
                dds_cache->add(new DDS(*dds), filename);
            }

            if(cf_fileid != -1)
                H5Fclose(cf_fileid);
            if(fileid != -1)
                H5Fclose(fileid);
 
        }
//dds->print(cout);
    
    }
    catch(InternalErr & e) {

        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);

        if(fileid != -1)
            H5Fclose(fileid);

        throw BESDapError(e.get_error_message(), true, e.get_error_code(),
                       __FILE__, __LINE__);
    }
    catch(Error & e) {

        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        if(fileid !=-1)
            H5Fclose(fileid);

        throw BESDapError(e.get_error_message(), false, e.get_error_code(),
                       __FILE__, __LINE__);
    }
    catch (BESError &e){
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        if(fileid !=-1)
            H5Fclose(fileid);

        throw;
    }
    catch(...) {

        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        if(fileid !=-1)
            H5Fclose(fileid);

       string s = "unknown exception caught building HDF5 DDS";
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }

}

void get_attr_contents(AttrTable*temp_table) {
    if(temp_table !=NULL) {
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
                   for (int i = 0; i <num_attrs;i++) 
                        cerr<<(*(temp_table->get_attr_vector(temp_table->get_name(top_it))))[i]<<" ";
                   cerr<<endl;
            }
            else {
                cerr<<"Coming to the attribute container.  "<<endl;
                cerr<<"container  name is "<<(*top_it)->name <<endl;
                AttrTable* sub_table = temp_table->get_attr_table(top_it);
                get_attr_contents(sub_table);
            }
            ++top_it;
        }

    }
}


#if 0
void get_attr_contents(AttrTable*temp_table) {
AttrTable::Attr_iter top_startit = temp_table->attr_begin();
AttrTable::Attr_iter top_endit = temp_table->attr_end();
AttrTable::Attr_iter top_it = top_startit;
while(top_it !=top_endit) {
    


    AttrTable* sub_table = temp_table->get_attr_table(top_it);
    if(sub_table!=0) {
        if(sub_table->get_attr_type(top_it)==Attr_container) {
            cerr<<"Coming to the attribute container.  "<<endl;
            cerr<<"container  name is "<<(*top_it)->name <<endl;
AttrTable::Attr_iter start_aiter=sub_table->attr_begin();
AttrTable::Attr_iter ait = start_aiter;
AttrTable::Attr_iter end_aiter = sub_table->attr_end();
while(ait != end_aiter) {
           
           cerr<<"Attribute name is "<<sub_table->get_name(ait)<<endl;
           cerr<<"Attribute type is "<<sub_table->get_type(ait)<<endl;
           unsigned int num_attrs = sub_table->get_attr_num(sub_table->get_name(ait));
           cerr<<"Attribute values are "<<endl;
           for (int i = 0; i <num_attrs;i++) 
             cerr<<(*(sub_table->get_attr_vector(sub_table->get_name(ait))))[i]<<" ";
           cerr<<endl;
 
++ait;
}
            get_attr_contents(sub_table);
        }
        else {
           cerr<<"Attribute name outer loop is "<<sub_table->get_name(top_it)<<endl;
           cerr<<"Attribute type outrt loop is "<<sub_table->get_type(top_it)<<endl;
           unsigned int num_attrs = sub_table->get_attr_num(sub_table->get_name(top_it));
           cerr<<"Attribute values outer are "<<endl;
           for (int i = 0; i <num_attrs;i++) 
             cerr<<(*(sub_table->get_attr_vector(sub_table->get_name(top_it))))[i]<<" ";
           cerr<<endl;
        }
    }
    ++top_it;

}
}
#endif





#if 0
// Convenient function that helps  build DDS and Data
void HDF5RequestHandler::get_dds_with_attributes(const string &filename, const string &container_name, DDS*dds) {

    // For the time being, separate CF file ID from the default file ID(mainly for debugging)
    hid_t fileid    = -1;
    hid_t cf_fileid = -1;

    try {

        // Look in memory cache to see if it's initialized
        DDS* cached_dds_ptr = 0;
        if (dds_cache && (cached_dds_ptr = static_cast<DDS*>(dds_cache->get(filename)))) {
            // copy the cached DDS into the BES response object. Assume that any cached DDS
            // includes the DAS information.
            BESDEBUG(HDF5_NAME, "DDS Cached hit for : " << filename << endl);
            *dds = *cached_dds_ptr; // Copy the referenced object
        }

        else {

            H5Eset_auto2(H5E_DEFAULT,NULL,NULL);
            if (!container_name.empty()) 
                dds->container_name(container_name);
            dds->filename(filename);

            // For the time being, not mess up CF's fileID with Default's fileID
            if(true == _usecf) {
// This block cannot be used to cache the data
#if 0
                string base_filename =  HDF5CFUtil::obtain_string_after_lastslash(filename);
                string dds_filename = "/tmp/"+base_filename+"_dds";
                FILE *dds_file = fopen(dds_filename.c_str(),"r");
cerr<<"before parsing "<<endl;
BaseTypeFactory tf;
DDS tdds(&tf,name_path(filename),"3.2");
tdds.filename(filename);
                //dds->parse(dds_file);
                tdds.parse(dds_file);
                //DDS *cache_dds = new DDS(tdds);
                cache_dds = new DDS(tdds);
if(dds!=NULL)
   delete dds;
dds = cache_dds;
tdds.print(cout);
dds->print(cout);
cerr<<"after parsing "<<endl;
//dds->print(cout);
 //               fclose(dds_file);
//#endif

#endif 
// end of this block
           
 
                cf_fileid = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
                if (cf_fileid < 0){
                    string invalid_file_msg="Could not open this HDF5 file ";
                    invalid_file_msg +=filename;
                    invalid_file_msg +=". It is very possible that this file is not an HDF5 file ";
                    invalid_file_msg +=" but with the .h5/.HDF5 suffix. Please check with the data";
                    invalid_file_msg +=" distributor.";
                    throw BESInternalError(invalid_file_msg,__FILE__,__LINE__);
                }
//#if 0
                read_cfdds(*dds,filename,cf_fileid);
//#endif
                // Generate the DDS cached file
                string base_filename =  HDF5CFUtil::obtain_string_after_lastslash(filename);
                string dds_filename = "/tmp/"+base_filename+"_dds";
                FILE *dds_file = fopen(dds_filename.c_str(),"w");
                dds->print(dds_file);
                fclose(dds_file);

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


            // Check DAS cache
            DAS *das = 0 ;

            if (das_cache && (das = static_cast<DAS*>(das_cache->get(filename)))) {
                BESDEBUG(HDF5_NAME, "DAS Cached hit for : " << filename << endl);
                dds->transfer_attributes(das); // no need to copy the cached DAS
            }

            else {

                das = new DAS ;
        
                if (!container_name.empty()) 
                    das->container_name(container_name);

                if (true == _usecf) {

                    // go to the CF option
                    read_cfdas( *das,filename,cf_fileid);
 
                }
                else { 

                    find_gloattr(fileid, *das);
                    depth_first(fileid, "/", *das);
                    close_fileid(fileid);
                }

                if(cf_fileid != -1)
                    H5Fclose(cf_fileid);

                Ancillary::read_ancillary_das( *das, filename ) ;

                dds->transfer_attributes(das);
        

                // Only free the DAS if it's not added to the cache
                if (das_cache) {
                    // add a copy
                    BESDEBUG(HDF5_NAME, "DAS added to the cache for : " << filename << endl);
                    //das_cache->add(new DAS(*das), filename);
                    das_cache->add(das, filename);
                }
                else {
                        delete das;
                }
            }
        
            if (dds_cache) {
            	// add a copy
                BESDEBUG(HDF5_NAME, "DDS added to the cache for : " << filename << endl);
                dds_cache->add(new DDS(*dds), filename);
            }
 
        }

//dds->print(cout);
    
    }
    catch(InternalErr & e) {

        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);

        if(fileid != -1)
            H5Fclose(fileid);

        throw BESDapError(e.get_error_message(), true, e.get_error_code(),
                       __FILE__, __LINE__);
    }
    catch(Error & e) {

        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        if(fileid !=-1)
            H5Fclose(fileid);

        throw BESDapError(e.get_error_message(), false, e.get_error_code(),
                       __FILE__, __LINE__);
    }
    catch (BESError &e){
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        if(fileid !=-1)
            H5Fclose(fileid);

        throw;
    }
    catch(...) {

        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        if(fileid !=-1)
            H5Fclose(fileid);

       string s = "unknown exception caught building HDF5 DDS";
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }

}
#endif

// Build DDS
bool HDF5RequestHandler::hdf5_build_dds(BESDataHandlerInterface & dhi)
{

    // Obtain the HDF5 file name.
    string filename = dhi.container->access();
 
    string container_name = dhi.container->get_symbolic_name();
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDDSResponse *bdds = dynamic_cast < BESDDSResponse * >(response);
    if( !bdds )
        throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;
    bdds->set_container(container_name);

    try {

        bool dds_from_dc = false;
        bool das_from_dc = false;
        bool build_data  = false;
        string dds_cache_fname,das_cache_fname;

        if(_use_disk_meta_cache == true) {

            string base_filename   =  HDF5CFUtil::obtain_string_after_lastslash(filename);
            if(_use_disk_dds_cache == true) {
                dds_cache_fname = _disk_meta_cache_path+"/" +base_filename+"_dds";
                if(access(dds_cache_fname.c_str(),F_OK) !=-1)
                    dds_from_dc = true;
            }

            das_cache_fname = _disk_meta_cache_path+"/" +base_filename+"_das";
            if(access(das_cache_fname.c_str(),F_OK) !=-1)
               das_from_dc = true;

        }

        get_dds_with_attributes(bdds, NULL,container_name,filename, dds_cache_fname,das_cache_fname,dds_from_dc,das_from_dc,build_data);

        // The following block reads dds from a dds cache file.   
#if 0
        string base_filename =  HDF5CFUtil::obtain_string_after_lastslash(filename);
        string dds_filename = "/tmp/"+base_filename+"_dds";

        BaseTypeFactory tf;
        DDS tdds(&tf,name_path(filename),"3.2");
        tdds.filename(filename);


        FILE *dds_file = fopen(dds_filename.c_str(),"r");
        tdds.parse(dds_file);
//cerr<<"before parsing "<<endl;
        DDS* cache_dds = new DDS(tdds);
        if(dds != NULL)
            delete dds;
        bdds->set_dds(cache_dds);
        fclose(dds_file);
#endif

        bdds->set_constraint( dhi ) ;
        bdds->clear_container() ;
    
    }
    catch(InternalErr & e) {

        throw BESDapError(e.get_error_message(), true, e.get_error_code(),
                       __FILE__, __LINE__);
    }
    catch(Error & e) {

        throw BESDapError(e.get_error_message(), false, e.get_error_code(),
                       __FILE__, __LINE__);
    }
    catch (BESError &e){
        throw;
    }
    catch(...) {

       string s = "unknown exception caught building HDF5 DDS";
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }

    return true;
}

bool HDF5RequestHandler::hdf5_build_data(BESDataHandlerInterface & dhi)
{

    if(true ==_usecf) { 
       
        if(true == _pass_fileid)
            return hdf5_build_data_with_IDs(dhi);
    }

    string filename = dhi.container->access();

    string container_name = dhi.container->get_symbolic_name();
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds = dynamic_cast < BESDataDDSResponse * >(response);
    if( !bdds )
        throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;
    bdds->set_container(container_name);

    try {

        bool dds_from_dc = false;
        bool das_from_dc = false;
        bool build_data  = true;
        string dds_cache_fname,das_cache_fname;

        if(_use_disk_meta_cache == true) {

            string base_filename   =  HDF5CFUtil::obtain_string_after_lastslash(filename);
            das_cache_fname = _disk_meta_cache_path+"/" +base_filename+"_das";

            if(access(das_cache_fname.c_str(),F_OK) !=-1)
               das_from_dc = true;

        }

        get_dds_with_attributes(NULL,bdds, container_name,filename, dds_cache_fname,das_cache_fname,dds_from_dc,das_from_dc,build_data);

        // The following block reads dds from a dds cache file.   
#if 0
        string base_filename =  HDF5CFUtil::obtain_string_after_lastslash(filename);
        string dds_filename = "/tmp/"+base_filename+"_dds";

        BaseTypeFactory tf;
        DDS tdds(&tf,name_path(filename),"3.2");
        tdds.filename(filename);


        FILE *dds_file = fopen(dds_filename.c_str(),"r");
        tdds.parse(dds_file);
//cerr<<"before parsing "<<endl;
        DDS* cache_dds = new DDS(tdds);
        if(dds != NULL)
            delete dds;
        bdds->set_dds(cache_dds);
        fclose(dds_file);
#endif

        bdds->set_constraint( dhi ) ;
        bdds->clear_container() ;
    
    }
    catch(InternalErr & e) {

        throw BESDapError(e.get_error_message(), true, e.get_error_code(),
                       __FILE__, __LINE__);
    }
    catch(Error & e) {

        throw BESDapError(e.get_error_message(), false, e.get_error_code(),
                       __FILE__, __LINE__);
    }
    catch (BESError &e){
        throw;
    }
    catch(...) {

       string s = "unknown exception caught building HDF5 DDS";
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }


#if 0
    // For the time being, separate CF file ID from the default file ID(mainly for debugging)
    hid_t fileid    = -1;
    hid_t cf_fileid = -1;

    BESDEBUG("h5","Building DataDDS without passing file IDs. "<<endl);
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds = dynamic_cast < BESDataDDSResponse * >(response);
    if( !bdds )
        throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    try {

        string container_name = bdds->get_explicit_containers() ? dhi.container->get_symbolic_name(): "";
        
        bdds->set_container(container_name);
        DDS* dds = bdds->get_dds();

        // Build a DDS in the empty DDS object
        // COMMENT OUT
        //get_dds_with_attributes(dhi.container->access(), container_name, dds);
        bdds->set_constraint( dhi ) ;
        bdds->clear_container() ;


    }
    catch(InternalErr & e) {

        throw BESDapError(e.get_error_message(), true, e.get_error_code(),
                       __FILE__, __LINE__);
    }
    catch(Error & e) {

       throw BESDapError(e.get_error_message(), false, e.get_error_code(),
                       __FILE__, __LINE__);
    }
    catch (BESError &e){
        throw;
    }
    catch(...) {

        string s = "unknown exception caught building HDF5 DataDDS";
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }
#endif

    return true;
}

// Obtain data when turning on the pass fileID key.The memory cache is not used.
bool HDF5RequestHandler::hdf5_build_data_with_IDs(BESDataHandlerInterface & dhi)
{

    BESDEBUG("h5","Building DataDDS by passing file IDs. "<<endl);
    hid_t cf_fileid = -1;

    string filename = dhi.container->access();
    
    H5Eset_auto2(H5E_DEFAULT,NULL,NULL);
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
    BESDataDDSResponse *bdds = dynamic_cast < BESDataDDSResponse * >(response);
    if( !bdds )
        throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    try {

        bdds->set_container( dhi.container->get_symbolic_name() ) ;

        HDF5DDS *hdds = new HDF5DDS(bdds->get_dds());
        delete bdds->get_dds();

        bdds->set_dds(hdds);
        
        hdds->setHDF5Dataset(cf_fileid);

        read_cfdds( *hdds,filename,cf_fileid);

        if (!hdds->check_semantics()) {   // DDS didn't comply with the DAP semantics 
            hdds->print(cerr);
            throw InternalErr(__FILE__, __LINE__,
                              "DDS check_semantics() failed. This can happen when duplicate variable names are defined.");
        }
        
        Ancillary::read_ancillary_dds( *hdds, filename ) ;

        DAS *das = new DAS ;
        BESDASResponse bdas( das ) ;
        bdas.set_container( dhi.container->get_symbolic_name() ) ;
        read_cfdas( *das,filename,cf_fileid);
        Ancillary::read_ancillary_das( *das, filename ) ;

        hdds->transfer_attributes(das);
        bdds->set_constraint( dhi ) ;
        bdds->clear_container() ;

    }

    catch(InternalErr & e) {
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        throw BESDapError(e.get_error_message(), true, e.get_error_code(),
                       __FILE__, __LINE__);
    }
    catch(Error & e) {
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        throw BESDapError(e.get_error_message(), false, e.get_error_code(),
                       __FILE__, __LINE__);
    }
    catch (BESError &e){
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        throw;
    }
    catch(...) {
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        string s = "unknown exception caught building HDF5 DataDDS";
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }

    return true;
}

bool HDF5RequestHandler::hdf5_build_dmr(BESDataHandlerInterface & dhi)
{


    // Extract the DMR Response object - this holds the DMR used by the
    // other parts of the framework.
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDMRResponse &bes_dmr = dynamic_cast<BESDMRResponse &>(*response);

    string filename = dhi.container->access();

    DMR *dmr = bes_dmr.get_dmr();

    // For the time being, separate CF file ID from the default file ID(mainly for debugging)
    hid_t fileid = -1;
    hid_t cf_fileid = -1;
 
    try {

        DMR* cached_dmr_ptr = 0;
        if (dmr_cache && (cached_dmr_ptr = static_cast<DMR*>(dmr_cache->get(filename)))) {
            // copy the cached DMR into the BES response object
            BESDEBUG(HDF5_NAME, "DMR Cached hit for : " << filename << endl);
            *dmr = *cached_dmr_ptr; // Copy the referenced object
        }
        else {// No cache

            H5Eset_auto2(H5E_DEFAULT,NULL,NULL);
            D4BaseTypeFactory MyD4TypeFactory;
            dmr->set_factory(&MyD4TypeFactory);
 
            if(true ==_usecf) {// CF option
       
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

       	    }// if(true == _usecf)	
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

            	bool use_dimscale = check_dimscale(fileid);
            	dmr->set_name(name_path(filename));
            	dmr->set_filename(name_path(filename));

            	D4Group* root_grp = dmr->root();
            	breadth_first(fileid,(char*)"/",root_grp,filename.c_str(),use_dimscale);

#if 0
           if(true == use_dimscale) 
                //breadth_first(fileid,(char*)"/",*dmr,root_grp,filename.c_str(),true);
                breadth_first(fileid,(char*)"/",root_grp,filename.c_str(),true);
           else 
                depth_first(fileid,(char*)"/",root_grp,filename.c_str());
                //depth_first(fileid,(char*)"/",*dmr,root_grp,filename.c_str());
#endif

           	close_fileid(fileid);

            }// else (default option)

            // If the cache is turned on, add the memory cache.
            if (dmr_cache) {
                // add a copy
                BESDEBUG(HDF5_NAME, "DMR added to the cache for : " << filename << endl);
                dmr_cache->add(new DMR(*dmr), filename);
            }
        }// else no cache
    }// try

    catch(InternalErr & e) {

        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        if(fileid !=-1)
            H5Fclose(fileid);

        throw BESDapError(e.get_error_message(), true, e.get_error_code(),
                       __FILE__, __LINE__);
    }
    catch(Error & e) {

        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        if(fileid !=-1)
            H5Fclose(fileid);
        throw BESDapError(e.get_error_message(), false, e.get_error_code(),
                       __FILE__, __LINE__);
    }
    catch (BESError &e){
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        if(fileid !=-1)
            H5Fclose(fileid);
        throw;
    }
    catch(...) {

        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        if(fileid !=-1)
            H5Fclose(fileid);
        string s = "unknown exception caught building HDF5 DMR";
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }

    //dmr->print(cout);

    // Instead of fiddling with the internal storage of the DHI object,
    // (by setting dhi.data[DAP4_CONSTRAINT], etc., directly) use these
    // methods to set the constraints. But, why? Ans: from Patrick is that
    // in the 'container' mode of BES each container can have a different
    // CE.
    bes_dmr.set_dap4_constraint(dhi);
    bes_dmr.set_dap4_function(dhi);
    dmr->set_factory(0);

    return true;
}

// This function is only used when EnableCF is true.
bool HDF5RequestHandler::hdf5_build_dmr_with_IDs(BESDataHandlerInterface & dhi)
{

    BESDEBUG("h5","Building DMR with passing file IDs. "<<endl);
    string filename = dhi.container->access();
    hid_t cf_fileid = -1;

    H5Eset_auto2(H5E_DEFAULT,NULL,NULL);
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

        ////Don't close the file ID,it will be closed by derived class.
        //if(cf_fileid !=-1)
         //   H5Fclose(cf_fileid);

    }
    catch(InternalErr & e) {

        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);

        throw BESDapError(e.get_error_message(), true, e.get_error_code(),
                       __FILE__, __LINE__);
    }
    catch(Error & e) {

        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);

        throw BESDapError(e.get_error_message(), false, e.get_error_code(),
                       __FILE__, __LINE__);
    }
    catch (BESError &e){
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);
        throw;
    }
    catch(...) {

        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);

        string s = "unknown exception caught building HDF5 DataDDS";
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }

    //dds.print(cout);
    //dds.print_das(cout);
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

    HDF5DMR *hdf5_dmr = new HDF5DMR(dmr);
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
    hdf5_dmr->set_factory(0);

    return true;
}

bool HDF5RequestHandler::hdf5_build_help(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESInfo *info = dynamic_cast<BESInfo *>(response);
    if( !info )
        throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    map<string,string> attrs ;
    attrs["name"] = MODULE_NAME ;
    attrs["version"] = MODULE_VERSION ;
    list<string> services ;
    BESServiceRegistry::TheRegistry()->services_handled( HDF5_NAME, services );
    if( services.size() > 0 )
        {
            string handles = BESUtil::implode( services, ',' ) ;
            attrs["handles"] = handles ;
        }
    info->begin_tag( "module", &attrs ) ;
    info->end_tag( "module" ) ;

    return true;
}

bool HDF5RequestHandler::hdf5_build_version(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESVersionInfo *info = dynamic_cast < BESVersionInfo * >(response);
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
    if(lrd_config_fpath=="" || lrd_config_fname=="")
        return false;

    // temp_line for storing info of one line in the config. file 
    string temp_line;

    // The full path of the configure file
    string mcache_config_fname = lrd_config_fpath+"/"+lrd_config_fname;
    
    //ifstream mcache_config_file("example.txt");
    // Open the configure file
    ifstream mcache_config_file(mcache_config_fname.c_str());

    // If the configuration file is not open, return false.
    if(mcache_config_file.is_open()==false){
        BESDEBUG(HDF5_NAME,"The large data memory cache configure file "<<mcache_config_fname );
        BESDEBUG(HDF5_NAME," cannot be opened."<<endl);
        return false;
    }

    // Read the configuration file line by line
    while(getline(mcache_config_file,temp_line)) {

        // Only consider lines that is no less than 2 characters and the 2nd character is space.
        if(temp_line.size()>1 && temp_line.at(1)==' ') {
            char sep=' ';
            string subline = temp_line.substr(2);
            vector<string> temp_name_list;

            // Include directories to store common latitude and longitude values
            if(temp_line.at(0)=='1') {
                HDF5CFUtil::Split_helper(temp_name_list,subline,sep);
                //lrd_cache_dir_list +=temp_name_list;
                lrd_cache_dir_list.insert(lrd_cache_dir_list.end(),temp_name_list.begin(),temp_name_list.end());
            }
            // Include directories not to store common latitude and longitude values
            else if(temp_line.at(0)=='0'){
                HDF5CFUtil::Split_helper(temp_name_list,subline,sep);
                //lrd_non_cache_dir_list +=temp_name_list;
                lrd_non_cache_dir_list.insert(lrd_non_cache_dir_list.end(),temp_name_list.begin(),temp_name_list.end());
            }
            // Include variable names that the server would like to store in the memory cache
            else if(temp_line.at(0)=='2') {
                
                // We need to handle the space case inside a variable path
                // either "" or '' needs to be used to identify a var path
                vector<int>dq_pos;
                vector<int>sq_pos;
                for(int i = 0; i<subline.size();i++){
                    if(subline[i]=='"') {
                        dq_pos.push_back(i);
                    }
                    else if(subline[i]=='\'')
                        sq_pos.push_back(i);
                }
                if(dq_pos.size()==0 && sq_pos.size()==0)
                    HDF5CFUtil::Split_helper(temp_name_list,subline,sep);
                else if((dq_pos.size()!=0) &&(dq_pos.size()%2==0)&& sq_pos.size()==0) {
                    int  dq_index= 0;
                    while(dq_index < dq_pos.size()){
                        if(dq_pos[dq_index+1]>(dq_pos[dq_index]+1)) {
                            temp_name_list.push_back
                            (subline.substr(dq_pos[dq_index]+1,dq_pos[dq_index+1]-dq_pos[dq_index]-1));
                        }
                        dq_index=dq_index + 2;
                    }
                }
                else if((sq_pos.size()!=0) &&(sq_pos.size()%2==0)&& dq_pos.size()==0) {
                    int  sq_index= 0;
                    while(sq_index < sq_pos.size()){
                        if(sq_pos[sq_index+1]>(sq_pos[sq_index]+1)) {
                            temp_name_list.push_back
                            (subline.substr(sq_pos[sq_index]+1,sq_pos[sq_index+1]-sq_pos[sq_index]-1));
                        }
                        sq_index=sq_index+2;
                    }
                }

                //lrd_var_cache_file_list +=temp_name_list;
                lrd_var_cache_file_list.insert(lrd_var_cache_file_list.end(),temp_name_list.begin(),temp_name_list.end());
            }
        }
    }


#if 0

for(int i =0; i<lrd_cache_dir_list.size();i++)
cerr<<"lrd cache list is "<<lrd_cache_dir_list[i] <<endl;
for(int i =0; i<lrd_non_cache_dir_list.size();i++)
cerr<<"lrd non cache list is "<<lrd_non_cache_dir_list[i] <<endl;
for(int i =0; i<lrd_var_cache_file_list.size();i++)
cerr<<"lrd var cache file list is "<<lrd_var_cache_file_list[i] <<endl;
#endif


    mcache_config_file.close();
    if(lrd_cache_dir_list.size()==0 && lrd_non_cache_dir_list.size()==0 && lrd_var_cache_file_list.size()==0)
        return false;
    else 
        return true;
}



bool HDF5RequestHandler::read_das_from_disk_cache(const string & cache_filename,DAS *das_ptr) {

    BESDEBUG(HDF5_NAME, "Coming to read_das_from_disk_cache() " << cache_filename << endl);
    bool ret_value = true;
    FILE *md_file = NULL;
    md_file = fopen(cache_filename.c_str(),"rb");

    if(NULL == md_file) {
            string bes_error = "An error occurred trying to open a metadata cache file  " + cache_filename;
            throw BESInternalError( bes_error, __FILE__, __LINE__);
    }
    else {
        
        int fd_md = fileno(md_file);
        struct flock *l_md;
        l_md = lock(F_RDLCK);

        // hold a read(shared) lock to read metadata from a file.
        if(fcntl(fd_md,F_SETLKW,l_md) == -1) {
            fclose(md_file);
            ostringstream oss;
            oss << "cache process: " << l_md->l_pid << " triggered a locking error: " << get_errno();
            throw BESInternalError( oss.str(), __FILE__, __LINE__);
        }

        try {
            das_ptr->parse(md_file);
        }
        catch(...) {
            if(fcntl(fd_md,F_SETLK,lock(F_UNLCK)) == -1) { 
                fclose(md_file);
                throw BESInternalError( "An error occurred trying to unlock the file" + get_errno(), __FILE__, __LINE__);
            }

            fclose(md_file);
            throw InternalErr(__FILE__,__LINE__,"Fail to parse a das cache file.");
        }
    }
    return ret_value;
 

}

bool HDF5RequestHandler::write_dds_to_disk_cache(const string& dds_cache_fname,DDS *dds_ptr) {

    BESDEBUG(HDF5_NAME, "Write DDS to disk cache " << dds_cache_fname << endl);
    FILE *dds_file = fopen(dds_cache_fname.c_str(),"w");
    dds_ptr->print(dds_file);
    fclose(dds_file);

}

bool HDF5RequestHandler::write_das_to_disk_cache(const string & das_cache_fname, DAS *das_ptr) {

    BESDEBUG(HDF5_NAME, "Write DAS to disk cache " << das_cache_fname << endl);
    FILE *das_file = fopen(das_cache_fname.c_str(),"w");
    das_ptr->print(das_file);
    fclose(das_file);

}



void HDF5RequestHandler::read_dds_from_disk_cache(BESDDSResponse* bdds, BESDataDDSResponse* data_bdds,bool build_data,const string & container_name,const string & h5_fname,
                              const string & dds_cache_fname,const string &das_cache_fname, hid_t h5_fd, bool das_from_dc) {

     
     BESDEBUG(HDF5_NAME, "Read DDS from disk cache " << dds_cache_fname << endl);

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
//cerr<<"before parsing "<<endl;
     tdds.parse(dds_file);
//cerr<<"after parsing "<<endl;
     DDS* cache_dds = new DDS(tdds);
#if 0
cerr<<"before dds "<<endl;
dds->dump(cerr);
cerr<<"after dds "<<endl;
cerr<<"before tdds "<<endl;
cache_dds->dump(cerr);
cerr<<"after tdds "<<endl;
#endif
     if(dds != NULL)
        delete dds;

     Ancillary::read_ancillary_dds( *cache_dds, h5_fname ) ;

     add_das_to_dds(cache_dds,container_name,h5_fname,das_cache_fname,h5_fd,das_from_dc);
cache_dds->print(cerr);
     if(true == build_data)
        data_bdds->set_dds(cache_dds);
     else 
        bdds->set_dds(cache_dds);
     fclose(dds_file);

    if (dds_cache) {
        // add a copy
        BESDEBUG(HDF5_NAME, "Reading DDS from Disk Cache routine, For memory cache, DDS added to the cache for : " << h5_fname << endl);
        dds_cache->add(new DDS(*cache_dds), h5_fname);
    }

}

void HDF5RequestHandler::add_das_to_dds(DDS *dds,const string &container_name, const string &filename, const string &das_cache_fname,hid_t h5_fd, bool das_from_dc) {

    BESDEBUG(HDF5_NAME, "Coming to add_das_to_dds() "  << endl);
    // Check DAS cache
    DAS *das = 0 ;
    if (das_cache && (das = static_cast<DAS*>(das_cache->get(filename)))) {
        BESDEBUG(HDF5_NAME, "DAS Cached hit for : " << filename << endl);
        dds->transfer_attributes(das); // no need to copy the cached DAS
    }

    else {

        das = new DAS ;
#if 0
        if (!container_name.empty())
            das->container_name(container_name);
#endif
        if(das_from_dc == true) 
            read_das_from_disk_cache(das_cache_fname,das);       
        else {
            bool h5_file_open = true;
            if(h5_fd == -1) 
                h5_file_open = false;
            if (true == _usecf) {
                    // go to the CF option
                    if(h5_file_open == false) 
                        h5_fd = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

                    read_cfdas( *das,filename,h5_fd);
                    if(h5_file_open == false) 
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

            if(das_cache_fname!="" && das_from_dc == false)
                write_das_to_disk_cache(das_cache_fname,das);
        }

        dds->transfer_attributes(das);

        // Only free the DAS if it's not added to the cache
        if (das_cache) {
                    // add a copy
            BESDEBUG(HDF5_NAME, "Reading DDS from Disk Cache routine, For memory cache, DAS added to the cache for : " << filename << endl);
                    //das_cache->add(new DAS(*das), filename);
            das_cache->add(das, filename);
        }
        else {
            delete das;
        }

    }
    
}

bool check_beskeys(const string key) {

    bool found = false;
    string doset ="";
    const string dosettrue ="true";
    const string dosetyes = "yes";

    TheBESKeys::TheKeys()->get_value( key, doset, found ) ;
    if( true == found ) {
        doset = BESUtil::lowercase( doset ) ;
        if( dosettrue == doset  || dosetyes == doset )
            return true;
    }
    return false;

}

// get_uint_key and get_float_key are copied from the netCDF handler. 

static unsigned int get_uint_key(const string &key, unsigned int def_val)
{
    bool found = false;
    string doset = "";

    TheBESKeys::TheKeys()->get_value(key, doset, found);
    if (true == found) {
        return atoi(doset.c_str()); // use better code TODO
    }
    else {
        return def_val;
    }
}

static float get_float_key(const string &key, float def_val)
{
    bool found = false;
    string doset = "";

    TheBESKeys::TheKeys()->get_value(key, doset, found);
    if (true == found) {
        return atof(doset.c_str()); // use better code TODO
    }
    else {
        return def_val;
    }
}

static string get_beskeys(const string &key) {

    bool found = false;
    string ret_value ="";

    TheBESKeys::TheKeys()->get_value( key, ret_value, found ) ;
    return ret_value;

}

