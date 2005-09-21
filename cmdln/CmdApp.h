// CmdApp.h

#include <fstream>

using std::ofstream ;
using std::ifstream ;

#include "OPeNDAPBaseApp.h"

class CmdClient ;

class CmdApp : public OPeNDAPBaseApp
{
private:
    CmdClient *			_client ;
    string			_hostStr ;
    string			_unixStr ;
    int				_portVal ;
    string			_cmd ;
    ofstream *			_outputStrm ;
    ifstream *			_inputStrm ;
    bool			_createdInputStrm ;
    int				_timeoutVal ;

    void			showVersion() ;
    void			showUsage() ;
    void			registerSignals() ;
public:
    				CmdApp() ;
    virtual			~CmdApp() ;
    virtual int			initialize( int argc, char **argv ) ;
    virtual int			run() ;

    CmdClient *			client() { return _client ; }
    static void			signalCannotConnect( int sig ) ;
    static void			signalTerminate( int sig ) ;
    static void			signalBrokenPipe( int sig ) ;
} ;

