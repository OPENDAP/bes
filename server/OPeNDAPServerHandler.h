// OPeNDAPServerHandler.h

#ifndef OPeNDAPServerHandler_h
#define OPeNDAPServerHandler_h 1

#include <string>

using std::string ;

#include "ServerHandler.h"

class Connection ;

class OPeNDAPServerHandler : public ServerHandler
{
private:
    string			_method ;
    void			execute( Connection *c ) ;
public:
    				OPeNDAPServerHandler() ;
    virtual			~OPeNDAPServerHandler() {}

    virtual void		handle( Connection *c ) ;
} ;

#endif // OPeNDAPServerHandler_h

