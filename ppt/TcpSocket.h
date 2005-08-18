// TcpSocket.h

// 2005 Copyright University Corporation for Atmospheric Research

#ifndef TcpSocket_h
#define TcpSocket_h 1

#include <string>

using std::string ;

#include "Socket.h"

class TcpSocket : public Socket
{
private:
    string			_host ;
    int				_portVal ;
public:
    				TcpSocket( const string &host, int portVal )
				    : Socket(),
				      _host( host ),
				      _portVal( portVal ) {}
				TcpSocket( int portVal )
				    : Socket(),
				      _host( "" ),
				      _portVal( portVal ) {}
    				TcpSocket( int socket,
				           const struct sockaddr_in &f )
				    : Socket( socket, f ),
				      _host( "" ),
				      _portVal( 0 ) {}
    virtual			~TcpSocket() {}
    virtual void		connect() ;
    virtual void		listen() ;

    virtual Socket *		newSocket( int socket,
                                           const struct sockaddr_in &f )
				{
				    return new TcpSocket( socket, f ) ;
				}
} ;

#endif // TcpSocket_h

// $Log: TcpSocket.h,v $
