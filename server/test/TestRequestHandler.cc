// TestRequestHandler.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of cdf-handler, a C++ server of OPeNDAP for access to cdf
// data

// Copyright (c) 2004-2009 OPeNDAP, Inc.
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
 
// Implementation for TestRequestHandler.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifdef __GNUG__
#pragma implementation
#endif

#include "TestRequestHandler.h"
#include "BESResponseNames.h"
#include "BESResponseHandler.h"
#include "BESInfo.h"

TestRequestHandler::TestRequestHandler( string name )
    : BESRequestHandler( name )
{
    add_method( HELP_RESPONSE, TestRequestHandler::cdf_build_help ) ;
}

TestRequestHandler::~TestRequestHandler()
{
}

bool
TestRequestHandler::cdf_build_help( BESDataHandlerInterface &dhi )
{
    BESInfo *info = (BESInfo *)dhi.response_handler->get_response_object() ;
    string key ;
    if( dhi.transmit_protocol == "HTTP" )
	key = (string)"Test.Help." + dhi.transmit_protocol ;
    else
	key = "Test.Help.TXT" ;
    info->add_data_from_file( key, "Test Handler Help" ) ;

    return true ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
TestRequestHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "TestRequestHandler::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESRequestHandler::dump( strm ) ;
    BESIndent::UnIndent() ;
}

