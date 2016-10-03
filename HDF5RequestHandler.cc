
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
#include <BESNotFoundError.h>
#include <BESInternalFatalError.h>
#include <TheBESKeys.h>
#include <BESDebug.h>
#include "h5get.h"
#include "config_hdf5.h"

#define HDF5_NAME "h5"
#include "h5cfdaputil.h"

using namespace std;
using namespace libdap;

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
unsigned int HDF5RequestHandler::_mcache_entries = 200;
unsigned int HDF5RequestHandler::_lrdcache_entries = 40;
unsigned int HDF5RequestHandler::_srdcache_entries = 200;
float HDF5RequestHandler::_cache_purge_level = 0.2;

// Metadata object cache at DAS,DDS and DMR.
ObjMemCache *HDF5RequestHandler::das_cache = 0;
ObjMemCache *HDF5RequestHandler::dds_cache = 0;
ObjMemCache *HDF5RequestHandler::dmr_cache = 0;

ObjMemCache *HDF5RequestHandler::lrddata_mem_cache = 0;
ObjMemCache *HDF5RequestHandler::srddata_mem_cache = 0;

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

    if(get_usecf()) {
        if(get_lrdcache_entries()) {
            lrdata_mem_cache = new ObjMemCache(get_lrdcache_entries(), get_cache_purge_level());
            if(true == check_beskeys("H5.LargeDataMemCacheConfig")) {
                obtain_lrd_common_cache_dirs();

            }
        }
        if(get_srdcache_entries()) {
            srdata_mem_cahce = new ObjMemCache(get_srdcache_entries(),get_cache_purge_level());

        }
    }


    BESDEBUG(HDF5_NAME, "Exiting HDF5RequestHandler::HDF5RequestHandler" << endl);
}

HDF5RequestHandler::~HDF5RequestHandler()
{
    
    // delete the cache.
    delete das_cache;
    delete dds_cache;
    delete dmr_cache;
    delete data_mem_cache;
     
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

            if (true == _usecf) {

                cf_fileid = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
                if (cf_fileid < 0){
                    throw BESNotFoundError((string) "Could not open this hdf5 file: "
                                   + filename, __FILE__, __LINE__);
                }

                read_cfdas( *das,filename,cf_fileid);
                H5Fclose(cf_fileid);
            }
            else {// Default option
                hid_t fileid = get_fileid(filename.c_str());
                if (fileid < 0) {
                    throw BESNotFoundError((string) "Could not open this hdf5 file: "
                                   + filename, __FILE__, __LINE__);
                }

                find_gloattr(fileid, *das);
                depth_first(fileid, "/", *das);
                close_fileid(fileid);
            }
             
            Ancillary::read_ancillary_das( *das, filename ) ;

            // If the cache is turned on
            if(das_cache) {
                // add a copy
                BESDEBUG(HDF5_NAME, "DAS added to the cache for : " << filename << endl);
                das_cache->add(new DAS(*das), filename);
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

            if (!container_name.empty()) 
                dds->container_name(container_name);
            dds->filename(filename);

            // For the time being, not mess up CF's fileID with Default's fileID
            if(true == _usecf) {
           
                cf_fileid = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
                if (cf_fileid < 0){
                    throw BESNotFoundError((string) "Could not open this hdf5 file: "
                                   + filename, __FILE__, __LINE__);
                }
                read_cfdds(*dds,filename,cf_fileid);

            }
            else {

                fileid = get_fileid(filename.c_str());
                if (fileid < 0) {
                    throw BESNotFoundError(string("hdf5_build_dds: ")
                                           + "Could not open hdf5 file: "
                                           + filename, __FILE__, __LINE__);
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

// Build DDS
bool HDF5RequestHandler::hdf5_build_dds(BESDataHandlerInterface & dhi)
{

    // Obtain the HDF5 file name.
    string filename = dhi.container->access();
 
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDDSResponse *bdds = dynamic_cast < BESDDSResponse * >(response);
    if( !bdds )
        throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    try {

        string container_name = bdds->get_explicit_containers() ? dhi.container->get_symbolic_name(): "";
        DDS *dds = bdds->get_dds();

        // Build a DDS in the empty DDS object
        get_dds_with_attributes(dhi.container->access(), container_name, dds);

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
        DDS* dds = bdds->get_dds();

        // Build a DDS in the empty DDS object
        get_dds_with_attributes(dhi.container->access(), container_name, dds);
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

    return true;
}

// Obtain data when turning on the pass fileID key.The memory cache is not used.
bool HDF5RequestHandler::hdf5_build_data_with_IDs(BESDataHandlerInterface & dhi)
{

    BESDEBUG("h5","Building DataDDS by passing file IDs. "<<endl);
    hid_t cf_fileid = -1;

    string filename = dhi.container->access();

    cf_fileid = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (cf_fileid < 0){
        throw BESNotFoundError((string) "Could not open this hdf5 file: "
                               + filename, __FILE__, __LINE__);
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

            D4BaseTypeFactory MyD4TypeFactory;
            dmr->set_factory(&MyD4TypeFactory);
 
            if(true ==_usecf) {// CF option
       
                if(true == _pass_fileid)
                    return hdf5_build_dmr_with_IDs(dhi);

                cf_fileid = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
                if (cf_fileid < 0){
                	throw BESNotFoundError((string) "Could not open this hdf5 file: "
                    	               + filename, __FILE__, __LINE__);
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
                    throw BESNotFoundError(string("hdf5_build_dmr: ")
                       	                   + "Could not open hdf5 file: "
                               	           + filename, __FILE__, __LINE__);
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

    cf_fileid = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (cf_fileid < 0){
         throw BESNotFoundError((string) "Could not open this hdf5 file: "
                               + filename, __FILE__, __LINE__);
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

static bool HDF5RequestHandler::obtain_lrd_common_cache_dirs() 
{

    //bool ret_value = false;
    string lrd_config_fpath;
    string lrd_config_fname;
    lrd_config_fpath = get_beskeys("H5.DataCachePath");
    lrd_config_fname = get_beskeys("H5.LargeDataMemCacheFileName");
    if(lrd_config_fpath=="" ||
       lrd_config_fname=="")
        return false;

    // The following code is borrowed from HDF-EOS5 augmentation tool
  /* Open the mapping file */
  fp = fopen(dimgeonames, "r");
  if(fp==NULL) {
    printf("Choose the file option but the dimension mapping file doesn't exist. \n");
    return -2;
  }

  /* Assume the number of characters in  each line doesn't exceed 1024.*/

  char mystring[1024];

  while(fgets(mystring, 1024, fp)!=NULL) {/* while((s = getline(&line, &n, fp)) != -1)  */

    line = mystring;

    if(line[0]=='#') /* comment */
      continue;

    if(strlen(line) <= 1) /* empty line */
      continue;

    /* trim out special characters such as space, tab or new line character*/
    char *token = trim(strtok_r(line, " \t\n\r", &saveptr));

    flag = atoi(token);   	
            dimname = trim(strtok_r(/*line*/NULL, " \t\n\r", &saveptr));
        geoname = trim(strtok_r(NULL, " \t\r\n", &saveptr));

    


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

