
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// HDF5RequestHandler.cc
/// \file HDF5RequestHandler.cc
/// \brief The implementation of the entry functions to execute the handlers
///
/// It includes build_das, build_dds and build_data etc.
/// \author James Gallagher <jgallagher@opendap.org>


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "hdf5_handler.h"
#include "HDF5RequestHandler.h"

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
#include "h5get.h"
#include "config_hdf5.h"
//#include "h5cfdap.h"

#define HDF5_NAME "h5"
using namespace std;
using namespace libdap;
#include <string>

// For the CF option
extern void read_cfdas(DAS &das, const string & filename);
 extern void read_cfdds(DDS &dds, const string & filename);


HDF5RequestHandler::HDF5RequestHandler(const string & name)
    :BESRequestHandler(name)
{
    add_handler(DAS_RESPONSE, HDF5RequestHandler::hdf5_build_das);
    add_handler(DDS_RESPONSE, HDF5RequestHandler::hdf5_build_dds);
    add_handler(DATA_RESPONSE, HDF5RequestHandler::hdf5_build_data);
    add_handler(HELP_RESPONSE, HDF5RequestHandler::hdf5_build_help);
    add_handler(VERS_RESPONSE, HDF5RequestHandler::hdf5_build_version);

}

HDF5RequestHandler::~HDF5RequestHandler()
{
}

bool HDF5RequestHandler::hdf5_build_das(BESDataHandlerInterface & dhi)
{
    // Obtain the HDF5 file name.
    string filename = dhi.container->access();

    // Obtain the BES object from the client
    BESResponseObject *response = dhi.response_handler->get_response_object() ;

    // Convert to the BES DAS response
    BESDASResponse *bdas = dynamic_cast < BESDASResponse * >(response) ;
    if( !bdas )
        throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    try {
//cerr<<"coming to build das "<<endl;
        bdas->set_container( dhi.container->get_symbolic_name() ) ;
        DAS *das = bdas->get_das();

        // Use the BES TheKeys to separate CF option from the default option

        bool found = false; // If not found, keep it for the default option
        string key = "H5.EnableCF";
        string doset;

        TheBESKeys::TheKeys()->get_value( key, doset, found ) ;
        if( found ) {
            // cerr<<"found it" <<endl;
            doset = BESUtil::lowercase( doset ) ;
            if( doset == "true" || doset == "yes" ) {
                // This is the CF option, go to the CF function
                // cerr<<"go to CF option "<<endl;
                read_cfdas( *das,filename);
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
        }
        else {// Not even find the key, will go to the default option
//cerr<<"default option" <<endl;
            hid_t fileid = get_fileid(filename.c_str());
            if (fileid < 0) {
                throw BESNotFoundError((string) "Could not open hdf file: "
                               + filename, __FILE__, __LINE__);
            }

            find_gloattr(fileid, *das);
            depth_first(fileid, "/", *das);
            close_fileid(fileid);
        }
             
        Ancillary::read_ancillary_das( *das, filename ) ;
        bdas->clear_container() ;
    }
    catch(InternalErr & e) {
    	// TODO You can collapse this to 'throw BESDapError(e.get_er ...);'
    	// Not a big deal...
        BESDapError ex(e.get_error_message(), true, e.get_error_code(),
                       __FILE__, __LINE__);
        throw ex;
    }
    catch(Error & e) {
        BESDapError ex(e.get_error_message(), false, e.get_error_code(),
                       __FILE__, __LINE__);
        throw ex;
    }
    catch(...) {
        string s = "unknown exception caught building HDF5 DAS";
        BESInternalFatalError ex(s, __FILE__, __LINE__);
        throw ex;
    }

    return true;
}

bool HDF5RequestHandler::hdf5_build_dds(BESDataHandlerInterface & dhi)
{

    bool found = false;
    bool usecf = false;

    string key="H5.EnableCF";
    string doset;

    // Obtain the BESKeys. If finding the key, "found" will be true.
    TheBESKeys::TheKeys()->get_value( key, doset, found ) ;

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
        // Set the filename/dataset name for the DDS to be the hdf5 filename.
        // The dataset name is showing up as 'virtual' which is the default
        // set by the bes. I think we probably want the real filename.
        // Added jhrg 12/28/12
        dds->set_dataset_name(dhi.container->get_symbolic_name());
#endif
        if( found ) {
            // cerr<<"found it" <<endl;

            // Obtain the value of the key.
            doset = BESUtil::lowercase( doset ) ;

            if( doset == "true" || doset == "yes" ) {
            //  This is the CF option, go to the CF function
                // cerr<<"go to CF option "<<endl;
                read_cfdds(*dds,filename);
                usecf = true;
           }
        }    

        hid_t fileid = -1;

        if(false == usecf) {

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

        if(false == usecf){ 

            find_gloattr(fileid, *das);
            depth_first(fileid, "/", *das);
            close_fileid(fileid);

        // WARNING: I add this line because for the default option, the handler will hold
        // the HDF5 object handles that are generated when handling DDS. If one only
        // wants to run DDS, I assume that those resources will be lost and may cause the
        // memory leaking..
        // So adding H5close() will release all the resources HDF5 is using.
        // Should ask James to see if this makes sense. KY-2011-11-17
        // 
            H5close();
        }
        else {
           // go to the CF option
            read_cfdas( *das,filename);
        }

        Ancillary::read_ancillary_das( *das, filename ) ;

        dds->transfer_attributes(das);

        bdds->set_constraint( dhi ) ;

        bdds->clear_container() ;
    }
    catch(InternalErr & e) {
        BESDapError ex(e.get_error_message(), true, e.get_error_code(),
                       __FILE__, __LINE__);
        throw ex;
    }
    catch(Error & e) {
        BESDapError ex(e.get_error_message(), false, e.get_error_code(),
                       __FILE__, __LINE__);
        throw ex;
    }
    catch(...) {
        string s = "unknown exception caught building HDF5 DDS";
        BESInternalFatalError ex(s, __FILE__, __LINE__);
        throw ex;
    }

    return true;
}

bool HDF5RequestHandler::hdf5_build_data(BESDataHandlerInterface & dhi)
{

    bool found = false;
    bool usecf = false;

    string key="H5.EnableCF";
    string doset;

    TheBESKeys::TheKeys()->get_value( key, doset, found ) ;
    if( found )
    {
        // cerr<<"found it" <<endl;
        doset = BESUtil::lowercase( doset ) ;
        if( doset == "true" || doset == "yes" ) {
            
           // This is the CF option, go to the CF function
            // cerr<<"go to CF option "<<endl;
            usecf = true;
        }
    }    

    string filename = dhi.container->access();
    hid_t fileid = -1;

    if(!usecf) { 
 
        // Obtain the HDF5 file ID. 
        fileid = get_fileid(filename.c_str());
        if (fileid < 0) {
            throw BESNotFoundError(string("hdf5_build_data: ")
                               + "Could not open hdf5 file: "
                               + filename, __FILE__, __LINE__);
        }
    }

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds = dynamic_cast < BESDataDDSResponse * >(response);
    if( !bdds )
        throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    try {

        bdds->set_container( dhi.container->get_symbolic_name() ) ;
        DataDDS *dds = bdds->get_dds();
#if 0
        //Added jhrg 12/28/12 See above
        dds->set_dataset_name(dhi.container->get_symbolic_name());
#endif
        if(!usecf) { // This is the default option
            depth_first(fileid, (char*)"/", *dds, filename.c_str());
        }
        else { // This is the CF option
            // Will add this later.
            read_cfdds( *dds,filename);
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

        if(!usecf) {// The default option
            
            // Obtain the global attributes and map them to DAS
            find_gloattr(fileid, *das);

            // Obtain the rest attributes and map them to DAS
            depth_first(fileid, "/", *das);

            // The HDF5 file id will be closed now!
            close_fileid(fileid);
        }
        else {// CF option
           read_cfdas( *das,filename);
           // cerr<<"end of DAS "<<endl;
        }

        Ancillary::read_ancillary_das( *das, filename ) ;

        dds->transfer_attributes(das);
        bdds->set_constraint( dhi ) ;
        bdds->clear_container() ;
    }
    catch(InternalErr & e) {
        BESDapError ex(e.get_error_message(), true, e.get_error_code(),
                       __FILE__, __LINE__);
        throw ex;
    }
    catch(Error & e) {
        BESDapError ex(e.get_error_message(), false, e.get_error_code(),
                       __FILE__, __LINE__);
        throw ex;
    }
    catch(...) {
        string s = "unknown exception caught building HDF5 DataDDS";
        BESInternalFatalError ex(s, __FILE__, __LINE__);
        throw ex;
    }

    return true;
}

bool HDF5RequestHandler::hdf5_build_help(BESDataHandlerInterface & dhi)
{
    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESInfo *info = dynamic_cast<BESInfo *>(response);
    if( !info )
        throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    map<string,string> attrs ;
    attrs["name"] = PACKAGE_NAME ;
    attrs["version"] = PACKAGE_VERSION ;
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
  
    info->add_module( PACKAGE_NAME, PACKAGE_VERSION ) ;

    return true;
}
