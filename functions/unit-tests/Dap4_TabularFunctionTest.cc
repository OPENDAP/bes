
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
#include <Byte.h>
#include <Int32.h>
#include <Float32.h>
#include <Float64.h>
#include <Array.h>
#include <Sequence.h>
#include <DDS.h>
#include <D4Group.h>
#include <D4Sequence.h>
#include <DMR.h>
#include <XMLWriter.h>
#include <D4RValue.h>
#include <util.h>
#include <GetOpt.h>
#include <debug.h>

#include "test/TestTypeFactory.h"
#include "test/D4TestTypeFactory.h"

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

class Dap4_TabularFunctionTest: public TestFixture
{
private:
    TestTypeFactory ttf;
    D4TestTypeFactory d4btf;
    ConstraintEvaluator ce;

    DMR *one_var_dmr;
    DMR *four_var_2_dmr;

public:
    Dap4_TabularFunctionTest(): one_var_dmr(0), four_var_2_dmr(0)
    {}
    ~Dap4_TabularFunctionTest()
    {}

    void setUp()
    {
        XMLWriter xml;
        try {
            DDS one_var(&ttf);
            one_var.parse((string)TEST_SRC_DIR + "/tabular/one_var.dds");
            one_var_dmr = new DMR(&d4btf, one_var);
            one_var_dmr->print_dap4(xml, false);
            DBG2(cerr << xml.get_doc() << endl);

            DDS four_var(&ttf);
            four_var.parse((string)TEST_SRC_DIR + "/tabular/four_var_2.dds");
            four_var_2_dmr = new DMR(&d4btf, four_var);
            four_var_2_dmr->print_dap4(xml, false);
            DBG2(cerr << xml.get_doc() << endl);
        }

        catch (Error & e) {
            cerr << "SetUp: " << e.get_error_message() << endl;
            throw;
        }
    }

    void tearDown()
    {
        delete one_var_dmr;
        delete four_var_2_dmr;
    }

    // A limited set of unit tests because the dap2 and dap4 versions
    // are so similar.

    void one_var_test() {
        // we know there's just one variable
        BaseType *btp = *(one_var_dmr->root()->var_begin());

        CPPUNIT_ASSERT(btp->type() == dods_array_c);

        // ... and it's an Int32 array
        Array *a = static_cast<Array*>(btp);

        // and it has four elements
        CPPUNIT_ASSERT(a->length() == 4);

        // call it; the function reads the values of the array
        D4RValue rv(a);
        D4RValueList *args = new D4RValueList;
        args->add_rvalue(&rv);
        BaseType *result = TabularFunction::function_dap4_tabular(args, *one_var_dmr);

        // Now check that the response types are as expected
        DBG(cerr << "a[0]: " << dynamic_cast<Int32&>(*(a->var(0))).value() << endl);
        CPPUNIT_ASSERT(dynamic_cast<Int32&>(*(a->var(0))).value() == 123456789);

        CPPUNIT_ASSERT(result->type() == dods_sequence_c);
        D4Sequence *s = static_cast<D4Sequence*>(result);

        // ... because we know it's an int32
        CPPUNIT_ASSERT(s->var("a")->type() == dods_int32_c);
        //BaseType *var_value(size_t row, const string &name)
        SequenceValues sv = s->value();
        DBG(cerr << "SequenceValues size: " << sv.size() << endl);
        CPPUNIT_ASSERT(sv.size() == 4);

        for (SequenceValues::size_type j = 0; j < sv.size(); ++j) {
            CPPUNIT_ASSERT(s->var_value(j, 0) != 0);
            Int32 *i = static_cast<Int32*>(s->var_value(j, 0));
            DBG(cerr << "Seq.a, row " << long_to_string(j) << ": " << i->value() << endl);
            CPPUNIT_ASSERT(i->value() == 123456789);
        }
    }

    void four_var_2_test() {
        // There are four arrays of varying types, each with four elements
        vector<BaseType*> arrays;
        for (DDS::Vars_iter i = four_var_2_dmr->root()->var_begin(), e = four_var_2_dmr->root()->var_end(); i != e; ++i) {
            CPPUNIT_ASSERT((*i)->type() == dods_array_c);
            CPPUNIT_ASSERT(static_cast<Array&>(**i).length() == 8);

            arrays.push_back(*i);
        }

        // and it has four elements
        D4RValue rv_0(arrays[0]), rv_1(arrays[1]), rv_2(arrays[2]), rv_3(arrays[3]) ;
        D4RValueList *args = new D4RValueList;
        args->add_rvalue(&rv_0);
        args->add_rvalue(&rv_1);
        args->add_rvalue(&rv_2);
        args->add_rvalue(&rv_3);

        // call it; the function reads the values of the array
        // BaseType *argv[] = {arrays[0], arrays[1], arrays[2], arrays[3]};
        BaseType *result = 0;
        try {
            result = TabularFunction::function_dap4_tabular(args, *four_var_2_dmr);
            // function_dap2_tabular(arrays.size(), &arrays[0], *four_var_2_dmr, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }

        CPPUNIT_ASSERT(result->type() == dods_sequence_c);
        D4Sequence *s = static_cast<D4Sequence*>(result);

        ostringstream oss;
        s->print_val_by_rows(oss);

        DBG(cerr << "The d4 sequence print rep: " << oss.str() << endl);

        // Number of columns is the number of arrays
        CPPUNIT_ASSERT((vector<D4Sequence>::size_type)(s->var_end() - s->var_begin()) == arrays.size());

        SequenceValues sv = s->value();
        DBG(cerr << "SequenceValues size (number of rows in the sequence): " << sv.size() << endl);
        CPPUNIT_ASSERT(sv.size() == 8);

        for (SequenceValues::size_type i = 0; i < sv.size(); ++i) {
            DBG(cerr << "row " << long_to_string(i));
            for (SequenceValues::size_type j = 0; j < arrays.size(); ++j) {
                CPPUNIT_ASSERT(s->var_value(i, j) != 0);
                BaseType *btp = s->var_value(i, j);
                switch (btp->type()) {
                case dods_byte_c:
                    DBG(cerr << ": " << (int)static_cast<Byte&>(*btp).value());
                    CPPUNIT_ASSERT(static_cast<Byte&>(*btp).value() == 255);

                    break;
                case dods_int32_c:
                    DBG(cerr << ": " << static_cast<Int32&>(*btp).value());
                    CPPUNIT_ASSERT(static_cast<Int32&>(*btp).value() == 123456789);

                    break;
                case dods_float32_c:
                    DBG(cerr << ": " << static_cast<Float32&>(*btp).value());
                    CPPUNIT_ASSERT(static_cast<Float32&>(*btp).value() == (float)99.999);

                    break;
                case dods_float64_c:
                    DBG(cerr << ": " << static_cast<Float64&>(*btp).value());
                    CPPUNIT_ASSERT(static_cast<Float64&>(*btp).value() == 99.999);

                    break;
                default:
                    CPPUNIT_FAIL("Did not find the correct type in response Sequence");
                }
            }
            DBG(cerr << endl);
        }
    }

    CPPUNIT_TEST_SUITE( Dap4_TabularFunctionTest );

    CPPUNIT_TEST(one_var_test);
    CPPUNIT_TEST(four_var_2_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(Dap4_TabularFunctionTest);

} // namespace libdap

int main(int argc, char*argv[]) {

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
            cerr << "Usage: Dap4_TabularFunctionTest has the following tests:" << endl;
            const std::vector<Test*> &tests = Dap4_TabularFunctionTest::suite()->getTests();
            unsigned int prefix_len = Dap4_TabularFunctionTest::suite()->getName().append("::").length();
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
            test = Dap4_TabularFunctionTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
