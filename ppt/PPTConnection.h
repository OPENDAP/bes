// PPTConnection.h

// 2005 Copyright University Corporation for Atmospheric Research

#ifndef PPTConnection_h
#define PPTConnection_h 1

#include "Connection.h"

class Socket ;

class PPTConnection : public Connection
{
protected:
				PPTConnection() {}

    void			writeBuffer( const string &buffer ) ;
    int				readBuffer( char *inBuff ) ;
public:
    virtual			~PPTConnection() {}

    virtual void		initConnection() = 0 ;
    virtual void		closeConnection() = 0 ;

    virtual void		send( const string &buffer ) ;
    virtual void		sendExit() ;
    virtual bool		receive( ostream *strm = 0 ) ;
} ;

#endif // PPTConnection_h

// $Log: PPTConnection.h,v $
