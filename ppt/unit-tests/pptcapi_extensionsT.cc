// pptcapi_extensionsT.cc

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

