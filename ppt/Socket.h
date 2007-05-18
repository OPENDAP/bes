// Socket.h

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

#ifndef Socket_h
#define Socket_h 1

#include <netinet/in.h>

#include <string>

using std::string ;

#include "BESObj.h"

class Socket : public BESObj
{
protected:
    int				_socket ;
    bool			_connected ;
    bool			_listening ;
    struct sockaddr_in		_from ;
    bool			_addr_set ;
public:
    				Socket()
				    : _socket( 0 ),
				      _connected( false ),
				      _listening( false ),
				      _addr_set( false ) {}
				Socket( int socket,
				        const struct sockaddr_in & )
				    : _socket( socket ),
				      _connected( true ),
				      _listening( false ),
				      _addr_set( true ) {}
    virtual			~Socket() { close() ; }
    virtual void		connect() = 0 ;
    virtual bool		isConnected() { return _connected ; }
    virtual void		listen() = 0 ;
    virtual bool		isListening() { return _listening ; }
    virtual void		close() ;
    virtual void		send( const string &str, int start, int end ) ;
    virtual int			receive( char *inBuff, int inSize ) ;
    virtual int			getSocketDescriptor()
				{
				    return _socket ;
				}

    virtual Socket *		newSocket( int socket,
                                           const struct sockaddr_in &f ) = 0 ;

    virtual void		dump( ostream &strm ) const ;
} ;

#endif // Socket_h

