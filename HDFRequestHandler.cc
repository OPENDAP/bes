// This file is part of the hdf4 data handler for the OPeNDAP data server.

// Copyright (c) 2005 OPeNDAP, Inc.
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
// You should have received a copy of the GNU Lesser General Public License
// along with this library; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
 
#include <iostream>
#include <fstream>

using std::ifstream ;
using std::cerr ;
using std::endl ;

#include "DAS.h"
#include "DDS.h"

#include "HDFRequestHandler.h"
#include "DODSResponseHandler.h"
#include "DODSResponseNames.h"
#include "DODSConstraintFuncs.h"
#include "DODSInfo.h"
#include "TheDODSKeys.h"
#include "DODSResponseException.h"

extern void read_das(DAS& das, const string& cachedir, const string& filename);
extern void read_dds(DDS& dds, const string& cachedir, const string& filename);
extern void register_funcs(DDS& dds);

HDFRequestHandler::HDFRequestHandler( string name )
    : DODSRequestHandler( name )
{
    add_handler( DAS_RESPONSE, HDFRequestHandler::hdf_build_das ) ;
    add_handler( DDS_RESPONSE, HDFRequestHandler::hdf_build_dds ) ;
    add_handler( DATA_RESPONSE, HDFRequestHandler::hdf_build_data ) ;
    add_handler( HELP_RESPONSE, HDFRequestHandler::hdf_build_help ) ;
    add_handler( VERS_RESPONSE, HDFRequestHandler::hdf_build_version ) ;
}

HDFRequestHandler::~HDFRequestHandler()
{
}

bool
HDFRequestHandler::hdf_build_das( DODSDataHandlerInterface &dhi )
{
    DAS *das = (DAS *)dhi.response_handler->get_response_object() ;

    read_das( *das, "/tmp/", dhi.container->get_real_name() );

    return true ;
}

bool
HDFRequestHandler::hdf_build_dds( DODSDataHandlerInterface &dhi )
{
    // Needs to use the factory class. jhrg 9/20/05
    DDS *dds = (DDS *)dhi.response_handler->get_response_object() ;

    read_dds( *dds, "/tmp/", dhi.container->get_real_name() );
    dds->parse_constraint( dhi.container->get_constraint() );

    return true ;
}

bool
HDFRequestHandler::hdf_build_data( DODSDataHandlerInterface &dhi )
{
    // Needs to use the factory class. jhrg 9/20/05
    DDS *dds = (DDS *)dhi.response_handler->get_response_object() ;

    dds->filename( dhi.container->get_real_name() );
    read_dds( *dds, "/tmp/", dhi.container->get_real_name() );
    dhi.post_constraint = dhi.container->get_constraint();

    return true ;
}

bool
HDFRequestHandler::hdf_build_help( DODSDataHandlerInterface &dhi )
{
    DODSInfo *info = (DODSInfo *)dhi.response_handler->get_response_object() ;
    info->add_data( (string)"No help for netCDF handler.\n" ) ;

#if 0
    info->add_data( (string)"cdf-dods help: " + hdf_version() + "\n" ) ;
    bool found = false ;
    string key = (string)"HDF.Help." + dhi.transmit_protocol ;
    string file = TheDODSKeys->get_key( key, found ) ;
    if( found == false )
    {
     info->add_data( "no help information available for cdf-dods\n" ) ;
    }
    else
    {
        ifstream ifs( file.c_str() ) ;
 if( !ifs )
     {
          info->add_data( "cdf-dods help file not found, help information not available\n" ) ;
       }
      else
   {
          char line[4096] ;
      while( !ifs.eof() )
            {
          ifs.getline( line, 4096 ) ;
            if( !ifs.eof() )
               {
                  info->add_data( line ) ;
               info->add_data( "\n" ) ;
           }
          }
      ifs.close() ;
      }
    }
#endif

    return true ;
}

bool
HDFRequestHandler::hdf_build_version( DODSDataHandlerInterface &dhi )
{
    DODSInfo *info = (DODSInfo *)dhi.response_handler->get_response_object() ;
    info->add_data( (string)"    hdf4 0.9\n" ) ;
    info->add_data( (string)"    libhdf-dods 0.9\n" ) ;
#if 0
    info->add_data( (string)"    " + nc_version() + "\n" ) ;
    info->add_data( (string)"        libcdf2.7\n" ) ;
#endif
    return true ;
}

