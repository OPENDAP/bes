// PPTServer.cc

// 2005 Copyright University Corporation for Atmospheric Research

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
