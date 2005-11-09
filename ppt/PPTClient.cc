// PPTClient.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>

#include <string>
#include <iostream>

using std::string ;
using std::cerr ;
using std::cout ;
using std::endl ;

#include "PPTClient.h"
#include "TcpSocket.h"
#include "UnixSocket.h"
#include "PPTProtocol.h"
#include "SocketException.h"
#include "PPTException.h"

PPTClient::PPTClient( const string &hostStr, int portVal )
    : _connected( false )
{
    _mySock = new TcpSocket( hostStr, portVal ) ;
    _mySock->connect() ;
    _connected = true ;
}
    
PPTClient::PPTClient( const string &unix_socket )
    : _connected( false )
{
    _mySock = new UnixSocket( unix_socket ) ;
    _mySock->connect() ;
    _connected = true ;
}

PPTClient::~PPTClient()
{
    if( _mySock )
    {
	if( _connected )
	{
	    closeConnection() ;
	}
	delete _mySock ;
	_mySock = 0 ;
    }
}

void
PPTClient::initConnection()
{
    try
    {
	writeBuffer( PPTProtocol::PPTCLIENT_TESTING_CONNECTION ) ;
    }
    catch( SocketException &e )
    {
	string msg = "Failed to initialize connection to server\n" ;
	msg += e.getMessage() ;
	throw PPTException( msg ) ;
    }

    char *inBuff = new char[4096] ;
    int bytesRead = readBuffer( inBuff ) ;
    string status( inBuff, 0, bytesRead ) ;
    delete [] inBuff ;
    if( status == PPTProtocol::PPT_PROTOCOL_UNDEFINED )
    {
	throw PPTException( "Could not connect to server, server may be down or busy" ) ;
    }
    if( status != PPTProtocol::PPTSERVER_CONNECTION_OK )
    {
	throw PPTException( "Server reported an invalid connection, \"" + status + "\"" ) ;
    }
}
    
void
PPTClient::closeConnection()
{
    if( _connected )
    {
	if( !_brokenPipe )
	{
	    try
	    {
		sendExit() ;
	    }
	    catch( SocketException e )
	    {
		cerr << "Failed to inform server that the client is exiting, "
		     << "continuing" << endl ;
		cerr << e.getMessage() << endl ;
	    }
	}

	_mySock->close() ;

	_connected = false ;
	_brokenPipe = false ;
    }
}

// $Log: PPTClient.cc,v $
