// urlT.C

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
#include <fstream>
#include <cstdlib>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::ifstream ;

#include "BESUtil.h"
#include "BESError.h"

class urlT: public TestFixture {
private:

public:
    urlT() {}
    ~urlT() {}

    void setUp()
    {
    } 

    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE( urlT ) ;

    CPPUNIT_TEST( do_test ) ;

    CPPUNIT_TEST_SUITE_END() ;

    void check( const string &found, const string &shouldbe, const string &msg )
    {
	if( shouldbe != found )
	{
	    cerr << "  " << msg << " FAIL" << endl ;
	    cerr << "    should be: " << shouldbe << endl ;
	    cerr << "    found: " << found << endl ;
	    CPPUNIT_ASSERT( shouldbe == found ) ;
	}
    }

    void tryme( const string &url,
	        const string &protocol,
	        const string &u,
	        const string &p,
	        const string &domain,
	        const string &port,
	        const string &path )
    {
	cerr << "**** Trying " << url << endl ;

	BESUtil::url url_parts ;
	BESUtil::url_explode( url, url_parts ) ;

	check( url_parts.protocol, protocol, "protocol" ) ;
	check( url_parts.uname, u, "u" ) ;
	check( url_parts.psswd, p, "p" ) ;
	check( url_parts.domain, domain, "domain" ) ;
	check( url_parts.port, port, "port" ) ;
	check( url_parts.path, path, "path" ) ;

	string url_s = BESUtil::url_create( url_parts ) ;
	if( url_s != url ) url_s += "/" ;
	check( url_s, url, "url" ) ;
    }

    void do_test()
    {
	cout << "*****************************************" << endl;
	cout << "Entered urlT::run" << endl;

	string var = "http://tw.rpi.edu" ;
	tryme( var, "http", "", "", "tw.rpi.edu", "", "" ) ;

	var = "http://tw.rpi.edu/" ;
	tryme( var, "http", "", "", "tw.rpi.edu", "", "" ) ;

	var = "http://tw.rpi.edu/web/person/PatrickWest" ;
	tryme( var, "http", "", "", "tw.rpi.edu", "", "web/person/PatrickWest" ) ;

	var = "https://scm.escience.rpi.edu/trac" ;
	tryme( var, "https", "", "", "scm.escience.rpi.edu", "", "trac" ) ;

	var = "https://myname@scm.escience.rpi.edu/trac" ;
	tryme( var, "https", "myname", "", "scm.escience.rpi.edu", "", "trac" ) ;

	var = "https://myname:mypwd@scm.escience.rpi.edu/trac" ;
	tryme( var, "https", "myname", "mypwd", "scm.escience.rpi.edu", "", "trac" ) ;

	var = "https://myname:mypwd@scm.escience.rpi.edu:8890/trac" ;
	tryme( var, "https", "myname", "mypwd", "scm.escience.rpi.edu", "8890", "trac" ) ;

	var = "ftp://orion.tw.rpi.edu/someother/dir/and/file.txt" ;
	tryme( var, "ftp", "", "", "orion.tw.rpi.edu", "", "someother/dir/and/file.txt" ) ;

	var = "ftp://myname@orion.tw.rpi.edu/someother/dir/and/file.txt" ;
	tryme( var, "ftp", "myname", "", "orion.tw.rpi.edu", "", "someother/dir/and/file.txt" ) ;

	var = "ftp://myname:mypwd@orion.tw.rpi.edu/someother/dir/and/file.txt" ;
	tryme( var, "ftp", "myname", "mypwd", "orion.tw.rpi.edu", "", "someother/dir/and/file.txt" ) ;

	var = "ftp://myname:mypwd@orion.tw.rpi.edu:89/someother/dir/and/file.txt" ;
	tryme( var, "ftp", "myname", "mypwd", "orion.tw.rpi.edu", "89", "someother/dir/and/file.txt" ) ;

	cout << "*****************************************" << endl;
	cout << "Returning from urlT::run" << endl;
    }
} ;

CPPUNIT_TEST_SUITE_REGISTRATION( urlT ) ;

int 
main( int, char** )
{
    CppUnit::TextTestRunner runner ;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() ) ;

    bool wasSuccessful = runner.run( "", false )  ;

    return wasSuccessful ? 0 : 1 ;
}

