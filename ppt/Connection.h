// Connection.h

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

#ifndef Connection_h
#define Connection_h 1

#include <iostream>
#include <string>
#include <map>

using std::ostream ;
using std::string ;
using std::map ;

#include "BESObj.h"
#include "Socket.h"

class Connection : public BESObj
{
protected:
    Socket			*_mySock ;
    ostream			*_out ;
    bool			_brokenPipe ;

    				Connection()
				    : _mySock( 0 ),
				      _out( 0 ),
				      _brokenPipe( false ) {}

    virtual void		send( const string &buffer ) = 0 ;
    virtual void		sendChunk( const string &buffer,
					   map<string,string> &extensions ) = 0 ;
public:
    virtual			~Connection() {}

    virtual void		initConnection() = 0 ;
    virtual void		closeConnection() = 0 ;

    virtual string		exit() = 0 ;

    virtual void		send( const string &buffer,
				      map<string,string> &extensions ) = 0 ;
    virtual void		sendExtensions( map<string,string> &extensions ) = 0 ;
    virtual void		sendExit() = 0 ;
    virtual bool		receive( map<string,string> &extensions,
                                         ostream *strm = 0 ) = 0 ;

    virtual Socket *		getSocket()
				{
				    return _mySock ;
				}

    virtual bool		isConnected()
				{
				    if( _mySock )
					return _mySock->isConnected() ;
				    return false ;
				}

    virtual void		setOutputStream( ostream *strm )
				{
				    _out = strm ;
				}
    virtual ostream *		getOutputStream()
				{
				    return _out ;
				}

    virtual void		brokenPipe()
				{
				    _brokenPipe = true ;
				}

    virtual unsigned int	getRecvChunkSize() = 0 ;
    virtual unsigned int	getSendChunkSize() = 0 ;

    virtual void		dump( ostream &strm ) const ;
} ;

#endif // Connection_h

