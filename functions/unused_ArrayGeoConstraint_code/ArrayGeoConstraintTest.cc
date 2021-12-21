
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2006 OPeNDAP, Inc.
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
#include <functions/unused_ArrayGeoConstraint_code/ArrayGeoConstraint.h>
#include <functions/unused_ArrayGeoConstraint_code/ce_functions.h>

//#define DODS_DEBUG

#include "GetOpt.h"

#include <libdap/BaseType.h>
#include <libdap/Byte.h>
#include <libdap/Int32.h>
#include <libdap/Float64.h>
#include <libdap/Str.h>
#include <libdap/Array.h>
#include <libdap/Grid.h>
#include <libdap/DDS.h>
#include <libdap/DAS.h>

#include "test/TestArray.h"
#include "test/TestTypeFactory.h"

#include <libdap/debug.h>

using namespace CppUnit;
using namespace libdap;
using namespace std;

int test_variable_sleep_interval = 0;


static bool debug = false;
#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

namespace libdap
{

class ArrayGeoConstraintTest:public TestFixture
{
private:
    TestTypeFactory btf;
    ConstraintEvaluator ce;

    TestArray *a1, *a2, *a3;

public:
    ArrayGeoConstraintTest()
    {}
    ~ArrayGeoConstraintTest()
    {}

    void setUp()
    {
        a1 = new TestArray("test1", new Int32("test1"));
        a1->append_dim(21); // latitude
        a1->append_dim(10); // longitude

        a2 = new TestArray("test2", new Int32("test2"));
        a2->append_dim(21); // latitude
        a2->append_dim(10); // longitude

        a3 = new TestArray("test3", new Byte("test3"));
        a3->append_dim(10); // latitude
        a3->append_dim(10); // longitude
        dods_byte tmp_data[10][10] =
                { { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
                  { 10,11,12,13,14,15,16,17,18,19},
                  { 20,21,22,23,24,25,26,27,28,29},
                  { 30,31,32,33,34,35,36,37,38,39},
                  { 40,41,42,43,44,45,46,47,48,49},
                  { 50,51,52,53,54,55,56,57,58,59},
                  { 60,61,62,63,64,65,66,67,68,69},
                  { 70,71,72,73,74,75,76,77,78,79},
                  { 80,81,82,83,84,85,86,87,88,89},
                  { 90,91,92,93,94,95,96,97,98,99} };
        a3->val2buf((void*)tmp_data);
        a3->set_read_p(true);


    }

    void tearDown()
    {
        delete a1; a1 = 0;
        delete a2; a2 = 0;
        delete a3; a3 = 0;
    }

    CPPUNIT_TEST_SUITE( ArrayGeoConstraintTest );

    CPPUNIT_TEST(constructor_test);
    CPPUNIT_TEST(build_lat_lon_maps_test);
    CPPUNIT_TEST(set_bounding_box_test);

    CPPUNIT_TEST_SUITE_END();

    void constructor_test() {
        TestArray *ta = new TestArray("test", new Int32("test"));
        ta->append_dim(10);

        try {
            ArrayGeoConstraint agc(ta);
            CPPUNIT_ASSERT(!"Constructor should throw");
        }
        catch (Error &e) {
            DBG(cerr << "Error: " << e.get_error_message() << endl);
            CPPUNIT_ASSERT("Caught Error");
        }

        try {
            ArrayGeoConstraint agc(ta, 10, 10, 89.9, 89);
            CPPUNIT_ASSERT(!"Constructor should throw");
        }
        catch (Error &e) {
            DBG(cerr << "Error: " << e.get_error_message() << endl);
            CPPUNIT_ASSERT("Caught Error");
        }

        ta->append_dim(20);
        try {
            ArrayGeoConstraint agc(ta, 10, 10, 89.9, 89);
            CPPUNIT_ASSERT(agc.d_extent.d_left == 10
                           && agc.d_extent.d_top == 10
                           && agc.d_extent.d_right == 89
                           && agc.d_extent.d_bottom == 89.9);
            CPPUNIT_ASSERT(agc.d_projection.d_name == "plat-carre"
                           && agc.d_projection.d_datum == "wgs84");
        }
        catch (Error &e) {
            DBG(cerr << "Error: " << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Constructor should not throw");
        }

        try {
            ArrayGeoConstraint agc(ta, 10, 10, 89.9, 89,
                                   "plat-carre", "wgs84");
            CPPUNIT_ASSERT(agc.d_extent.d_left == 10
                           && agc.d_extent.d_top == 10
                           && agc.d_extent.d_right == 89
                           && agc.d_extent.d_bottom == 89.9);
            CPPUNIT_ASSERT(agc.d_projection.d_name == "plat-carre"
                           && agc.d_projection.d_datum == "wgs84");
        }
        catch (Error &e) {
            DBG(cerr << "Error: " << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Constructor should not throw");
        }

        try {
            ArrayGeoConstraint agc(ta, 10, 10, 89.9, 89,
                                   "plat-carre", "huh?");
            CPPUNIT_ASSERT(!"Constructor should throw Error");
        }
        catch (Error &e) {
            DBG(cerr << "Error: " << e.get_error_message() << endl);
            CPPUNIT_ASSERT("Caught Error");
        }
    }

    void build_lat_lon_maps_test() {
        // build_lat_lon_maps() is called in the ctor
        ArrayGeoConstraint agc(a1, 90, 10, -90, 89);

        CPPUNIT_ASSERT(agc.get_lon()[0] == 10.0);
        DBG(cerr << "agc.get_lon()[9]: " << agc.get_lon()[9] << endl);
        CPPUNIT_ASSERT(agc.get_lon()[9] == 89.0);
        CPPUNIT_ASSERT(agc.get_lat()[0] == 90.0);
        CPPUNIT_ASSERT(agc.get_lat()[20] == -90.0);
    }

    void set_bounding_box_test() {
	try {
	    ArrayGeoConstraint agc1(a1, 90, 10, -90, 89);
	    agc1.set_bounding_box(90, 10, -90, 89);
	    CPPUNIT_ASSERT(agc1.get_longitude_index_left() == 0);
	    CPPUNIT_ASSERT(agc1.get_longitude_index_right() == 9);
	    CPPUNIT_ASSERT(agc1.get_latitude_index_top() == 0);
	    CPPUNIT_ASSERT(agc1.get_latitude_index_bottom() == 20);

	    ArrayGeoConstraint agc2(a2, 90, 0, -90, 359);
	    agc2.set_bounding_box(45, 10, -45, 89);
	    DBG(cerr << "agc2.get_longitude_index_left(): " << agc2.get_longitude_index_left() << endl); DBG(cerr << "agc2.get_longitude_index_right(): " << agc2.get_longitude_index_right() << endl); DBG(cerr << "agc2.get_latitude_index_top(): " << agc2.get_latitude_index_top() << endl); DBG(cerr << "agc2.get_latitude_index_bottom(): " << agc2.get_latitude_index_bottom() << endl);
	    CPPUNIT_ASSERT(agc2.get_longitude_index_left() == 0);
	    CPPUNIT_ASSERT(agc2.get_longitude_index_right() == 3);
	    CPPUNIT_ASSERT(agc2.get_latitude_index_top() == 5);
	    CPPUNIT_ASSERT(agc2.get_latitude_index_bottom() == 15);
	}
	catch (Error &e) {
	    CPPUNIT_FAIL(e.get_error_message());
	}
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(ArrayGeoConstraintTest);

} // namespace libdap


int main(int argc, char*argv[]) {
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "d");
    int option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
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
            test = string("ugrid::BindTest::") + argv[i++];

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}



#if 0


int
main( int, char** )
{
    CppUnit::TextTestRunner runner;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );

    bool wasSuccessful = runner.run( "", false ) ;

    return wasSuccessful ? 0 : 1;
}

#endif
