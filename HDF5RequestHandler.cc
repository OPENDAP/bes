
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

#include "HDF5RequestHandler.h"

#include "h5das.h"
#include "h5dds.h"
#include "config_hdf5.h"
#include "HDF5TypeFactory.h"

#include "DAS.h"
#include "DDS.h"
#include "DODSInfo.h"
#include "DODSResponseNames.h"
#include "DODSContainer.h"
#include "DODSResponseHandler.h"
#include "DODSVersionInfo.h"
#include "OPeNDAPDataNames.h"
#include "DODSHandlerException.h"

extern "C" {
    hid_t get_fileid(const char *filename);
}

HDF5RequestHandler::HDF5RequestHandler( string name )
    : DODSRequestHandler( name )
{
    add_handler( DAS_RESPONSE, HDF5RequestHandler::hdf5_build_das ) ;
    add_handler( DDS_RESPONSE, HDF5RequestHandler::hdf5_build_dds ) ;
    add_handler( DATA_RESPONSE, HDF5RequestHandler::hdf5_build_data ) ;
    add_handler( HELP_RESPONSE, HDF5RequestHandler::hdf5_build_help ) ;
    add_handler( VERS_RESPONSE, HDF5RequestHandler::hdf5_build_version ) ;
}

HDF5RequestHandler::~HDF5RequestHandler()
{
}

bool
HDF5RequestHandler::hdf5_build_das( DODSDataHandlerInterface &dhi )
{
    string filename = dhi.container->get_real_name() ;
    hid_t file1 = get_fileid( filename.c_str() ) ;
    if( file1 < 0 )
    {
	throw DODSHandlerException( string( "hdf4_build_das: " )
					  + "Could not open hdf5 file: "
	                                  + filename ) ;
    }

    DAS *das = (DAS *)dhi.response_handler->get_response_object() ;

    find_gloattr( file1, *das ) ;
    depth_first( file1, "/", *das, filename.c_str() ) ;

    return true ;
}

bool
HDF5RequestHandler::hdf5_build_dds( DODSDataHandlerInterface &dhi )
{
    string filename = dhi.container->get_real_name() ;
    hid_t file1 = get_fileid( filename.c_str() ) ;
    if( file1 < 0 )
    {
	throw DODSHandlerException( string( "hdf4_build_dds: " )
					  + "Could not open hdf5 file: "
	                                  + filename ) ;
    }

    DDS *dds = (DDS *)dhi.response_handler->get_response_object() ;

    HDF5TypeFactory *factory = new HDF5TypeFactory ;
    dds->set_factory( factory ) ;

    depth_first( file1, "/", *dds, filename.c_str() ) ;

    dhi.data[POST_CONSTRAINT] = dhi.container->get_constraint();

    dds->set_factory( NULL ) ;
    delete factory ;

    return true ;
}

bool
HDF5RequestHandler::hdf5_build_data( DODSDataHandlerInterface &dhi )
{
    string filename = dhi.container->get_real_name() ;
    hid_t file1 = get_fileid( filename.c_str() ) ;
    if( file1 < 0 )
    {
	throw DODSHandlerException( string( "hdf4_build_data: " )
					  + "Could not open hdf5 file: "
	                                  + filename ) ;
    }

    DDS *dds = (DDS *)dhi.response_handler->get_response_object() ;

    HDF5TypeFactory *factory = new HDF5TypeFactory ;
    dds->set_factory( factory ) ;

    depth_first( file1, "/", *dds, filename.c_str() ) ;

    dhi.data[POST_CONSTRAINT] = dhi.container->get_constraint();

    dds->set_factory( NULL ) ;
    delete factory ;

    return true ;
}

bool
HDF5RequestHandler::hdf5_build_help( DODSDataHandlerInterface &dhi )
{
    DODSInfo *info = (DODSInfo *)dhi.response_handler->get_response_object() ;
    //info->add_data( (string)"No help currently available for netCDF handler.\n" ) ;

    return true ;
}

bool
HDF5RequestHandler::hdf5_build_version( DODSDataHandlerInterface &dhi )
{
    DODSVersionInfo *info = (DODSVersionInfo *)dhi.response_handler->get_response_object() ;
    info->addHandlerVersion( PACKAGE_NAME, PACKAGE_VERSION ) ;
    return true ;
}

