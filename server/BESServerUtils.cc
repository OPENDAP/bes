// BESServerUtil.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "config.h"

#include <cstdlib>
#include <iostream>

using std::cout ;
using std::endl ;

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

