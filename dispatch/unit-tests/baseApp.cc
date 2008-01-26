// baseApp.C

#include <iostream>

using std::cerr ;
using std::endl ;

#include "baseApp.h"
#include "BESGlobalIQ.h"
#include "BESError.h"

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
	if( BESGlobalIQ::BESGlobalInit( argC, argV ) != true )
	{
	    retVal = 1 ;
	}
    }
    catch( BESError &e )
    {
	cerr << "Global Initialization failed" << endl ;
	cerr << e.get_message() << endl ;
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
    BESGlobalIQ::BESGlobalQuit() ;

    return sig ;
}

