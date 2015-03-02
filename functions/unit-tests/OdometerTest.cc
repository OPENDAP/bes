
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2015 OPeNDAP, Inc.
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

// Tests for the AISResources class.

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <sstream>
#include <vector>

#include <util.h>
#include <GetOpt.h>
#include <debug.h>

#include "test_config.h"
#include "test_utils.h"

#include "Odometer.h"

using namespace CppUnit;
using namespace libdap;
using namespace std;

static bool debug = false;
static bool debug2 = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);
#undef DBG2
#define DBG2(x) do { if (debug2) (x); } while(false);

namespace libdap
{

class OdometerTest: public TestFixture
{
private:

public:
	OdometerTest()
    {}
    ~OdometerTest()
    {}

    void setUp()
    {
        try {
        }
        catch (Error & e) {
            cerr << "SetUp: " << e.get_error_message() << endl;
            throw;
        }
    }

    void tearDown()
    {
    }

    void ctor_test() {
    	vector<unsigned int>shape(3);
    	shape.at(0) = 10;
    	shape.at(1) = 20;
    	shape.at(2) = 30;
    	Odometer od(shape);

    	CPPUNIT_ASSERT(od.offset() == 0);
    	CPPUNIT_ASSERT(od.end() == 10 * 20 *30);
    	vector<unsigned int>indices = od.indices();
    	CPPUNIT_ASSERT(indices.size() == 3);
    	CPPUNIT_ASSERT(indices.at(0) == 0);
    	CPPUNIT_ASSERT(indices.at(1) == 0);
    	CPPUNIT_ASSERT(indices.at(2) == 0);
    }

    void next_test() {
    	vector<unsigned int>shape(3);
    	shape.at(0) = 10;
    	shape.at(1) = 20;
    	shape.at(2) = 30;
    	Odometer od(shape);

    	for (int i = 0; i < 29; ++i )
    		od.next();

    	CPPUNIT_ASSERT(od.offset() != od.end());
    	CPPUNIT_ASSERT(od.offset() == 29);
    	vector<unsigned int>indices = od.indices();
    	CPPUNIT_ASSERT(indices.size() == 3);
    	CPPUNIT_ASSERT(indices.at(0) == 0);
    	CPPUNIT_ASSERT(indices.at(1) == 0);
    	CPPUNIT_ASSERT(indices.at(2) == 29);

    	od.next();

    	CPPUNIT_ASSERT(od.offset() != od.end());
    	CPPUNIT_ASSERT(od.offset() == 30);
    	indices = od.indices();
    	CPPUNIT_ASSERT(indices.size() == 3);
    	CPPUNIT_ASSERT(indices.at(0) == 0);
    	CPPUNIT_ASSERT(indices.at(1) == 1);
    	CPPUNIT_ASSERT(indices.at(2) == 0);

    	for (int i = 0; i < 600 - 30; ++i )
    		od.next();

    	CPPUNIT_ASSERT(od.offset() != od.end());
    	CPPUNIT_ASSERT(od.offset() == 600);
    	indices = od.indices();
    	CPPUNIT_ASSERT(indices.size() == 3);
    	CPPUNIT_ASSERT(indices.at(0) == 1);
    	CPPUNIT_ASSERT(indices.at(1) == 0);
    	CPPUNIT_ASSERT(indices.at(2) == 0);

    	// 6000 is the size of the array
    	for (int i = 0; i < 5999 - 600; ++i )
    		od.next();

    	CPPUNIT_ASSERT(od.offset() != od.end());
    	CPPUNIT_ASSERT(od.offset() == 5999);
    	indices = od.indices();
    	CPPUNIT_ASSERT(indices.size() == 3);
    	CPPUNIT_ASSERT(indices.at(0) == 9);
    	CPPUNIT_ASSERT(indices.at(1) == 19);
    	CPPUNIT_ASSERT(indices.at(2) == 29);

    	od.next();

    	CPPUNIT_ASSERT(od.offset() == od.end());
    }

    // This test always passes, but how long does it take?
    // We can try different versions of next() and see if there's
    // much difference.
    void time_test() {
    	vector<unsigned int>shape(3);
    	shape.at(0) = 1000;
    	shape.at(1) = 1000;
    	shape.at(2) = 1000;
    	Odometer od(shape);

    	unsigned int len = 1000 * 1000 * 1000;
    	for (int i = 0; i < len; ++i)
    		od.next();

    	CPPUNIT_ASSERT(od.offset() == od.end());
    }

    CPPUNIT_TEST_SUITE( OdometerTest );

    CPPUNIT_TEST(ctor_test);
    CPPUNIT_TEST(next_test);
    CPPUNIT_TEST(time_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(OdometerTest);

} // namespace libdap

int main(int argc, char*argv[]) {
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "dD");
    char option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'D':
            debug2 = 1;
            break;
        default:
            break;
        }

    bool wasSuccessful = true;
    string test = "";
    int i = getopt.optind;
     if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        while (i < argc) {
            test = string("libdap::OdometerTest::") + argv[i++];

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
