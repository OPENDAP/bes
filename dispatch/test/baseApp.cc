// baseApp.C

#include <iostream>

using std::cerr ;
using std::endl ;

#include "baseApp.h"
#include "DODSGlobalIQ.h"
#include "DODSException.h"

Application *Application::_theApplication = 0 ;

baseApp::
baseApp(void)
{
    Application::_theApplication = this ;
}

baseApp::
~baseApp(void)
{
    Application::_theApplication = 0 ;
}

int baseApp::
main( int argC, char **argV )
{
    int retVal = 0 ;
    if((retVal = initialize(argC, argV)) != 0)
    {
	cerr << "baseApp::initialize - failed" << endl ;
    }
    else
    {
	retVal = run() ;
	retVal = terminate(retVal) ;
    }
    return retVal ;
}

int baseApp::
initialize( int argC, char **argV )
{
    int retVal = 0 ;

    // set the locale for the applications, currently only C and POSIX are
    // supported
    setlocale( LC_ALL, "" ) ;

    // initialize application information
    try
    {
	if( DODSGlobalIQ::DODSGlobalInit( argC, argV ) != true )
	{
	    retVal = 1 ;
	}
    }
    catch( DODSException &e )
    {
	cerr << "Global Initialization failed" << endl ;
	cerr << e.get_error_description() << endl ;
    }

    return retVal ;
}

int baseApp::
run( void )
{
    return 1 ;
}

int baseApp::
terminate( int sig )
{
    if( sig )
    {
	cerr << "baseApp::terminating with value " << sig << endl ;
    }
    DODSGlobalIQ::DODSGlobalQuit() ;

    return sig ;
}

