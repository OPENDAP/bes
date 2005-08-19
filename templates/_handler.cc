// OPENDAP_TYPE_handler.cc

#include "config_OPENDAP_TYPE.h"

#include "OPENDAP_CLASSResponseNames.h"
#include "DODSCgi.h"
#include "DODSFilter.h"
#include "DODSGlobalIQ.h"
#include "DODSException.h"

int 
main(int argc, char *argv[])
{
    try
    {
	DODSGlobalIQ::DODSGlobalInit( argc, argv ) ;
    }
    catch( DODSException &e )
    {
	cerr << "Error initializing application" << endl ;
	cerr << "    " << e.get_error_description() << endl ;
	return 1 ;
    }

    DODSFilter df(argc, argv);

    DODSCgi d( OPENDAP_CLASS_NAME, df ) ;
    d.execute_request() ;

    DODSGlobalIQ::DODSGlobalQuit() ;

    return 0;
}

