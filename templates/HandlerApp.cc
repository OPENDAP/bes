// OPENDAP_CLASSHandlerApp.cc

#include <signal.h>
#include <unistd.h>

#include <iostream>

using std::cout ;
using std::cerr ;
using std::endl ;
using std::flush ;

#include "OPENDAP_CLASSHandlerApp.h"
#include "OPENDAP_CLASSResponseNames.h"
#include "DODSFilter.h"
#include "BESCgiInterface.h"

OPENDAP_CLASSHandlerApp::OPENDAP_CLASSHandlerApp()
    : _df( 0 )
{
}

OPENDAP_CLASSHandlerApp::~OPENDAP_CLASSHandlerApp()
{
    if( _df )
    {
	delete _df ;
	_df = 0 ;
    }
}

int
OPENDAP_CLASSHandlerApp::initialize( int argc, char **argv )
{
    BESBaseApp::initialize( argc, argv ) ;

    _df = new DODSFilter( argc, argv ) ;

    return 0 ;
}

int
OPENDAP_CLASSHandlerApp::run()
{
    BESCgiInterface d( OPENDAP_CLASS_NAME, *_df ) ;
    d.execute_request() ;

    return 0 ;
}

