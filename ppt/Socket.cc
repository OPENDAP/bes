// Socket.cc

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

#include "config.h"

#include <cstdio>
#include <cerrno>
#include <cstring>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "Socket.h"
#include "BESInternalError.h"

Socket::Socket( int socket, struct sockaddr *addr )
    : _socket( socket ),
      _connected( true ),
      _listening( false ),
      _addr_set( true )
{
    char ip[46];
    unsigned int port;
    /* ... */
    switch (addr->sa_family) {
	case AF_INET:
	    inet_ntop (AF_INET, &(((struct sockaddr_in *)addr)->sin_addr), ip, sizeof (ip));
	    port = ntohs (((struct sockaddr_in *)addr)->sin_port);
	    break;
	case AF_INET6:
	    inet_ntop (AF_INET6, &(((struct sockaddr_in6 *)addr)->sin6_addr), ip, sizeof (ip));
	    port = ntohs (((struct sockaddr_in6 *)addr)->sin6_port);
	    break;
	default:
	    snprintf (ip, sizeof (ip), "UNKNOWN FAMILY: %d", addr->sa_family);
	    port = 0;
	    break;
    }
    _port = port ;
    _ip = ip ;
}

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
    int bytes_written = write( _socket, send_str.c_str(), send_str.length() ) ;
    if( bytes_written == -1 )
    {
	string err( "socket failure, writing on stream socket" ) ;
	const char* error_info = strerror( errno ) ;
	if( error_info )
	    err += " " + (string)error_info ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
}

int Socket::receive(char *inBuff, const int inSize)
{
	int bytesRead = 0;

    //if ((bytesRead = read(_socket, inBuff, inSize)) < 1) {
	while ((bytesRead = read(_socket, inBuff, inSize)) < 1) {
	    if (errno == EINTR || errno == EAGAIN)
	        continue;

		string err("socket failure, reading on stream socket: ");
		const char *error_info = strerror(errno);
		if (error_info) err += " " + (string) error_info;
		throw BESInternalError(err, __FILE__, __LINE__);
	}
	//inBuff[bytesRead] = '\0' ;
	return bytesRead;
}

#if 0
// removed jhrg 5/5/11
void
Socket::sync()
{
#if 0
    // fsync does not work for sockets.
    fsync( _socket ) ;
#endif
}
#endif

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
Socket::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "Socket::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "socket: " << _socket << endl ;
    strm << BESIndent::LMarg << "is connected? " << _connected << endl ;
    strm << BESIndent::LMarg << "is listening? " << _listening << endl ;
    strm << BESIndent::LMarg << "socket address set? " << _addr_set << endl ;
    if( _addr_set )
    {
	strm << BESIndent::LMarg << "socket port: " << _port << endl;
	strm << BESIndent::LMarg << "socket ip: " << _ip << endl;
    }
    BESIndent::UnIndent() ;
}

