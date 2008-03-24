// pptcapi_hexstrtoiT.cc

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

