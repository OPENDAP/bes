// Socket.h

// 2005 Copyright University Corporation for Atmospheric Research

#ifndef Socket_h
#define Socket_h 1

#include <netinet/in.h>

#include <string>

using std::string ;

class Socket
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
				        const struct sockaddr_in &f )
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
} ;

#endif // Socket_h

// $Log: Socket.h,v $
