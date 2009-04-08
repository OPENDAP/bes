// constraintT.cc

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
#include <cstdlib>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "constraintT.h"
#include "BESFileContainer.h"
#include "BESDataHandlerInterface.h"
#include "BESConstraintFuncs.h"
#include "BESDataNames.h"
#include <test_config.h>

int constraintT::
run(void)
{
    cout << endl << "*****************************************" << endl;
    cout << "Entered constraintT::run" << endl;
    int retVal = 0;

    cout << endl << "*****************************************" << endl;
    cout << "Build the data and build the post constraint" << endl ;
    BESDataHandlerInterface dhi ;
    BESContainer *d1 = new BESFileContainer( "sym1", "real1", "type1" ) ;
    d1->set_constraint( "var1" ) ;
    dhi.containers.push_back( d1 ) ;

    BESContainer *d2 = new BESFileContainer( "sym2", "real2", "type2" ) ;
    d2->set_constraint( "var2" ) ;
    dhi.containers.push_back( d2 ) ;

    dhi.first_container() ;
    BESConstraintFuncs::post_append( dhi ) ;
    dhi.next_container() ;
    BESConstraintFuncs::post_append( dhi ) ;

    string should_be = "sym1.var1,sym2.var2" ;
    if( dhi.data[POST_CONSTRAINT] != should_be )
    {
	cerr << "bad things man" << endl ;
	cerr << "    post constraint: " << dhi.data[POST_CONSTRAINT] << endl;
	cerr << "    should be: " << should_be << endl;
    }
    else
    {
	cout << "good" << endl ;
	cout << "    post constraint: " << dhi.data[POST_CONSTRAINT] << endl;
	cout << "    should be: " << should_be << endl;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from constraintT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    string env_var = (string)"BES_CONF=" + TEST_SRC_DIR
                     + "/persistence_cgi_test.ini" ;
    putenv( (char *)env_var.c_str() ) ;
    Application *app = new constraintT();
    return app->main(argC, argV);
}

