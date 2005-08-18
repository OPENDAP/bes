// cmd_test.cc

#include <iostream>

using std::cout ;
using std::cerr ;
using std::endl ;

#include "OPeNDAPCmdParser.h"
#include "DODSResponseHandler.h"
#include "DODSDataHandlerInterface.h"
#include "DODSGlobalIQ.h"
#include "DODSException.h"

#include "ThePersistenceList.h"
#include "DODSContainerPersistence.h"
#include "TheDefineList.h"
#include "DODSDefine.h"
#include "DODSContainer.h"

int
main( int argc, char **argv )
{
    try
    {
	DODSGlobalIQ::DODSGlobalInit( argc, argv ) ;

	cout << "set" << endl ;
	{
	    OPeNDAPCmdParser p ;
	    DODSDataHandlerInterface dhi ;
	    p.parse( "set container values c1,/home/pwest/c1,cedar;", dhi  ) ;
	    DODSContainerPersistence *cp =
		    ThePersistenceList->find_persistence( "volatile" ) ;
	    if( cp )
	    {
		cp->add_container( "c1", "/home/pwest/c1", "cedar" ) ;
	    }
	}

	cout << "define" << endl ;
	{
	    OPeNDAPCmdParser p ;
	    DODSDataHandlerInterface dhi ;
	    p.parse( "define d1 as c1;", dhi  ) ;

	    DODSDefine *dd = new DODSDefine ;
	    DODSContainer c( "c1" ) ;
	    c.set_real_name( "/home/pwest/c1" ) ;
	    c.set_container_type( "cedar" ) ;
	    dd->containers.push_back( c ) ;
	    TheDefineList->add_def( "d1", dd ) ;
	}

	cout << "get" << endl ;
	{
	    OPeNDAPCmdParser p ;
	    DODSDataHandlerInterface dhi ;
	    p.parse( "get das for d1;", dhi  ) ;
	}

	DODSGlobalIQ::DODSGlobalQuit() ;
    }
    catch( DODSException &e )
    {
	cerr << "Caught Exception" << endl ;
	cerr << e.get_error_description() << endl ;
    }

    return 0 ;
}

