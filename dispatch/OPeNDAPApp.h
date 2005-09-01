// OPeNDAPApp.h

#ifndef A_OPeNDAPApp_H
#define A_OPeNDAPApp_H

#include <string>

using std::string ;

#include "OPeNDAPObj.h"

class OPeNDAPApp : public OPeNDAPObj
{
protected:
    string			_appName ;
    bool			_debug ;
    static OPeNDAPApp *		_theApplication;
                                OPeNDAPApp(void) {};
public:
    virtual			~OPeNDAPApp() {}
    virtual int			main(int argC, char **argV) = 0;
    virtual int			initialize(int argC, char **argV) = 0;
    virtual int			run(void) = 0;
    virtual int			terminate(int sig = 0) = 0;
    string			appName( void ) { return _appName ; }
    bool			debug( void ) { return _debug ; }
    void			setDebug( bool debug ) { _debug = debug ; }

    static OPeNDAPApp *		TheApplication(void) { return _theApplication; }
};

#endif

