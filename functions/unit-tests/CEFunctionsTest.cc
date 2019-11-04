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

//#define DODS_DEBUG

#include <GetOpt.h>
#include <BaseType.h>
#include <Int32.h>
#include <Float64.h>
#include <Str.h>
#include <Array.h>
#include <Grid.h>
#include <DDS.h>
#include <DAS.h>

#include <util.h>
#include <debug.h>
#include <BESDebug.h>

#include "GridFunction.h"
#include "LinearScaleFunction.h"
#include "BindNameFunction.h"
#include "MakeArrayFunction.h"

#include "test_config.h"

#include "test/TestTypeFactory.h"

#include "debug.h"

#if 1
#define TWO_GRID_DDS "ce-functions-testsuite/two_grid.dds"
#define TWO_GRID_DAS "ce-functions-testsuite/two_grid.das"
#else
#define TWO_GRID_DDS "unit-tests/ce-functions-testsuite/two_grid.dds"
#define TWO_GRID_DAS "unit-tests/ce-functions-testsuite/two_grid.das"
#endif

using namespace CppUnit;
using namespace libdap;
using namespace std;
using namespace functions;

int test_variable_sleep_interval = 0;

static bool debug = false;
#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

namespace functions {

class CEFunctionsTest: public TestFixture {
private:
    DDS *dds;
    TestTypeFactory btf;
    ConstraintEvaluator ce;
public:
    CEFunctionsTest() :
        dds(0)
    {
    }
    ~CEFunctionsTest()
    {
    }

    void setUp()
    {
        try {
            DBG(BESDebug::SetUp("cerr,all"));
            DBG(cerr << "setup() - BESDEBUG Enabled " << endl);

            dds = new DDS(&btf);
            string dds_file = (string) TEST_SRC_DIR + "/" + TWO_GRID_DDS;
            dds->parse(dds_file);
            DAS das;
            string das_file = (string) TEST_SRC_DIR + "/" + TWO_GRID_DAS;
            das.parse(das_file);
            dds->transfer_attributes(&das);
            DBG(dds->print_xml(stderr, false, "noBlob"));

            // Load values into the grid variables
            Grid & a = dynamic_cast<Grid &>(*dds->var("a"));
            Array & m1 = dynamic_cast<Array &>(**a.map_begin());

            dods_float64 first_a[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
            m1.val2buf(first_a);
            m1.set_read_p(true);

            dods_byte tmp_data[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
            a.get_array()->val2buf((void*) tmp_data);
            a.get_array()->set_read_p(true);

            Grid & b = dynamic_cast<Grid &>(*dds->var("b"));
            Array & m2 = dynamic_cast<Array &>(**b.map_begin());
            dods_float64 first_b[10] = { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };

            m2.val2buf(first_b);
            m2.set_read_p(true);
        }

        catch (Error & e) {
            cerr << "SetUp: " << e.get_error_message() << endl;
            throw;
        }
    }

    void tearDown()
    {
        delete dds;
        dds = 0;
    }

CPPUNIT_TEST_SUITE( CEFunctionsTest );

    // Test void projection_function_grid(int argc, BaseType *argv[], DDS &dds)

    CPPUNIT_TEST(no_arguments_test);
    CPPUNIT_TEST(one_argument_test);
    CPPUNIT_TEST(one_argument_not_a_grid_test);
    CPPUNIT_TEST(map_not_in_grid_test);
    CPPUNIT_TEST(one_dim_grid_test);
    CPPUNIT_TEST(one_dim_grid_two_expressions_test);
    CPPUNIT_TEST(one_dim_grid_noninclusive_values_test);
    CPPUNIT_TEST(one_dim_grid_descending_test);
    CPPUNIT_TEST(one_dim_grid_two_expressions_descending_test);
#if 0
    // grid() is not required to handle this case.
    CPPUNIT_TEST(values_outside_map_range_test);
#endif

    // Tests for linear_scale
    CPPUNIT_TEST(linear_scale_args_test);
    CPPUNIT_TEST(linear_scale_array_test);
    CPPUNIT_TEST(linear_scale_grid_test);
    CPPUNIT_TEST(linear_scale_grid_attributes_test);
    CPPUNIT_TEST(linear_scale_grid_attributes_test2);
    CPPUNIT_TEST(linear_scale_scalar_test);

    CPPUNIT_TEST(bind_name_test);
    CPPUNIT_TEST(make_array_test);
    CPPUNIT_TEST(make_array_test_bad_args);

#if 0
    // Not used and defined to throw by default. 2/23/11 jhrg
    CPPUNIT_TEST(function_dap_1_test);
    CPPUNIT_TEST(function_dap_2_test);
    CPPUNIT_TEST(function_dap_3_test);
#endif
    CPPUNIT_TEST_SUITE_END()
    ;

    void no_arguments_test()
    {
        try {
            BaseType *btp = 0;
            function_grid(0, 0, *dds, &btp);
            CPPUNIT_ASSERT(true);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"no_arguments_test() should not have failed");
        }
    }

    void one_argument_test()
    {
        try {
            BaseType *argv[1];
            argv[0] = dds->var("a");
            CPPUNIT_ASSERT(argv[0] && "dds->var should find this");
            BaseType *btp = 0;
            function_grid(1, argv, *dds, &btp);
            CPPUNIT_ASSERT("one_argument_not_a_grid_test() should work");
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"one_argument_test should not fail");
        }
    }

    void one_argument_not_a_grid_test()
    {
        try {
            BaseType *argv[1];
            argv[0] = dds->var("lat");
            CPPUNIT_ASSERT(argv[0] && "dds->var should find this, although it is not a grid");
            BaseType *btp = 0;
            function_grid(1, argv, *dds, &btp);
            CPPUNIT_ASSERT(!"one_argument_not_a_grid_test() should have failed");
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(true);
        }
    }

    void map_not_in_grid_test()
    {
        try {
            BaseType *argv[2];
            argv[0] = dds->var("a");
            CPPUNIT_ASSERT(argv[0] && "dds->var should find this");
            argv[1] = new Str("");
            string expression = "3<second<=7";
            dynamic_cast<Str*>(argv[1])->val2buf(&expression);
            dynamic_cast<Str*>(argv[1])->set_read_p(true);
            BaseType *btp = 0;
            function_grid(2, argv, *dds, &btp);
            CPPUNIT_ASSERT(!"map_not_in_grid_test() should have failed");
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(true);
        }
    }

    void one_dim_grid_test()
    {
        try {
            BaseType *argv[2];
            argv[0] = dds->var("a");
            CPPUNIT_ASSERT(argv[0] && "dds->var should find this");
            argv[1] = new Str("");
            string expression = "3<first<=7";
            dynamic_cast<Str*>(argv[1])->val2buf(&expression);
            dynamic_cast<Str*>(argv[1])->set_read_p(true);

            BaseType *btp = 0;
            function_grid(2, argv, *dds, &btp);
            Grid &g = dynamic_cast<Grid&>(*btp);

            //Grid &g = dynamic_cast<Grid&>(*argv[0]);
            Array &m = dynamic_cast<Array&>(**g.map_begin());
            CPPUNIT_ASSERT(m.dimension_start(m.dim_begin(), true) == 4);
            CPPUNIT_ASSERT(m.dimension_stop(m.dim_begin(), true) == 7);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"one_dim_grid_test() should have worked");
        }
    }

    void one_dim_grid_two_expressions_test()
    {
        try {
            BaseType *argv[3];
            argv[0] = dds->var("a");
            CPPUNIT_ASSERT(argv[0] && "dds->var should find this");

            argv[1] = new Str("");
            string expression = "first>3";
            dynamic_cast<Str*>(argv[1])->val2buf(&expression);
            dynamic_cast<Str*>(argv[1])->set_read_p(true);

            argv[2] = new Str("");
            expression = "first<=7";
            dynamic_cast<Str*>(argv[2])->val2buf(&expression);
            dynamic_cast<Str*>(argv[2])->set_read_p(true);

            //function_grid(3, argv, *dds);
            BaseType *btp = 0;
            function_grid(3, argv, *dds, &btp);
            Grid &g = dynamic_cast<Grid&>(*btp);

            //Grid &g = dynamic_cast<Grid&>(*function_grid(3, argv, *dds));
            //Grid &g = dynamic_cast<Grid&>(*argv[0]);
            Array &m = dynamic_cast<Array&>(**g.map_begin());
            CPPUNIT_ASSERT(m.dimension_start(m.dim_begin(), true) == 4);
            CPPUNIT_ASSERT(m.dimension_stop(m.dim_begin(), true) == 7);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"one_dim_grid_two_expressions_test() should have worked");
        }
    }

    void one_dim_grid_descending_test()
    {
        try {
            BaseType *argv[2];
            argv[0] = dds->var("b");
            CPPUNIT_ASSERT(argv[0] && "dds->var should find this");
            argv[1] = new Str("");
            string expression = "3<first<=7";
            dynamic_cast<Str*>(argv[1])->val2buf(&expression);
            dynamic_cast<Str*>(argv[1])->set_read_p(true);

            BaseType *btp = 0;
            function_grid(2, argv, *dds, &btp);
            Grid &g = dynamic_cast<Grid&>(*btp);

            //function_grid(2, argv, *dds);
            //Grid &g = dynamic_cast<Grid&>(*function_grid(2, argv, *dds));
            //Grid &g = dynamic_cast<Grid&>(*argv[0]);
            Array &m = dynamic_cast<Array&>(**g.map_begin());
            CPPUNIT_ASSERT(m.dimension_start(m.dim_begin(), true) == 2);
            CPPUNIT_ASSERT(m.dimension_stop(m.dim_begin(), true) == 5);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"one_dim_grid_test() should have worked");
        }
    }

    void one_dim_grid_two_expressions_descending_test()
    {
        try {
            BaseType *argv[3];
            argv[0] = dds->var("b");
            CPPUNIT_ASSERT(argv[0] && "dds->var should find this");

            argv[1] = new Str("");
            string expression = "first>3";
            dynamic_cast<Str*>(argv[1])->val2buf(&expression);
            dynamic_cast<Str*>(argv[1])->set_read_p(true);

            argv[2] = new Str("");
            expression = "first<=7";
            dynamic_cast<Str*>(argv[2])->val2buf(&expression);
            dynamic_cast<Str*>(argv[2])->set_read_p(true);

            BaseType *btp = 0;
            function_grid(3, argv, *dds, &btp);
            Grid &g = dynamic_cast<Grid&>(*btp);

            //function_grid(3, argv, *dds);
            //Grid &g = dynamic_cast<Grid&>(*function_grid(3, argv, *dds));
            //Grid &g = dynamic_cast<Grid&>(*argv[0]);
            Array &m = dynamic_cast<Array&>(**g.map_begin());
            CPPUNIT_ASSERT(m.dimension_start(m.dim_begin(), true) == 2);
            CPPUNIT_ASSERT(m.dimension_stop(m.dim_begin(), true) == 5);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"one_dim_grid_two_expressions_test() should have worked");
        }
    }

    void one_dim_grid_noninclusive_values_test()
    {
        try {
            BaseType *argv[2];
            argv[0] = dds->var("a");
            CPPUNIT_ASSERT(argv[0] && "dds->var should find this");
            argv[1] = new Str("");
            string expression = "7<first<=3";
            dynamic_cast<Str*>(argv[1])->val2buf(&expression);
            dynamic_cast<Str*>(argv[1])->set_read_p(true);

            BaseType *btp = 0;
            function_grid(2, argv, *dds, &btp);
            //Grid &g = dynamic_cast<Grid&>(*btp);

            // function_grid(2, argv, *dds);

            CPPUNIT_ASSERT(!"one_dim_grid_noninclusive_values_test() should not have worked");
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(true);
        }
    }

    // grid() is not required to handle this case. This test is not used.
    void values_outside_map_range_test()
    {
        try {
            BaseType *argv[2];
            argv[0] = dds->var("a");
            CPPUNIT_ASSERT(argv[0] && "dds->var should find this");
            argv[1] = new Str("");
            string expression = "3<=first<20";
            dynamic_cast<Str*>(argv[1])->val2buf(&expression);
            dynamic_cast<Str*>(argv[1])->set_read_p(true);

            BaseType *btp = 0;
            function_grid(2, argv, *dds, &btp);
            //Grid &g = dynamic_cast<Grid&>(*btp);

            // function_grid(2, argv, *dds);

            CPPUNIT_ASSERT(!"values_outside_map_range_test() should not have worked");
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(true);
        }
    }

    // linear_scale tests
    void linear_scale_args_test()
    {
        try {
            BaseType *btp = 0;
            function_dap2_linear_scale(0, 0, *dds, &btp);
            CPPUNIT_ASSERT(true);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"linear_scale_args_test: should not throw Error");
        }
    }

    void linear_scale_array_test()
    {
        try {
            Array *a = dynamic_cast<Grid&>(*dds->var("a")).get_array();
            CPPUNIT_ASSERT(a);
            BaseType *argv[3];
            argv[0] = a;
            argv[1] = new Float64("");
            dynamic_cast<Float64*>(argv[1])->set_value(0.1);            //m
            argv[2] = new Float64("");
            dynamic_cast<Float64*>(argv[2])->set_value(10);            //b
            BaseType *scaled = 0;
            function_dap2_linear_scale(3, argv, *dds, &scaled);
            CPPUNIT_ASSERT(scaled->type() == dods_array_c && scaled->var()->type() == dods_float64_c);
            double *values = extract_double_array(dynamic_cast<Array*>(scaled));
            CPPUNIT_ASSERT(values[0] == 10);
            CPPUNIT_ASSERT(values[1] == 10.1);
            CPPUNIT_ASSERT(values[9] == 10.9);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Error in linear_scale_grid_test()");
        }
    }

    void linear_scale_grid_test()
    {
        try {
            Grid *g = dynamic_cast<Grid*>(dds->var("a"));
            CPPUNIT_ASSERT(g);
            BaseType *argv[3];
            argv[0] = g;
            argv[1] = new Float64("");
            dynamic_cast<Float64*>(argv[1])->set_value(0.1);
            argv[2] = new Float64("");
            dynamic_cast<Float64*>(argv[2])->set_value(10);
            BaseType *scaled = 0;
            function_dap2_linear_scale(3, argv, *dds, &scaled);
            CPPUNIT_ASSERT(scaled->type() == dods_grid_c);
            Grid *g_s = dynamic_cast<Grid*>(scaled);
            CPPUNIT_ASSERT(g_s->get_array()->var()->type() == dods_float64_c);
            double *values = extract_double_array(g_s->get_array());
            CPPUNIT_ASSERT(values[0] == 10);
            CPPUNIT_ASSERT(values[1] == 10.1);
            CPPUNIT_ASSERT(values[9] == 10.9);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Error in linear_scale_grid_test()");
        }
    }

    void linear_scale_grid_attributes_test()
    {
        try {
            Grid *g = dynamic_cast<Grid*>(dds->var("a"));
            CPPUNIT_ASSERT(g);
            BaseType *argv[1];
            argv[0] = g;
            BaseType *scaled = 0;
            function_dap2_linear_scale(1, argv, *dds, &scaled);
            CPPUNIT_ASSERT(scaled->type() == dods_grid_c);
            Grid *g_s = dynamic_cast<Grid*>(scaled);
            CPPUNIT_ASSERT(g_s->get_array()->var()->type() == dods_float64_c);
            double *values = extract_double_array(g_s->get_array());
            CPPUNIT_ASSERT(values[0] == 10);
            CPPUNIT_ASSERT(values[1] == 10.1);
            CPPUNIT_ASSERT(values[9] == 10.9);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Error in linear_scale_grid_test()");
        }
    }

    // This tests the case where attributes are not found
    void linear_scale_grid_attributes_test2()
    {
        try {
            Grid *g = dynamic_cast<Grid*>(dds->var("b"));
            CPPUNIT_ASSERT(g);
            BaseType *argv[1];
            argv[0] = g;
            BaseType *btp = 0;
            function_dap2_linear_scale(1, argv, *dds, &btp);
            CPPUNIT_FAIL("Should not get here; no params passed and no attributes set for grid 'b'");
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT("Caught exception");
        }
    }

    void linear_scale_scalar_test()
    {
        try {
            Int32 *i = new Int32("linear_scale_test_int32");
            CPPUNIT_ASSERT(i);
            i->set_value(1);
            BaseType *argv[3];
            argv[0] = i;
            argv[1] = new Float64("");
            dynamic_cast<Float64*>(argv[1])->set_value(0.1);            //m
            argv[2] = new Float64("");
            dynamic_cast<Float64*>(argv[2])->set_value(10);            //b
            BaseType *scaled = 0;
            function_dap2_linear_scale(3, argv, *dds, &scaled);
            CPPUNIT_ASSERT(scaled->type() == dods_float64_c);

            CPPUNIT_ASSERT(dynamic_cast<Float64*>(scaled)->value() == 10.1);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Error in linear_scale_scalar_test()");
        }
    }
    void bind_name_test()
    {
        DBG(cerr << "bind_name_test() - BEGIN" << endl);

        try {

            string new_name = "new_name_for_you_buddy";
            BaseType *sourceVar = dds->var("a");
            DBG(
                cerr << "bind_name_test() - Source variable: " << sourceVar->type_name() << " " << sourceVar->name()
                    << endl);
            BaseType *argv[2];

            Str *name = new Str("");
            name->set_value(new_name);
            argv[0] = name;

            argv[1] = sourceVar;
            BaseType *result = 0;

            DBG(cerr << "bind_name_test() - Calling function_bind_name_dap2()" << endl);

            function_bind_name_dap2(2, argv, *dds, &result);

            DBG(
                cerr << "bind_name_test() - function_bind_name_dap2() returned " << result->type_name() << " "
                    << result->name() << endl);
            CPPUNIT_ASSERT(result->name() == new_name);

            delete name;

            Float64 myVar("myVar");
            myVar.set_value((dods_float64) 1.1);
            myVar.set_read_p(true);
            myVar.set_send_p(true);

            DBG(
                cerr << "bind_name_test() - function_bind_name_dap2() source variable: " << myVar.type_name() << " "
                    << myVar.name() << endl);

            new_name = "new_name_for_myVar";
            name = new Str("");
            name->set_value(new_name);
            argv[0] = name;

            argv[1] = &myVar;

            DBG(cerr << "bind_name_test() - Calling function_bind_name_dap4()" << endl);
            BaseType *result2 = 0;

            function_bind_name_dap2(2, argv, *dds, &result2);
            DBG(
                cerr << "bind_name_test() - function_bind_name_dap4() returned " << result2->type_name() << " "
                    << result2->name() << endl);
            CPPUNIT_ASSERT(result2->name() == new_name);

            delete name;

        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Error in bind_name_test()");
        }
        DBG(cerr << "bind_name_test() - END" << endl);
    }

    void make_array_test()
    {
        DBG(cerr << "make_array_test() - BEGIN" << endl);

        try {

            int dims[3] = { 5, 2, 3 };
            string shape = "[5][2][3]";

            BaseType *argv[32]; // two arguments plus all the values.
            int argc = 32;

            Str *type_name = new Str("type_name");
            type_name->set_value("Float64");
            argv[0] = type_name;

            Str *shape_str = new Str("shape");
            shape_str->set_value(shape);
            argv[1] = shape_str;

            Float64 *array_value;
            for (int i = 2; i < argc; i++) {
                float value = cos(0.1 * i);
                array_value = new Float64("array[" + long_to_string(i) + "]");
                array_value->set_value(value);

                argv[i] = array_value;
            }

            DBG(cerr << "make_array_test() - Calling function_make_dap4_array()" << endl);

            BaseType *result = 0;
            function_make_dap2_array(argc, argv, *dds, &result);

            CPPUNIT_ASSERT(result != 0);

            DBG(cerr << "make_array_test() - function_make_dap4_array() returned an " << result->type_name() << endl);

            CPPUNIT_ASSERT(result->type() == dods_array_c);

            Array *resultArray = dynamic_cast<Array*>(result);
            DBG(
                cerr
                    << "make_array_test() - resultArray has " + long_to_string(resultArray->dimensions(true))
                        + " dimensions " << endl);

            CPPUNIT_ASSERT(resultArray->dimensions(true) == 3);

            Array::Dim_iter p = resultArray->dim_begin();
            int i = 0;
            while (p != resultArray->dim_end()) {
                CPPUNIT_ASSERT(resultArray->dimension_size(p, true) == dims[i]);
                DBG(
                    cerr << "make_array_test() - dimension[" << long_to_string(i) << "]=" << long_to_string(dims[i])
                        << endl);
                ++p;
                i++;
            }

            for (int i = 0; i < argc; i++) {
                delete argv[i];
            }

        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Error in make_array_test()");
        }
        DBG(cerr << "make_array_test() - END" << endl);
    }

    void make_array_test_bad_args()
    {
        DBG(cerr << "make_array_test_bad_args() - BEGIN" << endl);

        try {

            string shape = "[5]";

            BaseType *argv[7]; // two arguments plus all the values.
            int argc = 7;

            Str *type_name = new Str("type_name");
            type_name->set_value("Float64");
            argv[0] = type_name;

            Str *shape_str = new Str("shape");
            shape_str->set_value(shape);
            argv[1] = shape_str;

            Str *svalue;
            Float64 *fvalue;
            for (int i = 2; i < argc; i++) {
                float value = cos(0.1 * i);
                if (i == 3) {
                    svalue = new Str("array[" + long_to_string(i) + "]");
                    svalue->set_value(double_to_string(value));
                    argv[i] = svalue;
                }
                else {
                    fvalue = new Float64("array[" + long_to_string(i) + "]");
                    fvalue->set_value(value);
                    argv[i] = fvalue;
                }
            }

            try {
                DBG(
                    cerr
                        << "make_array_test_bad_params() - Calling function_make_dap2_array() with incorrectly typed parameters"
                        << endl);
                BaseType *result = 0;
                function_make_dap2_array(argc, argv, *dds, &result);
                CPPUNIT_ASSERT(!result);
            }
            catch (Error &e) {
                DBG(
                    cerr << "make_array_test_bad_params() - Caught Expected Error. Message: " << e.get_error_message()
                        << endl);
                CPPUNIT_ASSERT("Expected Error in make_array_test_bad_params()");
            }

            for (int i = 0; i < argc; i++) {
                delete argv[i];
            }

        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Error in make_array_test_bad_params()");
        }
        DBG(cerr << "make_array_test_bad_params() - END" << endl);
    }

#if 0
    void function_dap_1_test() {
        try {
            Int32 *i = new Int32("function_dap_1_test_int32");
            CPPUNIT_ASSERT(i);
            i->set_value(2);
            BaseType *argv[1];
            argv[0] = i;

            ConstraintEvaluator unused;
            function_dap(1, argv, *dds, unused);

            CPPUNIT_ASSERT(dds->get_dap_major() == 2);
            CPPUNIT_ASSERT(dds->get_dap_minor() == 0);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_FAIL("Error in function_dap_1_test(): " + e.get_error_message());
        }
    }

    void function_dap_2_test() {
        try {
            Float64 *d = new Float64("function_dap_1_test_float64");
            CPPUNIT_ASSERT(d);
            d->set_value(3.2);
            BaseType *argv[1];
            argv[0] = d;

            ConstraintEvaluator unused;
            function_dap(1, argv, *dds, unused);

            CPPUNIT_ASSERT(dds->get_dap_major() == 3);
            CPPUNIT_ASSERT(dds->get_dap_minor() == 2);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_FAIL("Error in function_dap_2_test(): " + e.get_error_message());
        }
    }

    void function_dap_3_test() {
        try {
            cerr <<"In function_dap_3_test" << endl;
            ConstraintEvaluator unused;
            function_dap(0, 0, *dds, unused);

            CPPUNIT_FAIL("Should have thrown an exception on no args");
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT("Pass: Caught exception");
        }
    }
#endif
};

CPPUNIT_TEST_SUITE_REGISTRATION(CEFunctionsTest);

} // namespace functions

int main(int argc, char*argv[])
{

    GetOpt getopt(argc, argv, "dh");
    int option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: CEFunctionsTest has the following tests:" << endl;
            const std::vector<Test*> &tests = CEFunctionsTest::suite()->getTests();
            unsigned int prefix_len = CEFunctionsTest::suite()->getName().append("::").length();
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
            test = CEFunctionsTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
