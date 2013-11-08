// SocketListener.cc

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

#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <cstring>
#include <cerrno>

#if 0
#ifdef _ACCEPT_USES_SOCKLEN_T
#define SOCK_TYPE socklen_t
#else
#define SOCK_TYPE int
#endif
#endif

#include "SocketListener.h"
#include "BESInternalError.h"
#include "Socket.h"
#include "SocketConfig.h"
#include "BESDebug.h"

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
	string err = (string)"Already accepting connections, "
	             + "no more sockets can be added" ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
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
	    throw BESInternalError( "null socket passed", __FILE__, __LINE__ ) ;
	}
	else if( s->isConnected() )
	{
	    string err( "socket already connected, cannot listen" ) ;
	    throw BESInternalError( err, __FILE__, __LINE__ ) ;
	}
	else if( s->isListening() )
	{
	    string err( "socket already listening" ) ;
	    throw BESInternalError( err, __FILE__, __LINE__ ) ;
	}
    }
}

/** Use the select() system call to wait for an incoming connection */
Socket *
SocketListener::accept()
{
    BESDEBUG( "ppt", "SocketListener::accept() - START" << endl );

    int msgsock ;

    fd_set read_fd ;
    struct timeval timeout ;

    int maxfd ;

    for(;;)
    {
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

#if 0
        // jhrg 5/16/11
        //if( select( maxfd+1, &read_fd,
        //           (fd_set*)NULL, (fd_set*)NULL, &timeout) < 0 )
#endif

        while (select(maxfd + 1, &read_fd, (fd_set*) NULL, (fd_set*) NULL, &timeout) < 0) {
            switch (errno) {

            case EAGAIN:	// rerun select on interrupted calls, ...
                BESDEBUG( "ppt", "SocketListener::accept() - select encountered EAGAIN" << endl );
                break;
            case EINTR:
                BESDEBUG( "ppt", "SocketListener::accept() - select encountered EINTR" << endl );
                break;
#if 0
            case EBADF:		// or exit on error
            case EINVAL:
#endif
            default: {
                string err("select: ");
                const char *error_info = strerror(errno);
                if (error_info) err += (string) error_info;
                throw BESInternalError(err, __FILE__, __LINE__);
            }
            }
        }
        BESDEBUG( "ppt", "SocketListener::accept() - select() was successful. Processing sockets..." << endl );

        iter = _socket_list.begin() ;
        for( ; iter != _socket_list.end(); iter++ )
        {
            Socket *s_ptr = (*iter).second ;
            int s = s_ptr->getSocketDescriptor() ;
            if ( FD_ISSET( s, &read_fd ) )
            {
                BESDEBUG( "ppt", "SocketListener::accept() - FD_ISSET for "<< s_ptr->getIp() <<":"<<s_ptr->getPort() << endl );

                struct sockaddr from ;
                socklen_t len_from = sizeof( from ) ;

                BESDEBUG( "ppt", "SocketListener::accept() - Attempting to accept on "<< s_ptr->getIp() <<":"<<s_ptr->getPort() << endl );

                while ((msgsock = ::accept(s, &from, &len_from)) < 0) {
                    if (errno == EINTR) {
                        BESDEBUG( "ppt", "SocketListener::accept() - accept() was interrupted" << endl );

                        continue;
                    }
                    else {
                        string err("accept: ");
                        const char *error_info = strerror(errno);
                        if (error_info)
                            err += (string) error_info;
                        throw BESInternalError(err, __FILE__, __LINE__);
                    }
                }
                BESDEBUG( "ppt", "SocketListener::accept() - END (returning new Socket)" << endl );

                return s_ptr->newSocket( msgsock, (struct sockaddr *)&from ) ;
            }
        }
    }
    BESDEBUG( "ppt", "SocketListener::accept() - END (returning 0)" << endl );

    return 0 ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
SocketListener::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "SocketListener::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    if( _socket_list.size() )
    {
	strm << BESIndent::LMarg << "registered sockets:" << endl ;
	Socket_citer i = _socket_list.begin() ;
	Socket_citer ie = _socket_list.end() ;
	for( ; i != ie; i++ )
	{
	    strm << BESIndent::LMarg << "socket: " << (*i).first ;
	    Socket *s_ptr = (*i).second ;
	    s_ptr->dump( strm ) ;
	}
    }
    else
    {
	strm << BESIndent::LMarg << "registered sockets: none" << endl ;
    }
    BESIndent::UnIndent() ;
}

