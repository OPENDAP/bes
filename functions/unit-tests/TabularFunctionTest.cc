
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

#include <BaseType.h>
#include <Int32.h>
#include <Float64.h>
#include <Array.h>
#include <Sequence.h>
#include <DDS.h>
#include <util.h>
#include <GetOpt.h>
#include <debug.h>

#include "test/TestTypeFactory.h"

#include <test_config.h>

#include "TabularFunction.h"

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

class TabularFunctionTest:public TestFixture
{
private:
    TestTypeFactory btf;
    ConstraintEvaluator ce;

    DDS *one_var_dds;

public:
    TabularFunctionTest(): one_var_dds(0)
    {}
    ~TabularFunctionTest()
    {}

    void setUp()
    {
        // geo grid test data
        try {
            one_var_dds = new DDS(&btf);
            one_var_dds->parse((string)TEST_SRC_DIR + "/tabular/one_var.dds");

            DBG2(one_var_dds->print_xml(stderr, false, "No blob"));
        }

        catch (Error & e) {
            cerr << "SetUp: " << e.get_error_message() << endl;
            throw;
        }
    }

    void tearDown()
    {
        delete one_var_dds;
    }

    void one_var_test() {
        // we know there's just one variable
        BaseType *btp = *(one_var_dds->var_begin());

        CPPUNIT_ASSERT(btp->type() == dods_array_c);

        // ... and it's an Int32 array
        Array *a = static_cast<Array*>(btp);

        // and it has four elements
        CPPUNIT_ASSERT(a->length() == 4);

        // call it; the function reads the values of the array
        BaseType *argv[] = { a };
        BaseType *result = 0;
        function_dap2_tabular(1, argv, *one_var_dds, &result);

        // Now check that the response types are as expected
        DBG(cerr << "a[0]: " << dynamic_cast<Int32&>(*(a->var(0))).value() << endl);
        CPPUNIT_ASSERT(dynamic_cast<Int32&>(*(a->var(0))).value() == 123456789);

        CPPUNIT_ASSERT(result->type() == dods_sequence_c);
        Sequence *s = static_cast<Sequence*>(result);

        // ... because we know it's an int32
        CPPUNIT_ASSERT(s->var("a")->type() == dods_int32_c);
        //BaseType *var_value(size_t row, const string &name)
        SequenceValues sv = s->value();
        DBG(cerr << "SequenceValues size: " << sv.size() << endl);
        CPPUNIT_ASSERT(sv.size() == 4);

        for (int j = 0; j < sv.size(); ++j) {
            CPPUNIT_ASSERT(s->var_value(j, 0) != 0);
            Int32 *i = static_cast<Int32*>(s->var_value(j, 0));
            DBG(cerr << "Seq.a, row " << long_to_string(j) << ": " << i->value() << endl);
            CPPUNIT_ASSERT(i->value() == 123456789);
        }

    }


    CPPUNIT_TEST_SUITE( TabularFunctionTest );

    CPPUNIT_TEST(one_var_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(TabularFunctionTest);

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
            test = string("libdap::TabularFunctionTest::") + argv[i++];

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
