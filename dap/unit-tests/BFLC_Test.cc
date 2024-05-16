//
// Created by ndp on 5/16/24.
//
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <unistd.h>

#include <sys/types.h>
#include <stdio.h>

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <unistd.h>  // for stat
#include <sstream>


#include "test_utils.h"
#include "test_config.h"

using namespace CppUnit;
using namespace std;


static bool debug = false;
static bool debug_2 = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);
#undef DBG2
#define DBG2(x) do { if (debug_2) (x); } while(false);


#define MODULE "dap"
#define prolog std::string("BFLC_Test::").append(__func__).append("() - ")

static bool parser_debug = false;



class BFLC_Test: public TestFixture {

public:
    BFLC_Test(){}

    ~BFLC_Test()
    {
    }

    void setUp()
    {
        DBG2(cerr << prolog << "BEGIN" << endl);

        DBG2(cerr << prolog << "END" << endl);
    }

    void tearDown()
    {
        DBG2(cerr << prolog << "BEGIN" << endl);
        DBG2(cerr << prolog << "END" << endl);
    }


    void dummy_test(){
        DBG(cerr << endl << prolog << "BEGIN" << endl);
        DBG(cerr << prolog << "NOTHING WILL BE DONE." << endl);
        DBG(cerr << prolog << "END" << endl);
    }

    CPPUNIT_TEST_SUITE( BFLC_Test );

        CPPUNIT_TEST(dummy_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(BFLC_Test);

int main(int argc, char*argv[])
{
    int option_char;
    while ((option_char = getopt(argc, argv, "dDh")) != -1)
        switch (option_char) {
            case 'd':
                debug = 1;  // debug is a static global
                break;
            case 'D':
                debug_2 = 1;
                break;
            case 'h': {     // help - show test names
                cerr << "Usage: ResponseBuilderTest has the following tests:" << endl;
                const std::vector<Test*> &tests = BFLC_Test::suite()->getTests();
                unsigned int prefix_len = BFLC_Test::suite()->getName().append("::").size();
                for (std::vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                    cerr << (*i)->getName().replace(0, prefix_len, "") << endl;
                }
                break;
            }
            default:
                break;
        }

    argc -= optind;
    argv += optind;

    // clean out the response_cache dir

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    string test = "";
    if (0 == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        int i = 0;
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = BFLC_Test::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
