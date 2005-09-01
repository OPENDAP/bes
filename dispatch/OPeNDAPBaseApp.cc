// OPeNDAPBaseApp.C

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "OPeNDAPBaseApp.h"
#include "DODSGlobalIQ.h"
#include "DODSBasicException.h"

OPeNDAPApp *OPeNDAPApp::_theApplication = 0;

OPeNDAPBaseApp::
OPeNDAPBaseApp(void)
{
    OPeNDAPApp::_theApplication = this;
}

OPeNDAPBaseApp::
~OPeNDAPBaseApp(void)
{
    OPeNDAPApp::_theApplication = 0;
}

int OPeNDAPBaseApp::
main(int argC, char **argV)
{
    _appName = argV[0] ;
    int retVal = initialize( argC, argV ) ;
    if( retVal != 0 )
    {
	cerr << "OPeNDAPBaseApp::initialize - failed" << endl;
    }
    else
    {
	retVal = run() ;
	retVal = terminate( retVal ) ;
    }
    return retVal ;
}

int OPeNDAPBaseApp::
initialize(int argC, char **argV)
{
    int retVal = 0;

    // initialize application information
    try
    {
	DODSGlobalIQ::DODSGlobalInit( argC, argV ) ;
    }
    catch( DODSException &e )
    {
	cerr << "Error initializing application" << endl ;
	cerr << "    " << e.get_error_description() << endl ;
	retVal = 1 ;
    }

    return retVal;
}

int OPeNDAPBaseApp::
run(void)
{
    throw DODSBasicException( "OPeNDAPBaseApp::run - overload run operation" ) ;
    return 0;
}

int OPeNDAPBaseApp::
terminate( int sig )
{
    if( sig ) {
	cerr << "OPeNDAPBaseApp::terminating with value " << sig << endl ;
    }
    DODSGlobalIQ::DODSGlobalQuit() ;
    return sig ;
}

void OPeNDAPBaseApp::
dump( ostream &strm ) const
{
    strm << "OPeNDAPBaseApp::dump - (" << (void *)this << ")" << endl ;
}

