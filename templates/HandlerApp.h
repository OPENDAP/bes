// OPENDAP_CLASSHandlerApp.h

#include "OPeNDAPBaseApp.h"

class DODSFilter ;

class OPENDAP_CLASSHandlerApp : public OPeNDAPBaseApp
{
private:
    DODSFilter *		_df ;
public:
    				OPENDAP_CLASSHandlerApp() ;
    virtual			~OPENDAP_CLASSHandlerApp() ;
    virtual int			initialize( int argc, char **argv ) ;
    virtual int			run() ;
} ;

