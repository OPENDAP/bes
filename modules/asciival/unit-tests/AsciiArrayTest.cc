// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of asciival, software which can return an ASCII
// representation of the data read from a DAP server.

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

// Tests for the DataDDS class.

#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include "DDS.h"

//#include "name_map.h"
#include "AsciiArray.h"
#include "AsciiOutputFactory.h"
#include "test_config.h"
#include <GetOpt.h>

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

bool translate = false;
using namespace CppUnit;
using namespace std;

class AsciiArrayTest: public TestFixture {
private:
    DDS *dds1;
    AsciiArray *a, *b, *c, *d;
    AsciiOutputFactory *aof;

public:
    AsciiArrayTest()
    {
    }
    ~AsciiArrayTest()
    {
    }

    void setUp()
    {
        aof = new AsciiOutputFactory;
        dds1 = new DDS(aof, "ascii_array_test");
        try {
            string parsefile = (string) TEST_SRC_DIR + "/testsuite/AsciiArrayTest1.dds";
            dds1->parse(parsefile);
            DDS::Vars_iter p = dds1->var_begin();
            a = dynamic_cast<AsciiArray*>(*p++);
            b = dynamic_cast<AsciiArray*>(*p++);
            c = dynamic_cast<AsciiArray*>(*p++);
            d = dynamic_cast<AsciiArray*>(*p++);
        }
        catch (Error &e) {
            cerr << "Caught Error in setUp: " << e.get_error_message() << endl;
        }
        catch (exception &e) {
            cerr << "Caught std::exception in setUp: " << e.what() << endl;
        }
    }

    void tearDown()
    {
        delete aof;
        aof = 0;
        delete dds1;
        dds1 = 0;
    }

CPPUNIT_TEST_SUITE( AsciiArrayTest );

    CPPUNIT_TEST(test_get_nth_dim_size);
    CPPUNIT_TEST(test_get_shape_vector);
    CPPUNIT_TEST(test_get_index);

    CPPUNIT_TEST_SUITE_END()
    ;

    void test_get_nth_dim_size()
    {
        CPPUNIT_ASSERT(a->get_nth_dim_size(0) == 10);

        CPPUNIT_ASSERT(b->get_nth_dim_size(0) == 10);
        CPPUNIT_ASSERT(b->get_nth_dim_size(1) == 10);

        CPPUNIT_ASSERT(c->get_nth_dim_size(0) == 5);
        CPPUNIT_ASSERT(c->get_nth_dim_size(1) == 5);
        CPPUNIT_ASSERT(c->get_nth_dim_size(2) == 5);

        try {
            a->get_nth_dim_size((unsigned long) -1);
            CPPUNIT_ASSERT(false);
        }
        catch (InternalErr &ie) {
            CPPUNIT_ASSERT(true);
        }
        try {
            a->get_nth_dim_size(1);
            CPPUNIT_ASSERT(false);
        }
        catch (InternalErr &ie) {
            CPPUNIT_ASSERT(true);
        }

        try {
            c->get_nth_dim_size(3);
            CPPUNIT_ASSERT(false);
        }
        catch (InternalErr &ie) {
            CPPUNIT_ASSERT(true);
        }
    }

    void test_get_shape_vector()
    {
        try {
            vector<int> a_shape(1, 10);

#if defined(DODS_DEBUG)
            cerr << "a_shape: ";
            copy(a_shape.begin(), a_shape.end(),
                ostream_iterator<int>(cerr, ", "));
            cerr << endl;
            vector<int> response_shape = a->get_shape_vector(1);

            cerr << "response_shape: ";
            copy(response_shape.begin(), response_shape.end(),
                ostream_iterator<int>(cerr, ", "));
            cerr << endl;
#endif
            CPPUNIT_ASSERT(a->get_shape_vector(1) == a_shape);

            vector<int> b_shape(2, 10);
            CPPUNIT_ASSERT(b->get_shape_vector(2) == b_shape);

            vector<int> c_shape(3, 5);
            CPPUNIT_ASSERT(c->get_shape_vector(3) == c_shape);

            vector<int> d_shape(3);
            d_shape[0] = 3;
            d_shape[1] = 4;
            d_shape[2] = 5;

#if defined(DODS_DEBUG)
            cerr << "d_shape: ";
            copy(d_shape.begin(), d_shape.end(),
                ostream_iterator<int>(cerr, ", "));
            cerr << endl;
            response_shape = d->get_shape_vector(3);

            cerr << "response_shape: ";
            copy(response_shape.begin(), response_shape.end(),
                ostream_iterator<int>(cerr, ", "));
            cerr << endl;
#endif
            CPPUNIT_ASSERT(d->get_shape_vector(3) == d_shape);

            try {
                a->get_shape_vector(0);
                CPPUNIT_ASSERT(false);
            }
            catch (InternalErr &ie) {
                CPPUNIT_ASSERT(true);
            }
            try {
                a->get_shape_vector(2);
                CPPUNIT_ASSERT(false);
            }
            catch (InternalErr &ie) {
                CPPUNIT_ASSERT(true);
            }

            try {
                d->get_shape_vector(5);
                CPPUNIT_ASSERT(false);
            }
            catch (InternalErr &ie) {
                CPPUNIT_ASSERT(true);
            }
        }
        catch (Error &e) {
            cerr << "Error: " << e.get_error_message() << endl;
            CPPUNIT_ASSERT(false);
        }
    }

    void test_get_index()
    {
        try {
            vector<int> a_state(1);
            a_state[0] = 0;
            CPPUNIT_ASSERT(a->get_index(a_state) == 0);
            a_state[0] = 9;
            CPPUNIT_ASSERT(a->get_index(a_state) == 9);

            vector<int> b_state(2, 0);
            CPPUNIT_ASSERT(b->get_index(b_state) == 0);
            b_state[0] = 0;
            b_state[1] = 5;
            CPPUNIT_ASSERT(b->get_index(b_state) == 5);
            b_state[0] = 5;
            b_state[1] = 5;
            CPPUNIT_ASSERT(b->get_index(b_state) == 55);
            b_state[0] = 9;
            b_state[1] = 9;
            CPPUNIT_ASSERT(b->get_index(b_state) == 99);

            vector<int> d_state(4, 0);
            CPPUNIT_ASSERT(d->get_index(d_state) == 0);
            d_state[0] = 2;
            d_state[1] = 3;
            d_state[2] = 4;
            d_state[3] = 5;
            CPPUNIT_ASSERT(d->get_index(d_state) == 359);

            d_state[0] = 1;
            d_state[1] = 2;
            d_state[2] = 0;
            d_state[3] = 2;
            CPPUNIT_ASSERT(d->get_index(d_state) == 1 * (4 * 5 * 6) + 2 * (5 * 6) + 0 * (6) + 2);
        }
        catch (Error &e) {
            cerr << "Error: " << e.get_error_message() << endl;
            CPPUNIT_ASSERT(false);
        }
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(AsciiArrayTest);

int main(int argc, char*argv[])
{

    GetOpt getopt(argc, argv, "dh");
    int option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: AsciiArrayTest has the following tests:" << endl;
            const std::vector<Test*> &tests = AsciiArrayTest::suite()->getTests();
            unsigned int prefix_len = AsciiArrayTest::suite()->getName().append("::").length();
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
            test = AsciiArrayTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

