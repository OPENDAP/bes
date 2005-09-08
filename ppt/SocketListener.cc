// SocketListener.cc

// 2005 Copyright University Corporation for Atmospheric Research

#include <iostream>
#include <ctype.h>

#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>

using std::cout ;
using std::endl ;
using std::flush ;

#include "SocketListener.h"
#include "SocketException.h"
#include "Socket.h"
#include "SocketConfig.h"

SocketListener::SocketListener()
    : _accepting( false )
{
}

SocketListener::~SocketListener()
{
}

void
SocketListener::listen( Socket *s )
{
    if( _accepting )
    {
	string err( "Already accepting connections ... no more sockets can be added" ) ;
	throw SocketException( err, __FILE__, __LINE__ ) ;
    }

    if( s && !s->isConnected() && !s->isListening() )
    {
	s->listen() ;
	_socket_list[s->getSocketDescriptor()] = s ;
    }
    else
    {
	if( !s )
	{
	    throw SocketException( "null socket passed", __FILE__, __LINE__ ) ;
	}
	else if( s->isConnected() )
	{
	    string err( "socket already connected, cannot listen" ) ;
	    throw SocketException( err, __FILE__, __LINE__ ) ;
	}
	else if( s->isListening() )
	{
	    string err( "socket already listening" ) ;
	    throw SocketException( err, __FILE__, __LINE__ ) ;
	}
    }
}

Socket *
SocketListener::accept()
{
    int msgsock ;

    fd_set read_fd ;
    struct timeval timeout ;

    int maxfd ;

    for(;;)
    {
	cout << "accepting connections ... " << flush ;

	timeout.tv_sec = 120 ;
	timeout.tv_usec = 0 ;

	FD_ZERO( &read_fd ) ;

	maxfd = 0 ;
	Socket_citer iter = _socket_list.begin() ;
	for( ; iter != _socket_list.end(); iter++ )
	{
	    Socket *s_ptr = (*iter).second ;
	    int s = s_ptr->getSocketDescriptor() ;
	    if( s > maxfd ) maxfd = s ;
	    FD_SET( s, &read_fd ) ;
	}

	if( select( maxfd+1, &read_fd,
	            (fd_set*)NULL, (fd_set*)NULL, &timeout) < 0 )
	{
	    string err( "selecting sockets " ) ;
	    const char *error_info = strerror( errno ) ;
	    if( error_info )
		err += " " + (string)error_info ;
	    throw SocketException( err, __FILE__, __LINE__ ) ;
	}

	iter = _socket_list.begin() ;
	for( ; iter != _socket_list.end(); iter++ )
	{
	    Socket *s_ptr = (*iter).second ;
	    int s = s_ptr->getSocketDescriptor() ;
	    if ( FD_ISSET( s, &read_fd ) )  
	    {    
		struct sockaddr_in from ;
		int len_from = sizeof( from ) ;
#ifdef _ACCEPT_USES_SOCKLEN_T 
		msgsock = ::accept( s, (struct sockaddr *)&from,
				    (socklen_t *)&len_from ) ;
#else
		msgsock = ::accept( s, (struct sockaddr *)&from,
		                    &len_from ) ;
#endif
		cout << "ACCEPTED" << endl ;
		return s_ptr->newSocket( msgsock, from ) ;
	    }
	}
    }
    return 0 ;
}

// $Log: SocketListener.cc,v $
