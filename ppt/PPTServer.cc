// PPTServer.cc

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
using std::cout ;
using std::endl ;
using std::flush ;

#include "PPTServer.h"
#include "PPTException.h"
#include "PPTProtocol.h"
#include "SocketListener.h"
#include "ServerHandler.h"
#include "Socket.h"

PPTServer::PPTServer( ServerHandler *handler, SocketListener *listener )
{
    if( !handler )
    {
	string err( "Null handler passed to PPTServer" ) ;
	throw PPTException( err, __FILE__, __LINE__ ) ;
    }
    if( !listener )
    {
	string err( "Null listener passed to PPTServer" ) ;
	throw PPTException( err, __FILE__, __LINE__ ) ;
    }
    _handler = handler ;
    _listener = listener ;
}

PPTServer::~PPTServer()
{
}

void
PPTServer::initConnection()
{
    for(;;)
    {
	_mySock = _listener->accept() ;
	if( _mySock )
	{
	    // welcome the client
	    welcomeClient( ) ;

	    // now hand it off to the handler
	    _handler->handle( this ) ;
	}
    }
}

void
PPTServer::closeConnection()
{
    _mySock->close() ;
}

void
PPTServer::welcomeClient()
{
    cout << "Incoming connection, initiating handshake ... " << flush ;
    char *inBuff = new char[4096] ;
    int bytesRead = _mySock->receive( inBuff, 4096 ) ;
    string status( inBuff, bytesRead ) ;
    delete [] inBuff ;
    if( status != PPTProtocol::PPTCLIENT_TESTING_CONNECTION )
    {
	cout << "FAILED" << endl ;
	string err( "PPT Can not negotiate, " ) ;
	err += " client started the connection with " + status ;
	throw PPTException( err, __FILE__, __LINE__ ) ;
    }
    else
    {
	int len = PPTProtocol::PPTSERVER_CONNECTION_OK.length() ;
	_mySock->send( PPTProtocol::PPTSERVER_CONNECTION_OK, 0, len ) ;
	cout << "OK" << endl ;
    }
}

// $Log: PPTServer.cc,v $
