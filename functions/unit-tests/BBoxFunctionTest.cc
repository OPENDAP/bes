
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

#include <BaseType.h>
#include <Byte.h>
#include <Int32.h>
#include <Float32.h>
#include <Float64.h>
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

#include "BBoxFunction.h"

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

namespace functions
{

class BBoxFunctionTest:public TestFixture
{
private:
    TestTypeFactory btf;
    ConstraintEvaluator ce;

    DDS *float32_array, *float32_2d_array;

public:
    BBoxFunctionTest(): float32_array(0), float32_2d_array(0)
    {}
    ~BBoxFunctionTest()
    {}

    void setUp()
    {
        try {
            float32_array = new DDS(&btf);
            float32_array->parse((string)TEST_SRC_DIR + "/ce-functions-testsuite/float32_array.dds");
            DBG2(float32_array->print_xml(stderr, false, "No blob"));

            float32_2d_array = new DDS(&btf);
            float32_2d_array->parse((string)TEST_SRC_DIR + "/ce-functions-testsuite/float32_2d_array.dds");
            DBG2(float32_2d_array->print_xml(stderr, false, "No blob"));
        }
        catch (Error & e) {
            cerr << "SetUp: " << e.get_error_message() << endl;
            throw;
        }
    }

    void tearDown()
    {
        delete float32_array;
        delete float32_2d_array;
    }

    void no_arg_test() {
        BaseType *result = 0;
        try {
            BaseType *argv[] = { };
            function_dap2_bbox(0, argv, *float32_array /* DDS & */, &result);
            CPPUNIT_FAIL("bbox() Should throw an exception when called with no arguments");
        }
        catch (Error &e) {
            CPPUNIT_ASSERT(true);
        }
        catch (...) {
            CPPUNIT_FAIL("unknown exception.");
        }
    }

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

    CPPUNIT_TEST_SUITE( BBoxFunctionTest );

    CPPUNIT_TEST(no_arg_test);
    CPPUNIT_TEST(string_arg_test);
    CPPUNIT_TEST(float32_array_test);
    CPPUNIT_TEST(float32_2d_array_test);
    CPPUNIT_TEST(float32_2d_array_test_error);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(BBoxFunctionTest);

} // namespace functions

int main(int argc, char*argv[]) {

    GetOpt getopt(argc, argv, "dDh");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'D':
            debug2 = 1;
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: BBoxFunctionTest has the following tests:" << endl;
            const std::vector<Test*> &tests = BBoxFunctionTest::suite()->getTests();
            unsigned int prefix_len = BBoxFunctionTest::suite()->getName().append("::").length();
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
            test = BBoxFunctionTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
