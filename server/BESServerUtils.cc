// BESServerUtil.cc

#include <iostream>

using std::cout ;
using std::endl ;

#include "config.h"
#include "BESServerUtils.h"
#include "BESApp.h"
#include "BESDebug.h"

void
BESServerUtils::show_usage( const string &app_name )
{
    cout << app_name
         << ": -i <INSTALL_DIR> -c <CONFIG> -d <STREAM> -h "
	 << "-p <PORT> -s -u <UNIX_SOCKET> -v"
	 << endl << endl ;
    cout << "-i back-end server installation directory" << endl ;
    cout << "-c use back-end server configuration file CONFIG" << endl ;
    cout << "-d set debugging to cerr or <filename>" << endl ;
    cout << "-h show this help screen and exit" << endl ;
    cout << "-p set port to PORT" << endl ;
    cout << "-s specifies a secure server using SLL authentication" << endl ;
    cout << "-u set unix socket to UNIX_SOCKET" << endl ;
    cout << "-v echos version and exit" << endl ;
    cout << endl ;
    BESDebug::Help( cout ) ;
    exit( 0 ) ;
}

void
BESServerUtils::show_version( const string &app_name )
{
    cout << app_name
         << ": " << PACKAGE_STRING << endl ;
    exit( 0 ) ;
}

