
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

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <sstream>
#include <iterator>

#include <BaseType.h>
#include <Byte.h>
#include <Int32.h>
#include <Float32.h>
#include <Float64.h>
#include <Str.h>
#include <Array.h>
#include <Structure.h>
#include <DDS.h>
#include <DMR.h>
#include <D4RValue.h>
#include <util.h>
#include <GetOpt.h>
#include <debug.h>

#include <test/TestTypeFactory.h>
#include <test/TestCommon.h>
#include <test/D4TestTypeFactory.h>

#include "test_config.h"
#include "test_utils.h"

#include "MakeMaskFunction.h"

using namespace CppUnit;
using namespace libdap;
using namespace std;

int test_variable_sleep_interval = 0;

static bool debug = false;
static bool debug2 = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);
#undef DBG2
#define DBG2(x) do { if (debug2) (x); } while(false);

namespace functions
{

class MakeMaskFunctionTest:public TestFixture
{
private:
    TestTypeFactory btf;
    D4TestTypeFactory d4_ttf;

public:
    MakeMaskFunctionTest()
    { }

    ~MakeMaskFunctionTest()
    {}

    void setUp() {
#if 0
        try {
            // Set up the arrays
            one_d_array = new Array("one_d_array", new Int32("one_d_array"));
            one_d_array->append_dim(10, "one");

            vector<dods_int32> values(10);
            for (int i = 0; i < 10; ++i) {
                values[i] = 10 * sin(i * 18);
            }
            DBG2(cerr << "Initial one D Array data values: ");
            DBG2(copy(values.begin(), values.end(), std::ostream_iterator<dods_int32>(std::cerr, " ")));
            DBG2(cerr << endl);
            one_d_array->set_value(values, values.size());

            two_d_array = new Array("two_d_array", new Int32("two_d_array"));
            two_d_array->append_dim(10, "one");
            two_d_array->append_dim(5, "two");

            vector<dods_int32> values2(50);
            for (int i = 0; i < 10; ++i)
                for (int j = 0; j < 5; ++j)
                    values2[j*10 + i] = 10 * sin(i * 18);
            DBG2(cerr << "Initial two D Array data values: ");
            DBG2(copy(values2.begin(), values2.end(), std::ostream_iterator<dods_int32>(std::cerr, " ")));
            DBG2(cerr << endl);
            two_d_array->set_value(values2, values2.size());

            float_2d_array = new Array("float_2d_array", new Float32("float_2d_array"));
            float_2d_array->append_dim(10, "one");
            float_2d_array->append_dim(5, "two");

            vector<dods_float32> values3(50);
            for (int i = 0; i < 10; ++i)
                for (int j = 0; j < 5; ++j)
                    values3[j*10 + i] = sin(i * 18);
            DBG2(cerr << "Initial two D Array data values: ");
            DBG2(copy(values3.begin(), values3.end(), std::ostream_iterator<dods_float32>(std::cerr, " ")));
            DBG2(cerr << endl);
            float_2d_array->set_value(values3, values3.size());

            // set up masks - masks are always Byte Arrays
            one_d_mask = new Array("one_d_mask", new Byte("one_d_mask"));
            one_d_mask->append_dim(10, "one");
            one_d_mask->set_value(d_mask, d_mask.size());
            one_d_mask->set_read_p(true);

            two_d_mask = new Array("two_d_mask", new Byte("two_d_mask"));
            two_d_mask->append_dim(10, "one");
            two_d_mask->append_dim(5, "two");
            two_d_mask->set_value(d_mask2, d_mask2.size());
            two_d_mask->set_read_p(true);
        }
        catch (Error & e) {
            cerr << "SetUp: " << e.get_error_message() << endl;
            throw;
        }
#endif
    }

    void tearDown() { }

    void no_arg_test() {
        DBG(cerr << "In no_arg_test..." << endl);

        BaseType *result = 0;
        try {
            BaseType *argv[] = { };
            DDS *dds = new DDS(&btf, "empty");
            function_dap2_make_mask(0, argv, *dds /* DDS & */, &result);
            CPPUNIT_ASSERT(result->type() == dods_str_c);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("mask_array() Should throw an exception when called with no arguments");
        }
        catch (...) {
            CPPUNIT_FAIL("unknown exception.");
        }

        DBG(cerr << "Out no_arg_test" << endl);
    }

    void find_value_index_test() {
        // I googled for this way to init std::vector<>.
        // See http://www.cplusplus.com/reference/vector/vector/vector/
        double init_values[] = {1,2,3,4,5,6,7,8,9,10};
        vector<double> data (init_values, init_values + sizeof(init_values) / sizeof(double) );
        DBG2(cerr << "data values: ");
        DBG2(copy(data.begin(), data.end(), ostream_iterator<double>(cerr, " ")));
        DBG2(cerr << endl);

        CPPUNIT_ASSERT(find_value_index(4.0, data) == 3);

        CPPUNIT_ASSERT(find_value_index(11.0, data) == -1);
    }

    void find_value_indices_test() {
        double init_values[] = {1,2,3,4,5,6,7,8,9,10};
        vector<double> row (init_values, init_values + sizeof(init_values) / sizeof(double) );
        DBG2(cerr << "row values: ");
        DBG2(copy(row.begin(), row.end(), ostream_iterator<double>(cerr, " ")));
        DBG2(cerr << endl);

        double init_values2[] = {10,20,30,40,50,60,70,80,90,100};
        vector<double> col (init_values2, init_values2 + sizeof(init_values2) / sizeof(double) );
        DBG2(cerr << "col values: ");
        DBG2(copy(col.begin(), col.end(), ostream_iterator<double>(cerr, " ")));
        DBG2(cerr << endl);

        vector< vector<double> > maps;
        maps.push_back(row);
        maps.push_back(col);

        vector<double> tuple;
        tuple.push_back(4.0);
        tuple.push_back(40.0);

        vector<int> ind = find_value_indices(tuple, maps);
        CPPUNIT_ASSERT(ind.at(0) == 3);
        CPPUNIT_ASSERT(ind.at(1) == 3);

        vector<double> t2;
        t2.push_back(4.0);
        t2.push_back(41.5);

        ind = find_value_indices(t2, maps);
        CPPUNIT_ASSERT(ind.at(0) == 3);
        CPPUNIT_ASSERT(ind.at(1) == -1);
    }

    CPPUNIT_TEST_SUITE( MakeMaskFunctionTest );

    CPPUNIT_TEST(no_arg_test);
    CPPUNIT_TEST(find_value_index_test);
    CPPUNIT_TEST(find_value_indices_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(MakeMaskFunctionTest);

} // namespace functions

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
            test = string("functions::MakeMaskFunctionTest::") + argv[i++];

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
