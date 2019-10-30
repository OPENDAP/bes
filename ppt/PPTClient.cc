// PPTClient.cc

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
#include <iostream>
#include <sstream>

using std::string ;
using std::cerr ;
using std::cout ;
using std::endl ;
using std::ostringstream ;
using std::ostream;

#include "PPTClient.h"
#include "TcpSocket.h"
#include "UnixSocket.h"
#include "PPTProtocol.h"
#include "BESInternalError.h"
#include "BESSyntaxUserError.h"
#include "TheBESKeys.h"

#include "config.h"
#if defined HAVE_OPENSSL && defined NOTTHERE
#include "SSLClient.h"
#endif

PPTClient::PPTClient( const string &hostStr, int portVal, int timeout )
    : PPTConnection( timeout ),
      _connected( false ),
      _host( hostStr )
{
    // connect to the specified host at the specified socket to handle the
    // secure connection
    _mySock = new TcpSocket( hostStr, portVal ) ;
    _mySock->connect() ;
    _connected = _mySock->isConnected();
}
    
PPTClient::PPTClient( const string &unix_socket, int timeout )
    : PPTConnection( timeout ),
      _connected( false )
{
    // connect to the specified unix socket to handle the secure connection
    _mySock = new UnixSocket( unix_socket ) ;
    _mySock->connect() ;
    _connected = true ;
}

void
PPTClient::get_secure_files()
{
    bool found = false ;
    TheBESKeys::TheKeys()->get_value( "BES.ClientCertFile", _cfile, found ) ;
    if( !found || _cfile.empty() )
    {
	string err = "Unable to determine client certificate file." ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }

    found = false ;
    TheBESKeys::TheKeys()->get_value( "BES.ClientCertAuthFile", _cafile, found);
    if( !found || _cafile.empty() )
    {
	string err = "Unable to determine client certificate authority file." ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }

    found = false ;
    TheBESKeys::TheKeys()->get_value( "BES.ClientKeyFile", _kfile, found ) ;
    if( !found || _kfile.empty() )
    {
	string err = "Unable to determine client key file." ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }
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
	send( PPTProtocol::PPTCLIENT_TESTING_CONNECTION ) ;
    }
    catch( BESInternalError &e )
    {
	string msg = "Failed to initialize connection to server\n" ;
	msg += e.get_message() ;
	throw BESInternalError( msg, __FILE__, __LINE__ ) ;
    }

    // we're just getting tokens, not a big buffer, so don't need that big
    // of a buffer. pcw 05/31/08
    const int ppt_buffer_size = 64 ;
    char *inBuff = new char[ppt_buffer_size+1] ;
    int bytesRead = readBufferNonBlocking( inBuff, ppt_buffer_size ) ;
    if( bytesRead < 1 )
    {
	delete [] inBuff ;
	string err = "Could not connect to server, server may be down or busy" ;
	throw BESInternalError( err, __FILE__, __LINE__) ;
    }

    if( bytesRead > ppt_buffer_size )
	bytesRead = ppt_buffer_size ;
    inBuff[bytesRead] = '\0' ;
    string status( inBuff, 0, bytesRead ) ;
    delete [] inBuff ;

    if( status == PPTProtocol::PPT_PROTOCOL_UNDEFINED )
    {
	string err = "Could not connect to server, server may be down or busy" ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }

    if( status == PPTProtocol::PPTSERVER_AUTHENTICATE )
    {
	authenticateWithServer() ;
    }
    else if( status != PPTProtocol::PPTSERVER_CONNECTION_OK )
    {
	string err = "Server reported an invalid connection, \""
	             + status + "\"" ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
}

void
PPTClient::authenticateWithServer()
{
#if defined HAVE_OPENSSL && defined NOTTHERE
    // get the certificate and key file information
    get_secure_files() ;

    // send request for the authentication port
    send( PPTProtocol::PPTCLIENT_REQUEST_AUTHPORT ) ;

    // receive response with port, terminated with TERMINATE token. We are
    // exchanging a port number and a terminating token. The buffer doesn't
    // need to be too big. pcw 05/31/08
    const int ppt_buffer_size = 64 ;
    char *inBuff = new char[ppt_buffer_size+1] ;
    int bytesRead = readBufferNonBlocking( inBuff, ppt_buffer_size ) ;
    if( bytesRead < 1 )
    {
	delete [] inBuff ;
	string err = "Expecting secure port number response" ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }

    if( bytesRead > ppt_buffer_size )
    {
	bytesRead = ppt_buffer_size ;
    }
    inBuff[bytesRead] = '\0' ;
    ostringstream portResponse( inBuff ) ;
    delete [] inBuff ;

    int portVal = atoi( portResponse.str().c_str() ) ;
    if( portVal == 0 )
    {
	string err = "Expecting valid secure port number response" ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }

    // authenticate using SSLClient
    SSLClient client( _host, portVal, _cfile, _cafile, _kfile ) ;
    client.initConnection() ;
    client.closeConnection() ;

    // If it authenticates, good, if not then an exception is thrown. We
    // don't need to do anything else here.
#else
    throw BESInternalError( "Server has requested authentication, but OpenSSL is not built into this client", __FILE__, __LINE__ ) ;
#endif
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
	    catch( BESInternalError &e )
	    {
		cerr << "Failed to inform server that the client is exiting, "
		     << "continuing" << endl ;
		cerr << e.get_message() << endl ;
	    }
	}

	_mySock->close() ;

	_connected = false ;
	_brokenPipe = false ;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
PPTClient::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "PPTClient::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "connected? " << _connected << endl ;
    strm << BESIndent::LMarg << "host: " << _host << endl ;
    PPTConnection::dump( strm ) ;
    BESIndent::UnIndent() ;
}

