// prettyT.cc

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
#include <sstream>

using std::cout ;
using std::endl ;
using std::string ;
using std::ostringstream ;

#include "CmdPretty.h"
#include "BESError.h"
#include <test_config.h>

string b1 = "<root />\n" ;
string b2 = "<root a1=\"v1\" a2=\"v2\" a3=\"v3\">val</root>\n" ;
string b3 = "<root a1=\"v1\" a2=\"v2\" a3=\"v3\">\n\
    valvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalvalval\n\
</root>\n" ;
string b4 = "<root a1=\"v1\" a2=\"v2\" a3=\"v3\">\n\
    <sub1 a4=\"v4\">\n\
        <sub2.1 a5=\"v5\">sub2.1val</sub2.1>\n\
        <sub2.2 a6=\"v6\">sub2.2val</sub2.2>\n\
    </sub1>\n\
</root>\n" ;
string b5 = "This is not an xml document" ;
string b6 = (string)"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
	    + "<root a1=\"v1\" a2=\"v2\" a3=\"v3\">Not well formed"
	    + "</somethingelse>" ;

class prettyT: public TestFixture {
private:

public:
    prettyT() {}
    ~prettyT() {}

    void setUp()
    {
    } 

    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE( prettyT ) ;

    CPPUNIT_TEST( do_test ) ;

    CPPUNIT_TEST_SUITE_END() ;

    void do_test()
    {
	cout << endl << "*****************************************" << endl;
	cout << "Entered prettyT::run" << endl;

	try
	{
	    {
		string t = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><root />" ;
		ostringstream strm ;
		CmdPretty::make_pretty( t, strm ) ;
		string result = strm.str() ;
		cout << "result = " << endl << result << endl ;
		cout << "baseline = " << endl << b1 << endl ;
		CPPUNIT_ASSERT( result == b1 ) ;
	    }
	    {
		string t = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><root></root>" ;
		ostringstream strm ;
		CmdPretty::make_pretty( t, strm ) ;
		string result = strm.str() ;
		cout << "result = " << endl << result << endl ;
		cout << "baseline = " << endl << b1 << endl ;
		CPPUNIT_ASSERT( result == b1 ) ;
	    }
	    {
		string t = (string)"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
			   + "<root a1=\"v1\" a2=\"v2\" a3=\"v3\">val</root>" ;
		ostringstream strm ;
		CmdPretty::make_pretty( t, strm ) ;
		string result = strm.str() ;
		cout << "result = " << endl << result << endl ;
		cout << "baseline = " << endl << b2 << endl ;
		CPPUNIT_ASSERT( result == b2 ) ;
	    }
	    {
		string t = (string)"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
			   + "<root a1=\"v1\" a2=\"v2\" a3=\"v3\">"
			   + "valvalvalvalvalvalvalvalvalvalvalval"
			   + "valvalvalvalvalvalvalvalvalvalvalval"
			   + "valvalvalvalvalvalvalvalvalvalvalval"
			   + "valvalvalvalvalvalvalvalvalvalvalval"
			   + "valvalvalvalvalvalvalvalvalvalvalval"
			   + "valvalvalvalvalvalvalvalvalvalvalval"
			   + "valvalval</root>" ;
		ostringstream strm ;
		CmdPretty::make_pretty( t, strm ) ;
		string result = strm.str() ;
		cout << "result = " << endl << result << endl ;
		cout << "baseline = " << endl << b3 << endl ;
		CPPUNIT_ASSERT( result == b3 ) ;
	    }
	    {
		string t = (string)"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		       + "<root a1=\"v1\" a2=\"v2\" a3=\"v3\">"
		       + "<sub1 a4=\"v4\"><sub2.1 a5=\"v5\">sub2.1val</sub2.1>"
		       + "<sub2.2 a6=\"v6\">sub2.2val</sub2.2>"
		       + "</sub1></root>" ;
		ostringstream strm ;
		CmdPretty::make_pretty( t, strm ) ;
		string result = strm.str() ;
		cout << "result = " << endl << result << endl ;
		cout << "baseline = " << endl << b4 << endl ;
		CPPUNIT_ASSERT( result == b4 ) ;
	    }
	    {
		string t = (string)"This is not an xml document" ;
		ostringstream strm ;
		CmdPretty::make_pretty( t, strm ) ;
		string result = strm.str() ;
		cout << "result = " << endl << result << endl ;
		cout << "baseline = " << endl << b5 << endl ;
		CPPUNIT_ASSERT( result == b5 ) ;
	    }
	    {
		string t = (string)"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		       + "<root a1=\"v1\" a2=\"v2\" a3=\"v3\">Not well formed"
		       + "</somethingelse>" ;
		ostringstream strm ;
		CmdPretty::make_pretty( t, strm ) ;
		string result = strm.str() ;
		cout << "result = " << endl << result << endl ;
		cout << "baseline = " << endl << b6 << endl ;
		CPPUNIT_ASSERT( result == b6 ) ;
	    }
	}
	catch( BESError &e )
	{
	    cout << "failed with exception" << endl << e.get_message() << endl ;
	    CPPUNIT_ASSERT( false ) ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "Leaving prettyT::run" << endl;
    }

} ;

CPPUNIT_TEST_SUITE_REGISTRATION( prettyT ) ;

int 
main( int, char** )
{
    string env_var = (string)"BES_CONF=" + TEST_SRC_DIR + "/bes.conf" ;
    putenv( (char *)env_var.c_str() ) ;

    CppUnit::TextTestRunner runner ;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() ) ;

    bool wasSuccessful = runner.run( "", false )  ;

    return wasSuccessful ? 0 : 1 ;
}

