
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2012 OPeNDAP, Inc.
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

#define DODS_DEBUG
//#define DODS_DEBUG2

#include "BaseType.h"
#include "Array.h"
#include "Grid.h"

#include "reproj_functions.h"

#include "test/TestTypeFactory.h"

#include "util.h"
#include "debug.h"

#define THREE_ARRAY_1_DDS "three_array_1.dds"
#define THREE_ARRAY_1_DAS "three_array_1.das"

using namespace CppUnit;
using namespace libdap;
using namespace std;

int test_variable_sleep_interval = 0;

/**
 * Splits the string on the passed char. Returns vector of substrings.
 * TODO make this work on situations where multiple spaces doesn't hose the split()
 */
static vector<string> &split(const string &s, char delim, vector<string> &elems) {
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

/**
 * Splits the string on the passed char. Returns vector of substrings.
 */
static vector<string> split(const string &s, char delim = ' ') {
    vector<string> elems;
    return split(s, delim, elems);
}

class s2gTest:public TestFixture
{
private:
    DDS * dds;
    TestTypeFactory btf;
    ConstraintEvaluator ce;
public:
    s2gTest():dds(0)
    {}
    ~s2gTest()
    {}

    void setUp()
    {
        try {
            dds = new DDS(&btf);
            string dds_file = /*(string)TEST_SRC_DIR + "/" +*/ THREE_ARRAY_1_DDS ;
            dds->parse(dds_file);
            DAS das;
            string das_file = /*(string)TEST_SRC_DIR + "/" +*/ THREE_ARRAY_1_DAS ;
            das.parse(das_file);
            dds->transfer_attributes(&das);

            DBG(dds->print_xml(stderr, false, "noBlob"));

            // Load values into the array variables
            Array & t = dynamic_cast < Array & >(*dds->var("t"));
            Array & lon = dynamic_cast < Array & >(*dds->var("lon"));
            Array & lat = dynamic_cast < Array & >(*dds->var("lat"));

            dods_float64 t_vals[10][10];
            for (int i = 0; i < 10; ++i)
                for (int j = 0; j < 10; ++j)
                    t_vals[i][j] = j + (i * 10);
            t.set_value(&t_vals[0][0], 100);
            t.set_read_p(true);

            // Read real lat/lon values from a Level 1B file ascii dump
            fstream input("AIRS_AQUA_L1B_BRIGHTNESS_20101026_1617.nc.gz.ascii.txt", fstream::in);
            if (input.eof() || input.fail())
                throw Error("Could not open lat/lon data in SetUp.");
            // Read a line of text to get to the start of the data.
            string line;
            getline(input, line);
            if (input.eof() || input.fail())
                throw Error("Could not read lat/lon data in SetUp.");

            dods_float64 lon_vals[10][10];
            for (int i = 0; i < 10; ++i) {
                getline(input, line);
                if (input.eof() || input.fail())
                    throw Error("Could not read lon data from row " + long_to_string(i) + " in SetUp.");
                vector<string> vals = split(line, ',');
                for (unsigned int j = 1; j < vals.size(); ++j) {
                    DBG2(cerr << "loading in lon value: " << vals[j] << "' " << atof(vals[j].c_str()) << endl;)
                    lon_vals[i][j-1] = atof(vals[j].c_str());
                }
            }
            lon.set_value(&lon_vals[0][0], 100);
            lon.set_read_p(true);

            dods_float64 lat_vals[10][10];
            for (int i = 0; i < 10; ++i) {
                getline(input, line);
                if (input.eof() || input.fail())
                    throw Error("Could not read lat data from row " + long_to_string(i) + " in SetUp.");
                vector<string> vals = split(line, ',');
                for (unsigned int j = 1; j < vals.size(); ++j) {
                    DBG2(cerr << "loading in lat value: " << vals[j] << "' " << atof(vals[j].c_str()) << endl;)
                    lat_vals[i][j-1] = atof(vals[j].c_str());
                }
            }
            lat.set_value(&lat_vals[0][0], 100);
            lat.set_read_p(true);
        }

        catch (Error & e) {
            cerr << "SetUp (Error): " << e.get_error_message() << endl;
            throw;
        }
        catch(std::exception &e) {
            cerr << "SetUp (std::exception): " << e.what() << endl;
            throw;
        }
    }

    void tearDown()
    {
        delete dds; dds = 0;
    }

    CPPUNIT_TEST_SUITE( s2gTest );

    CPPUNIT_TEST(no_arguments_test);
    CPPUNIT_TEST(array_return_test);
    CPPUNIT_TEST(grid_return_test);

    CPPUNIT_TEST_SUITE_END();

    void no_arguments_test()
    {
        try {
            BaseType *btp = 0;
            function_swath2array(0, 0, *dds, &btp);
            CPPUNIT_ASSERT(true);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"no_arguments_test() should not have failed");
        }
    }

    void array_return_test()
    {
        try {
            BaseType *argv[3];
            argv[0] = dds->var("t");
            argv[1] = dds->var("lon");
            argv[2] = dds->var("lat");

            BaseType *btp = 0;
            function_swath2array(3, argv, *dds, &btp);

            DBG(cerr << "btp->name(): " << btp->name() << endl);
            CPPUNIT_ASSERT(btp->name() == "t");
            CPPUNIT_ASSERT(btp->type() == dods_array_c);

            // Extract data; I know it's 10x16 from debugging output
            dods_float64 values[10][16];
            Array *a = static_cast<Array*>(btp);
            a->value(&values[0][0]);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_FAIL("array_return_test");
        }
    }

#if 0
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
    void linear_scale_args_test() {
        try {
            BaseType *btp = 0;
            function_linear_scale(0, 0, *dds, &btp);
            CPPUNIT_ASSERT(true);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"linear_scale_args_test: should not throw Error");
        }
    }

    void linear_scale_array_test() {
        try {
            Array *a = dynamic_cast<Grid&>(*dds->var("a")).get_array();
            CPPUNIT_ASSERT(a);
            BaseType *argv[3];
            argv[0] = a;
            argv[1] = new Float64("");
            dynamic_cast<Float64*>(argv[1])->set_value(0.1);//m
            argv[2] = new Float64("");
            dynamic_cast<Float64*>(argv[2])->set_value(10);//b
            BaseType *scaled = 0;
            function_linear_scale(3, argv, *dds, &scaled);
            CPPUNIT_ASSERT(scaled->type() == dods_array_c
                           && scaled->var()->type() == dods_float64_c);
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

    void linear_scale_grid_test() {
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
            function_linear_scale(3, argv, *dds, &scaled);
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

    void linear_scale_grid_attributes_test() {
        try {
            Grid *g = dynamic_cast<Grid*>(dds->var("a"));
            CPPUNIT_ASSERT(g);
            BaseType *argv[1];
            argv[0] = g;
            BaseType *scaled = 0;
            function_linear_scale(1, argv, *dds, &scaled);
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
    void linear_scale_grid_attributes_test2() {
        try {
            Grid *g = dynamic_cast<Grid*>(dds->var("b"));
            CPPUNIT_ASSERT(g);
            BaseType *argv[1];
            argv[0] = g;
            BaseType *btp = 0;
            function_linear_scale(1, argv, *dds, &btp);
            CPPUNIT_FAIL("Should not get here; no params passed and no attributes set for grid 'b'");
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT("Caught exception");
        }
    }

    void linear_scale_scalar_test() {
        try {
            Int32 *i = new Int32("linear_scale_test_int32");
            CPPUNIT_ASSERT(i);
            i->set_value(1);
            BaseType *argv[3];
            argv[0] = i;
            argv[1] = new Float64("");
            dynamic_cast<Float64*>(argv[1])->set_value(0.1);//m
            argv[2] = new Float64("");
            dynamic_cast<Float64*>(argv[2])->set_value(10);//b
            BaseType *scaled = 0;
            function_linear_scale(3, argv, *dds, &scaled);
            CPPUNIT_ASSERT(scaled->type() == dods_float64_c);

            CPPUNIT_ASSERT(dynamic_cast<Float64*>(scaled)->value() == 10.1);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Error in linear_scale_scalar_test()");
        }
    }


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
    void grid_return_test()
    {
        try {
            BaseType *argv[3];
            argv[0] = dds->var("t");
            argv[1] = dds->var("lon");
            argv[2] = dds->var("lat");

            cerr << "Input values:" << endl;
            dods_float64 t_vals[10][10];
            Array *a = static_cast<Array*>(argv[0]);
            a->value(&t_vals[0][0]);
            for (int i = 0; i < 10; ++i) {
                for (int j = 0; j < 10; ++j) {
                    cerr << "t[" << i << "][" << j << "]: " <<  t_vals[i][j] << endl;
                }
            }

            BaseType *btp = 0;
            function_swath2grid(3, argv, *dds, &btp);

            DBG(cerr << "btp->name(): " << btp->name() << endl);
            CPPUNIT_ASSERT(btp->name() == "t");
            CPPUNIT_ASSERT(btp->type() == dods_grid_c);

            // Extract data; I know it's 10x16 from debugging output
            dods_float64 values[10][16];
            Grid *g = static_cast<Grid*>(btp);
            g->get_array()->value(&values[0][0]);

            Grid::Map_iter m = g->map_begin();
            dods_float64 lat[10];
            static_cast<Array*>(*m)->value(&lat[0]);

            ++m;
            dods_float64 lon[16];
            static_cast<Array*>(*m)->value(&lon[0]);

            cerr << "Output values:" << endl;
            for (int i = 0; i < 10; ++i) {
                for (int j = 0; j < 16; ++j) {
                    cerr << "t[" << i << "][" << j << "] == lon: " << lon[j] << ", lat: " << lat[i] << " val: " << values[i][j] << endl;
                }
            }
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_FAIL("array_return_test");
        }
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(s2gTest);

int
main( int, char** )
{
    CppUnit::TextTestRunner runner;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );

    bool wasSuccessful = runner.run( "", false ) ;

    return wasSuccessful ? 0 : 1;
}
