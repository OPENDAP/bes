// OPeNDAPBaseApp.h

#ifndef B_OPeNDAPBaseApp_H
#define B_OPeNDAPBaseApp_H

#include <OPeNDAPApp.h>

class OPeNDAPBaseApp : public OPeNDAPApp {
public:
                                OPeNDAPBaseApp( void ) ;
    virtual                     ~OPeNDAPBaseApp( void ) ;
    virtual int			main( int argC, char **argV ) ;
    virtual int			initialize( int argC, char **argV ) ;
    virtual int			run( void ) ;
    virtual int			terminate( int sig = 0 ) ;

    virtual void		dump( ostream &strm ) const ;
};

#endif

