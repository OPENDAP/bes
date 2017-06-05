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
using namespace functions;

int test_variable_sleep_interval = 0;

static bool debug = false;
static bool debug2 = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);
#undef DBG2
#define DBG2(x) do { if (debug2) (x); } while(false);

namespace functions {

class MakeMaskFunctionTest: public TestFixture {
private:
    TestTypeFactory btf;
    D4TestTypeFactory d4_ttf;

    Array *dim0, *dim1;

public:
    MakeMaskFunctionTest() :
        dim0(0), dim1(0)
    {
    }

    ~MakeMaskFunctionTest()
    {
    }

    void setUp()
    {
        try {
            // Set up the arrays
            const int dim_10 = 10;
            dim0 = new Array("dim0", new Float32("dim0"));
            dim0->append_dim(dim_10, "one");

            vector<dods_float32> values;
            for (int i = 0; i < dim_10; ++i) {
                values.push_back(dim_10 * sin(i * 180 / dim_10));
            }
            DBG2(cerr << "Initial one D Array data values: ");
            DBG2(copy(values.begin(), values.end(), ostream_iterator<dods_float32>(cerr, " ")));
            DBG2(cerr << endl);
            dim0->set_value(values, values.size());

            // Set up smaller array
            const int dim_4 = 4;
            dim1 = new Array("dim1", new Float32("dim1"));
            dim1->append_dim(dim_4, "one");

            values.clear();
            for (int i = 0; i < dim_4; ++i) {
                values.push_back(dim_4 * sin(i * 180 / dim_4));
            }
            DBG2(cerr << "Initial one D Array data values: ");
            DBG2(copy(values.begin(), values.end(), ostream_iterator<dods_float32>(cerr, " ")));
            DBG2(cerr << endl);
            dim1->set_value(values, values.size());

#if 0
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
#endif
        }
        catch (Error & e) {
            cerr << "SetUp: " << e.get_error_message() << endl;
            throw;
        }
    }

    void tearDown()
    {
    }

    void no_arg_test()
    {
        DBG(cerr << "In no_arg_test..." << endl);

        BaseType *result = 0;
        try {
            BaseType *argv[] = {};
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

    void find_value_index_test()
    {
        // I googled for this way to init std::vector<>.
        // See http://www.cplusplus.com/reference/vector/vector/vector/
        double init_values[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        vector<double> data(init_values, init_values + sizeof(init_values) / sizeof(double));
        DBG2(cerr << "data values: ");
        DBG2(copy(data.begin(), data.end(), ostream_iterator<double>(cerr, " ")));
        DBG2(cerr << endl);

        CPPUNIT_ASSERT(find_value_index(4.0, data) == 3);

        CPPUNIT_ASSERT(find_value_index(11.0, data) == -1);
    }

    void find_value_indices_test()
    {
        double init_values[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
        vector<double> row(init_values, init_values + sizeof(init_values) / sizeof(double));
        DBG2(cerr << "row values: ");
        DBG2(copy(row.begin(), row.end(), ostream_iterator<double>(cerr, " ")));
        DBG2(cerr << endl);

        double init_values2[] = { 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 };
        vector<double> col(init_values2, init_values2 + sizeof(init_values2) / sizeof(double));
        DBG2(cerr << "col values: ");
        DBG2(copy(col.begin(), col.end(), ostream_iterator<double>(cerr, " ")));
        DBG2(cerr << endl);

        vector<vector<double> > maps;
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

    void all_indices_valid_test()
    {
        vector<int> i1;
        i1.push_back(3);
        i1.push_back(7);

        CPPUNIT_ASSERT(all_indices_valid(i1));

        vector<int> i2;
        i2.push_back(3);
        i2.push_back(-1);

        CPPUNIT_ASSERT(!all_indices_valid(i2));
    }

    void make_mask_helper_test_1()
    {
        vector<dods_byte> mask(10);     // Caller must make a big enough mask

        vector<Array*> dims;
        dims.push_back(dim0);   // dim0 has 18 values that are 10 * sin(i * 18)

        Array *tuples = new Array("mask", new Float32("mask"));
        vector<dods_float32> tuples_values;
        tuples_values.push_back(10 * sin(2 * 18));
        tuples_values.push_back(10 * sin(3 * 18));
        tuples_values.push_back(10 * sin(4 * 18));
        tuples_values.push_back(10 * sin(5 * 18));
        tuples->set_value(tuples_values, tuples_values.size());

        DBG(cerr << "Tuples: ");
        DBG(copy(tuples_values.begin(), tuples_values.end(), ostream_iterator<dods_float32>(cerr, " ")));
        DBG(cerr << endl);

        // NB: mask is a value-result parameter passed by reference
        make_mask_helper<dods_float32>(dims, tuples, mask);
        DBG(cerr << "One D mask: ");
        DBG(copy(mask.begin(), mask.end(), ostream_iterator<int>(cerr, " ")));
        DBG(cerr << endl);

        CPPUNIT_ASSERT(mask.at(0) == 0);
        CPPUNIT_ASSERT(mask.at(1) == 0);
        CPPUNIT_ASSERT(mask.at(2) == 1);
        CPPUNIT_ASSERT(mask.at(3) == 1);
        CPPUNIT_ASSERT(mask.at(4) == 1);
        CPPUNIT_ASSERT(mask.at(5) == 1);
        CPPUNIT_ASSERT(mask.at(6) == 0);
        CPPUNIT_ASSERT(mask.at(7) == 0);
        CPPUNIT_ASSERT(mask.at(8) == 0);
        CPPUNIT_ASSERT(mask.at(9) == 0);
    }

    void make_mask_helper_test_2()
    {
        vector<dods_byte> mask(100);     // Caller must make a big enough mask

        vector<Array*> dims;
        dims.push_back(dim0);   // dim0 has 18 values that are 10 * sin(i * 18)
        dims.push_back(dim0);   // dim0 has 18 values that are 10 * sin(i * 18)

        Array *tuples = new Array("mask", new Float32("mask"));
        vector<dods_float32> tuples_values;
        tuples_values.push_back(10 * sin(2 * 18));
        tuples_values.push_back(10 * sin(2 * 18));
        tuples_values.push_back(10 * sin(3 * 18));
        tuples_values.push_back(10 * sin(3 * 18));
        tuples_values.push_back(10 * sin(4 * 18));
        tuples_values.push_back(10 * sin(4 * 18));
        tuples_values.push_back(10 * sin(5 * 18));
        tuples_values.push_back(10 * sin(5 * 18));
        tuples->set_value(tuples_values, tuples_values.size());

        DBG(cerr << "Tuples: ");
        DBG(copy(tuples_values.begin(), tuples_values.end(), std::ostream_iterator<dods_float32>(std::cerr, " ")));
        DBG(cerr << endl);

        // NB: mask is a value-result parameter passed by reference
        make_mask_helper<dods_float32>(dims, tuples, mask);
        DBG(cerr << "Two D mask: " << dec);
        DBG(copy(mask.begin(), mask.end(), std::ostream_iterator<int>(std::cerr, " ")));
        DBG(cerr << endl);

        CPPUNIT_ASSERT(mask.at(0) == 0);
        CPPUNIT_ASSERT(mask.at(21) == 0);
        CPPUNIT_ASSERT(mask.at(22) == 1);
        CPPUNIT_ASSERT(mask.at(33) == 1);
        CPPUNIT_ASSERT(mask.at(44) == 1);
        CPPUNIT_ASSERT(mask.at(55) == 1);
        CPPUNIT_ASSERT(mask.at(56) == 0);
        CPPUNIT_ASSERT(mask.at(70) == 0);
        CPPUNIT_ASSERT(mask.at(80) == 0);
        CPPUNIT_ASSERT(mask.at(99) == 0);
    }

    void make_mask_helper_test_3()
    {
        vector<dods_byte> mask(160);     // Caller must make a big enough mask

        vector<Array*> dims;
        dims.push_back(dim0);   // dim0 has 18 values that are 10 * sin(i * 18)
        dims.push_back(dim1);   // dim0 has 18 values that are 10 * sin(i * 18)
        dims.push_back(dim1);

        Array *tuples = new Array("mask", new Float32("mask"));
        vector<dods_float32> tuples_values;
        tuples_values.push_back(10 * sin(0 * 18));
        tuples_values.push_back(4 * sin(0 * 45));
        tuples_values.push_back(4 * sin(0 * 45));

        tuples_values.push_back(10 * sin(2 * 18));
        tuples_values.push_back(4 * sin(2 * 45));
        tuples_values.push_back(4 * sin(2 * 45));

        tuples_values.push_back(10 * sin(3 * 18));
        tuples_values.push_back(4 * sin(3 * 45));
        tuples_values.push_back(4 * sin(3 * 45));

        tuples->set_value(tuples_values, tuples_values.size());

        DBG(cerr << "Tuples: ");
        DBG(copy(tuples_values.begin(), tuples_values.end(), std::ostream_iterator<dods_float32>(std::cerr, " ")));
        DBG(cerr << endl);

        // NB: mask is a value-result parameter passed by reference
        make_mask_helper<dods_float32>(dims, tuples, mask);
        DBG(cerr << "Three D mask: " << dec);
        DBG(copy(mask.begin(), mask.end(), std::ostream_iterator<int>(std::cerr, " ")));
        DBG(cerr << endl);

        // To figure out the offsets I used what I knew to be the 3D indices of the tuples
        // (0,0,0 and 2,2,2 and 3,3,3) along with the algorithm in Odometer.h
        CPPUNIT_ASSERT(mask.at(0) == 1);
        CPPUNIT_ASSERT(mask.at(1) == 0);
        CPPUNIT_ASSERT(mask.at(41) == 0);
        CPPUNIT_ASSERT(mask.at(42) == 1);
        CPPUNIT_ASSERT(mask.at(43) == 0);
        CPPUNIT_ASSERT(mask.at(62) == 0);
        CPPUNIT_ASSERT(mask.at(63) == 1);
        CPPUNIT_ASSERT(mask.at(64) == 0);
        CPPUNIT_ASSERT(mask.at(159) == 0);
    }

CPPUNIT_TEST_SUITE( MakeMaskFunctionTest );

    CPPUNIT_TEST(no_arg_test);
    CPPUNIT_TEST(find_value_index_test);
    CPPUNIT_TEST(find_value_indices_test);
    CPPUNIT_TEST(all_indices_valid_test);
    CPPUNIT_TEST(make_mask_helper_test_1);
    CPPUNIT_TEST(make_mask_helper_test_2);
    CPPUNIT_TEST(make_mask_helper_test_3);

    CPPUNIT_TEST_SUITE_END()
    ;
};

CPPUNIT_TEST_SUITE_REGISTRATION(MakeMaskFunctionTest);

} // namespace functions

int main(int argc, char*argv[])
{

    GetOpt getopt(argc, argv, "dDh");
    char option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'D':
            debug2 = 1;
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: MakeMaskFunctionTest has the following tests:" << endl;
            const std::vector<Test*> &tests = MakeMaskFunctionTest::suite()->getTests();
            unsigned int prefix_len = MakeMaskFunctionTest::suite()->getName().append("::").length();
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
            test = MakeMaskFunctionTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
