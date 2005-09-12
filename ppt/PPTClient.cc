// PPTClient.cc

// 2005 Copyright University Corporation for Atmospheric Research

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
