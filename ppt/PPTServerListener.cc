// -*- C++ -*-

// (c) COPYRIGHT UCAR/HAO 1993-2002
// Please read the full copyright statement in the file COPYRIGHT.

#include <unistd.h>
#include <netdb.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h> // For Unix sockets


#include <string>

#include "PPTSocket.h"
#include "PPTException.h"
#include "PPTUtilities.h"

#include "PPTConfig.h"

#include "PPTServerListener.h"

extern "C" {

int errno;
}

PPTServerListener::PPTServerListener()
{
}

PPTServerListener::~PPTServerListener()
{
    close( _unix_socket ) ;
    unlink( _unix_socket_name.c_str() ) ; 
}

void
PPTServerListener::initializeTCPSocket( int p )
{
    int on = 1 ;
    struct sockaddr_in server ;
    server.sin_family = AF_INET ;
    server.sin_addr.s_addr = INADDR_ANY ;
    int port = p ;
    struct servent *sir = 0 ;
    sir = getservbyport( port, "tcp" ) ;
    if( sir )
    {
	string error = sir->s_name + (string)" is using my socket" ;
	throw PPTException( __FILE__, __LINE__, error ) ;
    }
    server.sin_port = htons( port ) ;
    write_out( "Trying to get TCP socket..." ) ;
    _socket = socket( AF_INET, SOCK_STREAM, 0 ) ;
    if( _socket != -1 )
    {
	write_out( "OK getting TCP socket\n" ) ;
	write_out( "Trying to set TCP socket properties..." ) ;
	if( !setsockopt( _socket, SOL_SOCKET, SO_REUSEADDR,
	                 (char*)&on, sizeof( on ) ) )
	{
	    write_out( "OK setting TCP socket properties\n" ) ;
	    write_out( "Trying to bind TCP socket..." ) ;
	    if( bind( _socket, (struct sockaddr*)&server, sizeof server) != -1 )
	    {
		write_out( "OK binding TCP socket\n" ) ;
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
		    throw PPTException( __FILE__, __LINE__, error ) ;
		}
		write_out( "Trying to listen in TCP socket..." ) ;
		if( listen( _socket, 5 ) == 0 )
		{
		    write_out( "OK listening: port " ) ;
#ifdef PPT_USE_LTOA
		    char tempbuf[50] ;
		    ltoa( ntohs( server.sin_port ), tempbuf, 10 ) ;
		    strcat( tempbuf, "\n" ) ;
		    write_out( tempbuf ) ;
#else
		    // seems that server.sin_port is uint16_t sin_port on
		    // Linux and may be alpha too which in Linux at least is
		    // defined as "typedef unsigned short int __uint16_t",
		    // let's cast it to int and writeout...
		    write_out( (int)ntohs( server.sin_port ) ) ;
		    // Let's add a carriage return so it looks nice ;
		    write_out( "\n" ) ;
#endif
		}
		else
		{
		    string error( "could not listen TCP socket" ) ;
		    const char* error_info = strerror( errno ) ;
		    if( error_info )
			error += " " + (string)error_info ;
		    throw PPTException( __FILE__, __LINE__, error ) ;
		}
	    }
	    else
	    {
		string error( "could not bind TCP socket" ) ;
		const char* error_info = strerror( errno ) ;
		if( error_info )
		    error += " " + (string)error_info ;
		throw PPTException( __FILE__, __LINE__, error ) ;
	    }
	}
	else
	{
	    string error( "cout not set SO_REUSEADDR on TCP socket" ) ;
	    const char* error_info = strerror( errno ) ;
	    if( error_info )
		error += " " + (string)error_info ;
	    throw PPTException( __FILE__, __LINE__, error ) ;
	}
    }
    else
    {
	string error( "cout not create socket" ) ;
	const char *error_info = strerror( errno ) ;
	if( error_info )
	    error += " " + (string)error_info ;
	throw PPTException( __FILE__, __LINE__, error ) ;
    }
}

void
PPTServerListener::initializeUnixSocket()
{
    int on = 1 ;
    static struct sockaddr_un server_add ;
    write_out( "Trying to get Unix socket..." ) ;
    _unix_socket = socket( AF_UNIX,SOCK_STREAM, 0 ) ;
    if( _unix_socket >= 0 )
    {
	write_out( "OK getting Unix socket\n" ) ;
	server_add.sun_family = AF_UNIX;
	strcpy( server_add.sun_path, _unix_socket_name.c_str() ) ;
	unlink( _unix_socket_name.c_str() ) ;
	write_out( "Trying to set Unix socket properties..." ) ;
	if( !setsockopt( _unix_socket, SOL_SOCKET, SO_REUSEADDR,
	                 (char*)&on, sizeof( on ) ) )
	{
	    write_out( "OK setting Unix socket properties\n" ) ;
	    write_out( "Trying to bind Unix socket..." ) ;
	    if( bind( _unix_socket, (struct sockaddr*)&server_add, sizeof( server_add.sun_family ) + strlen( server_add.sun_path ) ) != -1)
	    {
		string is_ok = (string)"OK binding Unix socket " + _unix_socket_name + (string)"\n" ;
		write_out( is_ok.c_str() ) ;
		write_out( "Trying to listen in Unix socket..." ) ;
		if( listen( _unix_socket, 5 ) == 0 )
		{
		    write_out( "OK listening to Unix socket\n" ) ;
		}
		else
		{
		    string error( "could not listen Unix socket" ) ;
		    const char* error_info = strerror( errno ) ;
		    if( error_info )
			error += " " + (string)error_info ;
		    throw PPTException( __FILE__, __LINE__, error ) ;
		}
	    }
	    else
	    {
		string error( "could not bind Unix socket" ) ;
		const char* error_info = strerror( errno ) ;
		if( error_info )
		    error += " " + (string)error_info ;
		throw PPTException( __FILE__, __LINE__, error ) ;
	    }
	}
	else
	{
	    string error( "cout not set SO_REUSEADDR on Unix socket" ) ;
	    const char *error_info = strerror( errno ) ;
	    if( error_info )
		error += " " + (string)error_info ;
	    throw PPTException( __FILE__, __LINE__, error ) ;
	}
    }
    else
    {
	string error( "could not get Unix socket" ) ;
	const char *error_info = strerror( errno ) ;
	if( error_info )
	    error += " " + (string)error_info ;
	throw PPTException( __FILE__, __LINE__, error ) ;
    }
}

void
PPTServerListener::startListening( int p, const char *u_sock )
{
    initializeTCPSocket( p ) ;
    _unix_socket_name = u_sock ;
    initializeUnixSocket() ;
}

PPTSocket
PPTServerListener::acceptConnections()
{
    int msgsock ;

    fd_set read_fd ;
    struct timeval timeout ;

    int maxfd ;

    for(;;)
    {
	timeout.tv_sec = 120 ;
	timeout.tv_usec = 0 ;

	FD_ZERO( &read_fd ) ;

	maxfd = _socket ;

	if( _unix_socket > _socket )
	    maxfd = _unix_socket ;

	FD_SET( _socket, &read_fd ) ;
	FD_SET( _unix_socket, &read_fd ) ;

	if( select( maxfd+1, &read_fd,
	            (fd_set*)NULL, (fd_set*)NULL, &timeout) < 0 )
	{
	    string error( "selecting sockets " ) ;
	    const char *error_info = strerror( errno ) ;
	    if( error_info )
		error += " " + (string)error_info ;
	    throw PPTException( __FILE__, __LINE__, error ) ;
	}

#ifdef _ACCEPT_USES_SOCKLEN_T 
	if ( FD_ISSET( _socket, &read_fd ) )  
	{    
	    struct sockaddr_in from ;
	    int len_from = sizeof( from ) ;
	    msgsock = accept( _socket, (struct sockaddr *)&from,
	                     (socklen_t *)&len_from ) ;
	    PPTSocket ts( msgsock ) ;
	    ts._type = PPT_TCP_SOCKET ;
	    ts.setAddress( from ) ;
	    return ts ;
	}
#else
	if( FD_ISSET( _socket, &read_fd ) ) 
	{     
	    struct sockaddr_in from ;
	    int len_from = sizeof( from ) ;
	    msgsock = accept( _socket, (struct sockaddr *)&from, &len_from ) ;
	    PPTSocket ts( msgsock ) ;
	    ts._type = PPT_TCP_SOCKET ;
	    ts.setAddress( from ) ;
	    return ts ;
	}
#endif

#ifdef _ACCEPT_USES_SOCKLEN_T 
	if( FD_ISSET( _unix_socket, &read_fd ) ) 
	{     
	    struct sockaddr_in from ;
	    int len_from = sizeof( from ) ;
	    msgsock = accept( _unix_socket, (struct sockaddr *)&from,
	                      (socklen_t *)&len_from ) ;
	    if( msgsock != -1 )
	    {
		PPTSocket ts( msgsock ) ;
		ts._type = PPT_UNIX_SOCKET ;
		ts.setAddress( from ) ;
		return ts ;
	    }
	    else
	    {
		string error( "accept failed" ) ;
		const char *error_info = strerror( errno ) ;
		if( error_info )
		    error += " " + (string)error_info ;
		throw PPTException( __FILE__, __LINE__, error ) ;
	    }
	}
#else
	if( FD_ISSET( _unix_socket, &read_fd ) ) 
	{     
	    struct sockaddr_in from ;
	    int len_from = sizeof( from ) ;
	    msgsock = accept( _unix_socket, (struct sockaddr *)&from,
	                      &len_from ) ;
	    if( msgsock != -1 )
	    {
		PPTSocket ts( msgsock ) ;
		ts._type = PPT_UNIX_SOCKET ;
		ts.setAddress( from ) ;
		return ts ;
	    }
	    else
	    {
		string error( "accept failed" ) ;
		const char *error_info = strerror( errno ) ;
		if( error_info )
		    error += " " + (string)error_info ;
		throw PPTException( __FILE__, __LINE__, error ) ;
	    }
	}
#endif
    }
}

