// PPTServer.cc

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
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <string>
#include <iostream>
#include <sstream>

using std::string ;
using std::cout ;
using std::endl ;
using std::flush ;
using std::ostringstream ;

#include "PPTServer.h"
#include "PPTException.h"
#include "PPTProtocol.h"
#include "SocketListener.h"
#include "ServerHandler.h"
#include "Socket.h"
#include "SSLServer.h"

PPTServer::PPTServer( ServerHandler *handler,
		      SocketListener *listener,
		      bool isSecure )
    : _handler( handler ),
      _listener( listener ),
      _secure( isSecure )
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
}

PPTServer::~PPTServer()
{
}

/** Using the info passed into the SocketLister, wait for an inbound 
    request (see SocketListener::accept()). When one is found, do the
    welcome message stuff (welcomeClient()) and then pass \c this to
    the handler's \c handle method. Note that \c this is a pointer to
    a PPTServer which is a kind of Connection. */
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
    if( _mySock ) _mySock->close() ;
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

    if( !_secure )
    {
	int len = PPTProtocol::PPTSERVER_CONNECTION_OK.length() ;
	_mySock->send( PPTProtocol::PPTSERVER_CONNECTION_OK, 0, len ) ;
	cout << "OK" << endl ;
    }
    else
    {
	authenticateClient() ;
	cout << "OK" << endl ;
    }
}

void
PPTServer::authenticateClient()
{
    // let the client know that it needs to authenticate
    int len = PPTProtocol::PPTSERVER_AUTHENTICATE.length() ;
    _mySock->send( PPTProtocol::PPTSERVER_AUTHENTICATE, 0, len ) ;

    // wait for the client request for the secure port
    char *inBuff = new char[4096] ;
    int bytesRead = _mySock->receive( inBuff, 4096 ) ;
    string portRequest( inBuff, bytesRead ) ;
    delete [] inBuff ;
    if( portRequest != PPTProtocol::PPTCLIENT_REQUEST_AUTHPORT )
    {
	string err( "Secure connection ... expecting request for port" ) ;
	err += " client requested " + portRequest ;
	throw PPTException( err, __FILE__, __LINE__ ) ;
    }

    // send the secure port number back to the client
    ostringstream portResponse ;
    portResponse << 10003 << PPTProtocol::PPT_COMPLETE_DATA_TRANSMITION ;
    len = portResponse.str().length() ;
    _mySock->send( portResponse.str(), 0, len ) ;

    // create a secure server object and authenticate
    string cfile = "/home/pwest/temp/ssl/keys/cacert.pem" ;
    string kfile = "/home/pwest/temp/ssl/keys/privkey.pem" ;
    SSLServer server( 10003, cfile, kfile ) ;
    server.initConnection() ;
    server.closeConnection() ;

    // if it authenticates, good, if not, an exception is thrown, no need to
    // do anything else here.
}

// $Log: PPTServer.cc,v $
