
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

bool check_beskeys(const string);
// For the CF option
extern void read_cfdas(DAS &das, const string & filename,hid_t fileid);
extern void read_cfdds(DDS &dds, const string & filename,hid_t fileid);

// Cache map variables.
// Leave here for future investigation
#if 0
map<string,DAS> HDF5RequestHandler::das_cache;
map<string,DDS> HDF5RequestHandler::dds_cache;
map<string,DataDDS> HDF5RequestHandler::data_dds_cache;
#endif

//bool HDF5RequestHandler::_use_memcache                = false;
//
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
    add_handler(DAS_RESPONSE, HDF5RequestHandler::hdf5_build_das);
    add_handler(DDS_RESPONSE, HDF5RequestHandler::hdf5_build_dds);
    add_handler(DATA_RESPONSE, HDF5RequestHandler::hdf5_build_data);
    add_handler(DMR_RESPONSE, HDF5RequestHandler::hdf5_build_dmr);
    add_handler(DAP4DATA_RESPONSE, HDF5RequestHandler::hdf5_build_dmr);

    add_handler(HELP_RESPONSE, HDF5RequestHandler::hdf5_build_help);
    add_handler(VERS_RESPONSE, HDF5RequestHandler::hdf5_build_version);

    //_use_memcache                = check_beskeys("H5.EnableMemCache");
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

#if 0

if(true == usecf) cerr<<"usecf is true"<<endl;
else cerr<<"usecf is false"<<endl;
if(true == pass_fileid) cerr<<"pass_fileid is true"<<endl;
else cerr<<"pass_fileid is false"<<endl;
if(true == disable_structmeta) cerr<<"disable_structmeta is true"<<endl;
else cerr<<"disable_structmeta is false"<<endl;


if(true == keep_var_leading_underscore) cerr<<"keep_var_leading_underscore is true"<<endl;
else cerr<<"keep_var_leading_underscore is false"<<endl;
if(true == check_name_clashing) cerr<<"check_name_clashing is true"<<endl;
else cerr<<"check_name_clashing is false"<<endl;
if(true == add_path_attrs) cerr<<"add_path_attrs is true"<<endl;
else cerr<<"add_path_attrs is false"<<endl;

if(true == drop_long_string) cerr<<"drop_long_string is true"<<endl;
else cerr<<"drop_long_string is false"<<endl;
if(true == check_ignore_obj) cerr<<"check_ignore_obj is true"<<endl;
else cerr<<"check_ignore_obj is false"<<endl;
#endif

}

HDF5RequestHandler::~HDF5RequestHandler()
{
}

bool HDF5RequestHandler::hdf5_build_das(BESDataHandlerInterface & dhi)
{

#if 0
    bool found_key = false;
    bool usecf = false;
    string key = "H5.EnableCF";
    string doset;
#endif

    // For the time being, separate CF file ID from the default file ID(mainly for debugging)
    hid_t cf_fileid = -1;

#if 0
    TheBESKeys::TheKeys()->get_value( key, doset, found_key ) ;

    if(true ==found_key ) {
        doset = BESUtil::lowercase( doset ) ;
        if( doset == "true" || doset == "yes" ) 
            usecf = true;
    }
#endif
         
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

#if 0
        // Check if the mem_cache is set 
        if(true == _use_memcache) {

            BESDEBUG("h5", "Using the memory DAS cache" << endl);
            map<string, DAS>::iterator das_cache_iter = das_cache.find(filename);
            if (das_cache_iter != das_cache.end()) {
                BESDEBUG("h5", "Found DAS in the memory DAS cache" << endl);
                *das = das_cache_iter->second;
                bdas->clear_container();
                return true;
            }
        }
#endif
        if (true == _usecf) {

            cf_fileid = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
            if (cf_fileid < 0){
                throw BESNotFoundError((string) "Could not open this hdf5 file: "
                               + filename, __FILE__, __LINE__);
            }

            read_cfdas( *das,filename,cf_fileid);
            H5Fclose(cf_fileid);
        }
        else {
               // go to the default option
//cerr<<"default option" <<endl;
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

        //if(true == _use_memcache) 
        //    das_cache[filename] = *das;
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

bool HDF5RequestHandler::hdf5_build_dds(BESDataHandlerInterface & dhi)
{

    // For the time being, separate CF file ID from the default file ID(mainly for debugging)
    hid_t fileid    = -1;
    hid_t cf_fileid = -1;

    // Obtain the HDF5 file name.
    string filename = dhi.container->access();
 
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDDSResponse *bdds = dynamic_cast < BESDDSResponse * >(response);
    if( !bdds )
        throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    try {
        bdds->set_container( dhi.container->get_symbolic_name() ) ;
        DDS *dds = bdds->get_dds();

#if 0
        if(true == _use_memcache) {

            BESDEBUG("h5", "Using the DDS mem_cache" << endl);
            map<string, DDS>::iterator dds_cache_iter = dds_cache.find(filename);
            if (dds_cache_iter != dds_cache.end()) {
                BESDEBUG("h5", "Found DDS in the mem_cache" << endl);
                *dds = dds_cache_iter->second;
                bdds->set_constraint(dhi);
                bdds->clear_container();
                return true;
            }
        }
#endif

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

        DAS *das = new DAS ;
        BESDASResponse bdas( das ) ;
        bdas.set_container( dhi.container->get_symbolic_name() ) ;

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

#if 0
        if(true == _use_memcache) {

            // Use this because DDS doesn't have a no-arg ctor which
            // map::operator[] requires. KY 3/3/16 from JG's comments
            dds_cache.insert(pair<string, DDS>(filename, *dds));

        }
#endif

        bdds->set_constraint( dhi ) ;

        bdds->clear_container() ;
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

        bdds->set_container( dhi.container->get_symbolic_name() ) ;

#if 0
        HDF5DDS *hdds = new HDF5DDS(bdds->get_dds());
        delete bdds->get_dds();

        bdds->set_dds(hdds);
        
        hdds->setHDF5Dataset(cf_fileid);
#endif
        DataDDS* dds = bdds->get_dds();
        dds->filename(filename);

#if 0
        if(true == _use_memcache) {
            BESDEBUG("h5", "Using the DataDDS cache" << endl);
            map<string, DataDDS>::iterator data_dds_cache_iter = data_dds_cache.find(filename);
            if (data_dds_cache_iter != data_dds_cache.end()) {
                BESDEBUG("h5", "Found DataDDS in the DataDDS cache" << endl);
                *dds = data_dds_cache_iter->second;
                bdds->set_constraint(dhi);
                bdds->clear_container();
                return true;
            }
        }
#endif

        if(true == _usecf) {
            cf_fileid = H5Fopen(filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
            if(cf_fileid < 0){
                throw BESNotFoundError((string) "Could not open this hdf5 file: "
                                   + filename, __FILE__, __LINE__);
            }
        }
        else {
            // Obtain the HDF5 file ID. 
            fileid = get_fileid(filename.c_str());
            if (fileid < 0) {
                throw BESNotFoundError(string("hdf5_build_data: ")
                                   + "Could not open hdf5 file: "
                                   + filename, __FILE__, __LINE__);
            }
        }

        // DataDDS *dds = bdds->get_dds();
        if(true == _usecf) { 

            // This is the CF option
            read_cfdds( *dds,filename,cf_fileid);
        }
        else {
            depth_first(fileid, (char*)"/", *dds, filename.c_str());
        }

        if (!dds->check_semantics()) {   // DDS didn't comply with the DAP semantics 
            dds->print(cerr);
            throw InternalErr(__FILE__, __LINE__,
                              "DDS check_semantics() failed. This can happen when duplicate variable names are defined.");
        }
        
        Ancillary::read_ancillary_dds( *dds, filename ) ;

        DAS *das = new DAS ;
        BESDASResponse bdas( das ) ;
        bdas.set_container( dhi.container->get_symbolic_name() ) ;

        if(true == _usecf) {

            // CF option
            read_cfdas( *das,filename,cf_fileid);

        }
        else {
            
            // Obtain the global attributes and map them to DAS
            find_gloattr(fileid, *das);

            // Obtain the rest attributes and map them to DAS
            depth_first(fileid, "/", *das);

            // The HDF5 file id will be closed now!
            close_fileid(fileid);
        }

        Ancillary::read_ancillary_das( *das, filename ) ;

        dds->transfer_attributes(das);
#if 0
        if (true == _use_memcache) {
            // Use this because DDS doesn't have a no-arg ctor which
            // map::operator[] requires. jhrg 2/18/16
            data_dds_cache.insert(pair<string, DataDDS>(filename, *dds));
        }
#endif

        bdds->set_constraint( dhi ) ;
        bdds->clear_container() ;

        ////close the file ID.
        if(cf_fileid !=-1)
            H5Fclose(cf_fileid);

    }
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

        string s = "unknown exception caught building HDF5 DataDDS";
        throw BESInternalFatalError(s, __FILE__, __LINE__);
    }

    return true;
}

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

//#ifdef USE_DAP4
bool HDF5RequestHandler::hdf5_build_dmr(BESDataHandlerInterface & dhi)
{

#if 0
    bool found = false;
    bool usecf = false;

    string key="H5.EnableCF";
    string doset;

    TheBESKeys::TheKeys()->get_value( key, doset, found ) ;
    if(true == found )
    {
        // cerr<<"found it" <<endl;
        doset = BESUtil::lowercase( doset ) ;
        if( doset == "true" || doset == "yes" ) {
            
           // This is the CF option, go to the CF function
            // cerr<<"go to CF option "<<endl;
            usecf = true;
        }
    }    
#endif

    // Extract the DMR Response object - this holds the DMR used by the
    // other parts of the framework.
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDMRResponse &bes_dmr = dynamic_cast<BESDMRResponse &>(*response);

    DMR *dmr = bes_dmr.get_dmr();
    D4BaseTypeFactory MyD4TypeFactory;
    dmr->set_factory(&MyD4TypeFactory);
 
    // For the time being, separate CF file ID from the default file ID(mainly for debugging)
    hid_t fileid = -1;
    hid_t cf_fileid = -1;

    string filename = dhi.container->access();

    try {
        if(true ==_usecf) { 
       
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

        }
        else {

            // Obtain the HDF5 file ID. 
            fileid = get_fileid(filename.c_str());
            if (fileid < 0) {
                throw BESNotFoundError(string("hdf5_build_dmr: ")
                                   + "Could not open hdf5 file: "
                                   + filename, __FILE__, __LINE__);
            }

            bool use_dimscale = check_dimscale(fileid);
//if(true == use_dimscale)
//    cerr<<"this file contains dimension scales."<<endl;
            dmr->set_name(name_path(filename));
            dmr->set_filename(name_path(filename));

            D4Group* root_grp = dmr->root();

           //depth_first(fileid,(char*)"/",root_grp,filename.c_str());
           //depth_first(fileid,(char*)"/",*dmr,root_grp,filename.c_str(),use_dimscale);
           if(true == use_dimscale) 
                //breadth_first(fileid,(char*)"/",*dmr,root_grp,filename.c_str(),use_dimscale);
                breadth_first(fileid,(char*)"/",*dmr,root_grp,filename.c_str(),true);
           else 
                depth_first(fileid,(char*)"/",*dmr,root_grp,filename.c_str());

           close_fileid(fileid);

        }
    }

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
    //dmr->set_factory(new D4BaseTypeFactory);
    dmr->build_using_dds(dds);
    //dmr->print(cout);

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

