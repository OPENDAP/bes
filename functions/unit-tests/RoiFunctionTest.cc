
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

namespace libdap
{

static Array *get_empty_slices_array()
{
    // Make the prototype
    Structure *proto = new Structure("bbox");
    proto->add_var_nocopy(new Int32("start"));
    proto->add_var_nocopy(new Int32("stop"));
    proto->add_var_nocopy(new Str("name"));

    return new Array("bbox", proto);
}

static Structure *get_new_slice_element(int start, int stop, const string &name)
{
    Structure *proto = new Structure("bbox");
    Int32 *istart = new Int32("start");
    istart->set_value(start);
    proto->add_var_nocopy(istart);

    Int32 *istop = new Int32("stop");
    istop->set_value(stop);
    proto->add_var_nocopy(istop);

    Str *iname = new Str("name");
    iname->set_value(name);
    proto->add_var_nocopy(iname);

    return proto;
}

class RoiFunctionTest:public TestFixture
{
private:
    TestTypeFactory btf;
    ConstraintEvaluator ce;

    DDS *float32_array, *float32_2d_array;

public:
    RoiFunctionTest(): float32_array(0), float32_2d_array(0)
    {}
    ~RoiFunctionTest()
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

    void float32_array_test() {
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
            Array *slices = get_empty_slices_array();
            slices->append_dim(1, "slices");
            slices->set_vec_nocopy(0, get_new_slice_element(9, 18, "a_value"));

            BaseType *argv[] = { a, slices };
            function_dap2_roi(2, argv, *float32_array /* DDS & */, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error:" + e.get_error_message());
        }

        string baseline = readTestBaseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/float32_array_roi.baseline.xml");
        ostringstream oss;
        result->print_xml(oss);

        DBG(cerr << "DDX of roi()'s response: " << endl << oss.str() << endl);

        // print_xml doesn't show the affect of the constraint, so use the same
        // baseline as for the bbox tests
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

        oss.str(""); oss.clear();
        result_struct->print_val(oss, "    ", false);
        DBG(cerr << "Result values: " << oss.str() << endl);
        baseline = readTestBaseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/float32_array_roi.baseline");

        CPPUNIT_ASSERT(oss.str() == baseline);
#if 0
        // we know it's a Structure * and it has one element because the test above passed
        Structure *indices = static_cast<Structure*>(result_array->var(0));
        CPPUNIT_ASSERT(indices != 0);

        Constructor::Vars_iter i = indices->var_begin();
        CPPUNIT_ASSERT(i != indices->var_end());
        CPPUNIT_ASSERT((*i)->name() == "start");
        CPPUNIT_ASSERT((*i)->type() == dods_int32_c);
        CPPUNIT_ASSERT(static_cast<Int32*>(*i)->value() == 10);    // values are hardwired in the initial version of this function

        ++i;
        CPPUNIT_ASSERT(i != indices->var_end());
        CPPUNIT_ASSERT((*i)->name() == "stop");
        CPPUNIT_ASSERT((*i)->type() == dods_int32_c);
        CPPUNIT_ASSERT(static_cast<Int32*>(*i)->value() == 20);

        ++i;
        CPPUNIT_ASSERT(i != indices->var_end());
        CPPUNIT_ASSERT((*i)->name() == "name");
        CPPUNIT_ASSERT((*i)->type() == dods_str_c);
        CPPUNIT_ASSERT(static_cast<Str*>(*i)->value() == "a_values");
#endif
    }

    void float32_2d_array_test() {
#if 0
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
            Float64 *min = new Float64("min");
            min->set_value(40.0);
            min->set_read_p(true);
            Float64 *max = new Float64("max");
            max->set_value(60.0);
            max->set_read_p(true);

            BaseType *argv[] = { a, min, max };
            function_dap2_bbox(3, argv, *float32_2d_array /* DDS & */, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error: " + e.get_error_message());
        }

        ostringstream oss;
        result->print_xml(oss);

        DBG(cerr << "DDX of bbox()'s response: " << endl << oss.str() << endl);

        string baseline = readTestBaseline(string(TEST_SRC_DIR) + "/ce-functions-testsuite/float32_2d_array_ddx.baseline.xml");
        CPPUNIT_ASSERT(oss.str() == baseline);

        CPPUNIT_ASSERT(result->type() == dods_array_c);

        Array *result_array = static_cast<Array*>(result);

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
        CPPUNIT_ASSERT(static_cast<Int32*>(*i)->value() == 10);    // values are hardwired in the initial version of this function

        ++i;
        CPPUNIT_ASSERT(i != indices->var_end());
        CPPUNIT_ASSERT((*i)->name() == "stop");
        CPPUNIT_ASSERT((*i)->type() == dods_int32_c);
        CPPUNIT_ASSERT(static_cast<Int32*>(*i)->value() == 20);

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
        CPPUNIT_ASSERT(static_cast<Int32*>(*i)->value() == 10);    // values are hardwired in the initial version of this function

        ++i;
        CPPUNIT_ASSERT(i != indices->var_end());
        CPPUNIT_ASSERT((*i)->name() == "stop");
        CPPUNIT_ASSERT((*i)->type() == dods_int32_c);
        CPPUNIT_ASSERT(static_cast<Int32*>(*i)->value() == 20);

        ++i;
        CPPUNIT_ASSERT(i != indices->var_end());
        CPPUNIT_ASSERT((*i)->name() == "name");
        CPPUNIT_ASSERT((*i)->type() == dods_str_c);
        CPPUNIT_ASSERT(static_cast<Str*>(*i)->value() == "cols");
#endif
    }

    CPPUNIT_TEST_SUITE( RoiFunctionTest );

    CPPUNIT_TEST(float32_array_test);
    CPPUNIT_TEST(float32_2d_array_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(RoiFunctionTest);

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
            test = string("libdap::RoiFunctionTest::") + argv[i++];

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
