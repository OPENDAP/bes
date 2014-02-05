// PPTServer.cc

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

#include <config.h>

#include <string>
#include <sstream>
#include <cstdlib>

using std::string ;
using std::ostringstream ;

#include "PPTServer.h"
#include "BESInternalError.h"
#include "BESSyntaxUserError.h"
#include "PPTProtocol.h"
#include "SocketListener.h"
#include "ServerHandler.h"
#include "Socket.h"
#include "TheBESKeys.h"
#include "BESDebug.h"

#include "config.h"
#if defined HAVE_OPENSSL && defined NOTTHERE
#include "SSLServer.h"
#endif

#define PPT_SERVER_DEFAULT_TIMEOUT 1

PPTServer::PPTServer( ServerHandler *handler,
		      SocketListener *listener,
		      bool isSecure )
    : PPTConnection( PPT_SERVER_DEFAULT_TIMEOUT),
      _handler( handler ),
      _listener( listener ),
      _secure( isSecure )
{
    if( !handler )
    {
	string err( "Null handler passed to PPTServer" ) ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
    if( !listener )
    {
	string err( "Null listener passed to PPTServer" ) ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
#if !defined HAVE_OPENSSL && defined NOTTHERE
    if( _secure )
    {
	string err("Server requested to be secure but OpenSSL is not built in");
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
#endif

    // get the certificate and key file information
    if( _secure )
    {
	get_secure_files() ;
    }
}

PPTServer::~PPTServer()
{
}

void
PPTServer::get_secure_files()
{
    bool found = false ;
    TheBESKeys::TheKeys()->get_value( "BES.ServerCertFile", _cfile, found ) ;
    if( !found || _cfile.empty() )
    {
	string err = "Unable to determine server certificate file." ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }

    found = false ;
    TheBESKeys::TheKeys()->get_value( "BES.ServerCertAuthFile", _cafile, found);
    if( !found || _cafile.empty() )
    {
	string err = "Unable to determine server certificate authority file." ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }

    found = false ;
    TheBESKeys::TheKeys()->get_value( "BES.ServerKeyFile", _kfile, found ) ;
    if( !found || _kfile.empty() )
    {
	string err = "Unable to determine server key file." ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }

    found = false ;
    string portstr ;
    TheBESKeys::TheKeys()->get_value( "BES.ServerSecurePort", portstr, found ) ;
    if( !found || portstr.empty() )
    {
	string err = "Unable to determine secure connection port." ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }
    _securePort = atoi( portstr.c_str() ) ;
    if( !_securePort )
    {
	string err = (string)"Unable to determine secure connection port "
	             + "from string " + portstr ;
	throw BESSyntaxUserError( err, __FILE__, __LINE__ ) ;
    }
}

/** Using the info passed into the SocketLister, wait for an inbound
    request (see SocketListener::accept()). When one is found, do the
    welcome message stuff (welcomeClient()) and then pass \c this to
    the handler's \c handle method. Note that \c this is a pointer to
    a PPTServer which is a kind of Connection. */
void PPTServer::initConnection()
{
    for (;;)
    {
        BESDEBUG( "ppt", "PPTServer::initConnection() - Calling SocketListener::accept()" << endl );
        _mySock = _listener->accept();
        BESDEBUG( "ppt", "PPTServer::initConnection() - Connection accepted. Connected=" << _mySock->isConnected() << endl );
        if (_mySock)
        {
            BESDEBUG( "ppt", "PPTServer::initConnection() - Checking allowConnection() " << endl );

            if (_mySock->allowConnection() == true)
            {
                BESDEBUG( "ppt", "PPTServer::initConnection() - allowConnection() is TRUE. " << endl );

                // welcome the client
                BESDEBUG( "ppt", "PPTServer::initConnection() - Calling welcomeClient()" << endl );
                if (welcomeClient() != -1) {
                    // now hand it off to the handler
                    _handler->handle(this);

                    // Added this call to close - when the PPTServer class is used by
                    // a server that gets a number of connections on the same port,
                    // one per command, not closing the sockets after a command results
                    // in lots of sockets in the 'CLOSE_WAIT' status.
                    _mySock->close();
                }
            }
            else
            {
                BESDEBUG( "ppt", "PPTServer::initConnection() - allowConnection() is FALSE! Closing Socket. " << endl );
                _mySock->close();
            }
        }
    }
}

void
PPTServer::closeConnection()
{
    if( _mySock )
        _mySock->close() ;
}

int PPTServer::welcomeClient()
{
    // Doing a non blocking read in case the connection is being initiated
    // by a non-bes client. Don't want this to block. pcw - 3/5/07
    // int bytesRead = _mySock->receive( inBuff, ppt_buffer_size ) ;
    //
    // We are receiving handshaking tokens, so the buffer doesn't need to be
    // all that big. pcw - 05/31/08
    unsigned int ppt_buffer_size = 64;
    char *inBuff = new char[ppt_buffer_size + 1];
    int bytesRead = readBufferNonBlocking(inBuff, ppt_buffer_size);

    // if the read of the initial connection fails or blocks, then return
    if (bytesRead == -1)
    {
        _mySock->close();
        delete[] inBuff;
        return -1;
    }

    string status(inBuff, bytesRead);
    delete[] inBuff;

    if (status != PPTProtocol::PPTCLIENT_TESTING_CONNECTION)
    {
        /* If cannot negotiate with the client then we don't want to exit
         * by throwing an exception, we want to return and let the caller
         * clean up the connection
         */
        string err("PPT cannot negotiate, ");
        err += " client started the connection with " + status;
        BESDEBUG( "ppt", err << endl );
        //throw BESInternalError( err, __FILE__, __LINE__ ) ;
        send(err);
        _mySock->close();
        return -1;
    }

    if (!_secure)
    {
        send(PPTProtocol::PPTSERVER_CONNECTION_OK);
        BESDEBUG( "ppt", "Sent " << PPTProtocol::PPTSERVER_CONNECTION_OK << " to PPT client." << endl );
    }
    else
    {
        authenticateClient();
    }

    return 0;
}

void
PPTServer::authenticateClient()
{
#if defined HAVE_OPENSSL && defined NOTTHERE
    BESDEBUG( "ppt", "requiring secure connection: port = "
		     << _securePort << endl ) ;
    // let the client know that it needs to authenticate
    send( PPTProtocol::PPTSERVER_AUTHENTICATE ) ;

    // wait for the client request for the secure port
    // We are waiting for a ppt tocken requesting the secure port number.
    // The buffer doesn't need to be all that big. pcw - 05/31/08
    unsigned int ppt_buffer_size = 64 ;
    char *inBuff = new char[ppt_buffer_size] ;
    int bytesRead = _mySock->receive( inBuff, ppt_buffer_size ) ;
    string portRequest( inBuff, bytesRead ) ;
    delete [] inBuff ;
    if( portRequest != PPTProtocol::PPTCLIENT_REQUEST_AUTHPORT )
    {
	string err( "Secure connection ... expecting request for port" ) ;
	err += " client requested " + portRequest ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }

    // send the secure port number back to the client
    ostringstream portResponse ;
    portResponse << _securePort << PPTProtocol::PPT_COMPLETE_DATA_TRANSMITION ;
    send( portResponse.str() ) ;

    // create a secure server object and authenticate
    SSLServer server( _securePort, _cfile, _cafile, _kfile ) ;
    server.initConnection() ;
    server.closeConnection() ;

    // if it authenticates, good, if not, an exception is thrown, no need to
    // do anything else here.
#else
    string err = (string)"Authentication requested for this server "
                 + "but OpenSSL is not built into the server" ;
    throw BESInternalError( err, __FILE__, __LINE__ ) ;
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
    if( _secure )
    {
	BESIndent::Indent() ;
	strm << BESIndent::LMarg << "cert file: " << _cfile << endl ;
	strm << BESIndent::LMarg << "cert authority file: " << _cafile << endl ;
	strm << BESIndent::LMarg << "key file: " << _kfile << endl ;
	strm << BESIndent::LMarg << "secure port: " << _securePort << endl ;
	BESIndent::UnIndent() ;
    }
    PPTConnection::dump( strm ) ;
    BESIndent::UnIndent() ;
}

