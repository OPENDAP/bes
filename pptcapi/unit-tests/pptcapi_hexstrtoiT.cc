// pptcapi_hexstrtoiT.cc

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

#include <string>
#include <iostream>

using std::cout ;
using std::endl ;
using std::string ;

extern "C"
{
#include "pptcapi.h"
    int pptcapi_hexstr_to_i( char *hexstr, int *result, char **error ) ;
}

class pptcapi_hexstrtoiT: public TestFixture {
private:

public:
    pptcapi_hexstrtoiT() {}
    ~pptcapi_hexstrtoiT() {}

    void setUp()
    {
    } 

    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE( pptcapi_hexstrtoiT ) ;

    CPPUNIT_TEST( do_test ) ;

    CPPUNIT_TEST_SUITE_END() ;

    void do_test()
    {
	cout << endl << "*****************************************" << endl;
	cout << "Entered pptcapi_hexstrtoiT::run" << endl;

	{
	    char *error = 0 ;
	    char *hexstr = "0005" ;
	    int result = 0 ;
	    int res = pptcapi_hexstr_to_i( hexstr, &result, &error ) ;
	    CPPUNIT_ASSERT( res == PPTCAPI_OK ) ;
	    cout << "result = " << result << endl ;
	    CPPUNIT_ASSERT( result == 5 ) ;
	    CPPUNIT_ASSERT( !error ) ;
	}

	{
	    char *error = 0 ;
	    char *hexstr = "000d" ;
	    int result = 0 ;
	    int res = pptcapi_hexstr_to_i( hexstr, &result, &error ) ;
	    CPPUNIT_ASSERT( res == PPTCAPI_OK ) ;
	    cout << "result = " << result << endl ;
	    CPPUNIT_ASSERT( result == 13 ) ;
	    CPPUNIT_ASSERT( !error ) ;
	}

	{
	    char *error = 0 ;
	    char *hexstr = "002d" ;
	    int result = 0 ;
	    int res = pptcapi_hexstr_to_i( hexstr, &result, &error ) ;
	    CPPUNIT_ASSERT( res == PPTCAPI_OK ) ;
	    cout << "result = " << result << endl ;
	    CPPUNIT_ASSERT( result == 45 ) ;
	    CPPUNIT_ASSERT( !error ) ;
	}

	{
	    char *error = 0 ;
	    char *hexstr = "00cd" ;
	    int result = 0 ;
	    int res = pptcapi_hexstr_to_i( hexstr, &result, &error ) ;
	    CPPUNIT_ASSERT( res == PPTCAPI_OK ) ;
	    cout << "result = " << result << endl ;
	    CPPUNIT_ASSERT( result == 205 ) ;
	    CPPUNIT_ASSERT( !error ) ;
	}

	{
	    char *error = 0 ;
	    char *hexstr = "1c4d" ;
	    int result = 0 ;
	    int res = pptcapi_hexstr_to_i( hexstr, &result, &error ) ;
	    CPPUNIT_ASSERT( res == PPTCAPI_OK ) ;
	    cout << "result = " << result << endl ;
	    CPPUNIT_ASSERT( result == 7245 ) ;
	    CPPUNIT_ASSERT( !error ) ;
	}

	{
	    char *error = 0 ;
	    char *hexstr = "00xz" ;
	    int result = 0 ;
	    int res = pptcapi_hexstr_to_i( hexstr, &result, &error ) ;
	    CPPUNIT_ASSERT( res == PPTCAPI_ERROR ) ;
	    cout << "result = " << result << endl ;
	    CPPUNIT_ASSERT( result == 0 ) ;
	    CPPUNIT_ASSERT( error ) ;
	    cout << "error = " << error << endl ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "Leaving pptcapi_hexstrtoiT::run" << endl;
    }

} ;

CPPUNIT_TEST_SUITE_REGISTRATION( pptcapi_hexstrtoiT ) ;

int 
main( int, char** )
{
    CppUnit::TextTestRunner runner ;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() ) ;

    bool wasSuccessful = runner.run( "", false )  ;

    return wasSuccessful ? 0 : 1 ;
}

