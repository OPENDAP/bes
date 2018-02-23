// resplistT.C

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

#include "BESResponseHandlerList.h"
#include "TestResponseHandler.h"
#include <GetOpt.h>

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

class resplistT: public TestFixture {
private:

public:
    resplistT()
    {
    }
    ~resplistT()
    {
    }

    void setUp()
    {
    }

    void tearDown()
    {
    }

CPPUNIT_TEST_SUITE( resplistT );

    CPPUNIT_TEST( do_test );

    CPPUNIT_TEST_SUITE_END()
    ;

    void do_test()
    {
        BESResponseHandler *rh = 0;

        cout << "*****************************************" << endl;
        cout << "Entered resplistT::run" << endl;

        cout << "*****************************************" << endl;
        cout << "add the 5 response handlers" << endl;
        BESResponseHandlerList *rhl = BESResponseHandlerList::TheList();
        char num[10];
        for (int i = 0; i < 5; i++) {
            sprintf(num, "resp%d", i);
            cout << "    adding " << num << endl;
            CPPUNIT_ASSERT( rhl->add_handler( num, TestResponseHandler::TestResponseBuilder ) == true );
        }

        cout << "*****************************************" << endl;
        cout << "try to add resp3 again" << endl;
        CPPUNIT_ASSERT( rhl->add_handler( "resp3", TestResponseHandler::TestResponseBuilder ) == false );

        cout << "*****************************************" << endl;
        cout << "finding the handlers" << endl;
        for (int i = 4; i >= 0; i--) {
            sprintf(num, "resp%d", i);
            cout << "    finding " << num << endl;
            rh = rhl->find_handler(num);
            CPPUNIT_ASSERT( rh );
#if 0
            CPPUNIT_ASSERT( rh->get_name() == num );
#endif
        }

        cout << "*****************************************" << endl;
        cout << "finding non-existant handler" << endl;
        rh = rhl->find_handler("not_there");
        CPPUNIT_ASSERT( !rh );

        cout << "*****************************************" << endl;
        cout << "removing resp2" << endl;
        CPPUNIT_ASSERT( rhl->remove_handler( "resp2" ) == true );
        rh = rhl->find_handler("resp2");
        CPPUNIT_ASSERT( !rh );

        cout << "*****************************************" << endl;
        cout << "add resp2 back" << endl;
        CPPUNIT_ASSERT( rhl->add_handler( "resp2", TestResponseHandler::TestResponseBuilder ) == true );

        rh = rhl->find_handler("resp2");
        CPPUNIT_ASSERT( rh );
#if 0
        CPPUNIT_ASSERT( rh->get_name() == "resp2" );
#endif

        cout << "*****************************************" << endl;
        cout << "Returning from resplistT::run" << endl;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( resplistT );

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
            cerr << "Usage: resplistT has the following tests:" << endl;
            const std::vector<Test*> &tests = resplistT::suite()->getTests();
            unsigned int prefix_len = resplistT::suite()->getName().append("::").length();
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
            test = resplistT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

