// encodeT.C

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

#include <iostream>
#include <fstream>
#include <cstdlib>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::ifstream ;

#include "encodeT.h"
#include "BESProcessEncodedString.h"
#include "test_config.h"

int
encodeT::run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered encodeT::run" << endl;
    int retVal = 0;

    string teststr = "request=%22This%20is%20a%20test%3B%22&username=pwest" ;
    BESProcessEncodedString pes( teststr.c_str() ) ;
    string request = pes.get_key( "request" ) ;
    cout << "request = " << request << endl ;
    if( request != "\"This is a test;\"" )
    {
	cerr << "Resulting request incorrect" << endl ;
	return 1 ;
    }
    else
    {
	cout << "Resulting request correct" << endl ;
    }
    string username = pes.get_key( "username" ) ;
    cout << "username = " << username << endl ;
    if( username != "pwest" )
    {
	cerr << "Resulting username incorrect" << endl ;
	return 1 ;
    }
    else
    {
	cout << "Resulting username correct" << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from encodeT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    string env_var = (string)"BES_CONF=" + TEST_SRC_DIR + "/bes.conf" ;
    putenv( (char *)env_var.c_str() ) ;
    Application *app = new encodeT();
    return app->main(argC, argV);
}

