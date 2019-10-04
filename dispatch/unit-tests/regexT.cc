// regexT.C

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

#include <iostream>

using std::cerr;
using std::cout;
using std::endl;
using std::string;

#include "BESRegex.h"
#include "BESError.h"
#include <GetOpt.h>

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

class regexT: public TestFixture {
private:

public:
    regexT()
    {
    }
    ~regexT()
    {
    }

    void setUp()
    {
    }

    void tearDown()
    {
    }

CPPUNIT_TEST_SUITE( regexT );

    CPPUNIT_TEST( do_test );

    CPPUNIT_TEST_SUITE_END()
    ;

    void do_test()
    {
        cout << "*****************************************" << endl;
        cout << "Entered regexT::run" << endl;

        try {
            cout << "*****************************************" << endl;
            cout << "Match reg ex 123456 against string 01234567" << endl;
            BESRegex reg_expr("123456");
            string inQuestion = "01234567";
            int result = reg_expr.match(inQuestion.c_str(), inQuestion.length());
            CPPUNIT_ASSERT( result == 6 );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"Failed to match" );
        }

        try {
            cout << "*****************************************" << endl;
            cout << "Match reg ex ^123456$ against string 01234567" << endl;
            BESRegex reg_expr("^123456$");
            string inQuestion = "01234567";
            int result = reg_expr.match(inQuestion.c_str(), inQuestion.length());
            CPPUNIT_ASSERT( result == -1 );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"Failed to match" );
        }

        try {
            cout << "*****************************************" << endl;
            cout << "Match reg ex ^123456$ against string 123456" << endl;
            cout << "    besregtest include \"^123456$;\" 123456 matches all 6 of 6 characters" << endl;
            BESRegex reg_expr("^123456$");
            string inQuestion = "123456";
            int result = reg_expr.match(inQuestion.c_str(), inQuestion.length());
            CPPUNIT_ASSERT( result == 6 );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"Failed to match" );
        }

        try {
            cout << "*****************************************" << endl;
            cout << "Match reg ex \".*\\.nc$;\" against string fnoc1.nc" << endl;
            BESRegex reg_expr(".*\\.nc$");
            string inQuestion = "fnoc1.nc";
            int result = reg_expr.match(inQuestion.c_str(), inQuestion.length());
            CPPUNIT_ASSERT( result == 8 );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"Failed to match" );
        }

        try {
            cout << "*****************************************" << endl;
            cout << "Match reg ex \".*\\.nc$;\" against string fnoc1.ncd" << endl;
            BESRegex reg_expr(".*\\.nc$");
            string inQuestion = "fnoc1.ncd";
            int result = reg_expr.match(inQuestion.c_str(), inQuestion.length());
            CPPUNIT_ASSERT( result == -1 );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"Failed to match" );
        }

        try {
            cout << "*****************************************" << endl;
            cout << "Match reg ex .*\\.(nc|NC)(\\.gz|\\.bz2|\\.Z)?$ against string fnoc1.nc" << endl;
            BESRegex reg_expr(".*\\.(nc|NC)(\\.gz|\\.bz2|\\.Z)?$");
            string inQuestion = "fnoc1.nc";
            int result = reg_expr.match(inQuestion.c_str(), inQuestion.length());
            CPPUNIT_ASSERT( result == 8 );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"Failed to match" );
        }

        try {
            cout << "*****************************************" << endl;
            cout << "Match reg ex .*\\.(nc|NC)(\\.gz|\\.bz2|\\.Z)?$ against string fnoc1.nc.gz" << endl;
            BESRegex reg_expr(".*\\.(nc|NC)(\\.gz|\\.bz2|\\.Z)?$");
            string inQuestion = "fnoc1.nc.gz";
            int result = reg_expr.match(inQuestion.c_str(), inQuestion.length());
            CPPUNIT_ASSERT( result == 11 );
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT( !"Failed to match" );
        }

        cout << "*****************************************" << endl;
        cout << "Returning from regexT::run" << endl;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( regexT );

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
            cerr << "Usage: regexT has the following tests:" << endl;
            const std::vector<Test*> &tests = regexT::suite()->getTests();
            unsigned int prefix_len = regexT::suite()->getName().append("::").length();
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
            test = regexT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

