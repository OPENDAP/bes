// SocketListener.h

// 2005 Copyright University Corporation for Atmospheric Research

#ifndef SocketListener_h
#define SocketListener_h 1

#include <map>

using std::map ;

class Socket ;

class SocketListener
{
private:
    map< int, Socket *> _socket_list ;
    typedef map< int, Socket *>::const_iterator Socket_citer ;
    typedef map< int, Socket *>::iterator Socket_iter ;
    bool			_accepting ;
public:
				SocketListener() ;
    virtual			~SocketListener() ;
    virtual void		listen( Socket *s ) ;
    virtual Socket *		accept() ;
} ;

#endif // SocketListener_h

// $Log: SocketListener.h,v $
