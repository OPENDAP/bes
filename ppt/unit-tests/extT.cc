// extT.cc

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace CppUnit ;

#include <fcntl.h>

#include <string>
#include <iostream>
#include <sstream>
#include <list>

using std::cout ;
using std::endl ;
using std::string ;
using std::ostringstream ;
using std::list ;

#include "ExtConn.h"
#include "BESError.h"

list<string> try_list ;

class extT: public TestFixture {
private:

public:
    extT() {}
    ~extT() {}

    void setUp()
    {
    } 

    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE( extT ) ;

    CPPUNIT_TEST( do_test ) ;

    CPPUNIT_TEST_SUITE_END() ;

    int check_extensions( int expected_size,
			  map<string,string> &expected_vars,
			  map<string,string> &extensions )
    {
	CPPUNIT_ASSERT( expected_size == extensions.size() ) ;

	map<string,string>::const_iterator i = expected_vars.begin() ;
	map<string,string>::const_iterator ie = expected_vars.end() ;
	map<string,string>::const_iterator f = extensions.end() ;
	for( ; i != ie; i++ )
	{
	    string var = (*i).first ;
	    string val = (*i).second ;
	    f = extensions.find( var ) ;
	    CPPUNIT_ASSERT( f != extensions.end() ) ;
	    cout << "var " << var << " = " << (*f).second << ", should be " << val << endl ;
	    CPPUNIT_ASSERT( (*f).second == val ) ;
	}
    }

    void do_try_list()
    {
	ExtConn conn ;

	list<string>::const_iterator i = try_list.begin() ;
	list<string>::const_iterator ie = try_list.end() ;
	for( ; i != ie; i++ )
	{
	    cout << endl << "*****************************************" << endl;
	    cout << "trying \"" << (*i) << "\"" << endl ;
	    try
	    {
		map<string,string> extensions ;
		conn.read_extensions( extensions, (*i) ) ;
		map<string,string>::const_iterator xi = extensions.begin() ;
		map<string,string>::const_iterator xe = extensions.end() ;
		for( ; xi != xe; xi++ )
		{
		    cout << (*xi).first ;
		    if( !((*xi).second).empty() )
			cout << " = " << (*xi).second ;
		    cout << endl ;
		}
	    }
	    catch( BESError &e )
	    {
		cout << "Caught exception" << endl << e.get_message() << endl ;
	    }
	}
    }

    void do_test()
    {
	if( try_list.size() )
	{
	    do_try_list() ;
	    return ;
	}

	ExtConn conn ;
	cout << endl << "*****************************************" << endl;
	cout << "Entered extT::run" << endl;

	{
	    cout << endl << "*****************************************" << endl;
	    cout << "read_extension with \"var1;var2=val2\", missing end semicolon" << endl;
	    try
	    {
		map<string,string> extensions ;
		conn.read_extensions( extensions, "var1;var2=val2" ) ;
		cout << "Should have failed with malformed extension" << endl ;
		CPPUNIT_ASSERT( false ) ;
	    }
	    catch( BESError &e )
	    {
		cout << "SUCCEEDED with exception" << endl << e.get_message()
		     << endl ;
		CPPUNIT_ASSERT( true ) ;
	    }
	}
	{
	    cout << endl << "*****************************************" << endl;
	    cout << "read_extension with \"var1=;\", missing value" << endl;
	    try
	    {
		map<string,string> extensions ;
		conn.read_extensions( extensions, "var1=;" ) ;
		cout << "Should have failed with malformed extension" << endl ;
		CPPUNIT_ASSERT( false ) ;
	    }
	    catch( BESError &e )
	    {
		cout << "SUCCEEDED with exception" << endl << e.get_message()
		     << endl ;
		CPPUNIT_ASSERT( true ) ;
	    }
	}
	{
	    cout << endl << "*****************************************" << endl;
	    cout << "read_extension with \"var1;\"" << endl;
	    try
	    {
		map<string,string> extensions ;
		map<string,string> expected ;
		conn.read_extensions( extensions, "var1;" ) ;
		expected["var1"] = "" ;
		check_extensions( 1, expected, extensions ) ;
	    }
	    catch( BESError &e )
	    {
		cout << "FAILED with exception" << endl << e.get_message()
		     << endl ;
		CPPUNIT_ASSERT( false ) ;
	    }
	}
	{
	    cout << endl << "*****************************************" << endl;
	    cout << "read_extension with \"var1=val1;var2;var3=val3;\"" << endl;
	    try
	    {
		map<string,string> extensions ;
		map<string,string> expected ;
		conn.read_extensions( extensions, "var1=val1;var2;var3=val3;" ) ;
		expected["var1"] = "val1" ;
		expected["var2"] = "" ;
		expected["var3"] = "val3" ;
		check_extensions( 3, expected, extensions ) ;
	    }
	    catch( BESError &e )
	    {
		cout << "FAILED with exception" << endl << e.get_message()
		     << endl ;
		CPPUNIT_ASSERT( false ) ;
	    }
	}

	cout << endl << "*****************************************" << endl;
	cout << "Leaving extT::run" << endl;
    }

} ;

CPPUNIT_TEST_SUITE_REGISTRATION( extT ) ;

int 
main( int argc, char** argv )
{
    if( argc > 1 )
    {
	for( int i = 1; i < argc; i++ )
	{
	    try_list.push_back( argv[i] ) ;
	}
    }

    CppUnit::TextTestRunner runner ;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() ) ;

    bool wasSuccessful = runner.run( "", false )  ;

    return wasSuccessful ? 0 : 1 ;
}

