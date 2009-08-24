// initT.C

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

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace CppUnit ;

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "BESGlobalIQ.h"
#include "TheCat.h"
#include "TheDog.h"

class initT: public TestFixture {
private:

public:
    initT() {}
    ~initT() {}

    void setUp()
    {
    } 

    void tearDown()
    {
	BESGlobalIQ::BESGlobalQuit() ;
    }

    CPPUNIT_TEST_SUITE( initT ) ;

    CPPUNIT_TEST( do_test ) ;

    CPPUNIT_TEST_SUITE_END() ;

    void do_test()
    {
	cout << endl << "*****************************************" << endl;
	cout << "Entered initT::run" << endl;
	int retVal = 0;

	cout << endl << "*****************************************" << endl;
	cout << "Using TheCat and TheDog" << endl;
	CPPUNIT_ASSERT( TheCat ) ;
	CPPUNIT_ASSERT( TheDog ) ;

	string cat_name = TheCat->get_name() ;
	CPPUNIT_ASSERT( cat_name == "Muffy" ) ;

	string dog_name = TheDog->get_name() ;
	CPPUNIT_ASSERT( dog_name == "Killer" ) ;

	cout << endl << "*****************************************" << endl;
	cout << "Returning from initT::run" << endl;
    }
} ;

CPPUNIT_TEST_SUITE_REGISTRATION( initT ) ;

int 
main( int argC, char**argV )
{
    BESGlobalIQ::BESGlobalInit( argC, argV ) ;

    CppUnit::TextTestRunner runner ;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() ) ;

    bool wasSuccessful = runner.run( "", false )  ;

    return wasSuccessful ? 0 : 1 ;
}

