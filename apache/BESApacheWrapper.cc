// BESApacheWrapper.cc

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

#include <string>

using std::string ;

#include "BESApacheWrapper.h"
#include "BESApacheRequests.h"
#include "BESApacheInterface.h"
#include "BESProcessEncodedString.h"
#include "BESGlobalIQ.h"
#include "BESDefaultModule.h"
#include "BESDefaultCommands.h"

BESApacheWrapper::BESApacheWrapper()
{
    _data_request = 0 ;
    _user_name = 0 ;
    _token = 0 ;
    _requests = 0 ;

    BESDefaultModule::initialize( 0, 0 ) ;
    BESDefaultCommands::initialize( 0, 0 ) ;

    BESGlobalIQ::BESGlobalInit( 0, 0 ) ;
}

BESApacheWrapper::~BESApacheWrapper()
{
    if( _data_request )
    {
	delete [] _data_request ;
	_data_request = 0 ;
    }
    if( _user_name )
    {
	delete [] _user_name ;
	_user_name = 0 ;
    }
    if( _token )
    {
	delete [] _token ;
	_token = 0 ;
    }
    if ( _requests )
    {
	delete _requests ;
	_requests = 0 ;
    }
    BESGlobalIQ::BESGlobalQuit() ;
}

/** @brief Execute the given request using BESApacheInterface interface

    @param re BESDataRequestInterface filled in by the apache module holding
    the request information.
    @return Whether the request was successful or not
    @see _BESDataRequestInterface
 */
int
BESApacheWrapper::call_BES( const BESDataRequestInterface & re )
{
    BESApacheInterface interface( re ) ;
    int status = interface.execute_request() ;
    if( status != 0 )
    {
	interface.finish_with_error( status ) ;
    }
    return status ;
}

/** @brief Find the request from the URL and convert it to readable format

    @param s URL to convert into an OpenDAP request
 */
void
BESApacheWrapper::process_request(const char*s)
{
    BESProcessEncodedString h( s ) ;
    string str = h.get_key( "request" ) ;
    _requests = new BESApacheRequests( str ) ;
}

const char *
BESApacheWrapper::get_first_request()
{
    if( _requests )
    {
	BESApacheRequests::requests_citer rcurr = _requests->get_first_request() ;
	BESApacheRequests::requests_citer rend = _requests->get_end_request() ;
	if( rcurr == rend )
	    return 0 ;
	return (*rcurr).c_str() ;
    }
    return 0 ;
}

const char *
BESApacheWrapper::get_next_request()
{
    if( _requests )
    {
	static BESApacheRequests::requests_citer rcurr = _requests->get_first_request() ;
	static BESApacheRequests::requests_citer rend = _requests->get_end_request() ;
	if( rcurr == rend )
	    return 0 ;
	++rcurr ;
	if( rcurr == rend )
	    return 0 ;
	return (*rcurr).c_str() ;
    }
    return 0 ;
}

/** @brief Find the username from the URL and convert it to readable format

    @param s URL to convert into an OpenDAP user name
    @return Resulting OpenDAP user name
 */
const char *
BESApacheWrapper::process_user(const char*s)
{
    BESProcessEncodedString h( s ) ;
    string str = h.get_key( "username" ) ;
    if( str == "" )
    {
	_user_name = new char[strlen( str.c_str() ) + 1] ;
	strcpy( _user_name, str.c_str() ) ;
    }
    else
    {
	_user_name = new char[strlen( str.c_str() ) + 20] ;
	sprintf( _user_name, "OpenDAP.remoteuser=%s", str.c_str() ) ;
    }
    return _user_name ;
}

/** @brief Find the session token from the URL and convert it to readable format

    @param s URL to convert into an OpenDAP session token
    @return Resulting OpenDAP user name
 */
const char *
BESApacheWrapper::process_token(const char*s)
{
    BESProcessEncodedString h( s ) ;
    string str = h.get_key( "token" ) ;
    _token = new char[strlen( str.c_str() ) + 1] ;
    strcpy( _token, str.c_str() ) ;
    return _token ;
}

