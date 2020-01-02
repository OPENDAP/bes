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

#include "BBoxUnionFunction.h"
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

class BBoxUnionFunctionTest: public TestFixture {
private:
    TestTypeFactory btf;
    ConstraintEvaluator ce;

    DDS *float32_array, *float32_2d_array;

public:
    BBoxUnionFunctionTest() :
        float32_array(0), float32_2d_array(0)
    {
    }
    ~BBoxUnionFunctionTest()
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

    void no_arg_test()
    {
        BaseType *result = 0;
        try {
            BaseType *argv[] = {0};
            function_dap2_bbox_union(0, argv, *float32_array /* DDS & */, &result);
            CPPUNIT_FAIL("bbox_union() Should throw an exception when called with no arguments");
        }
        catch (Error &e) {
            CPPUNIT_ASSERT(true);
        }
        catch (...) {
            CPPUNIT_FAIL("unknown exception.");
        }
    }

    void union_test_1()
    {
        BaseType *result = 0;
        try {
            // Must set up the args as per the CE parser
            unique_ptr<Array> bbox_1 = roi_bbox_build_empty_bbox(1, "bbox_1");
            bbox_1->set_vec_nocopy(0, roi_bbox_build_slice(3, 10, "a_values"));

            unique_ptr<Array> bbox_2 = roi_bbox_build_empty_bbox(1, "bbox_1");
            bbox_2->set_vec_nocopy(0, roi_bbox_build_slice(8, 18, "a_values"));

            Str oper("oper");
            oper.set_value("union");
            BaseType *argv[] = { bbox_1.get(), bbox_2.get(), &oper };
            function_dap2_bbox_union(3, argv, *float32_array /* DDS & */, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error:" + e.get_error_message());
        }

        string baseline = read_test_baseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/bbox_union_1.baseline.xml");
        ostringstream oss;
        result->print_xml(oss);

        DBG(cerr << "DDX of bbox_union()'s response: " << endl << oss.str() << endl);

        CPPUNIT_ASSERT(oss.str() == baseline);

        CPPUNIT_ASSERT(result->type() == dods_array_c);

        Array *result_bbox = static_cast<Array*>(result);

        oss.str("");
        oss.clear();
        result_bbox->print_val(oss);
        DBG(cerr << "resulting bbox: " << endl << oss.str());

        baseline = read_test_baseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/bbox_union_1.baseline");
        CPPUNIT_ASSERT(oss.str() == baseline);
    }

    void union_test_2()
    {
        BaseType *result = 0;
        try {
            // Must set up the args as per the CE parser
            unique_ptr<Array> bbox_1 = roi_bbox_build_empty_bbox(1, "bbox_1");
            bbox_1->set_vec_nocopy(0, roi_bbox_build_slice(3, 10, "a_values"));

            unique_ptr<Array> bbox_2 = roi_bbox_build_empty_bbox(1, "bbox_1");
            bbox_2->set_vec_nocopy(0, roi_bbox_build_slice(2, 18, "a_values"));

            Str oper("oper");
            oper.set_value("union");
            BaseType *argv[] = { bbox_1.get(), bbox_2.get(), &oper };
            function_dap2_bbox_union(3, argv, *float32_array /* DDS & */, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error:" + e.get_error_message());
        }

        string baseline = read_test_baseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/bbox_union_1.baseline.xml");
        ostringstream oss;
        result->print_xml(oss);

        DBG(cerr << "DDX of bbox_union()'s response: " << endl << oss.str() << endl);

        CPPUNIT_ASSERT(oss.str() == baseline);

        CPPUNIT_ASSERT(result->type() == dods_array_c);

        Array *result_bbox = static_cast<Array*>(result);

        oss.str("");
        oss.clear();
        result_bbox->print_val(oss);
        DBG(cerr << "resulting bbox: " << endl << oss.str());

        baseline = read_test_baseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/bbox_union_2.baseline");
        CPPUNIT_ASSERT(oss.str() == baseline);
    }

    void union_test_3()
    {
        BaseType *result = 0;
        try {
            // Must set up the args as per the CE parser
            unique_ptr<Array> bbox_1 = roi_bbox_build_empty_bbox(1, "bbox_1");
            bbox_1->set_vec_nocopy(0, roi_bbox_build_slice(3, 5, "a_values"));

            unique_ptr<Array> bbox_2 = roi_bbox_build_empty_bbox(1, "bbox_1");
            bbox_2->set_vec_nocopy(0, roi_bbox_build_slice(7, 10, "a_values"));

            Str oper("oper");
            oper.set_value("union");
            BaseType *argv[] = { bbox_1.get(), bbox_2.get(), &oper };
            function_dap2_bbox_union(3, argv, *float32_array /* DDS & */, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error:" + e.get_error_message());
        }

        string baseline = read_test_baseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/bbox_union_1.baseline.xml");
        ostringstream oss;
        result->print_xml(oss);

        DBG(cerr << "DDX of bbox_union()'s response: " << endl << oss.str() << endl);

        CPPUNIT_ASSERT(oss.str() == baseline);

        CPPUNIT_ASSERT(result->type() == dods_array_c);

        Array *result_bbox = static_cast<Array*>(result);

        oss.str("");
        oss.clear();
        result_bbox->print_val(oss);
        DBG(cerr << "resulting bbox: " << endl << oss.str());

        baseline = read_test_baseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/bbox_union_3.baseline");
        CPPUNIT_ASSERT(oss.str() == baseline);
    }

    void intersection_test_1()
    {
        BaseType *result = 0;
        try {
            // Must set up the args as per the CE parser
            unique_ptr<Array> bbox_1 = roi_bbox_build_empty_bbox(1, "bbox_1");
            bbox_1->set_vec_nocopy(0, roi_bbox_build_slice(3, 10, "a_values"));

            unique_ptr<Array> bbox_2 = roi_bbox_build_empty_bbox(1, "bbox_1");
            bbox_2->set_vec_nocopy(0, roi_bbox_build_slice(7, 11, "a_values"));

            Str oper("oper");
            oper.set_value("inter");
            BaseType *argv[] = { bbox_1.get(), bbox_2.get(), &oper };
            function_dap2_bbox_union(3, argv, *float32_array /* DDS & */, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error:" + e.get_error_message());
        }

        string baseline = read_test_baseline(
            string(TEST_SRC_DIR) + "/ce-functions-testsuite/bbox_intersection_1.baseline.xml");
        ostringstream oss;
        result->print_xml(oss);

        DBG(cerr << "DDX of bbox_union()'s response: " << endl << oss.str() << endl);

        CPPUNIT_ASSERT(oss.str() == baseline);

        CPPUNIT_ASSERT(result->type() == dods_array_c);

        Array *result_bbox = static_cast<Array*>(result);

        oss.str("");
        oss.clear();
        result_bbox->print_val(oss);
        DBG(cerr << "resulting bbox: " << endl << oss.str());

        baseline = read_test_baseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/bbox_inter_1.baseline");
        CPPUNIT_ASSERT(oss.str() == baseline);
    }

    void intersection_test_2()
    {
        BaseType *result = 0;
        try {
            // Must set up the args as per the CE parser
            unique_ptr<Array> bbox_1 = roi_bbox_build_empty_bbox(1, "bbox_1");
            bbox_1->set_vec_nocopy(0, roi_bbox_build_slice(5, 7, "a_values"));

            unique_ptr<Array> bbox_2 = roi_bbox_build_empty_bbox(1, "bbox_1");
            bbox_2->set_vec_nocopy(0, roi_bbox_build_slice(4, 11, "a_values"));

            Str oper("oper");
            oper.set_value("inter");
            BaseType *argv[] = { bbox_1.get(), bbox_2.get(), &oper };
            function_dap2_bbox_union(3, argv, *float32_array /* DDS & */, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error:" + e.get_error_message());
        }

        string baseline = read_test_baseline(
            string(TEST_SRC_DIR) + "/ce-functions-testsuite/bbox_intersection_1.baseline.xml");
        ostringstream oss;
        result->print_xml(oss);

        DBG(cerr << "DDX of bbox_union()'s response: " << endl << oss.str() << endl);

        CPPUNIT_ASSERT(oss.str() == baseline);

        CPPUNIT_ASSERT(result->type() == dods_array_c);

        Array *result_bbox = static_cast<Array*>(result);

        oss.str("");
        oss.clear();
        result_bbox->print_val(oss);
        DBG(cerr << "resulting bbox: " << endl << oss.str());

        baseline = read_test_baseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/bbox_inter_2.baseline");
        CPPUNIT_ASSERT(oss.str() == baseline);
    }

    void intersection_test_3()
    {
        BaseType *result = 0;
        try {
            // Must set up the args as per the CE parser
            unique_ptr<Array> bbox_1 = roi_bbox_build_empty_bbox(1, "bbox_1");
            bbox_1->set_vec_nocopy(0, roi_bbox_build_slice(5, 7, "a_values"));

            unique_ptr<Array> bbox_2 = roi_bbox_build_empty_bbox(1, "bbox_1");
            bbox_2->set_vec_nocopy(0, roi_bbox_build_slice(8, 11, "a_values"));

            Str oper("oper");
            oper.set_value("inter");
            BaseType *argv[] = { bbox_1.get(), bbox_2.get(), &oper };
            function_dap2_bbox_union(3, argv, *float32_array /* DDS & */, &result);
            CPPUNIT_FAIL("Expected an exception to be thrown");
        }
        catch (Error &e) {
            DBG(cerr << "Got the expected error:" + e.get_error_message());
            CPPUNIT_ASSERT(true);
        }
        catch (...) {
            CPPUNIT_FAIL("Expected an Error exception, got something else.");
        }
    }

    // Test 2d (these are actual boxes - not just line segments)
    void union_test_4()
    {
        BaseType *result = 0;
        try {
            // Must set up the args as per the CE parser
            unique_ptr<Array> bbox_1 = roi_bbox_build_empty_bbox(2, "bbox_1");
            bbox_1->set_vec_nocopy(0, roi_bbox_build_slice(3, 5, "a_values"));
            bbox_1->set_vec_nocopy(1, roi_bbox_build_slice(3, 5, "a_values"));

            unique_ptr<Array> bbox_2 = roi_bbox_build_empty_bbox(2, "bbox_2");
            bbox_2->set_vec_nocopy(0, roi_bbox_build_slice(7, 10, "a_values"));
            bbox_2->set_vec_nocopy(1, roi_bbox_build_slice(7, 10, "a_values"));

            Str oper("oper");
            oper.set_value("union");
            BaseType *argv[] = { bbox_1.get(), bbox_2.get(), &oper };
            function_dap2_bbox_union(3, argv, *float32_array /* DDS & */, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error:" + e.get_error_message());
        }

        string baseline = read_test_baseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/bbox_union_2.baseline.xml");
        ostringstream oss;
        result->print_xml(oss);

        DBG(cerr << "DDX of bbox_union()'s response: " << endl << oss.str() << endl);

        CPPUNIT_ASSERT(oss.str() == baseline);

        CPPUNIT_ASSERT(result->type() == dods_array_c);

        Array *result_bbox = static_cast<Array*>(result);

        oss.str("");
        oss.clear();
        result_bbox->print_val(oss);
        DBG(cerr << "resulting bbox: " << endl << oss.str());

        baseline = read_test_baseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/bbox_union_4.baseline");
        CPPUNIT_ASSERT(oss.str() == baseline);
    }

    void union_test_5()
    {
        BaseType *result = 0;
        try {
            // Must set up the args as per the CE parser
            unique_ptr<Array> bbox_1 = roi_bbox_build_empty_bbox(2, "bbox_1");
            bbox_1->set_vec_nocopy(0, roi_bbox_build_slice(3, 5, "a_values"));
            bbox_1->set_vec_nocopy(1, roi_bbox_build_slice(3, 5, "a_values"));

            unique_ptr<Array> bbox_2 = roi_bbox_build_empty_bbox(2, "bbox_2");
            bbox_2->set_vec_nocopy(0, roi_bbox_build_slice(4, 7, "a_values"));
            bbox_2->set_vec_nocopy(1, roi_bbox_build_slice(4, 7, "a_values"));

            Str oper("oper");
            oper.set_value("union");
            BaseType *argv[] = { bbox_1.get(), bbox_2.get(), &oper };
            function_dap2_bbox_union(3, argv, *float32_array /* DDS & */, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error:" + e.get_error_message());
        }

        string baseline = read_test_baseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/bbox_union_2.baseline.xml");
        ostringstream oss;
        result->print_xml(oss);

        DBG(cerr << "DDX of bbox_union()'s response: " << endl << oss.str() << endl);

        CPPUNIT_ASSERT(oss.str() == baseline);

        CPPUNIT_ASSERT(result->type() == dods_array_c);

        Array *result_bbox = static_cast<Array*>(result);

        oss.str("");
        oss.clear();
        result_bbox->print_val(oss);
        DBG(cerr << "resulting bbox: " << endl << oss.str());

        baseline = read_test_baseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/bbox_union_5.baseline");
        CPPUNIT_ASSERT(oss.str() == baseline);
    }

    void union_test_6()
    {
        BaseType *result = 0;
        try {
            // Must set up the args as per the CE parser
            unique_ptr<Array> bbox_1 = roi_bbox_build_empty_bbox(2, "bbox_1");
            bbox_1->set_vec_nocopy(0, roi_bbox_build_slice(3, 6, "a_values"));
            bbox_1->set_vec_nocopy(1, roi_bbox_build_slice(3, 6, "a_values"));

            unique_ptr<Array> bbox_2 = roi_bbox_build_empty_bbox(2, "bbox_2");
            bbox_2->set_vec_nocopy(0, roi_bbox_build_slice(4, 5, "a_values"));
            bbox_2->set_vec_nocopy(1, roi_bbox_build_slice(4, 5, "a_values"));

            Str oper("oper");
            oper.set_value("union");
            BaseType *argv[] = { bbox_1.get(), bbox_2.get(), &oper };
            function_dap2_bbox_union(3, argv, *float32_array /* DDS & */, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error:" + e.get_error_message());
        }

        string baseline = read_test_baseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/bbox_union_2.baseline.xml");
        ostringstream oss;
        result->print_xml(oss);

        DBG(cerr << "DDX of bbox_union()'s response: " << endl << oss.str() << endl);

        CPPUNIT_ASSERT(oss.str() == baseline);

        CPPUNIT_ASSERT(result->type() == dods_array_c);

        Array *result_bbox = static_cast<Array*>(result);

        oss.str("");
        oss.clear();
        result_bbox->print_val(oss);
        DBG(cerr << "resulting bbox: " << endl << oss.str());

        baseline = read_test_baseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/bbox_union_6.baseline");
        CPPUNIT_ASSERT(oss.str() == baseline);
    }

    void intersection_test_4()
    {
        BaseType *result = 0;
        try {
            // Must set up the args as per the CE parser
            unique_ptr<Array> bbox_1 = roi_bbox_build_empty_bbox(2, "bbox_1");
            bbox_1->set_vec_nocopy(0, roi_bbox_build_slice(3, 6, "a_values"));
            bbox_1->set_vec_nocopy(1, roi_bbox_build_slice(3, 6, "a_values"));

            unique_ptr<Array> bbox_2 = roi_bbox_build_empty_bbox(2, "bbox_2");
            bbox_2->set_vec_nocopy(0, roi_bbox_build_slice(4, 5, "a_values"));
            bbox_2->set_vec_nocopy(1, roi_bbox_build_slice(4, 5, "a_values"));

            Str oper("oper");
            oper.set_value("inter");
            BaseType *argv[] = { bbox_1.get(), bbox_2.get(), &oper };
            function_dap2_bbox_union(3, argv, *float32_array /* DDS & */, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error:" + e.get_error_message());
        }

        string baseline = read_test_baseline(
            string(TEST_SRC_DIR) + "/ce-functions-testsuite/bbox_intersection_2.baseline.xml");
        ostringstream oss;
        result->print_xml(oss);

        DBG(cerr << "DDX of bbox_union()'s response: " << endl << oss.str() << endl);

        CPPUNIT_ASSERT(oss.str() == baseline);

        CPPUNIT_ASSERT(result->type() == dods_array_c);

        Array *result_bbox = static_cast<Array*>(result);

        oss.str("");
        oss.clear();
        result_bbox->print_val(oss);
        DBG(cerr << "resulting bbox: " << endl << oss.str());

        baseline = read_test_baseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/bbox_inter_4.baseline");
        CPPUNIT_ASSERT(oss.str() == baseline);
    }

    void intersection_test_5()
    {
        BaseType *result = 0;
        try {
            // Must set up the args as per the CE parser
            unique_ptr<Array> bbox_1 = roi_bbox_build_empty_bbox(2, "bbox_1");
            bbox_1->set_vec_nocopy(0, roi_bbox_build_slice(3, 7, "a_values"));
            bbox_1->set_vec_nocopy(1, roi_bbox_build_slice(3, 7, "a_values"));

            unique_ptr<Array> bbox_2 = roi_bbox_build_empty_bbox(2, "bbox_2");
            bbox_2->set_vec_nocopy(0, roi_bbox_build_slice(4, 9, "a_values"));
            bbox_2->set_vec_nocopy(1, roi_bbox_build_slice(4, 9, "a_values"));

            Str oper("oper");
            oper.set_value("inter");
            BaseType *argv[] = { bbox_1.get(), bbox_2.get(), &oper };
            function_dap2_bbox_union(3, argv, *float32_array /* DDS & */, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error:" + e.get_error_message());
        }

        string baseline = read_test_baseline(
            string(TEST_SRC_DIR) + "/ce-functions-testsuite/bbox_intersection_2.baseline.xml");
        ostringstream oss;
        result->print_xml(oss);

        DBG(cerr << "DDX of bbox_union()'s response: " << endl << oss.str() << endl);

        CPPUNIT_ASSERT(oss.str() == baseline);

        CPPUNIT_ASSERT(result->type() == dods_array_c);

        Array *result_bbox = static_cast<Array*>(result);

        oss.str("");
        oss.clear();
        result_bbox->print_val(oss);
        DBG(cerr << "resulting bbox: " << endl << oss.str());

        baseline = read_test_baseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/bbox_inter_5.baseline");
        CPPUNIT_ASSERT(oss.str() == baseline);
    }

    void intersection_test_6()
    {
        BaseType *result = 0;
        try {
            // Must set up the args as per the CE parser
            unique_ptr<Array> bbox_1 = roi_bbox_build_empty_bbox(2, "bbox_1");
            bbox_1->set_vec_nocopy(0, roi_bbox_build_slice(3, 5, "a_values"));
            bbox_1->set_vec_nocopy(1, roi_bbox_build_slice(3, 5, "a_values"));

            unique_ptr<Array> bbox_2 = roi_bbox_build_empty_bbox(2, "bbox_2");
            bbox_2->set_vec_nocopy(0, roi_bbox_build_slice(7, 9, "a_values"));
            bbox_2->set_vec_nocopy(1, roi_bbox_build_slice(7, 9, "a_values"));

            Str oper("oper");
            oper.set_value("inter");
            BaseType *argv[] = { bbox_1.get(), bbox_2.get(), &oper };
            function_dap2_bbox_union(3, argv, *float32_array /* DDS & */, &result);

            CPPUNIT_FAIL("Expected an exception to be thrown");
        }
        catch (Error &e) {
            DBG(cerr << "Got the expected error:" + e.get_error_message());
            CPPUNIT_ASSERT(true);
        }
        catch (...) {
            CPPUNIT_FAIL("Expected an Error exception, got something else.");
        }
    }

CPPUNIT_TEST_SUITE( BBoxUnionFunctionTest );

    CPPUNIT_TEST(no_arg_test);
    CPPUNIT_TEST(union_test_1);
    CPPUNIT_TEST(union_test_2);
    CPPUNIT_TEST(union_test_3);

    CPPUNIT_TEST(intersection_test_1);
    CPPUNIT_TEST(intersection_test_2);
    CPPUNIT_TEST(intersection_test_3);

    CPPUNIT_TEST(union_test_4);
    CPPUNIT_TEST(union_test_5);
    CPPUNIT_TEST(union_test_6);

    CPPUNIT_TEST(intersection_test_4);
    CPPUNIT_TEST(intersection_test_5);
    CPPUNIT_TEST(intersection_test_6);

    CPPUNIT_TEST_SUITE_END()
    ;
};

CPPUNIT_TEST_SUITE_REGISTRATION(BBoxUnionFunctionTest);

} // namespace functions

int main(int argc, char*argv[])
{

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
            cerr << "Usage: BBoxUnionFunctionTest has the following tests:" << endl;
            const std::vector<Test*> &tests = BBoxUnionFunctionTest::suite()->getTests();
            unsigned int prefix_len = BBoxUnionFunctionTest::suite()->getName().append("::").length();
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
            test = BBoxUnionFunctionTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
