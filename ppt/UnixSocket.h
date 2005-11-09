// UnixSocket.h

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

#ifndef UnixSocket_h
#define UnixSocket_h 1

#include <string>

using std::string ;

#include "Socket.h"

class UnixSocket : public Socket
{
private:
    string			_unixSocket ;
    string			_tempSocket ;
public:
    				UnixSocket( const string &unixSocket )
				    : _unixSocket( unixSocket ),
				      _tempSocket( "" ) {}
    				UnixSocket( int socket,
				            const struct sockaddr_in &f )
				    : Socket( socket, f ),
				      _unixSocket( "" ),
				      _tempSocket( "" ) {}
    virtual			~UnixSocket() {}
    virtual void		connect() ;
    virtual void		close() ;
    virtual void		listen() ;

    virtual Socket *		newSocket( int socket,
                                           const struct sockaddr_in &f )
				{
				    return new UnixSocket( socket, f ) ;
				}
} ;

#endif // UnixSocket_h

// $Log: UnixSocket.h,v $
