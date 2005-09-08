// TcpSocket.cc

// 2005 Copyright University Corporation for Atmospheric Research

#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <iostream>

using std::cout ;
using std::endl ;
using std::flush ;

#include "TcpSocket.h"
#include "SocketConfig.h"
#include "SocketException.h"

void
TcpSocket::connect()
{
    if( _listening )
    {
	string err( "Socket is already listening" ) ;
	throw SocketException( err, __FILE__, __LINE__ ) ;
    }

    if( _connected )
    {
	string err( "Socket is already connected" ) ;
	throw SocketException( err, __FILE__, __LINE__ ) ;
    }

    if( _host == "" )
	_host = "localhost" ;

    struct protoent *pProtoEnt ;
    struct sockaddr_in sin ;
    struct hostent *ph ;
    long address ;
    if( isdigit( _host[0] ) )
    {
	if( ( address = inet_addr( _host.c_str() ) ) == -1 )
	{
	    string err( "Invalid host ip address " ) ;
	    err += _host ;
	    throw SocketException( err, __FILE__, __LINE__ ) ;
	}
	sin.sin_addr.s_addr = address ;
	sin.sin_family = AF_INET ;
    }
    else
    {
	if( ( ph = gethostbyname( _host.c_str() ) ) == NULL )
	{
	    switch( h_errno )
	    {
		case HOST_NOT_FOUND:
		{
		    string err( "No such host " ) ;
		    err += _host ;
		    throw SocketException( err, __FILE__, __LINE__ ) ;
		}
		case TRY_AGAIN:
		{
		    string err( "Host " ) ;
		    err += _host + " is busy, try again later" ;
		    throw SocketException( err, __FILE__, __LINE__ ) ;
		}
		case NO_RECOVERY:
		{
		    string err( "DNS error for host " ) ;
		    err += _host ;
		    throw SocketException( err, __FILE__, __LINE__ ) ;
		}
		case NO_ADDRESS:
		{
		    string err( "No IP address for host " ) ;
		    err += _host ;
		    throw SocketException( err, __FILE__, __LINE__ ) ;
		}
		default:
		{
		    throw SocketException( "unknown error", __FILE__, __LINE__ ) ;
		}
	    }
	}
	else
	{
	    sin.sin_family = ph->h_addrtype ;
	    for( char **p =ph->h_addr_list; *p != NULL; p++ )
	    {
		struct in_addr in ;
		(void)memcpy( &in.s_addr, *p, sizeof( in.s_addr ) ) ;
		memcpy( (char*)&sin.sin_addr, (char*)&in, sizeof( in ) ) ;
	    }
	}
    }
    sin.sin_port = htons( _portVal ) ;
    pProtoEnt = getprotobyname( "tcp" ) ;
    int descript = socket( AF_INET, SOCK_STREAM, pProtoEnt->p_proto ) ;
    if( descript != -1 )
    {
	cout << "Trying to connect to socket ... " << flush ;
	if( ::connect( descript, (struct sockaddr*)&sin, sizeof( sin ) ) != 1 )
	{
	    cout << "OK: connected to " << _host
	         << " at " << inet_ntoa( sin.sin_addr ) << endl ;
	    _socket = descript ;
	    _connected = true ;
	}
    }
}

void
TcpSocket::listen()
{
    if( _connected )
    {
	string err( "Socket is already connected" ) ;
	throw SocketException( err, __FILE__, __LINE__ ) ;
    }

    if( _listening )
    {
	string err( "Socket is already listening" ) ;
	throw SocketException( err, __FILE__, __LINE__ ) ;
    }

    int on = 1 ;
    struct sockaddr_in server ;
    server.sin_family = AF_INET ;
    server.sin_addr.s_addr = INADDR_ANY ;
    struct servent *sir = 0 ;
    sir = getservbyport( _portVal, "tcp" ) ;
    if( sir )
    {
	string error = sir->s_name + (string)" is using my socket" ;
	throw SocketException( error, __FILE__, __LINE__ ) ;
    }
    server.sin_port = htons( _portVal ) ;
    cout << "Trying to get TCP socket ... " << flush ;
    _socket = socket( AF_INET, SOCK_STREAM, 0 ) ;
    if( _socket != -1 )
    {
	cout << "OK" << endl ;
	cout << "Trying to set TCP socket properties ... " << flush ;
	if( !setsockopt( _socket, SOL_SOCKET, SO_REUSEADDR,
	                 (char*)&on, sizeof( on ) ) )
	{
	    cout << "OK" << endl ;
	    cout << "Trying to bind TCP socket ... " << flush ;
	    if( bind( _socket, (struct sockaddr*)&server, sizeof server) != -1 )
	    {
		cout << "OK" << endl ;
		int length = sizeof( server ) ;
#ifdef _GETSOCKNAME_USES_SOCKLEN_T	
		if( getsockname( _socket, (struct sockaddr *)&server,
		                 (socklen_t *)&length ) == -1 )
#else
		if( getsockname( _socket, (struct sockaddr *)&server,
		                 &length ) == -1 )
#endif
		{
		    string error( "getting socket name" ) ;
		    const char* error_info = strerror( errno ) ;
		    if( error_info )
			error += " " + (string)error_info ;
		    throw SocketException( error, __FILE__, __LINE__ ) ;
		}
		cout << "Trying to listen in TCP socket ... " << flush ;
		if( ::listen( _socket, 5 ) == 0 )
		{
		    _listening = true ;
		    cout << "OK listening: port " ;
#ifdef SOCKET_USE_LTOA
		    char tempbuf[50] ;
		    ltoa( ntohs( server.sin_port ), tempbuf, 10 ) ;
		    cout << tempbuf << endl ;
#else
		    // seems that server.sin_port is uint16_t sin_port on
		    // Linux and may be alpha too which in Linux at least is
		    // defined as "typedef unsigned short int __uint16_t",
		    // let's cast it to int and writeout...
		    cout << (int)ntohs( server.sin_port ) << endl ;
#endif
		}
		else
		{
		    cout << "FAILED" << endl ;
		    string error( "could not listen TCP socket" ) ;
		    const char* error_info = strerror( errno ) ;
		    if( error_info )
			error += " " + (string)error_info ;
		    throw SocketException( error, __FILE__, __LINE__ ) ;
		}
	    }
	    else
	    {
		cout << "FAILED" << endl ;
		string error( "could not bind TCP socket" ) ;
		const char* error_info = strerror( errno ) ;
		if( error_info )
		    error += " " + (string)error_info ;
		throw SocketException( error, __FILE__, __LINE__ ) ;
	    }
	}
	else
	{
	    cout << "FAILED" << endl ;
	    string error( "cout not set SO_REUSEADDR on TCP socket" ) ;
	    const char* error_info = strerror( errno ) ;
	    if( error_info )
		error += " " + (string)error_info ;
	    throw SocketException( error, __FILE__, __LINE__ ) ;
	}
    }
    else
    {
	cout << "FAILED" << endl ;
	string error( "cout not create socket" ) ;
	const char *error_info = strerror( errno ) ;
	if( error_info )
	    error += " " + (string)error_info ;
	throw SocketException( error, __FILE__, __LINE__ ) ;
    }
}

// $Log: TcpSocket.cc,v $
