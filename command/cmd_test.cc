// cmd_test.cc

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

using std::cout ;
using std::cerr ;
using std::endl ;

#include "OPeNDAPCmdParser.h"
#include "DODSResponseHandler.h"
#include "DODSDataHandlerInterface.h"
#include "DODSGlobalIQ.h"
#include "DODSException.h"

#include "ContainerStorageList.h"
#include "ContainerStorage.h"
#include "DODSDefineList.h"
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
	    ContainerStorage *cp = ContainerStorageList::TheList->find_persistence( "volatile" ) ;
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
	    DODSDefineList::TheList()->add_def( "d1", dd ) ;
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

