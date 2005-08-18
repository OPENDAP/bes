// ServerApp.h

#include <string>

using std::string ;

#include "OPeNDAPBaseApp.h"

class ServerApp : public OPeNDAPBaseApp
{
private:
    int				_portVal ;
    bool			_gotPort ;
    string			_unixSocket ;
    void			showVersion() ;
public:
    				ServerApp() ;
    virtual			~ServerApp() ;
    virtual int			initialize( int argc, char **argv ) ;
    virtual int			run() ;

    static void			signalTerminate( int sig ) ;
    static void			signalRestart( int sig ) ;
    static void			showUsage() ;
} ;

