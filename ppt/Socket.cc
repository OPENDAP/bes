// Socket.cc

// 2005 Copyright University Corporation for Atmospheric Research

#include <unistd.h>
#include <errno.h>

#include "Socket.h"
#include "SocketException.h"

void
Socket::close()
{
    if( _connected )
    {
	::close( _socket ) ;
	_socket = 0 ;
	_connected = false ;
	_listening = false ;
    }
}

void
Socket::send( const string &str, int start, int end )
{
    string send_str = str.substr( start, end ) ;
    if( write( _socket, send_str.c_str(), send_str.length() ) == -1 )
    {
	string err( "socket failure, writing on stream socket" ) ;
	const char* error_info = strerror( errno ) ;
	if( error_info )
	    err += " " + (string)error_info ;
	throw SocketException( err, __FILE__, __LINE__ ) ;
    }
}

int	
Socket::receive( char *inBuff, int inSize )
{
    int bytesRead = 0 ;
    if( ( bytesRead = read( _socket, inBuff, inSize ) ) < 1 )
    {
	string err( "socket failure, reading on stream socket: " ) ;
	const char *error_info = strerror( errno ) ;
	if( error_info )
	    err += " " + (string)error_info ;
	throw SocketException( err, __FILE__, __LINE__ ) ;
    }
    inBuff[bytesRead] = '\0' ;
    return bytesRead ;
}

// $Log: Socket.cc,v $
