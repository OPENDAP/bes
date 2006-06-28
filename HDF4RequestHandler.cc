
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
 
// HDF4RequestHandler.cc

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "HDF4RequestHandler.h"
#include "BESResponseNames.h"
#include "DAS.h"
#include "DDS.h"
#include "BESInfo.h"
#include "BESResponseHandler.h"
#include "BESVersionInfo.h"
#include "HDFTypeFactory.h"
#include "TheBESKeys.h"
#include "BESKeysException.h"
#include "BESDataNames.h"
#include "ConstraintEvaluator.h"
#include "config_hdf.h"

extern void read_das(DAS& das, const string& cachedir, const string& filename);
extern void read_dds(DDS& dds, const string& cachedir, const string& filename);
extern void register_funcs(ConstraintEvaluator& dds);

string HDF4RequestHandler::_cachedir = "" ;

HDF4RequestHandler::HDF4RequestHandler( string name )
    : BESRequestHandler( name )
{
    add_handler( DAS_RESPONSE, HDF4RequestHandler::hdf4_build_das ) ;
    add_handler( DDS_RESPONSE, HDF4RequestHandler::hdf4_build_dds ) ;
    add_handler( DATA_RESPONSE, HDF4RequestHandler::hdf4_build_data ) ;
    add_handler( HELP_RESPONSE, HDF4RequestHandler::hdf4_build_help ) ;
    add_handler( VERS_RESPONSE, HDF4RequestHandler::hdf4_build_version ) ;

    if( HDF4RequestHandler::_cachedir == "" )
    {
	bool found = false ;
	_cachedir = TheBESKeys::TheKeys()->get_key( "HDF4.CacheDir", found ) ;
	if( !found || _cachedir == "" )
	    _cachedir = "/tmp" ;

	string dummy = _cachedir + "/dummy" ;
	int fd = open( dummy.c_str(), O_CREAT|O_WRONLY|O_TRUNC ) ;
	unlink( dummy.c_str() ) ;
	if( fd == -1 )
	{
	    if( _cachedir == "/tmp" )
	    {
		close(fd);
		string err = "Could not create a file in the cache directory ("
			     + _cachedir + ")" ;
		throw BESKeysException( err, __FILE__, __LINE__ ) ;
	    }
	    _cachedir = "/tmp" ;
	}
	close(fd);
    }
}

HDF4RequestHandler::~HDF4RequestHandler()
{
}

bool
HDF4RequestHandler::hdf4_build_das( BESDataHandlerInterface &dhi )
{
    DAS *das = (DAS *)dhi.response_handler->get_response_object() ;

    read_das( *das, _cachedir, dhi.container->get_real_name() ) ;

    return true ;
}

bool
HDF4RequestHandler::hdf4_build_dds( BESDataHandlerInterface &dhi )
{
    DDS *dds = (DDS *)dhi.response_handler->get_response_object() ;

    HDFTypeFactory *factory = new HDFTypeFactory ;
    dds->set_factory( factory ) ;
    ConstraintEvaluator ce;

    read_dds( *dds, _cachedir, dhi.container->get_real_name() ) ;

    dhi.data[POST_CONSTRAINT] = dhi.container->get_constraint();

    dds->set_factory( NULL ) ;
    delete factory ;

    return true ;
}

bool
HDF4RequestHandler::hdf4_build_data( BESDataHandlerInterface &dhi )
{
    DDS *dds = (DDS *)dhi.response_handler->get_response_object() ;

    HDFTypeFactory *factory = new HDFTypeFactory ;
    dds->set_factory( factory ) ;
    ConstraintEvaluator ce;

    dds->filename( dhi.container->get_real_name() );

    read_dds( *dds, _cachedir, dhi.container->get_real_name() ) ;
    register_funcs( ce ) ;

    dds->set_factory( NULL ) ;
    delete factory ;

    return true ;
}

bool
HDF4RequestHandler::hdf4_build_help( BESDataHandlerInterface &dhi )
{
    BESInfo *info = (BESInfo *)dhi.response_handler->get_response_object() ;
    string handles = (string)DAS_RESPONSE
                     + "," + DDS_RESPONSE
                     + "," + DATA_RESPONSE
                     + "," + HELP_RESPONSE
                     + "," + VERS_RESPONSE ;
    info->add_tag( "handles", handles ) ;
    info->add_tag( "version", PACKAGE_STRING ) ;

    return true ;
}

bool
HDF4RequestHandler::hdf4_build_version( BESDataHandlerInterface &dhi )
{
    BESVersionInfo *info = (BESVersionInfo *)dhi.response_handler->get_response_object() ;
    info->addHandlerVersion( PACKAGE_NAME, PACKAGE_VERSION ) ;
    return true ;
}

