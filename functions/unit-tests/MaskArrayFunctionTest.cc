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

#include "MaskArrayFunction.h"

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

class MaskArrayFunctionTest: public TestFixture {
private:
    TestTypeFactory btf;
    D4TestTypeFactory d4_ttf;
    // ConstraintEvaluator ce;

    Array *one_d_array, *two_d_array;
    Array *float_2d_array;
    Array *one_d_mask, *two_d_mask;

    vector<dods_byte> d_mask, d_mask2;

public:
    MaskArrayFunctionTest() :
        one_d_array(0), two_d_array(0), float_2d_array(0), one_d_mask(0), two_d_mask(0), d_mask(10), d_mask2(50)
    {
        for (int i = 0; i < 10; ++i)
            d_mask.at(i) = (i < 5) ? 1 : 0;

        for (int i = 0; i < 10; ++i)
            for (int j = 0; j < 5; ++j)
                d_mask2.at(j * 10 + i) = (i < 5) ? 1 : 0;
    }

    ~MaskArrayFunctionTest()
    {
    }

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
            DBG2(copy(values.begin(), values.end(), std::ostream_iterator<dods_int32>(std::cerr, " ")));
            DBG2(cerr << endl);
            one_d_array->set_value(values, values.size());

            two_d_array = new Array("two_d_array", new Int32("two_d_array"));
            two_d_array->append_dim(10, "one");
            two_d_array->append_dim(5, "two");

            vector<dods_int32> values2(50);
            for (int i = 0; i < 10; ++i)
                for (int j = 0; j < 5; ++j)
                    values2[j * 10 + i] = 10 * sin(i * 18);
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
                    values3[j * 10 + i] = sin(i * 18);
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
    }

    void tearDown()
    {
        delete one_d_array;
        one_d_array = 0;
        delete two_d_array;
        two_d_array = 0;

        delete one_d_mask;
        one_d_mask = 0;
        delete two_d_mask;
        two_d_mask = 0;
    }

    void no_arg_test()
    {
        DBG(cerr << "In no_arg_test..." << endl);

        BaseType *result = 0;
        try {
            BaseType *argv[] = {};
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

    void mask_array_helper_one_d_test()
    {
        DBG(cerr << "In mask_array_helper_one_d_test..." << endl);

        try {
            mask_array_helper<dods_int32>(one_d_array, 0, d_mask);

            vector<dods_int32> data(one_d_array->length());
            one_d_array->value(&data[0]);
            DBG(cerr << "Masked data values: ");
            DBG(copy(data.begin(), data.end(), std::ostream_iterator<dods_int32>(std::cerr, " ")));
            DBG(cerr << endl);

            CPPUNIT_ASSERT(data[4] == 2 && data[5] == 0);       // ...could test all the values
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error: " + e.get_error_message());
        }

        DBG(cerr << "Out mask_array_helper_one_d_test" << endl);
    }

    void mask_array_helper_two_d_test()
    {
        DBG(cerr << "In mask_array_helper_two_d_test..." << endl);

        try {
            mask_array_helper<dods_int32>(two_d_array, 0, d_mask2);

            vector<dods_int32> data(two_d_array->length());
            two_d_array->value(&data[0]);
            DBG(cerr << "Masked data values: ");
            DBG(copy(data.begin(), data.end(), std::ostream_iterator<dods_int32>(std::cerr, " ")));
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

        DBG(cerr << "Out mask_array_helper_two_d_test" << endl);
    }

    void mask_array_helper_float_2d_test()
    {
        DBG(cerr << "In mask_array_helper_float_2d_test..." << endl);

        try {
            mask_array_helper<dods_float32>(float_2d_array, 0, d_mask2);

            vector<dods_float32> data(float_2d_array->length());
            float_2d_array->value(&data[0]);
            DBG(cerr << "Masked data values: ");
            DBG(copy(data.begin(), data.end(), std::ostream_iterator<dods_float32>(std::cerr, " ")));
            DBG(cerr << endl);

            CPPUNIT_ASSERT(double_eq(data[4], 0.253823) && data[5] == 0);       // ...could test all the values
            CPPUNIT_ASSERT(double_eq(data[14], 0.253823) && data[15] == 0);
            CPPUNIT_ASSERT(double_eq(data[24], 0.253823) && data[25] == 0);
            CPPUNIT_ASSERT(double_eq(data[34], 0.253823) && data[35] == 0);
            CPPUNIT_ASSERT(double_eq(data[44], 0.253823) && data[45] == 0);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error: " + e.get_error_message());
        }

        DBG(cerr << "Out mask_array_helper_float_2d_test" << endl);
    }

    void float32_2d_mask_array_test()
    {
        DBG(cerr << "In float32_2d_mask_array_test..." << endl);

        BaseType *result = 0;
        try {
            // Must set up the args as per the CE parser
            Float64 no_data("no_data");
            no_data.set_value(0.0);
            no_data.set_read_p(true);

            DDS dds(&btf, "empty");

            BaseType *argv[] = { float_2d_array, &no_data, two_d_mask };
            function_mask_dap2_array(3, argv, dds, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error: " + e.get_error_message());
        }

        ostringstream oss;
        result->print_xml(oss);

        DBG(cerr << "DDX of mask_array()'s response: " << endl << oss.str() << endl);

        string baseline = read_test_baseline(
            string(TEST_SRC_DIR) + "/ce-functions-testsuite/float32_2d_mask_array.baseline.xml");
        CPPUNIT_ASSERT(oss.str() == baseline);

        CPPUNIT_ASSERT(result->type() == dods_array_c);

        Array *result_array = static_cast<Array*>(result);

        DBG(oss.str(""));
        DBG(oss.clear());
        DBG(result_array->print_val(oss));
        DBG(cerr << "Result value: " << endl << oss.str() << endl);

        vector<dods_float32> vals(result_array->length());
        result_array->value(&vals[0]);

        CPPUNIT_ASSERT(double_eq(vals[4], 0.253823));
        CPPUNIT_ASSERT(double_eq(vals[5], 0.0));

        DBG(cerr << "Out float32_2d_mask_array_test" << endl);
    }

    void general_mask_array_test()
    {
        DBG(cerr << "In general_mask_array_test..." << endl);

        BaseType *result = 0;
        try {
            // Must set up the args as per the CE parser
            Float64 no_data("no_data");
            no_data.set_value(0.0);
            no_data.set_read_p(true);

            DDS dds(&btf, "empty");

            BaseType *argv[] = { two_d_array, float_2d_array, &no_data, two_d_mask };
            function_mask_dap2_array(4, argv, dds, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error: " + e.get_error_message());
        }

        ostringstream oss;
        result->print_xml(oss);

        DBG(cerr << "DDX of mask_array()'s response: " << endl << oss.str() << endl);

        string baseline = read_test_baseline(
            string(TEST_SRC_DIR) + "/ce-functions-testsuite/general_mask_array.baseline.xml");
        CPPUNIT_ASSERT(oss.str() == baseline);

        CPPUNIT_ASSERT(result->type() == dods_structure_c);
        Structure *result_struct = static_cast<Structure*>(result);
        CPPUNIT_ASSERT(result_struct->var_begin() != result_struct->var_end());
        CPPUNIT_ASSERT(result_struct->var_begin() + 1 != result_struct->var_end());
        CPPUNIT_ASSERT(result_struct->var_begin() + 2 == result_struct->var_end());

        Array *array_1 = static_cast<Array*>(*(result_struct->var_begin()));

        DBG(oss.str(""));
        DBG(oss.clear());
        DBG(array_1->print_val(oss));
        DBG(cerr << "Result value: " << endl << oss.str() << endl);

        vector<dods_int32> vals_1(array_1->length());
        array_1->value(&vals_1[0]);

        CPPUNIT_ASSERT(vals_1[4] == 2);
        CPPUNIT_ASSERT(vals_1[5] == 0);

        Array *array_2 = static_cast<Array*>(*(result_struct->var_begin() + 1));

        DBG(oss.str(""));
        DBG(oss.clear());
        DBG(array_2->print_val(oss));
        DBG(cerr << "Result value: " << endl << oss.str() << endl);

        vector<dods_float32> vals_2(array_2->length());
        array_2->value(&vals_2[0]);

        CPPUNIT_ASSERT(double_eq(vals_2[4], 0.253823));
        CPPUNIT_ASSERT(double_eq(vals_2[5], 0.0));

        DBG(cerr << "Out general_mask_array_test" << endl);
    }

    void dap4_general_mask_array_test()
    {
        DBG(cerr << "In dap4_general_mask_array_test..." << endl);

        BaseType *result = 0;
        try {
            // Must set up the args as per the CE parser
            Float64 no_data("no_data");
            no_data.set_value(0.0);
            no_data.set_read_p(true);

            DMR dmr(&d4_ttf, "empty");

            //BaseType *argv[] = { two_d_array, float_2d_array, &no_data, two_d_mask };
            D4RValueList *args = new D4RValueList;
            args->add_rvalue(new D4RValue(two_d_array));
            args->add_rvalue(new D4RValue(float_2d_array));
            args->add_rvalue(new D4RValue(&no_data));
            args->add_rvalue(new D4RValue(two_d_mask));

            result = function_mask_dap4_array(args, dmr);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error: " + e.get_error_message());
        }

        ostringstream oss;
        result->print_xml(oss);

        DBG(cerr << "DDX of mask_array()'s response: " << endl << oss.str() << endl);

        string baseline = read_test_baseline(
            string(TEST_SRC_DIR) + "/ce-functions-testsuite/general_mask_array.baseline.xml");
        CPPUNIT_ASSERT(oss.str() == baseline);

        CPPUNIT_ASSERT(result->type() == dods_structure_c);
        Structure *result_struct = static_cast<Structure*>(result);
        CPPUNIT_ASSERT(result_struct->var_begin() != result_struct->var_end());
        CPPUNIT_ASSERT(result_struct->var_begin() + 1 != result_struct->var_end());
        CPPUNIT_ASSERT(result_struct->var_begin() + 2 == result_struct->var_end());

        Array *array_1 = static_cast<Array*>(*(result_struct->var_begin()));

        DBG(oss.str(""));
        DBG(oss.clear());
        DBG(array_1->print_val(oss));
        DBG(cerr << "Result value: " << endl << oss.str() << endl);

        vector<dods_int32> vals_1(array_1->length());
        array_1->value(&vals_1[0]);

        CPPUNIT_ASSERT(vals_1[4] == 2);
        CPPUNIT_ASSERT(vals_1[5] == 0);

        Array *array_2 = static_cast<Array*>(*(result_struct->var_begin() + 1));

        DBG(oss.str(""));
        DBG(oss.clear());
        DBG(array_2->print_val(oss));
        DBG(cerr << "Result value: " << endl << oss.str() << endl);

        vector<dods_float32> vals_2(array_2->length());
        array_2->value(&vals_2[0]);

        CPPUNIT_ASSERT(double_eq(vals_2[4], 0.253823));
        CPPUNIT_ASSERT(double_eq(vals_2[5], 0.0));

        DBG(cerr << "Out dap4_general_mask_array_test" << endl);
    }

CPPUNIT_TEST_SUITE( MaskArrayFunctionTest );

    CPPUNIT_TEST(no_arg_test);
    CPPUNIT_TEST(mask_array_helper_one_d_test);
    CPPUNIT_TEST(mask_array_helper_two_d_test);
    CPPUNIT_TEST(mask_array_helper_float_2d_test);

    CPPUNIT_TEST(float32_2d_mask_array_test);
    CPPUNIT_TEST(general_mask_array_test);
    CPPUNIT_TEST(dap4_general_mask_array_test);

    CPPUNIT_TEST_SUITE_END()
    ;
};

CPPUNIT_TEST_SUITE_REGISTRATION(MaskArrayFunctionTest);

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
            cerr << "Usage: MaskArrayFunctionTest has the following tests:" << endl;
            const std::vector<Test*> &tests = MaskArrayFunctionTest::suite()->getTests();
            unsigned int prefix_len = MaskArrayFunctionTest::suite()->getName().append("::").length();
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
            test = MaskArrayFunctionTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
