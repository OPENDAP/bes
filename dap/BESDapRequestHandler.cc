// BESDapRequestHandler.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <libdap/util.h>

using namespace libdap ;

#include "BESDapRequestHandler.h"
#include "BESResponseHandler.h"
#include "BESDapNames.h"
#include "BESResponseNames.h"
#include "BESVersionInfo.h"

BESDapRequestHandler::BESDapRequestHandler( const string &name )
    : BESRequestHandler( name )
{
    add_method( HELP_RESPONSE, BESDapRequestHandler::dap_build_help ) ;
    add_method( VERS_RESPONSE, BESDapRequestHandler::dap_build_version ) ;
}

BESDapRequestHandler::~BESDapRequestHandler()
{
}

bool
BESDapRequestHandler::dap_build_help( BESDataHandlerInterface &dhi )
{
    BESResponseObject *response = dhi.response_handler->get_response_object() ;
    BESInfo *info = dynamic_cast < BESInfo * >(response) ;
    if( !info )
       throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    map<string,string, std::less<>> attrs ;
    attrs["name"] = libdap_name() ;
    attrs["version"] = libdap_version() ;
    info->begin_tag( "module", &attrs ) ;
    info->add_data_from_file( "DAP.Help", "DAP Help" ) ;
    info->end_tag( "module" ) ;

    return true ;
}

bool
BESDapRequestHandler::dap_build_version( BESDataHandlerInterface &dhi )
{
    BESResponseObject *response = dhi.response_handler->get_response_object() ;
    BESVersionInfo *info = dynamic_cast < BESVersionInfo * >(response) ;
    if( !info )
       throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    info->add_library( libdap_name(), libdap_version() ) ;
    list<string> versions ;
    versions.push_back( "2.0" ) ;
    versions.push_back( "3.0" ) ;
    versions.push_back( "3.2" ) ;
    info->add_service( OPENDAP_SERVICE, versions ) ;

    return true ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESDapRequestHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESDapRequestHandler::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESRequestHandler::dump( strm ) ;
    BESIndent::UnIndent() ;
}

