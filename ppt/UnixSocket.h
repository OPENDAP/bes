// UnixSocket.h

// 2005 Copyright University Corporation for Atmospheric Research

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
