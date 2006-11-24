// Socket.cc

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
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

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
	strm << BESIndent::LMarg << "socket address: " << (void *)&_from << endl;
    }
    BESIndent::UnIndent() ;
}

