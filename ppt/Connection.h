// Connection.h

// 2005 Copyright University Corporation for Atmospheric Research

#ifndef Connection_h
#define Connection_h 1

#include <iostream>
#include <string>

using std::ostream ;
using std::string ;

#include "Socket.h"

class Connection
{
protected:
    Socket			*_mySock ;
    ostream			*_out ;
    bool			_brokenPipe ;

    				Connection()
				    : _mySock( 0 ),
				      _out( 0 ),
				      _brokenPipe( false ) {}
public:
    virtual			~Connection() {}

    virtual void		initConnection() = 0 ;
    virtual void		closeConnection() = 0 ;

    virtual void		send( const string &buffer ) = 0 ;
    virtual void		sendExit() = 0 ;
    virtual bool		receive( ostream *strm = 0 ) = 0 ;

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
} ;

#endif // Connection_h

// $Log: Connection.h,v $
