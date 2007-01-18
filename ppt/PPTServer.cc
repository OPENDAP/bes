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
#include <sstream>

using std::string ;
using std::ostringstream ;

#include "PPTServer.h"
#include "PPTException.h"
#include "PPTProtocol.h"
#include "SocketListener.h"
#include "ServerHandler.h"
#include "Socket.h"
#include "TheBESKeys.h"

#include "config.h"
#ifdef HAVE_OPENSSL
#include "SSLServer.h"
#endif

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
#ifndef HAVE_OPENSSL
    if( _secure )
    {
	string err("Server requested to be secure but OpenSSL is not built in");
	throw PPTException( err, __FILE__, __LINE__ ) ;
    }
#endif

    // get the certificate and key file information
    get_secure_files() ;
}

PPTServer::~PPTServer()
{
}

void
PPTServer::get_secure_files()
{
    bool found = false ;
    _cfile = TheBESKeys::TheKeys()->get_key( "BES.ServerCertFile", found ) ;
    if( !found || _cfile.empty() )
    {
	throw PPTException( "Unable to determine client certificate file.",
			    __FILE__, __LINE__ ) ;
    }

    _kfile = TheBESKeys::TheKeys()->get_key( "BES.ServerKeyFile", found ) ;
    if( !found || _kfile.empty() )
    {
	throw PPTException( "Unable to determine client key file.",
			    __FILE__, __LINE__ ) ;
    }

    string portstr = TheBESKeys::TheKeys()->get_key( "BES.ServerSecurePort",
						     found ) ;
    if( !found || portstr.empty() )
    {
	throw PPTException( "Unable to determine secure connection port.",
			    __FILE__, __LINE__ ) ;
    }
    _securePort = atoi( portstr.c_str() ) ;
    if( !_securePort )
    {
	string err = (string)"Unable to determine secure connection port "
	             + "from string " + portstr ;
	throw PPTException( err, __FILE__, __LINE__ ) ;
    }
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
    char *inBuff = new char[PPT_PROTOCOL_BUFFER_SIZE] ;
    int bytesRead = _mySock->receive( inBuff, PPT_PROTOCOL_BUFFER_SIZE ) ;
    string status( inBuff, bytesRead ) ;
    delete [] inBuff ;
    if( status != PPTProtocol::PPTCLIENT_TESTING_CONNECTION )
    {
	string err( "PPT Can not negotiate, " ) ;
	err += " client started the connection with " + status ;
	throw PPTException( err, __FILE__, __LINE__ ) ;
    }

    if( !_secure )
    {
	int len = PPTProtocol::PPTSERVER_CONNECTION_OK.length() ;
	_mySock->send( PPTProtocol::PPTSERVER_CONNECTION_OK, 0, len ) ;
    }
    else
    {
	authenticateClient() ;
    }
}

void
PPTServer::authenticateClient()
{
#ifdef HAVE_OPENSSL
    // let the client know that it needs to authenticate
    int len = PPTProtocol::PPTSERVER_AUTHENTICATE.length() ;
    _mySock->send( PPTProtocol::PPTSERVER_AUTHENTICATE, 0, len ) ;

    // wait for the client request for the secure port
    char *inBuff = new char[PPT_PROTOCOL_BUFFER_SIZE] ;
    int bytesRead = _mySock->receive( inBuff, PPT_PROTOCOL_BUFFER_SIZE ) ;
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
    portResponse << _securePort << PPTProtocol::PPT_COMPLETE_DATA_TRANSMITION ;
    len = portResponse.str().length() ;
    _mySock->send( portResponse.str(), 0, len ) ;

    // create a secure server object and authenticate
    SSLServer server( _securePort, _cfile, _kfile ) ;
    server.initConnection() ;
    server.closeConnection() ;

    // if it authenticates, good, if not, an exception is thrown, no need to
    // do anything else here.
#else
    throw PPTException( "Authentication requrested for this server but OpenSSL is not built into the server", __FILE__, __LINE__ ) ;
#endif
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
PPTServer::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "PPTServer::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    if( _handler )
    {
	strm << BESIndent::LMarg << "server handler:" << endl ;
	BESIndent::Indent() ;
	_handler->dump( strm ) ;
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "server handler: null" << endl ;
    }
    ServerHandler *		_handler ;
    if( _listener )
    {
	strm << BESIndent::LMarg << "listener:" << endl ;
	BESIndent::Indent() ;
	_listener->dump( strm ) ;
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "listener: null" << endl ;
    }
    strm << BESIndent::LMarg << "secure? " << _secure << endl ;
    PPTConnection::dump( strm ) ;
    BESIndent::UnIndent() ;
}

