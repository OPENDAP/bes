// BESDapRequestHandler.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>


#include "BESDapRequestHandler.h"
#include "BESResponseHandler.h"
#include "BESResponseNames.h"
#include "BESVersionInfo.h"
#include "BESDataNames.h"

BESDapRequestHandler::BESDapRequestHandler( const string &name )
    : BESRequestHandler( name )
{
    add_handler( HELP_RESPONSE, BESDapRequestHandler::dap_build_help ) ;
    add_handler( VERS_RESPONSE, BESDapRequestHandler::dap_build_version ) ;
}

BESDapRequestHandler::~BESDapRequestHandler()
{
}

bool
BESDapRequestHandler::dap_build_help( BESDataHandlerInterface &dhi )
{
    BESInfo *info = (BESInfo *)dhi.response_handler->get_response_object() ;
    info->begin_tag( "DAP" ) ;
    info->add_data_from_file( "DAP.Help", "DAP Help" ) ;
    info->end_tag( "DAP" ) ;

    return true ;
}

bool
BESDapRequestHandler::dap_build_version( BESDataHandlerInterface &dhi )
{
    BESVersionInfo *info = (BESVersionInfo *)dhi.response_handler->get_response_object() ;
    info->begin_tag( "DAP" ) ;
    info->add_tag( "version", "2.0" ) ;
    info->add_tag( "version", "3.0" ) ;
    info->add_tag( "version", "3.1" ) ;
    info->end_tag( "DAP" ) ;

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

