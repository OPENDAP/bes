
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "h5_handler.h"
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
#include "H5Git.h"

#define HDF5_NAME "h5"

/// An external object that handles NASA EOS HDF5 files for grid generation 
/// and meta data parsing.
extern H5EOS eos;

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
    string filename = dhi.container->access();
    hid_t file1 = get_fileid(filename.c_str());
    if (file1 < 0) {
        throw BESNotFoundError((string) "Could not open hdf file: "
                               + filename, __FILE__, __LINE__);
    }
    if (eos.check_eos(file1))
        eos.set_dimension_array();

    BESResponseObject *response = dhi.response_handler->get_response_object() ;
    BESDASResponse *bdas = dynamic_cast < BESDASResponse * >(response) ;
    if( !bdas )
        throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    try {
        bdas->set_container( dhi.container->get_symbolic_name() ) ;
        DAS *das = bdas->get_das();

        find_gloattr(file1, *das);
        depth_first(file1, "/", *das);
        close_fileid(file1);

        Ancillary::read_ancillary_das( *das, filename ) ;

        bdas->clear_container() ;
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
        string s = "unknown exception caught building HDF5 DAS";
        BESInternalFatalError ex(s, __FILE__, __LINE__);
        throw ex;
    }

    return true;
}

bool HDF5RequestHandler::hdf5_build_dds(BESDataHandlerInterface & dhi)
{
    string filename = dhi.container->access();
    hid_t file1 = get_fileid(filename.c_str());
    if (file1 < 0) {
        throw BESNotFoundError(string("hdf5_build_dds: ")
                               + "Could not open hdf5 file: "
                               + filename, __FILE__, __LINE__);
    }
    if (eos.check_eos(file1))
        eos.set_dimension_array();

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDDSResponse *bdds = dynamic_cast < BESDDSResponse * >(response);
    if( !bdds )
        throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;
  
    try {
        bdds->set_container( dhi.container->get_symbolic_name() ) ;
        DDS *dds = bdds->get_dds();

        depth_first(file1, "/", *dds, filename.c_str());

        Ancillary::read_ancillary_dds( *dds, filename ) ;

        DAS *das = new DAS ;
        BESDASResponse bdas( das ) ;
        bdas.set_container( dhi.container->get_symbolic_name() ) ;

        find_gloattr(file1, *das);
        depth_first(file1, "/", *das);
        close_fileid(file1);

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
    string filename = dhi.container->access();
    hid_t file1 = get_fileid(filename.c_str());
    if (file1 < 0) {
        throw BESNotFoundError(string("hdf5_build_data: ")
                               + "Could not open hdf5 file: "
                               + filename, __FILE__, __LINE__);
    }
    if (eos.check_eos(file1))
        eos.set_dimension_array();

    BESResponseObject *response = dhi.response_handler->get_response_object();
    BESDataDDSResponse *bdds = dynamic_cast < BESDataDDSResponse * >(response);
    if( !bdds )
        throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    try {
        bdds->set_container( dhi.container->get_symbolic_name() ) ;
        DataDDS *dds = bdds->get_dds();

        depth_first(file1, "/", *dds, filename.c_str());

        Ancillary::read_ancillary_dds( *dds, filename ) ;

        DAS *das = new DAS ;
        BESDASResponse bdas( das ) ;
        bdas.set_container( dhi.container->get_symbolic_name() ) ;

        find_gloattr(file1, *das);
        depth_first(file1, "/", *das);
        close_fileid(file1);

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
