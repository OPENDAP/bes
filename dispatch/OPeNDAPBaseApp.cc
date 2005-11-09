// OPeNDAPBaseApp.C

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.org>
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
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>

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
	string newerr = "Error initializing application: " ;
	newerr += e.get_error_description() ;
	throw DODSBasicException( newerr ) ;
    }
    catch( ... )
    {
	string newerr = "Error initializing application: " ;
	newerr += "caught unknown exception" ;
	throw DODSBasicException( newerr ) ;
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

