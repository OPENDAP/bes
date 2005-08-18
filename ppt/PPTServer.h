// PPTServer.h

// 2005 Copyright University Corporation for Atmospheric Research

#ifndef PPTServer_h
#define PPTServer_h 1

#include "PPTConnection.h"

class ServerHandler ;
class SocketListener ;
class Socket ;

class PPTServer : public PPTConnection
{
private:
    ServerHandler *		_handler ;
    SocketListener *		_listener ;

    void			welcomeClient() ;
public:
    				PPTServer( ServerHandler *handler,
					   SocketListener *listener ) ;
    virtual			~PPTServer() ;

    virtual void		initConnection() ;
    virtual void		closeConnection() ;
} ;

#endif // PPTServer_h

// $Log: PPTServer.h,v $
