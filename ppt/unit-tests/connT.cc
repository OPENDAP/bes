// convertTypeT.cc

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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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
using namespace std ;

#include <string>
#include <iostream>
#include <sstream>

using std::cout ;
using std::endl ;
using std::string ;
using std::ostringstream ;

#include "ConnTest.h"
#include "ConnTestStrs.h"
#include "BESError.h"
#include <GetOpt.h>

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

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

	try
	{
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
		catch( BESError &e )
		{
		    cout << "failed with exception" << endl << e.get_message()
			 << endl ;
		    CPPUNIT_ASSERT( false ) ;
		}
	    }
	}
	catch( BESError &e )
	{
	    cout << "failed with exception" << endl << e.get_message()
		 << endl ;
	    CPPUNIT_ASSERT( false ) ;
	}

	cout << endl << "*****************************************" << endl;
	cout << "Leaving connT::run" << endl;
    }

} ;

CPPUNIT_TEST_SUITE_REGISTRATION( connT ) ;

int main(int argc, char*argv[]) {

    GetOpt getopt(argc, argv, "dh");
    char option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: connT has the following tests:" << endl;
            const std::vector<Test*> &tests = connT::suite()->getTests();
            unsigned int prefix_len = connT::suite()->getName().append("::").length();
            for (std::vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                cerr << (*i)->getName().replace(0, prefix_len, "") << endl;
            }
            break;
        }
        default:
            break;
        }

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    string test = "";
    int i = getopt.optind;
    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = connT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

