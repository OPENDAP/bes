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

#include <iterator>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <BaseType.h>
#include <Byte.h>
#include <Int32.h>
#include <UInt32.h>
#include <Float32.h>
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
using namespace functions;

int test_variable_sleep_interval = 0;

static bool debug = false;
static bool debug2 = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);
#undef DBG2
#define DBG2(x) do { if (debug2) (x); } while(false);

namespace functions {

class TabularFunctionTest: public TestFixture {
private:
    TestTypeFactory btf;
    ConstraintEvaluator ce;

    DDS *one_var_dds, *one_var_2_dds;
    DDS *four_var_dds, *four_var_2_dds;
    DDS *four_var_mixed;

public:
    TabularFunctionTest() :
        one_var_dds(0), one_var_2_dds(0), four_var_dds(0), four_var_2_dds(0), four_var_mixed(0)
    {
    }
    ~TabularFunctionTest()
    {
    }

    void setUp()
    {
        try {
            one_var_dds = new DDS(&btf);
            one_var_dds->parse((string) TEST_SRC_DIR + "/tabular/one_var.dds");
            DBG2(one_var_dds->print_xml(stderr, false, "No blob"));

            one_var_2_dds = new DDS(&btf);
            one_var_2_dds->parse((string) TEST_SRC_DIR + "/tabular/one_var_2.dds");
            DBG2(one_var_2_dds->print_xml(stderr, false, "No blob"));

            four_var_dds = new DDS(&btf);
            four_var_dds->parse((string) TEST_SRC_DIR + "/tabular/four_var.dds");
            DBG2(four_var_dds->print_xml(stderr, false, "No blob"));

            four_var_2_dds = new DDS(&btf);
            four_var_2_dds->parse((string) TEST_SRC_DIR + "/tabular/four_var_2.dds");
            DBG2(four_var_2_dds->print_xml(stderr, false, "No blob"));

            four_var_mixed = new DDS(&btf);
            four_var_mixed->parse((string) TEST_SRC_DIR + "/tabular/four_var_mixed.dds");
            DBG2(four_var_mixed->print_xml(stderr, false, "No blob"));
        }

        catch (Error & e) {
            cerr << "SetUp: " << e.get_error_message() << endl;
            throw;
        }
    }

    void tearDown()
    {
        delete one_var_dds;
        delete one_var_2_dds;
        delete four_var_dds;
        delete four_var_2_dds;
    }

    void one_var_test()
    {
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
        TabularFunction::function_dap2_tabular(1, argv, *one_var_dds, &result);

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

        for (SequenceValues::size_type j = 0; j < sv.size(); ++j) {
            CPPUNIT_ASSERT(s->var_value(j, 0) != 0);
            Int32 *i = static_cast<Int32*>(s->var_value(j, 0));
            DBG(cerr << "Seq.a, row " << long_to_string(j) << ": " << i->value() << endl);
            CPPUNIT_ASSERT(i->value() == 123456789);
        }
    }

    void one_var_2_test()
    {
        // we know there's just one variable
        BaseType *btp = *(one_var_2_dds->var_begin());

        CPPUNIT_ASSERT(btp->type() == dods_array_c);

        // ... and it's an Int32 array
        Array *a = static_cast<Array*>(btp);

        // and it has four elements
        CPPUNIT_ASSERT(a->length() == 12);

        // call it; the function reads the values of the array
        BaseType *argv[] = { a };
        BaseType *result = 0;
        TabularFunction::function_dap2_tabular(1, argv, *one_var_dds, &result);

        // Now check that the response types are as expected
        DBG(cerr << "a[11]: " << dynamic_cast<Int32&>(*(a->var(11))).value() << endl);
        CPPUNIT_ASSERT(dynamic_cast<Int32&>(*(a->var(11))).value() == 123456789);

        CPPUNIT_ASSERT(result->type() == dods_sequence_c);
        Sequence *s = static_cast<Sequence*>(result);

        // ... because we know it's an int32
        CPPUNIT_ASSERT(s->var("a")->type() == dods_int32_c);
        //BaseType *var_value(size_t row, const string &name)
        SequenceValues sv = s->value();
        DBG(cerr << "SequenceValues size: " << sv.size() << endl);
        CPPUNIT_ASSERT(sv.size() == 12);

        for (SequenceValues::size_type j = 0; j < sv.size(); ++j) {
            CPPUNIT_ASSERT(s->var_value(j, 0) != 0);
            Int32 *i = static_cast<Int32*>(s->var_value(j, 0));
            DBG(cerr << "Seq.a, row " << long_to_string(j) << ": " << i->value() << endl);
            CPPUNIT_ASSERT(i->value() == 123456789);
        }
    }

    void compute_array_shape_test()
    {
        BaseType *btp = *(four_var_dds->var_begin());
        CPPUNIT_ASSERT(btp->type() == dods_array_c);

        vector<unsigned long> shape = TabularFunction::array_shape(static_cast<Array*>(btp));

        DBG(cerr << "shape size: " << shape.size() << endl);
        CPPUNIT_ASSERT(shape.size() == 1);
        CPPUNIT_ASSERT(shape[0] == 4);
    }

    void compute_array_shape_test_2()
    {
        BaseType *btp = *(four_var_2_dds->var_begin());
        CPPUNIT_ASSERT(btp->type() == dods_array_c);

        vector<unsigned long> shape = TabularFunction::array_shape(static_cast<Array*>(btp));

        DBG(cerr << "shape size: " << shape.size() << endl);
        CPPUNIT_ASSERT(shape.size() == 2);
        CPPUNIT_ASSERT(shape[0] == 4);
        CPPUNIT_ASSERT(shape[1] == 2);
    }

    void array_shape_matches_test()
    {
        //(Array *a, vector<unsigned long> shape)
        BaseType *btp = *(four_var_dds->var_begin());
        CPPUNIT_ASSERT(btp->type() == dods_array_c);

        vector<unsigned long> shape = TabularFunction::array_shape(static_cast<Array*>(btp));

        DBG(cerr << "shape size: " << shape.size() << endl);
        CPPUNIT_ASSERT(shape.size() == 1);
        CPPUNIT_ASSERT(shape[0] == 4);

        CPPUNIT_ASSERT(TabularFunction::shape_matches(static_cast<Array*>(btp), shape));

        btp = *(four_var_dds->var_begin() + 1);
        CPPUNIT_ASSERT(btp->type() == dods_array_c);

        CPPUNIT_ASSERT(TabularFunction::shape_matches(static_cast<Array*>(btp), shape));
    }

    void array_shape_matches_test_2()
    {
        //(Array *a, vector<unsigned long> shape)
        BaseType *btp = *(four_var_2_dds->var_begin());
        CPPUNIT_ASSERT(btp->type() == dods_array_c);

        vector<unsigned long> shape = TabularFunction::array_shape(static_cast<Array*>(btp));

        DBG(cerr << "shape size: " << shape.size() << endl);
        CPPUNIT_ASSERT(shape.size() == 2);
        CPPUNIT_ASSERT(shape[0] == 4);
        CPPUNIT_ASSERT(shape[1] == 2);

        CPPUNIT_ASSERT(TabularFunction::shape_matches(static_cast<Array*>(btp), shape));

        btp = *(four_var_2_dds->var_begin() + 1);
        CPPUNIT_ASSERT(btp->type() == dods_array_c);

        CPPUNIT_ASSERT(TabularFunction::shape_matches(static_cast<Array*>(btp), shape));
    }

    void build_sequence_values_test()
    {
        DBG(cerr << "Starting build_sequence_values_test..." << endl);

        vector<Array*> arrays;
        for (DDS::Vars_iter i = four_var_2_dds->var_begin(), e = four_var_2_dds->var_end(); i != e; ++i) {
            CPPUNIT_ASSERT((*i)->type() == dods_array_c);
            arrays.push_back(static_cast<Array*>(*i));
        }

        vector<unsigned long> shape = TabularFunction::array_shape(arrays.at(0));
        DBG(cerr << "shape size: " << shape.size() << endl);
        CPPUNIT_ASSERT(shape.size() == 2);
        CPPUNIT_ASSERT(shape[0] == 4);
        CPPUNIT_ASSERT(shape[1] == 2);

        unsigned long num_values = TabularFunction::number_of_values(shape);
        CPPUNIT_ASSERT(num_values == 8);

        SequenceValues sv(num_values);
        try {
            TabularFunction::read_values(arrays);
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }

        try {
            TabularFunction::build_sequence_values(arrays, sv);
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }

        for (SequenceValues::size_type i = 0; i < sv.size(); ++i) {
            BaseTypeRow *row = sv.at(i);
            for (BaseTypeRow::size_type j = 0; j < row->size(); ++j) {
                switch (row->at(j)->type()) {
                case dods_byte_c: {
                    dods_byte v = static_cast<Byte*>(row->at(j))->value();
                    DBG(cerr << "b(" << i << "," << j << "): " << v << endl);
                    break;
                }
                case dods_int32_c: {
                    dods_int32 v = static_cast<Int32*>(row->at(j))->value();
                    DBG(cerr << "i(" << i << "," << j << "): " << v << endl);
                    break;
                }
                case dods_float32_c: {
                    dods_float32 f = static_cast<Float32*>(row->at(j))->value();
                    DBG(cerr << "f(" << i << "," << j << "): " << f << endl);
                    break;
                }
                case dods_float64_c: {
                    dods_float64 f = static_cast<Float64*>(row->at(j))->value();
                    DBG(cerr << "f(" << i << "," << j << "): " << f << endl);
                    break;
                }
                default:
                    CPPUNIT_FAIL("Wrong type in four_var_2_dds");
                    break;
                }
            }
        }

        CPPUNIT_ASSERT(sv.size() == 8);
        for (SequenceValues::size_type i = 0; i < sv.size(); ++i) {
            BaseTypeRow *row = sv.at(i);
            CPPUNIT_ASSERT(row->size() == 4);
            CPPUNIT_ASSERT(row->at(0)->type() == dods_byte_c);
            CPPUNIT_ASSERT(row->at(1)->type() == dods_int32_c);
            CPPUNIT_ASSERT(row->at(2)->type() == dods_float32_c);
            CPPUNIT_ASSERT(row->at(3)->type() == dods_float64_c);
        }
    }

    // Test the simple case where the shape of the indep and dep vars are the same.
    // Normally the function is not used in that case, but we're testing it for
    // completeness' sake.
    void combine_sequence_values_test_1()
    {
        DBG(cerr << "Starting combine_sequence_values_test_1 ..." << endl);

        vector<Array*> arrays;
        for (DDS::Vars_iter i = four_var_mixed->var_begin(), e = four_var_mixed->var_end(); i != e; ++i) {
            CPPUNIT_ASSERT((*i)->type() == dods_array_c);
            arrays.push_back(static_cast<Array*>(*i));
        }

        vector<Array*> a, b;
        a.push_back(arrays[0]);
        b.push_back(arrays[1]);

        vector<unsigned long> a_shape = TabularFunction::array_shape(a.at(0));
        vector<unsigned long> b_shape = TabularFunction::array_shape(b.at(0));

        unsigned long num_a_values = TabularFunction::number_of_values(a_shape);
        CPPUNIT_ASSERT(num_a_values == 2);
        SequenceValues a_sv(num_a_values);
        TabularFunction::read_values(a);
        TabularFunction::build_sequence_values(a, a_sv);
        CPPUNIT_ASSERT(a_sv.size() == 2);

        unsigned long num_b_values = TabularFunction::number_of_values(b_shape);
        //CPPUNIT_ASSERT(num_b_values == 8);
        CPPUNIT_ASSERT(num_b_values == 2);
        SequenceValues b_sv(num_b_values);

        TabularFunction::read_values(b);
        TabularFunction::build_sequence_values(b, b_sv);
        CPPUNIT_ASSERT(b_sv.size() == 2);

        SequenceValues &result = b_sv;
        try {
            TabularFunction::combine_sequence_values(b_sv, a_sv);
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }

        // This loop is mostly so that we can see the values when using -d
        for (SequenceValues::size_type i = 0; i < result.size(); ++i) {
            BaseTypeRow *row = result.at(i);
            for (BaseTypeRow::size_type j = 0; j < row->size(); ++j) {
                switch (row->at(j)->type()) {
                case dods_byte_c: {
                    dods_byte v = static_cast<Byte*>(row->at(j))->value();
                    DBG(cerr << "b(" << i << "," << j << "): " << v << endl);
                    break;
                }
                case dods_int32_c: {
                    dods_int32 v = static_cast<Int32*>(row->at(j))->value();
                    DBG(cerr << "i(" << i << "," << j << "): " << v << endl);
                    break;
                }
                case dods_float32_c: {
                    dods_float32 f = static_cast<Float32*>(row->at(j))->value();
                    DBG(cerr << "f(" << i << "," << j << "): " << f << endl);
                    break;
                }
                case dods_float64_c: {
                    dods_float64 f = static_cast<Float64*>(row->at(j))->value();
                    DBG(cerr << "f(" << i << "," << j << "): " << f << endl);
                    break;
                }
                default:
                    CPPUNIT_FAIL("Wrong type in four_var_2_dds");
                    break;
                }
            }
        }

        CPPUNIT_ASSERT(result.size() == 2);
        for (SequenceValues::size_type i = 0; i < result.size(); ++i) {
            BaseTypeRow *row = result.at(i);
            CPPUNIT_ASSERT(row->size() == 2);
            CPPUNIT_ASSERT(row->at(0)->type() == dods_int32_c);
            CPPUNIT_ASSERT(row->at(1)->type() == dods_byte_c);
        }
    }

    void combine_sequence_values_test_2()
    {
        DBG(cerr << "Starting combine_sequence_values_test_2 ..." << endl);

        vector<Array*> arrays;
        for (DDS::Vars_iter i = four_var_mixed->var_begin(), e = four_var_mixed->var_end(); i != e; ++i) {
            CPPUNIT_ASSERT((*i)->type() == dods_array_c);
            arrays.push_back(static_cast<Array*>(*i));
        }

        vector<Array*> indep_vars, dep_vars;
        indep_vars.push_back(arrays[0]);
        indep_vars.push_back(arrays[1]);

        dep_vars.push_back(arrays[2]);

        vector<unsigned long> indep_shape = TabularFunction::array_shape(indep_vars.at(0));
        vector<unsigned long> dep_shape = TabularFunction::array_shape(dep_vars.at(0));

        unsigned long num_indep_values = TabularFunction::number_of_values(indep_shape);
        CPPUNIT_ASSERT(num_indep_values == 2);
        SequenceValues indep_sv(num_indep_values);
        TabularFunction::read_values(indep_vars);
        TabularFunction::build_sequence_values(indep_vars, indep_sv);
        CPPUNIT_ASSERT(indep_sv.size() == 2);

        unsigned long num_dep_values = TabularFunction::number_of_values(dep_shape);
        CPPUNIT_ASSERT(num_dep_values == 8);
        SequenceValues dep_sv(num_dep_values);
        TabularFunction::read_values(dep_vars);
        TabularFunction::build_sequence_values(dep_vars, dep_sv);
        CPPUNIT_ASSERT(dep_sv.size() == 8);

        SequenceValues &result = dep_sv;
        try {
            TabularFunction::combine_sequence_values(dep_sv, indep_sv);
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }

        // This loop is mostly so that we can see the values when using -d
        for (SequenceValues::size_type i = 0; i < result.size(); ++i) {
            BaseTypeRow *row = result.at(i);
            for (BaseTypeRow::size_type j = 0; j < row->size(); ++j) {
                switch (row->at(j)->type()) {
                case dods_byte_c: {
                    dods_byte v = static_cast<Byte*>(row->at(j))->value();
                    DBG(cerr << "b(" << i << "," << j << "): " << v << endl);
                    break;
                }
                case dods_int32_c: {
                    dods_int32 v = static_cast<Int32*>(row->at(j))->value();
                    DBG(cerr << "i(" << i << "," << j << "): " << v << endl);
                    break;
                }
                case dods_float32_c: {
                    dods_float32 f = static_cast<Float32*>(row->at(j))->value();
                    DBG(cerr << "f(" << i << "," << j << "): " << f << endl);
                    break;
                }
                case dods_float64_c: {
                    dods_float64 f = static_cast<Float64*>(row->at(j))->value();
                    DBG(cerr << "f(" << i << "," << j << "): " << f << endl);
                    break;
                }
                default:
                    CPPUNIT_FAIL("Wrong type in four_var_2_dds");
                    break;
                }
            }
        }

        CPPUNIT_ASSERT(result.size() == 8);
        for (SequenceValues::size_type i = 0; i < result.size(); ++i) {
            BaseTypeRow *row = result.at(i);
            CPPUNIT_ASSERT(row->size() == 3);
            CPPUNIT_ASSERT(row->at(0)->type() == dods_float32_c);
            CPPUNIT_ASSERT(row->at(1)->type() == dods_byte_c);
            CPPUNIT_ASSERT(row->at(2)->type() == dods_int32_c);
        }
    }

    void add_index_columns_test()
    {
        DBG(cerr << "Starting add_index_columns_test ..." << endl);

        vector<Array*> arrays;
        for (DDS::Vars_iter i = four_var_mixed->var_begin(), e = four_var_mixed->var_end(); i != e; ++i) {
            CPPUNIT_ASSERT((*i)->type() == dods_array_c);
            arrays.push_back(static_cast<Array*>(*i));
        }

        vector<Array*> indep_vars, dep_vars;
        indep_vars.push_back(arrays[0]);
        indep_vars.push_back(arrays[1]);

        dep_vars.push_back(arrays[2]);

        vector<unsigned long> indep_shape = TabularFunction::array_shape(indep_vars.at(0));
        vector<unsigned long> dep_shape = TabularFunction::array_shape(dep_vars.at(0));

        unsigned long num_indep_values = TabularFunction::number_of_values(indep_shape);
        CPPUNIT_ASSERT(num_indep_values == 2);
        //SequenceValues indep_sv(num_indep_values);
        //TabularFunction::read_values(indep_vars);
        //TabularFunction::build_sequence_values(indep_vars, indep_sv);
        //CPPUNIT_ASSERT(indep_sv.size() == 2);

        unsigned long num_dep_values = TabularFunction::number_of_values(dep_shape);
        CPPUNIT_ASSERT(num_dep_values == 8);
        SequenceValues dep_sv(num_dep_values);

        TabularFunction::read_values(dep_vars);
        CPPUNIT_ASSERT(dep_vars.size() == 1);

        TabularFunction::add_index_column(indep_shape, dep_shape, dep_vars);
        CPPUNIT_ASSERT(dep_vars.size() == 2);
        CPPUNIT_ASSERT(dep_vars.at(0)->type() == dods_array_c);
        CPPUNIT_ASSERT(dep_vars.at(0)->var()->type() == dods_uint32_c);
        DBG(cerr << "first dep var name: " << dep_vars.at(0)->name() << endl);
        DBG(cerr << "second dep var name: " << dep_vars.at(1)->name() << endl);

        vector<dods_uint32> synth(8);
        dep_vars.at(0)->value(&synth[0]);

        CPPUNIT_ASSERT(synth.at(0) == 0);
        CPPUNIT_ASSERT(synth.at(7) == 3);
    }

    void four_var_test()
    {
        // There are four arrays of varying types, each with four elements
        vector<BaseType*> arrays;
        for (DDS::Vars_iter i = four_var_dds->var_begin(), e = four_var_dds->var_end(); i != e; ++i) {
            CPPUNIT_ASSERT((*i)->type() == dods_array_c);
            CPPUNIT_ASSERT(static_cast<Array&>(**i).length() == 4);

            arrays.push_back(*i);
        }

        // and it has four elements

        // call it; the function reads the values of the array
        BaseType *result = 0;
        try {
            TabularFunction::function_dap2_tabular(arrays.size(), &arrays[0], *four_var_dds, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }

        CPPUNIT_ASSERT(result->type() == dods_sequence_c);
        Sequence *s = static_cast<Sequence*>(result);

        // Number of columns is the number of arrays
        CPPUNIT_ASSERT((vector<BaseType*>::size_type )distance(s->var_begin(), s->var_end()) == arrays.size());

        SequenceValues sv = s->value();
        DBG(cerr << "SequenceValues size (number of rows in the sequence): " << sv.size() << endl);
        CPPUNIT_ASSERT(sv.size() == 4);

        for (SequenceValues::size_type i = 0; i < sv.size(); ++i) {
            DBG(cerr << "row " << long_to_string(i));
            for (SequenceValues::size_type j = 0; j < arrays.size(); ++j) {
                CPPUNIT_ASSERT(s->var_value(i, j) != 0);
                BaseType *btp = s->var_value(i, j);
                switch (btp->type()) {
                case dods_byte_c:
                    DBG(cerr << ": " << (int )static_cast<Byte&>(*btp).value())
                    ;
                    CPPUNIT_ASSERT(static_cast<Byte&>(*btp).value() == 255);

                    break;
                case dods_int32_c:
                    DBG(cerr << ": " << static_cast<Int32&>(*btp).value())
                    ;
                    CPPUNIT_ASSERT(static_cast<Int32&>(*btp).value() == 123456789);

                    break;
                case dods_float32_c:
                    DBG(cerr << ": " << static_cast<Float32&>(*btp).value())
                    ;
                    CPPUNIT_ASSERT(static_cast<Float32&>(*btp).value() == (float )99.999);

                    break;
                case dods_float64_c:
                    DBG(cerr << ": " << static_cast<Float64&>(*btp).value())
                    ;
                    CPPUNIT_ASSERT(static_cast<Float64&>(*btp).value() == 99.999);

                    break;
                default:
                    CPPUNIT_FAIL("Did not find the correct type in response Sequence");
                }
            }
            DBG(cerr << endl);
        }
    }

    void four_var_2_test()
    {
        // There are four arrays of varying types, each with four elements
        vector<BaseType*> arrays;
        for (DDS::Vars_iter i = four_var_2_dds->var_begin(), e = four_var_2_dds->var_end(); i != e; ++i) {
            CPPUNIT_ASSERT((*i)->type() == dods_array_c);
            CPPUNIT_ASSERT(static_cast<Array&>(**i).length() == 8);

            arrays.push_back(*i);
        }

        // and it has four elements

        // call it; the function reads the values of the array
        BaseType *result = 0;
        try {
            TabularFunction::function_dap2_tabular(arrays.size(), &arrays[0], *four_var_2_dds, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }

        CPPUNIT_ASSERT(result->type() == dods_sequence_c);
        Sequence *s = static_cast<Sequence*>(result);

        // Number of columns is the number of arrays; just being pedantic with the cast..
        CPPUNIT_ASSERT((vector<BaseType*>::size_type )distance(s->var_begin(), s->var_end()) == arrays.size());

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
                    DBG(cerr << ": " << (int )static_cast<Byte&>(*btp).value())
                    ;
                    CPPUNIT_ASSERT(static_cast<Byte&>(*btp).value() == 255);

                    break;
                case dods_int32_c:
                    DBG(cerr << ": " << static_cast<Int32&>(*btp).value())
                    ;
                    CPPUNIT_ASSERT(static_cast<Int32&>(*btp).value() == 123456789);

                    break;
                case dods_float32_c:
                    DBG(cerr << ": " << static_cast<Float32&>(*btp).value())
                    ;
                    CPPUNIT_ASSERT(static_cast<Float32&>(*btp).value() == (float )99.999);

                    break;
                case dods_float64_c:
                    DBG(cerr << ": " << static_cast<Float64&>(*btp).value())
                    ;
                    CPPUNIT_ASSERT(static_cast<Float64&>(*btp).value() == 99.999);

                    break;
                default:
                    CPPUNIT_FAIL("Did not find the correct type in response Sequence");
                }
            }
            DBG(cerr << endl);
        }
    }

    // This test uses the four_var_mixed DDS, but it only passes the first three vars to the tabular() function
    void four_var_mixed_test_1()
    {
        DBG(cerr << "Starting four_var_mixed_test ..." << endl);

        vector<BaseType*> arrays;
        for (DDS::Vars_iter i = four_var_mixed->var_begin(), e = four_var_mixed->var_end(); i != e; ++i) {
            CPPUNIT_ASSERT((*i)->type() == dods_array_c);
            arrays.push_back(static_cast<Array*>(*i));
        }
        arrays.pop_back();      // This is just an easy way to get the only the first three of the vars...

        BaseType *result = 0;
        try {
            TabularFunction::function_dap2_tabular(arrays.size(), &arrays[0], *four_var_mixed, &result);
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }

        CPPUNIT_ASSERT(result->type() == dods_sequence_c);
        Sequence *s = static_cast<Sequence*>(result);

        // Number of columns is the number of arrays; just being pedantic with the cast..
        // Note that because the single dep var is one dimension bigger than the independent
        // variables, an extra column has been added to the result.
        CPPUNIT_ASSERT((vector<BaseType*>::size_type )distance(s->var_begin(), s->var_end()) == arrays.size() + 1);

        SequenceValues sv = s->value();
        DBG(cerr << "SequenceValues size (number of rows in the sequence): " << sv.size() << endl);
        CPPUNIT_ASSERT(sv.size() == 8);

        for (SequenceValues::size_type i = 0; i < sv.size(); ++i) {
            DBG(cerr << "row " << long_to_string(i));
            for (unsigned long j = 0; j < arrays.size() + 1; ++j) {
                CPPUNIT_ASSERT(s->var_value(i, j) != 0);
                BaseType *btp = s->var_value(i, j);
                switch (btp->type()) {
                case dods_byte_c:
                    DBG(cerr << " b: " << (int )static_cast<Byte&>(*btp).value())
                    ;
                    CPPUNIT_ASSERT(static_cast<Byte&>(*btp).value() == 255);
                    break;

                case dods_int32_c:
                    DBG(cerr << " i: " << static_cast<Int32&>(*btp).value())
                    ;
                    CPPUNIT_ASSERT(static_cast<Int32&>(*btp).value() == 123456789);
                    break;

                case dods_uint32_c:
                    DBG(cerr << " ui: " << static_cast<UInt32&>(*btp).value())
                    ;
                    CPPUNIT_ASSERT(static_cast<UInt32&>(*btp).value() < 8);
                    break;

                case dods_float32_c:
                    DBG(cerr << " f: " << static_cast<Float32&>(*btp).value())
                    ;
                    CPPUNIT_ASSERT(static_cast<Float32&>(*btp).value() == (float )99.999);
                    break;

                default:
                    DBG(cerr << "Error: Found unexpected " << btp->name() << ", " << btp->type_name() << endl)
                    ;
                    CPPUNIT_FAIL("Did not find the correct type in response Sequence");
                }
            }
            DBG(cerr << endl);
        }

        for (SequenceValues::size_type i = 0; i < sv.size(); ++i) {
            BaseTypeRow *row = sv.at(i);
            CPPUNIT_ASSERT(row->size() == 4);
            CPPUNIT_ASSERT(row->at(0)->type() == dods_uint32_c);
            CPPUNIT_ASSERT(row->at(1)->type() == dods_float32_c);
            CPPUNIT_ASSERT(row->at(2)->type() == dods_byte_c);
            CPPUNIT_ASSERT(row->at(3)->type() == dods_int32_c);
        }
    }

    void one_var_2_print_val_test()
    {
        // we know there's just one variable
        BaseType *btp = *(one_var_2_dds->var_begin());

        CPPUNIT_ASSERT(btp->type() == dods_array_c);

        // ... and it's an Int32 array
        Array *a = static_cast<Array*>(btp);

        // and it has four elements
        CPPUNIT_ASSERT(a->length() == 12);

        // call it; the function reads the values of the array
        BaseType *argv[] = { a };
        BaseType *result = 0;
        TabularFunction::function_dap2_tabular(1, argv, *one_var_dds, &result);

        CPPUNIT_ASSERT(result->type() == dods_sequence_c);
        Sequence *s = static_cast<Sequence*>(result);

        ostringstream oss;
        s->print_val_by_rows(oss);

        DBG(cerr << "The sequence print rep: " << oss.str() << endl);

        // compare it to a baseline file
    }

CPPUNIT_TEST_SUITE( TabularFunctionTest );

    CPPUNIT_TEST(one_var_test);
    CPPUNIT_TEST(one_var_2_test);
    CPPUNIT_TEST(one_var_2_print_val_test);

    CPPUNIT_TEST(compute_array_shape_test);
    CPPUNIT_TEST(compute_array_shape_test_2);
    CPPUNIT_TEST(array_shape_matches_test);
    CPPUNIT_TEST(array_shape_matches_test_2);

    CPPUNIT_TEST(build_sequence_values_test);
    CPPUNIT_TEST(combine_sequence_values_test_1);
    CPPUNIT_TEST(combine_sequence_values_test_2);
    CPPUNIT_TEST(add_index_columns_test);

    CPPUNIT_TEST(four_var_test);
    CPPUNIT_TEST(four_var_2_test);
    CPPUNIT_TEST(four_var_mixed_test_1);

    CPPUNIT_TEST_SUITE_END()
    ;
};

CPPUNIT_TEST_SUITE_REGISTRATION(TabularFunctionTest);

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
            cerr << "Usage: TabularFunctionTest has the following tests:" << endl;
            const std::vector<Test*> &tests = TabularFunctionTest::suite()->getTests();
            unsigned int prefix_len = TabularFunctionTest::suite()->getName().append("::").length();
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
            test = TabularFunctionTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
