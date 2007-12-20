// convertTypeT.cc

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace CppUnit ;

#include <string>
#include <iostream>
#include <sstream>

using std::cout ;
using std::endl ;
using std::string ;
using std::ostringstream ;

#include "ConnTest.h"
#include "ConnTestStrs.h"
#include "PPTException.h"

class connT: public TestFixture {
private:

public:
    connT() {}
    ~connT() {}

    void setUp()
    {
    } 

    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE( connT ) ;

    CPPUNIT_TEST( do_test ) ;

    CPPUNIT_TEST_SUITE_END() ;

    void do_test()
    {
	cout << endl << "*****************************************" << endl;
	cout << "Entered connT::run" << endl;

	ConnTest ct ;
	map<string,string> no_extensions ;
	map<string,string> extensions ;

	cout << endl << "*****************************************" << endl;
	cout << "Send " << test_str[0] << endl;
	ct.send( test_str[0], no_extensions ) ;

	cout << endl << "*****************************************" << endl;
	cout << "Send " << test_str[1] << " with extensions var1=val1 and var2" << endl;
	extensions["var1"] = "val1" ;
	extensions["var2"] = "" ;
	ct.send( test_str[1], extensions ) ;

	{
	    cout << endl << "*****************************************" << endl;
	    cout << "receive " << test_str[0] << endl;
	    ostringstream ostrm ;
	    map<string,string> rec_extensions ;
	    ct.receive( rec_extensions, &ostrm ) ;
	    cout << "****" << endl << "\"" << ostrm.str() << "\"" << endl << "****" << endl ;
	    CPPUNIT_ASSERT( ostrm.str() == test_str[0] ) ;
	    CPPUNIT_ASSERT( rec_extensions.size() == 0 ) ;
	}
	{
	    cout << endl << "*****************************************" << endl;
	    cout << "receive " << test_str[0] << " again (receive is broken up)" << endl;
	    ostringstream ostrm ;
	    map<string,string> rec_extensions ;
	    ct.receive( rec_extensions, &ostrm ) ;
	    cout << "****" << endl << "\"" << ostrm.str() << "\"" << endl << "****" << endl ;
	    CPPUNIT_ASSERT( ostrm.str() == test_str[0] ) ;
	    CPPUNIT_ASSERT( rec_extensions.size() == 0 ) ;
	}
	{
	    cout << endl << "*****************************************" << endl;
	    cout << "receive " << test_str[1] << " (includes extensions)" << endl;
	    try
	    {
		ostringstream ostrm ;
		map<string,string> rec_extensions ;
		bool done = false ;
		while( !done )
		    done = ct.receive( rec_extensions, &ostrm ) ;
		cout << "****" << endl << "\"" << ostrm.str() << "\"" << endl << "****" << endl ;
		CPPUNIT_ASSERT( ostrm.str() == "" ) ;
		CPPUNIT_ASSERT( rec_extensions.size() == 2 ) ;
		CPPUNIT_ASSERT( rec_extensions["var1"] == "val1" ) ;
		CPPUNIT_ASSERT( rec_extensions["var2"] == "" ) ;

		map<string,string> no_extensions ;
		done = false ;
		while( !done )
		    done = ct.receive( no_extensions, &ostrm ) ;
		cout << "****" << endl << "\"" << ostrm.str() << "\"" << endl << "****" << endl ;
		CPPUNIT_ASSERT( ostrm.str() == test_str[1] ) ;
		CPPUNIT_ASSERT( no_extensions.size() == 0 ) ;
	    }
	    catch( PPTException &e )
	    {
		cout << "failed with exception" << endl << e.getMessage() << endl ;
		CPPUNIT_ASSERT( false ) ;
	    }
	}

	cout << endl << "*****************************************" << endl;
	cout << "Leaving connT::run" << endl;
    }

} ;

CPPUNIT_TEST_SUITE_REGISTRATION( connT ) ;

int 
main( int, char** )
{
    CppUnit::TextTestRunner runner ;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() ) ;

    bool wasSuccessful = runner.run( "", false )  ;

    return wasSuccessful ? 0 : 1 ;
}

