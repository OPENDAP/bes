
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

#include <BaseType.h>
#include <Byte.h>
#include <Int32.h>
#include <Str.h>
#include <Array.h>
#include <Structure.h>
#include <DDS.h>
#include <util.h>
#include <GetOpt.h>
#include <debug.h>

#include <test/TestTypeFactory.h>
#include <test/TestCommon.h>

#include "test_config.h"
#include "test_utils.h"

#include "MaskArrayFunction.h"

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

class MaskArrayFunctionTest:public TestFixture
{
private:
    TestTypeFactory btf;
    // ConstraintEvaluator ce;

    Array *one_d_array, *two_d_array;
    Array *one_d_mask, *two_d_mask;

    vector<dods_byte> d_mask, d_mask2;

public:
    MaskArrayFunctionTest(): one_d_array(0), two_d_array(0), one_d_mask(0), two_d_mask(0)
    {
        d_mask.reserve(10);
        for (int i = 0; i < 10; ++i)
            d_mask[i] = (i < 5) ? 1: 0;

        d_mask2.reserve(50);
        for (int i = 0; i < 10; ++i)
            for (int j = 0; j < 5; ++j)
                d_mask2[j*10 + i] = (i < 5) ? 1: 0;
    }

    ~MaskArrayFunctionTest()
    {}

    void setUp()
    {
        try {
            // Set up the arrays
            one_d_array = new Array("one_d_array", new Int32("one_d_array"));
            one_d_array->append_dim(10, "one");

            vector<dods_int32> values(10);
            for (int i = 0; i < 10; ++i) {
                values[i] = 10 * sin(i * 18);
            }
            DBG2(cerr << "Initial one D Array data values: ");
            DBG2(copy(values.begin(), values.end(), std::ostream_iterator<dods_int32>(std::cerr, ", ")));
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
            DBG2(copy(values2.begin(), values2.end(), std::ostream_iterator<dods_int32>(std::cerr, ", ")));
            DBG2(cerr << endl);
            two_d_array->set_value(values2, values2.size());

            // set up masks - masks are always Byte Arrays
            one_d_mask = new Array("one_d_mask", new Byte("one_d_mask"));
            one_d_mask->append_dim(10, "one");
            one_d_mask->set_value(d_mask, d_mask.size());

            two_d_mask = new Array("two_d_mask", new Byte("two_d_mask"));
            two_d_mask->append_dim(10, "one");
            two_d_mask->append_dim(5, "two");
            two_d_mask->set_value(d_mask2, d_mask2.size());
        }
        catch (Error & e) {
            cerr << "SetUp: " << e.get_error_message() << endl;
            throw;
        }
    }

    void tearDown()
    {
        delete one_d_array; one_d_array = 0;
        delete two_d_array; two_d_array = 0;

        delete one_d_mask; one_d_mask = 0;
        delete two_d_mask; two_d_mask = 0;
    }

    void no_arg_test() {
        DBG(cerr << "In no_arg_test..." << endl);

        BaseType *result = 0;
        try {
            BaseType *argv[] = { };
            DDS *dds = new DDS(&btf, "empty");
            function_mask_dap2_array(0, argv, *dds /* DDS & */, &result);
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

    void mask_array_one_d_test()
    {
        DBG(cerr << "In mask_array_one_d_test..." << endl);

        try {
            mask_array(one_d_array, 0, d_mask);

            vector<dods_int32> data(one_d_array->length());
            one_d_array->value(&data[0]);
            DBG(cerr << "Masked data values: ");
            DBG(copy(data.begin(), data.end(), std::ostream_iterator<dods_int32>(std::cerr, ", ")));
            DBG(cerr << endl);

            CPPUNIT_ASSERT(data[4] == 2 && data[5] == 0);       // ...could test all the values
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error: " + e.get_error_message());
        }

        DBG(cerr << "Out mask_array_one_d_test" << endl);
    }

    void mask_array_two_d_test()
    {
        DBG(cerr << "In mask_array_two_d_test..." << endl);

        try {
            mask_array(two_d_array, 0, d_mask2);

            vector<dods_int32> data(two_d_array->length());
            two_d_array->value(&data[0]);
            DBG(cerr << "Masked data values: ");
            DBG(copy(data.begin(), data.end(), std::ostream_iterator<dods_int32>(std::cerr, ", ")));
            DBG(cerr << endl);

            CPPUNIT_ASSERT(data[4] == 2 && data[5] == 0);       // ...could test all the values
            CPPUNIT_ASSERT(data[14] == 2 && data[15] == 0);
            CPPUNIT_ASSERT(data[24] == 2 && data[25] == 0);
            CPPUNIT_ASSERT(data[34] == 2 && data[35] == 0);
            CPPUNIT_ASSERT(data[44] == 2 && data[45] == 0);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error: " + e.get_error_message());
        }

        DBG(cerr << "Out mask_array_two_d_test" << endl);
    }

#if 0
    void string_arg_test() {
        BaseType *result = 0;
        try {
            Array a("values", new Str("values"));
            a.append_dim(5, "strings");

            // Must set up the args as per the CE parser
            Float64 min("min");
            min.set_value(40.0);
            min.set_read_p(true);
            Float64 max("max");
            max.set_value(50.0);
            max.set_read_p(true);
            BaseType *argv[] = { &a, &min, &max };
            function_dap2_bbox(0, argv, *float32_array /* DDS & */, &result);

            CPPUNIT_FAIL("bbox() Should throw an exception when called with a Str array");
        }
        catch (Error &e) {
            CPPUNIT_ASSERT(true);
        }
        catch (...) {
            CPPUNIT_FAIL("unknown exception.");
        }
    }

    void float32_array_test() {
        BaseType *btp = *(float32_array->var_begin());

        // It's an array
        CPPUNIT_ASSERT(btp->type() == dods_array_c);

        // ... and it's an Float32 array
        Array *a = static_cast<Array*>(btp);
        CPPUNIT_ASSERT(a->var()->type() == dods_float32_c);

        //  it has thirty elements
        CPPUNIT_ASSERT(a->length() == 30);

        TestCommon *tc = dynamic_cast<TestCommon*>(a);
        CPPUNIT_ASSERT(tc != 0);
        tc->set_series_values(true);

        DBG2(a->read());
        DBG2(cerr << "Array 'a': ");
        DBG2(a->print_val(cerr));

        BaseType *result = 0;
        try {
            // Must set up the args as per the CE parser
            Float64 min("min");
            min.set_value(40.0);
            min.set_read_p(true);
            Float64 max("max");
            max.set_value(50.0);
            max.set_read_p(true);

            BaseType *argv[] = { a, &min, &max };
            function_dap2_bbox(3, argv, *float32_array /* DDS & */, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error:" + e.get_error_message());
        }

        string baseline = readTestBaseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/float32_array_bbox.baseline.xml");
        ostringstream oss;
        result->print_xml(oss);

        DBG(cerr << "DDX of bbox()'s response: " << endl << oss.str() << endl);

        CPPUNIT_ASSERT(oss.str() == baseline);

        CPPUNIT_ASSERT(result->type() == dods_array_c);

        Array *result_array = static_cast<Array*>(result);

        DBG(oss.str(""));
        DBG(oss.clear());
        DBG(result->print_val(oss));
        DBG(cerr << "Result value: " << endl << oss.str() << endl);

        // we know it's a Structure * and it has one element because the test above passed
        Structure *indices = static_cast<Structure*>(result_array->var(0));
        CPPUNIT_ASSERT(indices != 0);

        Constructor::Vars_iter i = indices->var_begin();
        CPPUNIT_ASSERT(i != indices->var_end());
        CPPUNIT_ASSERT((*i)->name() == "start");
        CPPUNIT_ASSERT((*i)->type() == dods_int32_c);
        CPPUNIT_ASSERT(static_cast<Int32*>(*i)->value() == 8);    // values are hardwired in the initial version of this function

        ++i;
        CPPUNIT_ASSERT(i != indices->var_end());
        CPPUNIT_ASSERT((*i)->name() == "stop");
        CPPUNIT_ASSERT((*i)->type() == dods_int32_c);
        CPPUNIT_ASSERT(static_cast<Int32*>(*i)->value() == 26);

        ++i;
        CPPUNIT_ASSERT(i != indices->var_end());
        CPPUNIT_ASSERT((*i)->name() == "name");
        CPPUNIT_ASSERT((*i)->type() == dods_str_c);
        CPPUNIT_ASSERT(static_cast<Str*>(*i)->value() == "a_values");
    }

    void float32_2d_array_test() {
        BaseType *btp = *(float32_2d_array->var_begin());

        // It's an array
        CPPUNIT_ASSERT(btp->type() == dods_array_c);

        // ... and it's an Float32 array
        Array *a = static_cast<Array*>(btp);
        CPPUNIT_ASSERT(a->var()->type() == dods_float32_c);

        //  it has thirty elements
        CPPUNIT_ASSERT(a->length() == 1020);

        TestCommon *tc = dynamic_cast<TestCommon*>(a);
        CPPUNIT_ASSERT(tc != 0);
        tc->set_series_values(true);

        DBG2(a->read());
        DBG2(cerr << "Array 'a': ");
        DBG2(a->print_val(cerr));

        BaseType *result = 0;
        try {
            // Must set up the args as per the CE parser
            Float64 min("min");
            min.set_value(40.0);
            min.set_read_p(true);
            Float64 max("max");
            max.set_value(50.0);
            max.set_read_p(true);

            BaseType *argv[] = { a, &min, &max };
            function_dap2_bbox(3, argv, *float32_2d_array /* DDS & */, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error: " + e.get_error_message());
        }

        ostringstream oss;
        result->print_xml(oss);

        DBG(cerr << "DDX of bbox()'s response: " << endl << oss.str() << endl);

        string baseline = readTestBaseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/float32_2d_array_bbox.baseline.xml");
        CPPUNIT_ASSERT(oss.str() == baseline);

        CPPUNIT_ASSERT(result->type() == dods_array_c);

        Array *result_array = static_cast<Array*>(result);

        DBG(oss.str(""));
        DBG(oss.clear());
        DBG(result->print_val(oss));
        DBG(cerr << "Result value: " << endl << oss.str() << endl);

        // we know it's a Structure * and it has one element because the test above passed
        Structure *indices = static_cast<Structure*>(result_array->var(0));
        CPPUNIT_ASSERT(indices != 0);

        Constructor::Vars_iter i = indices->var_begin();
        CPPUNIT_ASSERT(i != indices->var_end());
        CPPUNIT_ASSERT((*i)->name() == "start");
        CPPUNIT_ASSERT((*i)->type() == dods_int32_c);
        CPPUNIT_ASSERT(static_cast<Int32*>(*i)->value() == 0);

        ++i;
        CPPUNIT_ASSERT(i != indices->var_end());
        CPPUNIT_ASSERT((*i)->name() == "stop");
        CPPUNIT_ASSERT((*i)->type() == dods_int32_c);
        CPPUNIT_ASSERT(static_cast<Int32*>(*i)->value() == 29);

        ++i;
        CPPUNIT_ASSERT(i != indices->var_end());
        CPPUNIT_ASSERT((*i)->name() == "name");
        CPPUNIT_ASSERT((*i)->type() == dods_str_c);
        CPPUNIT_ASSERT(static_cast<Str*>(*i)->value() == "rows");

        indices = static_cast<Structure*>(result_array->var(1));
        CPPUNIT_ASSERT(indices != 0);

        i = indices->var_begin();
        CPPUNIT_ASSERT(i != indices->var_end());
        CPPUNIT_ASSERT((*i)->name() == "start");
        CPPUNIT_ASSERT((*i)->type() == dods_int32_c);
        CPPUNIT_ASSERT(static_cast<Int32*>(*i)->value() == 0);

        ++i;
        CPPUNIT_ASSERT(i != indices->var_end());
        CPPUNIT_ASSERT((*i)->name() == "stop");
        CPPUNIT_ASSERT((*i)->type() == dods_int32_c);
        CPPUNIT_ASSERT(static_cast<Int32*>(*i)->value() == 33);

        ++i;
        CPPUNIT_ASSERT(i != indices->var_end());
        CPPUNIT_ASSERT((*i)->name() == "name");
        CPPUNIT_ASSERT((*i)->type() == dods_str_c);
        CPPUNIT_ASSERT(static_cast<Str*>(*i)->value() == "cols");
    }

    // Do we get an exceptin when there's an empty box?
    void float32_2d_array_test_error() {
        BaseType *btp = *(float32_2d_array->var_begin());

        // It's an array
        CPPUNIT_ASSERT(btp->type() == dods_array_c);

        // ... and it's an Float32 array
        Array *a = static_cast<Array*>(btp);
        CPPUNIT_ASSERT(a->var()->type() == dods_float32_c);

        //  it has thirty elements
        CPPUNIT_ASSERT(a->length() == 1020);

        TestCommon *tc = dynamic_cast<TestCommon*>(a);
        CPPUNIT_ASSERT(tc != 0);
        tc->set_series_values(true);

        DBG2(a->read());
        DBG2(cerr << "Array 'a': ");
        DBG2(a->print_val(cerr));

        BaseType *result = 0;
        try {
            // Must set up the args as per the CE parser
            Float64 min("min");
            min.set_value(50.0);
            min.set_read_p(true);
            Float64 max("max");
            max.set_value(51.0);
            max.set_read_p(true);

            BaseType *argv[] = { a, &min, &max };
            function_dap2_bbox(3, argv, *float32_2d_array /* DDS & */, &result);
            CPPUNIT_FAIL("Expected an exception to be thrown");
        }
        catch (Error &e) {
            DBG(cerr << "Caught expected exception Error: " << e.get_error_message() << endl);
            CPPUNIT_ASSERT(true);
        }
        catch (...) {
            CPPUNIT_FAIL("Unexpected exception thrown");
        }
    }
#endif

    CPPUNIT_TEST_SUITE( MaskArrayFunctionTest );

    CPPUNIT_TEST(no_arg_test);
    CPPUNIT_TEST(mask_array_one_d_test);
    CPPUNIT_TEST(mask_array_two_d_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(MaskArrayFunctionTest);

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
            test = string("functions::MaskArrayFunctionTest::") + argv[i++];

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
