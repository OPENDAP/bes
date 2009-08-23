// TcpSocket.cc

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
#include <fcntl.h>
#include <netinet/tcp.h>

#ifdef HAVE_LIBWRAP
extern "C" {
#include "tcpd.h"
int allow_severity;
int deny_severity;
}
#endif

#include <cstring>
#include <cerrno>

#include <iostream>
#include <sstream>

using std::cerr ;
using std::endl ;
using std::istringstream ;

#include "TcpSocket.h"
#include "SocketConfig.h"
#include "TheBESKeys.h"
#include "BESDebug.h"
#include "BESInternalError.h"
#include "BESInternalFatalError.h"

void
TcpSocket::connect()
{
    if( _listening )
    {
	string err( "Socket is already listening" ) ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }

    if( _connected )
    {
	string err( "Socket is already connected" ) ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
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
	    throw BESInternalError( err, __FILE__, __LINE__ ) ;
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
                        throw BESInternalError( err, __FILE__, __LINE__ ) ;
                    }
		case TRY_AGAIN:
                    {
                        string err( "Host " ) ;
                        err += _host + " is busy, try again later" ;
                        throw BESInternalError( err, __FILE__, __LINE__ ) ;
                    }
		case NO_RECOVERY:
                    {
                        string err( "DNS error for host " ) ;
                        err += _host ;
                        throw BESInternalError( err, __FILE__, __LINE__ ) ;
                    }
		case NO_ADDRESS:
                    {
                        string err( "No IP address for host " ) ;
                        err += _host ;
                        throw BESInternalError( err, __FILE__, __LINE__ ) ;
                    }
		default:
                    {
                        throw BESInternalError( "unknown error", __FILE__, __LINE__ ) ;
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
    if( !pProtoEnt )
    {
	string err( "Error retreiving tcp protocol information" ) ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
    
    _connected = false;
    int descript = socket( AF_INET, SOCK_STREAM, pProtoEnt->p_proto ) ;

    /*
    unsigned int sockbufsize = 0 ;
    int size = sizeof(int) ;
    int err = getsockopt( descript, IPPROTO_TCP, TCP_MAXSEG,
			  (void *)&sockbufsize, (socklen_t*)&size) ;
    cerr << "max size of tcp segment = " << sockbufsize << endl ;
    */
    
    if( descript == -1 ) 
    {
        string err("getting socket descriptor: ");
        const char* error_info = strerror(errno);
        if(error_info)
            err += (string)error_info;
        throw BESInternalError( err, __FILE__, __LINE__ ) ;
    } else {
        long holder;
        _socket = descript;

        //set socket to non-blocking mode
        holder = fcntl(_socket, F_GETFL, NULL);
        holder = holder | O_NONBLOCK;
        fcntl(_socket, F_SETFL, holder);
    
	// we must set the send and receive buffer sizes before the connect call
	setTcpRecvBufferSize( ) ;
	setTcpSendBufferSize( ) ;
      
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
                    throw BESInternalError( err, __FILE__, __LINE__ ) ;

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
                        throw BESInternalError( err, __FILE__, __LINE__ ) ;
	      
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
                throw BESInternalError( err, __FILE__, __LINE__ ) ;
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
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }

    if( _listening )
    {
	string err( "Socket is already listening" ) ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
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
	throw BESInternalError( error, __FILE__, __LINE__ ) ;
    }
    server.sin_port = htons( _portVal ) ;
    _socket = socket( AF_INET, SOCK_STREAM, 0 ) ;
    if( _socket != -1 )
    {
	if( setsockopt( _socket, SOL_SOCKET, SO_REUSEADDR,
	                 (char*)&on, sizeof( on ) ) )
	{
	    string error( "could not set SO_REUSEADDR on TCP socket" ) ;
	    const char* error_info = strerror( errno ) ;
	    if( error_info )
		error += " " + (string)error_info ;
	    throw BESInternalError( error, __FILE__, __LINE__ ) ;
	}

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
		throw BESInternalError( error, __FILE__, __LINE__ ) ;
	    }

	    // The send and receive buffer sizes must be set before the call to
	    // ::listen.
	    setTcpRecvBufferSize( ) ;
	    setTcpSendBufferSize( ) ;

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
		throw BESInternalError( error, __FILE__, __LINE__ ) ;
	    }
	}
	else
	{
	    string error( "could not bind TCP socket" ) ;
	    const char* error_info = strerror( errno ) ;
	    if( error_info )
		error += " " + (string)error_info ;
	    throw BESInternalError( error, __FILE__, __LINE__ ) ;
	}
    }
    else
    {
	string error( "could not create socket" ) ;
	const char *error_info = strerror( errno ) ;
	if( error_info )
	    error += " " + (string)error_info ;
	throw BESInternalError( error, __FILE__, __LINE__ ) ;
    }
}

/** @brief set the size of the TCP receive buffer
 *
 * Two parameters are set in the BES configuration file. They are:
 *
 * BES.SetSockRecvSize=Yes|No
 * BES.SockRecvSize=<num>
 *
 * The first tells us if the BES should use the buffer size specified in the
 * configuration file. If set to yes, then get the second parameter and use
 * setsockopt to set the receive buffer size. Do NOT set the internal buffer
 * sizes here. This will happen in the Get calls above that call getsockopt.
 *
 * We are trusting that the administrator is not setting the size too small
 * or too big.
 *
 * @throws BESInternalFatalError if unable to set the buffer size as
 * specified in the configuration file
 */
void
TcpSocket::setTcpRecvBufferSize()
{
    if( !_haveRecvBufferSize )
    {
	bool found = false ;
	string setit ;
	try
	{
	    setit = TheBESKeys::TheKeys()->get_key( "BES.SetSockRecvSize", found );
	}
	catch( ... )
	{
	    // ignore any exceptions caught trying to get this key. The
	    // client also calls this function.
	    setit = "No" ;
	}
	if( setit == "Yes" || setit == "yes" || setit == "Yes" )
	{
	    string sizestr
		= TheBESKeys::TheKeys()->get_key( "BES.SockRecvSize", found ) ;
	    istringstream sizestrm( sizestr ) ;
	    unsigned int sizenum = 0 ;
	    sizestrm >> sizenum ;
	    if( !sizenum )
	    {
		string err = "Socket Recv Size malformed: " + sizestr ;
		throw BESInternalFatalError( err, __FILE__, __LINE__ ) ;
	    }

	    // call setsockopt
	    int err = setsockopt( _socket, SOL_SOCKET, SO_RCVBUF,
			      (char *)&sizenum, (socklen_t)sizeof(sizenum) ) ;
	    int myerrno = errno ;
	    if( err == -1 )
	    {
		char *serr = strerror( myerrno ) ;
		string err = "Failed to set the socket receive buffer size: " ;
		if( serr )
		    err += serr ;
		else
		    err += "unknow error occurred" ;
		throw BESInternalFatalError( err, __FILE__, __LINE__ ) ;
	    }

	    BESDEBUG( "ppt", "Tcp receive buffer size set to "
			     << (unsigned long)sizenum << endl ) ;
	}
    }
}

/** @brief set the size of the TCP send buffer
 *
 * Two parameters are set in the BES configuration file. They are:
 *
 * BES.SetSockSendSize?=Yes|No
 * BES.SockSendSize?=<num>
 *
 * The first tells us if the BES should use the buffer size specified in the
 * configuration file. If set to yes, then get the second parameter and use
 * setsockopt to set the send buffer size. If set to no call getsockopt to
 * get the size of the buffer.
 *
 * We are trusting that the administrator is not setting the size too small
 * or too big.
 *
 * @throws BESInternalFatalError if unable to set the buffer size as
 * specified in the configuration file
 */
void
TcpSocket::setTcpSendBufferSize()
{
    bool found = false ;
    string setit ;
    try
    {
	setit = TheBESKeys::TheKeys()->get_key( "BES.SetSockSendSize", found );
    }
    catch( ... )
    {
	// ignore any exceptions caught trying to get this key. The
	// client also calls this function.
	setit = "No" ;
    }
    if( setit == "Yes" || setit == "yes" || setit == "Yes" )
    {
	string sizestr
	    = TheBESKeys::TheKeys()->get_key( "BES.SockSendSize", found ) ;
	istringstream sizestrm( sizestr ) ;
	unsigned int sizenum = 0 ;
	sizestrm >> sizenum ;
	if( !sizenum )
	{
	    string err = "Socket Send Size malformed: " + sizestr ;
	    throw BESInternalFatalError( err, __FILE__, __LINE__ ) ;
	}

	// call setsockopt
	int err = setsockopt( _socket, SOL_SOCKET, SO_SNDBUF,
			  (char *)&sizenum, (socklen_t)sizeof(sizenum) ) ;
	int myerrno = errno ;
	if( err == -1 )
	{
	    char *serr = strerror( myerrno ) ;
	    string err = "Failed to set the socket send buffer size: " ;
	    if( serr )
		err += serr ;
	    else
		err += "unknow error occurred" ;
	    throw BESInternalFatalError( err, __FILE__, __LINE__ ) ;
	}

	BESDEBUG( "ppt", "Tcp send buffer size set to "
			 << (unsigned long)sizenum << endl ) ;
    }
}

/** @brief get the tcp receive buffer size using getsockopt
 *
 * Get the receive buffer size for this socket descriptor using the getsockopt
 * system function. We do this to maximize the performance of TCP sockets
 *
 * @throws BESInternalFatalError if we are unable to get the size of the
 * receive buffer
 */
unsigned int	
TcpSocket::getRecvBufferSize()
{
    if( !_haveRecvBufferSize )
    {
	// call getsockopt and set the internal variables to the result
	unsigned int sizenum = 0 ;
	socklen_t sizelen = sizeof(sizenum) ;
	int err = getsockopt( _socket, SOL_SOCKET, SO_RCVBUF,
			  (char *)&sizenum, (socklen_t *)&sizelen ) ;
	int myerrno = errno ;
	if( err == -1 )
	{
	    char *serr = strerror( myerrno ) ;
	    string err = "Failed to get the socket receive buffer size: " ;
	    if( serr )
		err += serr ;
	    else
		err += "unknow error occurred" ;
	    throw BESInternalFatalError( err, __FILE__, __LINE__ ) ;
	}

	BESDEBUG( "ppt", "Tcp receive buffer size is "
			 << (unsigned long)sizenum << endl ) ;

	_haveRecvBufferSize = true ;
	_recvBufferSize = sizenum ;
    }
    return _recvBufferSize ;
}

/** @brief get the tcp send buffer size using getsockopt
 *
 * Get the send buffer size for this socket descriptor using the getsockopt
 * system function. We do this to maximize the performance of TCP sockets
 *
 * @throws BESInternalFatalError if we are unable to get the size of the
 * send buffer
 */
unsigned int
TcpSocket::getSendBufferSize()
{
    if( !_haveSendBufferSize )
    {
	// call getsockopt and set the internal variables to the result
	unsigned int sizenum = 0 ;
	socklen_t sizelen = sizeof(sizenum) ;
	int err = getsockopt( _socket, SOL_SOCKET, SO_SNDBUF,
			  (char *)&sizenum, (socklen_t *)&sizelen ) ;
	int myerrno = errno ;
	if( err == -1 )
	{
	    char *serr = strerror( myerrno ) ;
	    string err = "Failed to get the socket send buffer size: " ;
	    if( serr )
		err += serr ;
	    else
		err += "unknow error occurred" ;
	    throw BESInternalFatalError( err, __FILE__, __LINE__ ) ;
	}

	BESDEBUG( "ppt", "Tcp send buffer size is "
			  << (unsigned long)sizenum << endl ) ;

	_haveSendBufferSize = true ;
	_sendBufferSize = sizenum ;
    }
    return _sendBufferSize ;
}

/** @brief is there any wrapper code for unix sockets
 *
 */
bool
TcpSocket::allowConnection()
{
    bool retval = true ;

#ifdef HAVE_LIBWRAP
    struct request_info req ;
    request_init( &req, RQ_DAEMON, "besdaemon", RQ_FILE,
		  getSocketDescriptor(), 0 ) ;
    fromhost() ;

    if( STR_EQ( eval_hostname(), paranoid ) && hosts_access() )
    {
	retval = false ;
    }
#endif

    return retval ;
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
    strm << BESIndent::LMarg << "have recv buffer size: " << _haveRecvBufferSize
         << endl ;
    strm << BESIndent::LMarg << "recv buffer size: " << _recvBufferSize
         << endl ;
    strm << BESIndent::LMarg << "have send buffer size: " << _haveSendBufferSize
         << endl ;
    strm << BESIndent::LMarg << "send buffer size: " << _sendBufferSize
         << endl ;
    Socket::dump( strm ) ;
    BESIndent::UnIndent() ;
}

