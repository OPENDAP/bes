// convertTypeT.cc

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace CppUnit ;

#include "config.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <fcntl.h>

#include <string>
#include <iostream>
#include <sstream>

using std::cout ;
using std::endl ;
using std::string ;
using std::ostringstream ;

#include "PPTStreamBuf.h"
#include "PPTProtocol.h"

string result = (string)"00001f4" + "d" + "<1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567" + "0000070" + "d" + "890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890><1234567890>" + "0000000" + "d" ;

class sbT: public TestFixture {
private:

public:
    sbT() {}
    ~sbT() {}

    void setUp()
    {
    } 

    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE( sbT ) ;

    CPPUNIT_TEST( do_test ) ;

    CPPUNIT_TEST_SUITE_END() ;

    void do_test()
    {
	cout << endl << "*****************************************" << endl;
	cout << "Entered sbT::run" << endl;

	int fd = open( "./sbT.out", O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH ) ;
	PPTStreamBuf fds( fd, 500 ) ;
	std::streambuf *holder ;
	holder = cout.rdbuf() ;
	cout.rdbuf( &fds ) ;
	for( int u=0; u< 51; u++ )
	{
	    cout << "<1234567890>" ;
	}
	fds.finish() ;
	cout.rdbuf( holder ) ;
	close( fd ) ;

	fd = open( "./sbT.out", O_RDONLY, S_IRUSR ) ;
	char buffer[4096] ;
	int bytesRead = read( fd, (char *)buffer, 4096 ) ;
	close( fd ) ;
	buffer[bytesRead] = '\0' ;
	string str( buffer ) ;
	cout << "****" << endl << str << endl << "****" << endl ;
	CPPUNIT_ASSERT( str == result ) ;

	CPPUNIT_ASSERT( true ) ;

	cout << endl << "*****************************************" << endl;
	cout << "Leaving sbT::run" << endl;
    }

} ;

CPPUNIT_TEST_SUITE_REGISTRATION( sbT ) ;

int 
main( int, char** )
{
    CppUnit::TextTestRunner runner ;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() ) ;

    bool wasSuccessful = runner.run( "", false )  ;

    return wasSuccessful ? 0 : 1 ;
}

