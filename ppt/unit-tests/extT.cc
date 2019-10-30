// extT.cc

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

using namespace CppUnit;

#include <fcntl.h>

#include <string>
#include <iostream>
#include <sstream>
#include <list>

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ostringstream;
using std::list;
using std::map;

#include "ExtConn.h"
#include "BESError.h"
#include <GetOpt.h>

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

list<string> try_list;

class extT: public TestFixture {
private:

public:
    extT()
    {
    }
    ~extT()
    {
    }

    void setUp()
    {
    }

    void tearDown()
    {
    }

CPPUNIT_TEST_SUITE( extT );

    CPPUNIT_TEST( do_test );

    CPPUNIT_TEST_SUITE_END()
    ;

    int check_extensions(int expected_size, map<string, string> &expected_vars, map<string, string> &extensions)
    {
        CPPUNIT_ASSERT( expected_size == extensions.size() );

        map<string, string>::const_iterator i = expected_vars.begin();
        map<string, string>::const_iterator ie = expected_vars.end();
        map<string, string>::const_iterator f = extensions.end();
        for (; i != ie; i++) {
            string var = (*i).first;
            string val = (*i).second;
            f = extensions.find(var);
            CPPUNIT_ASSERT( f != extensions.end() );
            cout << "var " << var << " = " << (*f).second << ", should be " << val << endl;
            CPPUNIT_ASSERT( (*f).second == val );
        }
    }

    void do_try_list()
    {
        ExtConn conn;

        list<string>::const_iterator i = try_list.begin();
        list<string>::const_iterator ie = try_list.end();
        for (; i != ie; i++) {
            cout << endl << "*****************************************" << endl;
            cout << "trying \"" << (*i) << "\"" << endl;
            try {
                map<string, string> extensions;
                conn.read_extensions(extensions, (*i));
                map<string, string>::const_iterator xi = extensions.begin();
                map<string, string>::const_iterator xe = extensions.end();
                for (; xi != xe; xi++) {
                    cout << (*xi).first;
                    if (!((*xi).second).empty()) cout << " = " << (*xi).second;
                    cout << endl;
                }
            }
            catch (BESError &e) {
                cout << "Caught exception" << endl << e.get_message() << endl;
            }
        }
    }

    void do_test()
    {
        if (try_list.size()) {
            do_try_list();
            return;
        }

        ExtConn conn;
        cout << endl << "*****************************************" << endl;
        cout << "Entered extT::run" << endl;

        {
            cout << endl << "*****************************************" << endl;
            cout << "read_extension with \"var1;var2=val2\", missing end semicolon" << endl;
            try {
                map<string, string> extensions;
                conn.read_extensions(extensions, "var1;var2=val2");
                cout << "Should have failed with malformed extension" << endl;
                CPPUNIT_ASSERT( false );
            }
            catch (BESError &e) {
                cout << "SUCCEEDED with exception" << endl << e.get_message() << endl;
                CPPUNIT_ASSERT( true );
            }
        }
        {
            cout << endl << "*****************************************" << endl;
            cout << "read_extension with \"var1=;\", missing value" << endl;
            try {
                map<string, string> extensions;
                conn.read_extensions(extensions, "var1=;");
                cout << "Should have failed with malformed extension" << endl;
                CPPUNIT_ASSERT( false );
            }
            catch (BESError &e) {
                cout << "SUCCEEDED with exception" << endl << e.get_message() << endl;
                CPPUNIT_ASSERT( true );
            }
        }
        {
            cout << endl << "*****************************************" << endl;
            cout << "read_extension with \"var1;\"" << endl;
            try {
                map<string, string> extensions;
                map<string, string> expected;
                conn.read_extensions(extensions, "var1;");
                expected["var1"] = "";
                check_extensions(1, expected, extensions);
            }
            catch (BESError &e) {
                cout << "FAILED with exception" << endl << e.get_message() << endl;
                CPPUNIT_ASSERT( false );
            }
        }
        {
            cout << endl << "*****************************************" << endl;
            cout << "read_extension with \"var1=val1;var2;var3=val3;\"" << endl;
            try {
                map<string, string> extensions;
                map<string, string> expected;
                conn.read_extensions(extensions, "var1=val1;var2;var3=val3;");
                expected["var1"] = "val1";
                expected["var2"] = "";
                expected["var3"] = "val3";
                check_extensions(3, expected, extensions);
            }
            catch (BESError &e) {
                cout << "FAILED with exception" << endl << e.get_message() << endl;
                CPPUNIT_ASSERT( false );
            }
        }

        cout << endl << "*****************************************" << endl;
        cout << "Leaving extT::run" << endl;
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( extT );

int main(int argc, char*argv[])
{

    GetOpt getopt(argc, argv, "dh");
    char option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'h': {     // help - show test names
            std::cerr << "Usage: extT has the following tests:" << std::endl;
            const std::vector<Test*> &tests = extT::suite()->getTests();
            unsigned int prefix_len = extT::suite()->getName().append("::").length();
            for (std::vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                std::cerr << (*i)->getName().replace(0, prefix_len, "") << std::endl;
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
            test = extT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

