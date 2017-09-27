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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace CppUnit ;

#include <iostream>
#include <fstream>
#include <cstdlib>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::ifstream ;

#include "BESProcessEncodedString.h"
#include "TheBESKeys.h"
#include <test_config.h>

class encodeT: public TestFixture {
private:

public:
    encodeT() {}
    ~encodeT() {}

    void setUp()
    {
	string bes_conf = (string)TEST_SRC_DIR + "/bes.conf" ;
	TheBESKeys::ConfigFile = bes_conf ;
    } 

    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE( encodeT ) ;

    CPPUNIT_TEST( do_test ) ;

    CPPUNIT_TEST_SUITE_END() ;

    void do_test()
    {
	cout << "*****************************************" << endl;
	cout << "Entered encodeT::run" << endl;

	string teststr = "request=%22This%20is%20a%20test%3B%22&username=pwest" ;
	BESProcessEncodedString pes( teststr.c_str() ) ;
	string request = pes.get_key( "request" ) ;
	cout << "request = " << request << endl ;
	CPPUNIT_ASSERT( request == "\"This is a test;\"" ) ;

	string username = pes.get_key( "username" ) ;
	cout << "username = " << username << endl ;
	CPPUNIT_ASSERT( username == "pwest" ) ;

	cout << "*****************************************" << endl;
	cout << "Returning from encodeT::run" << endl;
    }
} ;

CPPUNIT_TEST_SUITE_REGISTRATION( encodeT ) ;

int 
main( int, char** )
{
    CppUnit::TextTestRunner runner ;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() ) ;

    bool wasSuccessful = runner.run( "", false )  ;

    return wasSuccessful ? 0 : 1 ;
}

