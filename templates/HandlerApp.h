// OPENDAP_CLASSHandlerApp.h

#include "BESBaseApp.h"

class DODSFilter ;

class OPENDAP_CLASSHandlerApp : public BESBaseApp
{
private:
    DODSFilter *		_df ;
public:
    				OPENDAP_CLASSHandlerApp() ;
    virtual			~OPENDAP_CLASSHandlerApp() ;
    virtual int			initialize( int argc, char **argv ) ;
    virtual int			run() ;
} ;

