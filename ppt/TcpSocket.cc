// TcpSocket.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>

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
    
    _connected = false;
    int descript = socket( AF_INET, SOCK_STREAM, pProtoEnt->p_proto ) ;
    
    if( descript == -1 ) 
    {
        string err("getting socket descriptor: ");
        const char* error_info = strerror(errno);
        if(error_info)
            err += (string)error_info;
        throw SocketException( err, __FILE__, __LINE__ ) ;
    } else {
        long holder;
        _socket = descript;

        //set socket to non-blocking mode
        holder = fcntl(_socket, F_GETFL, NULL);
        holder = holder | O_NONBLOCK;
        fcntl(_socket, F_SETFL, holder);
      
        int res = ::connect( descript, (struct sockaddr*)&sin, sizeof( sin ) );

      
        if( res == -1 ) 
        {
            if(errno == EINPROGRESS) {
	  
                fd_set write_fd ;
                struct timeval timeout ;
                int maxfd = _socket;
	  
                timeout.tv_sec = 5;
                timeout.tv_usec = 0;
	  
                FD_ZERO( &write_fd);
                FD_SET( _socket, &write_fd );
	  
                if( select( maxfd+1, NULL, &write_fd, NULL, &timeout) < 0 ) {
	  
                    //reset socket to blocking mode
                    holder = fcntl(_socket, F_GETFL, NULL);
                    holder = holder & (~O_NONBLOCK);
                    fcntl(_socket, F_SETFL, holder);
	    
                    //throw error - select could not resolve socket
                    string err( "selecting sockets: " ) ;
                    const char *error_info = strerror( errno ) ;
                    if( error_info )
                        err += (string)error_info ;
                    throw SocketException( err, __FILE__, __LINE__ ) ;

                } 
                else 
                {

                    //check socket status
                    socklen_t lon;
                    int valopt;
                    lon = sizeof(int);
                    getsockopt(_socket, SOL_SOCKET, SO_ERROR, (void*) &valopt, &lon);
	    
                    if(valopt) 
                    {

                        //reset socket to blocking mode
                        holder = fcntl(_socket, F_GETFL, NULL);
                        holder = holder & (~O_NONBLOCK);
                        fcntl(_socket, F_SETFL, holder);
	      
                        //throw error - did not successfully connect
                        string err("Did not successfully connect to server\n");
                        err += "Server may be down or you may be trying on the wrong port";
                        throw SocketException( err, __FILE__, __LINE__ ) ;
	      
                    } 
                    else 
                    {
                        //reset socket to blocking mode
                        holder = fcntl(_socket, F_GETFL, NULL);
                        holder = holder & (~O_NONBLOCK);
                        fcntl(_socket, F_SETFL, holder);
	      
                        //succesful connetion to server
                        _connected = true;
                    }
                }
            } 
            else 
            {

                //reset socket to blocking mode
                holder = fcntl(_socket, F_GETFL, NULL);
                holder = holder & (~O_NONBLOCK);
                fcntl(_socket, F_SETFL, holder);
	  
                //throw error - errno was not EINPROGRESS
                string err("socket connect: ");
                const char* error_info = strerror(errno);
                if(error_info)
                    err += (string)error_info;
                throw SocketException( err, __FILE__, __LINE__ ) ;
            }
        }
        else
        {
            // The socket connect request completed immediately
            // even that the socket was in non-blocking mode
            
            //reset socket to blocking mode
            holder = fcntl(_socket, F_GETFL, NULL);
            holder = holder & (~O_NONBLOCK);
            fcntl(_socket, F_SETFL, holder);
            _connected = true;
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
    _socket = socket( AF_INET, SOCK_STREAM, 0 ) ;
    if( _socket != -1 )
    {
	if( !setsockopt( _socket, SOL_SOCKET, SO_REUSEADDR,
	                 (char*)&on, sizeof( on ) ) )
	{
	    if( bind( _socket, (struct sockaddr*)&server, sizeof server) != -1 )
	    {
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
		if( ::listen( _socket, 5 ) == 0 )
		{
		    _listening = true ;
		}
		else
		{
		    string error( "could not listen TCP socket" ) ;
		    const char* error_info = strerror( errno ) ;
		    if( error_info )
			error += " " + (string)error_info ;
		    throw SocketException( error, __FILE__, __LINE__ ) ;
		}
	    }
	    else
	    {
		string error( "could not bind TCP socket" ) ;
		const char* error_info = strerror( errno ) ;
		if( error_info )
		    error += " " + (string)error_info ;
		throw SocketException( error, __FILE__, __LINE__ ) ;
	    }
	}
	else
	{
	    string error( "could not set SO_REUSEADDR on TCP socket" ) ;
	    const char* error_info = strerror( errno ) ;
	    if( error_info )
		error += " " + (string)error_info ;
	    throw SocketException( error, __FILE__, __LINE__ ) ;
	}
    }
    else
    {
	string error( "could not create socket" ) ;
	const char *error_info = strerror( errno ) ;
	if( error_info )
	    error += " " + (string)error_info ;
	throw SocketException( error, __FILE__, __LINE__ ) ;
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
TcpSocket::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "TcpSocket::dump - ("
         << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "host: " << _host << endl ;
    strm << BESIndent::LMarg << "port: " << _portVal << endl ;
    Socket::dump( strm ) ;
    BESIndent::UnIndent() ;
}

