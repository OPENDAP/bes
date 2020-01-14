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

#include "test_config.h"
#include "test_utils.h"

#include "RoiFunction.h"
#include "roi_util.h"

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

class RoiFunctionTest: public TestFixture {
private:
    TestTypeFactory btf;
    ConstraintEvaluator ce;

    DDS *float32_array, *float32_2d_array, *float32_array2;

public:
    RoiFunctionTest() :
        float32_array(0), float32_2d_array(0), float32_array2(0)
    {
    }
    ~RoiFunctionTest()
    {
    }

    void setUp()
    {
        try {
            float32_array = new DDS(&btf);
            float32_array->parse((string) TEST_SRC_DIR + "/ce-functions-testsuite/float32_array.dds");
            DBG2(float32_array->print_xml(stderr, false, "No blob"));

            float32_2d_array = new DDS(&btf);
            float32_2d_array->parse((string) TEST_SRC_DIR + "/ce-functions-testsuite/float32_2d_array.dds");
            DBG2(float32_2d_array->print_xml(stderr, false, "No blob"));

            float32_array2 = new DDS(&btf);
            float32_array2->parse((string) TEST_SRC_DIR + "/ce-functions-testsuite/float32_array2.dds");
            DBG2(float32_array2->print_xml(stderr, false, "No blob"));
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
        delete float32_array2;
    }

    void float32_array_test()
    {
        BaseType *btp = *(float32_array->var_begin());

        // It's an array
        CPPUNIT_ASSERT(btp->type() == dods_array_c);

        // ... and it's an Float32 array
        Array *a = static_cast<Array*>(btp);
        CPPUNIT_ASSERT(a->var()->type() == dods_float32_c);

        //  it has thirty elements
        CPPUNIT_ASSERT(a->length() == 30);

        BaseType *result = 0;
        try {
            // Must set up the args as per the CE parser
            unique_ptr<Array> slices = roi_bbox_build_empty_bbox(1, "slices");
            slices->set_vec_nocopy(0, roi_bbox_build_slice(9, 18, "a_values"));

            BaseType *argv[] = { a, slices.get() };
            function_dap2_roi(2, argv, *float32_array /* DDS & */, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error:" + e.get_error_message());
        }

        string baseline = read_test_baseline(
            string(TEST_SRC_DIR) + "/ce-functions-testsuite/float32_array_roi.baseline.xml");
        ostringstream oss;
        result->print_xml(oss);

        DBG(cerr << "DDX of roi()'s response: " << endl << oss.str() << endl);

        CPPUNIT_ASSERT(oss.str() == baseline);

        CPPUNIT_ASSERT(result->type() == dods_structure_c);

        Structure *result_struct = static_cast<Structure*>(result);
        result_struct->read();

        DBG(oss.str(""));
        DBG(oss.clear());
        // print the decl separately to see the constrained size
        DBG(result_struct->print_decl(oss, "    ", true, false, true));
        DBG(cerr << "Result decl, showing constraint: " << endl << oss.str() << endl);

        Constructor::Vars_iter i = result_struct->var_begin();
        CPPUNIT_ASSERT(i != result_struct->var_end());  // test not empty

        CPPUNIT_ASSERT((*i)->type() == dods_array_c);
        CPPUNIT_ASSERT(static_cast<Array*>(*i)->var()->type() == dods_float32_c);
        CPPUNIT_ASSERT(static_cast<Array*>(*i)->dimensions() == 1);
        Array::Dim_iter first_dim = static_cast<Array*>(*i)->dim_begin();
        CPPUNIT_ASSERT(static_cast<Array*>(*i)->dimension_size(first_dim, true) == 10);

        oss.str("");
        oss.clear();
        result_struct->print_val(oss, "    ", false);
        DBG(cerr << "Result values: " << oss.str() << endl);
        baseline = read_test_baseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/float32_array_roi.baseline");

        CPPUNIT_ASSERT(oss.str() == baseline);
    }

    void float32_2d_array_test()
    {
        BaseType *btp = *(float32_2d_array->var_begin());

        // It's an array
        CPPUNIT_ASSERT(btp->type() == dods_array_c);

        // ... and it's an Float32 array
        Array *a = static_cast<Array*>(btp);
        CPPUNIT_ASSERT(a->var()->type() == dods_float32_c);

        //  it has thirty elements
        CPPUNIT_ASSERT(a->length() == 1020);

        BaseType *result = 0;
        try {
            // Must set up the args as per the CE parser
            unique_ptr<Array> slices = roi_bbox_build_empty_bbox(2, "slices");
            slices->set_vec_nocopy(0, roi_bbox_build_slice(9, 18, "rows"));
            slices->set_vec_nocopy(1, roi_bbox_build_slice(9, 18, "cols"));

            BaseType *argv[] = { a, slices.get() };
            function_dap2_roi(2, argv, *float32_array /* DDS & */, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error:" + e.get_error_message());
        }

        string baseline = read_test_baseline(
            string(TEST_SRC_DIR) + "/ce-functions-testsuite/float32_2d_array_roi.baseline.xml");
        ostringstream oss;
        result->print_xml(oss);

        DBG(cerr << "DDX of roi()'s response: " << endl << oss.str() << endl);

        CPPUNIT_ASSERT(oss.str() == baseline);

        CPPUNIT_ASSERT(result->type() == dods_structure_c);

        Structure *result_struct = static_cast<Structure*>(result);
        result_struct->read();

        DBG(oss.str(""));
        DBG(oss.clear());
        // print the decl separately to see the constrained size
        DBG(result_struct->print_decl(oss, "    ", true, false, true));
        DBG(cerr << "Result decl, showing constraint: " << endl << oss.str() << endl);

        Constructor::Vars_iter i = result_struct->var_begin();
        CPPUNIT_ASSERT(i != result_struct->var_end());  // test not empty

        CPPUNIT_ASSERT((*i)->type() == dods_array_c);
        CPPUNIT_ASSERT(static_cast<Array*>(*i)->var()->type() == dods_float32_c);
        CPPUNIT_ASSERT(static_cast<Array*>(*i)->dimensions() == 2);
        Array::Dim_iter first_dim = static_cast<Array*>(*i)->dim_begin();
        CPPUNIT_ASSERT(static_cast<Array*>(*i)->dimension_size(first_dim, true) == 10);

        oss.str("");
        oss.clear();
        result_struct->print_val(oss, "    ", false);
        DBG(cerr << "Result values: " << oss.str() << endl);
        baseline = read_test_baseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/float32_2d_array_roi.baseline");

        CPPUNIT_ASSERT(oss.str() == baseline);
    }

    void float32_array2_test()
    {
        BaseType *btp = *float32_array2->var_begin();
        // It's an array
        CPPUNIT_ASSERT(btp->type() == dods_array_c);
        // ... and it's an Float32 array
        Array *a = static_cast<Array*>(btp);
        CPPUNIT_ASSERT(a->var()->type() == dods_float32_c);

        btp = *(float32_array2->var_begin() + 1);
        CPPUNIT_ASSERT(btp->type() == dods_array_c);
        // ... and it's an Float32 array
        Array *b = static_cast<Array*>(btp);
        CPPUNIT_ASSERT(b->var()->type() == dods_float32_c);

        BaseType *result = 0;
        try {
            // Must set up the args as per the CE parser
            unique_ptr<Array> slices = roi_bbox_build_empty_bbox(1, "slices");
            slices->set_vec_nocopy(0, roi_bbox_build_slice(9, 18, "values"));

            BaseType *argv[] = { a, b, slices.get() };
            function_dap2_roi(3, argv, *float32_array2 /* DDS & */, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error:" + e.get_error_message());
        }

        string baseline = read_test_baseline(
            string(TEST_SRC_DIR) + "/ce-functions-testsuite/float32_array2_roi.baseline.xml");
        ostringstream oss;
        result->print_xml(oss);

        DBG(cerr << "DDX of roi()'s response: " << endl << oss.str() << endl);

        CPPUNIT_ASSERT(oss.str() == baseline);

        CPPUNIT_ASSERT(result->type() == dods_structure_c);

        Structure *result_struct = static_cast<Structure*>(result);
        result_struct->read();

        DBG(oss.str(""));
        DBG(oss.clear());
        // print the decl separately to see the constrained size
        DBG(result_struct->print_decl(oss, "    ", true, false, true));
        DBG(cerr << "Result decl, showing constraint: " << endl << oss.str() << endl);

        // Look at the first var
        Constructor::Vars_iter i = result_struct->var_begin();
        CPPUNIT_ASSERT(i != result_struct->var_end());  // test not empty

        CPPUNIT_ASSERT((*i)->type() == dods_array_c);
        CPPUNIT_ASSERT(static_cast<Array*>(*i)->var()->type() == dods_float32_c);
        CPPUNIT_ASSERT(static_cast<Array*>(*i)->dimensions() == 1);
        Array::Dim_iter first_dim = static_cast<Array*>(*i)->dim_begin();
        CPPUNIT_ASSERT(static_cast<Array*>(*i)->dimension_size(first_dim, true) == 10);

        // Look at teh second var
        ++i;
        CPPUNIT_ASSERT(i != result_struct->var_end());  // test not empty

        CPPUNIT_ASSERT((*i)->type() == dods_array_c);
        CPPUNIT_ASSERT(static_cast<Array*>(*i)->var()->type() == dods_float32_c);
        CPPUNIT_ASSERT(static_cast<Array*>(*i)->dimensions() == 1);
        first_dim = static_cast<Array*>(*i)->dim_begin();
        CPPUNIT_ASSERT(static_cast<Array*>(*i)->dimension_size(first_dim, true) == 10);

        // That should be it
        ++i;
        CPPUNIT_ASSERT(i == result_struct->var_end());

        // Now look at the values of structure that holds the results
        oss.str("");
        oss.clear();
        result_struct->print_val(oss, "    ", false);
        DBG(cerr << "Result values: " << oss.str() << endl);
        baseline = read_test_baseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/float32_array2_roi.baseline");

        CPPUNIT_ASSERT(oss.str() == baseline);
    }

CPPUNIT_TEST_SUITE( RoiFunctionTest );

    CPPUNIT_TEST(float32_array_test);
    CPPUNIT_TEST(float32_2d_array_test);
    CPPUNIT_TEST(float32_array2_test);

    CPPUNIT_TEST_SUITE_END()
    ;
};

CPPUNIT_TEST_SUITE_REGISTRATION(RoiFunctionTest);

} // namespace functions

int main(int argc, char*argv[])
{

    GetOpt getopt(argc, argv, "dDh");
    int option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'D':
            debug2 = 1;
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: RoiFunctionTest has the following tests:" << endl;
            const std::vector<Test*> &tests = RoiFunctionTest::suite()->getTests();
            unsigned int prefix_len = RoiFunctionTest::suite()->getName().append("::").length();
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
            test = RoiFunctionTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
