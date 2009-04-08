// pptcapi_extensionsT.cc

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
    int pptcapi_read_extensions( struct pptcapi_extensions **extensions,
				 char *buffer, char **error ) ;
}

class pptcapi_extensionsT: public TestFixture {
private:

public:
    pptcapi_extensionsT() {}
    ~pptcapi_extensionsT() {}

    void setUp()
    {
    } 

    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE( pptcapi_extensionsT ) ;

    CPPUNIT_TEST( do_test ) ;

    CPPUNIT_TEST_SUITE_END() ;

    void do_test()
    {
	cout << endl << "*****************************************" << endl;
	cout << "Entered pptcapi_extensionsT::run" << endl;

	{
	    struct pptcapi_extensions *extensions = 0 ;
	    char *buffer = "n1=v1;n2;n3=v3;" ;
	    char *error = 0 ;
	    int result = pptcapi_read_extensions( &extensions, buffer, &error );
	    CPPUNIT_ASSERT( result == PPTCAPI_OK ) ;
	    CPPUNIT_ASSERT( extensions ) ;
	    CPPUNIT_ASSERT( !error ) ;
	    int test = 1 ;
	    while( extensions )
	    {
		if( test == 1 )
		{
		    CPPUNIT_ASSERT( extensions->name ) ;
		    cout << "name = \"" << extensions->name << "\"" << endl ;
		    CPPUNIT_ASSERT( string( "n1" ) == extensions->name ) ;
		    CPPUNIT_ASSERT( extensions->value ) ;
		    cout << "value = \"" << extensions->value << "\"" << endl ;
		    CPPUNIT_ASSERT( string( "v1" ) == extensions->value ) ;
		}
		else if( test == 2 )
		{
		    CPPUNIT_ASSERT( extensions->name ) ;
		    cout << "name = \"" << extensions->name << "\"" << endl ;
		    CPPUNIT_ASSERT( string( "n2" ) == extensions->name ) ;
		    CPPUNIT_ASSERT( !extensions->value ) ;
		}
		else if( test == 3 )
		{
		    CPPUNIT_ASSERT( extensions->name ) ;
		    cout << "name = \"" << extensions->name << "\"" << endl ;
		    CPPUNIT_ASSERT( string( "n3" ) == extensions->name ) ;
		    CPPUNIT_ASSERT( extensions->value ) ;
		    cout << "value = \"" << extensions->value << "\"" << endl ;
		    CPPUNIT_ASSERT( string( "v3" ) == extensions->value ) ;
		}
		else
		{
		    CPPUNIT_ASSERT( !"too many extensions" ) ;
		}
		extensions = extensions->next ;
		test++ ;
	    }
	    CPPUNIT_ASSERT( test == 4 ) ;
	    pptcapi_free_extensions_struct( extensions ) ;
	    extensions = 0 ;
	}

	{
	    struct pptcapi_extensions *extensions = 0 ;
	    char *buffer = "n1=v1;" ;
	    char *error = 0 ;
	    int result = pptcapi_read_extensions( &extensions, buffer, &error );
	    CPPUNIT_ASSERT( result == PPTCAPI_OK ) ;
	    CPPUNIT_ASSERT( extensions ) ;
	    CPPUNIT_ASSERT( !error ) ;
	    int test = 1 ;
	    while( extensions )
	    {
		if( test == 1 )
		{
		    CPPUNIT_ASSERT( extensions->name ) ;
		    cout << "name = \"" << extensions->name << "\"" << endl ;
		    CPPUNIT_ASSERT( string( "n1" ) == extensions->name ) ;
		    CPPUNIT_ASSERT( extensions->value ) ;
		    cout << "value = \"" << extensions->value << "\"" << endl ;
		    CPPUNIT_ASSERT( string( "v1" ) == extensions->value ) ;
		}
		else
		{
		    CPPUNIT_ASSERT( !"too many extensions" ) ;
		}
		extensions = extensions->next ;
		test++ ;
	    }
	    CPPUNIT_ASSERT( test == 2 ) ;
	    pptcapi_free_extensions_struct( extensions ) ;
	    extensions = 0 ;
	}

	{
	    struct pptcapi_extensions *extensions = 0 ;
	    char *buffer = "n1;" ;
	    char *error = 0 ;
	    int result = pptcapi_read_extensions( &extensions, buffer, &error );
	    CPPUNIT_ASSERT( result == PPTCAPI_OK ) ;
	    CPPUNIT_ASSERT( extensions ) ;
	    CPPUNIT_ASSERT( !error ) ;
	    int test = 1 ;
	    while( extensions )
	    {
		if( test == 1 )
		{
		    CPPUNIT_ASSERT( extensions->name ) ;
		    cout << "name = \"" << extensions->name << "\"" << endl ;
		    CPPUNIT_ASSERT( string( "n1" ) == extensions->name ) ;
		    CPPUNIT_ASSERT( !extensions->value ) ;
		}
		else
		{
		    CPPUNIT_ASSERT( !"too many extensions" ) ;
		}
		extensions = extensions->next ;
		test++ ;
	    }
	    CPPUNIT_ASSERT( test == 2 ) ;
	    pptcapi_free_extensions_struct( extensions ) ;
	    extensions = 0 ;
	}

	{
	    struct pptcapi_extensions *extensions = 0 ;
	    char *buffer = "n1=v1" ;
	    char *error = 0 ;
	    int result = pptcapi_read_extensions( &extensions, buffer, &error );
	    CPPUNIT_ASSERT( result == PPTCAPI_ERROR ) ;
	    CPPUNIT_ASSERT( !extensions ) ;
	    CPPUNIT_ASSERT( error ) ;
	    cout << "error = " << error << endl ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "Leaving pptcapi_extensionsT::run" << endl;
    }

} ;

CPPUNIT_TEST_SUITE_REGISTRATION( pptcapi_extensionsT ) ;

int 
main( int, char** )
{
    CppUnit::TextTestRunner runner ;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() ) ;

    bool wasSuccessful = runner.run( "", false )  ;

    return wasSuccessful ? 0 : 1 ;
}

