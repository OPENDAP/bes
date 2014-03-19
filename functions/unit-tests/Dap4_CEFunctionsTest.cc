
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
#include <Array.h>
#include <Str.h>
#include <Array.h>
#include <Grid.h>
#include <DDS.h>
#include <DAS.h>
#include <DMR.h>
#include <D4ParserSax2.h>
#include <D4BaseTypeFactory.h>
#include <D4RValue.h>

#include <util.h>
#include <debug.h>

#include "dispatch/BESDebug.h"

#include "GridFunction.h"
#include "LinearScaleFunction.h"
#include "MakeArrayFunction.h"

//#include "ce_functions.h"
#include "test_config.h"

#include "test/TestTypeFactory.h"

#include "debug.h"

#if 1
#define TWO_ARRAYS_DMR "ce-functions-testsuite/two_arrays.dmr"
#else
#define TWO_ARRAYS_DMR "unit-tests/ce-functions-testsuite/two_arrays.dmr"
#endif

using namespace CppUnit;
using namespace libdap;
using namespace std;

int test_variable_sleep_interval = 0;

static bool debug = false;
#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

class CEFunctionsTest:public TestFixture
{
private:
    DMR *two_arrays_dmr;
    D4ParserSax2      *d4_parser;
    D4BaseTypeFactory *d4_btf;
    ConstraintEvaluator ce;
public:
    CEFunctionsTest()
    {}
    ~CEFunctionsTest()
    {}
    string


    readTestBaseline(const string &fn)
    {
        int length;

        ifstream is;
        is.open (fn.c_str(), ios::binary );

        // get length of file:
        is.seekg (0, ios::end);
        length = is.tellg();

        // back to start
        is.seekg (0, ios::beg);

        // allocate memory:
        vector<char> buffer(length+1);

        // read data as a block:
        is.read (&buffer[0], length);
        is.close();
        buffer[length] = '\0';

        return string(&buffer[0]);
    }

    void setUp()
    {
        try {
            DBG(cerr << "-------------------------------------------" << endl);
        	DBG(cerr << "setup() - BEGIN " << endl);

    		DBG(BESDebug::SetUp("cerr,all"));
        	DBG(cerr << "setup() - BESDEBUG Enabled " << endl);

            d4_parser = new D4ParserSax2();
        	DBG(cerr << "setup() - Built D4ParserSax2() " << endl);

        	d4_btf = new D4BaseTypeFactory();
        	DBG(cerr << "setup() - Built D4BaseTypeFactory() " << endl);

        	two_arrays_dmr = new DMR(d4_btf);
        	DBG(cerr << "setup() - Built DMR(D4BaseTypeFactory *) " << endl);

        	string dmr_filename = (string)TEST_SRC_DIR +  "/" + TWO_ARRAYS_DMR ;
        	DBG(cerr << "setup() - Parsing DMR file " << dmr_filename << endl);
        	d4_parser->intern(readTestBaseline(dmr_filename), two_arrays_dmr, false);
        	DBG(cerr << "setup() - Parsed DMR from file " << dmr_filename << endl);

            DBG(two_arrays_dmr->dump(cerr));

            BaseType *a_var = two_arrays_dmr->root()->var("a");
            BaseType *b_var = two_arrays_dmr->root()->var("b");


            // Load values into the grid variables
            Array & a = dynamic_cast < Array & >(*a_var);
            Array & b = dynamic_cast < Array & >(*b_var);


            dods_float64 first_a[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
            a.val2buf(first_a);
            a.set_read_p(true);

            dods_float64 first_b[10] = { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
            b.val2buf(first_b);
            b.set_read_p(true);
        	DBG(cerr << "setup() - Values loaded into " << two_arrays_dmr->name() << endl);

        	DBG(cerr << "setup() - END " << endl);

        }

        catch (Error & e) {
            cerr << "SetUp: " << e.get_error_message() << endl;
            throw;
        }
    }

    void tearDown()
    {
    	DBG(cerr << "tearDown() - BEGIN " << endl);



    	delete two_arrays_dmr; two_arrays_dmr = 0;
        delete d4_btf; d4_btf = 0;
        delete d4_parser; d4_parser = 0;



        DBG(cerr << "tearDown() - END " << endl);
        DBG(cerr << "--------------------------------------------" << endl);

    }

    CPPUNIT_TEST_SUITE( CEFunctionsTest );

    // Test void projection_function_grid(int argc, BaseType *argv[], DDS &dds)


    // Tests for linear_scale
    CPPUNIT_TEST(linear_scale_args_test);
    CPPUNIT_TEST(linear_scale_array_test);
    CPPUNIT_TEST(linear_scale_scalar_test);

    //CPPUNIT_TEST(linear_scale_grid_test);
    //CPPUNIT_TEST(linear_scale_grid_attributes_test);
    //CPPUNIT_TEST(linear_scale_grid_attributes_test2);


    CPPUNIT_TEST(make_array_test);


    CPPUNIT_TEST_SUITE_END();



    // linear_scale tests
    void linear_scale_args_test() {
    	DBG(cerr << "linear_scale_args_test() - BEGIN " << endl);
        try {
            function_dap4_linear_scale(0, *two_arrays_dmr);
            CPPUNIT_ASSERT(true);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"linear_scale_args_test: should not throw Error");
        }
    	DBG(cerr << "linear_scale_args_test() - END " << endl);
    }

    void linear_scale_array_test() {
    	DBG(cerr << "linear_scale_array_test() - BEGIN " << endl);
        try {
            D4RValueList params;

            Array & a = dynamic_cast < Array & >(*two_arrays_dmr->root()->var("a"));


            params.add_rvalue(new D4RValue(&a));
            params.add_rvalue(new D4RValue(0.1));
            params.add_rvalue(new D4RValue(10.0));


            BaseType *scaled = function_dap4_linear_scale(&params, *two_arrays_dmr);


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
    	DBG(cerr << "linear_scale_array_test() - END " << endl);
    }

    void linear_scale_scalar_test() {
        try {
            Int32 *i = new Int32("linear_scale_test_int32");
            CPPUNIT_ASSERT(i);
            i->set_value(1);


            D4RValueList params;

            params.add_rvalue(new D4RValue(i));
            params.add_rvalue(new D4RValue(0.1));
            params.add_rvalue(new D4RValue(10.0));


            BaseType *scaled =  function_dap4_linear_scale(&params, *two_arrays_dmr);
            CPPUNIT_ASSERT(scaled->type() == dods_float64_c);

            CPPUNIT_ASSERT(dynamic_cast<Float64*>(scaled)->value() == 10.1);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Error in linear_scale_scalar_test()");
        }
    }




#if 0

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

    void linear_scale_grid_attributes_test() {
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
    void linear_scale_grid_attributes_test2() {
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
            function_dap2_linear_scale(3, argv, *dds, &scaled);
            CPPUNIT_ASSERT(scaled->type() == dods_float64_c);

            CPPUNIT_ASSERT(dynamic_cast<Float64*>(scaled)->value() == 10.1);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Error in linear_scale_scalar_test()");
        }
    }

#endif


    void make_array_test() {
        DBG(cerr << "make_array_test() - BEGIN" << endl);

        try {


            int dims[3] = {5,2,3};
            long int length = 1;
            string shape = "";

            for(int i=0; i<3 ; i++){
                shape += "["+long_to_string(dims[i])+"]";
                length *= dims[i];
            }

            D4RValueList params;
            params.add_rvalue(new D4RValue("Float32")); // type
            params.add_rvalue(new D4RValue(shape));
			for(int i=0; i<length; i++ ){
                params.add_rvalue(new D4RValue(i*0.1));
            }

            DBG(cerr << "make_array_test() - Calling function_make_dap4_array()" << endl);

            BaseType *result =  function_make_dap4_array(&params, *two_arrays_dmr);
            DBG(cerr << "make_array_test() - function_make_dap4_array() returned an "<< result->type_name() << endl);

            CPPUNIT_ASSERT(result->type() == dods_array_c);

            Array *resultArray = dynamic_cast<Array*>(result);
            DBG(cerr << "make_array_test() - resultArray has "+
            		long_to_string(resultArray->dimensions(true))+
            		" dimensions " << endl);

            CPPUNIT_ASSERT(resultArray->dimensions(true) == 3);

            Array::Dim_iter p = resultArray->dim_begin();
            int i = 0;
            while ( p != resultArray->dim_end() ) {
                CPPUNIT_ASSERT(resultArray->dimension_size(p, true) == dims[i]);
                DBG(cerr << "make_array_test() - dimension["<< long_to_string(i) << "]="<< long_to_string(dims[i]) << endl);
                ++p;
                i++;
            }

        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Error in make_array_test()");
        }
        DBG(cerr << "make_array_test() - END" << endl);
    }


};

CPPUNIT_TEST_SUITE_REGISTRATION(CEFunctionsTest);




int main(int argc, char*argv[]) {
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "d");
    char option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
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
            test = string("ugrid::BindTest::") + argv[i++];

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

