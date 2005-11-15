// SocketListener.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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

/** Use the select() system call to wait for an incoming connection */
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
