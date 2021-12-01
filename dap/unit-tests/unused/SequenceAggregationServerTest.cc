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

#include "config.h"

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <memory>

#include <libdap/DDS.h>
#include <libdap/DataDDS.h>
#include <libdap/Int32.h>
#include <libdap/Float32.h>
#include <libdap/Sequence.h>
#include <libdap/Structure.h>

#include <libdap/GetOpt.h>

#include <libdap/debug.h>

#include <test/TestTypeFactory.h>

#include "BESDapSequenceAggregationServer.h"

#include "test_config.h"

using namespace CppUnit;
using namespace std;
using namespace libdap;

int test_variable_sleep_interval = 0;

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

class SequenceAggregationServerTest: public TestFixture {
private:
    BESDapSequenceAggregationServer *agg_server;

    auto_ptr<DDS> build_dds_with_containers(const string &dds1, const string &dds2) {
        auto_ptr<DDS> dds(new DDS(new TestTypeFactory));

        DBG(cerr << "Building using " << dds1 << " and " << dds2 << endl);

        try {
            dds->container_name("c1");
            dds->parse(dds1);

            dds->container_name("c2");
            dds->parse(dds2);
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }

        DBG(cerr << "Built DDS: ");
        DBG(dds->print(cerr));
        DBG(cerr <<endl);

        return dds;
    }

    auto_ptr<DataDDS> build_data_dds_with_containers(const string &dds1, const string &dds2) {
        auto_ptr<DataDDS> data_dds(new DataDDS(new TestTypeFactory));

        DBG(cerr << "Building using " << dds1 << " and " << dds2 << endl);

        try {
            data_dds->container_name("c1");
            data_dds->parse(dds1);

            data_dds->container_name("c2");
            data_dds->parse(dds2);
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }

        DBG(cerr << "Built DataDDS: ");
        DBG(data_dds->print(cerr));
        DBG(cerr <<endl);

        return data_dds;
    }

public:
    SequenceAggregationServerTest() : agg_server(new BESDapSequenceAggregationServer("test")) {

    }

    ~SequenceAggregationServerTest() {
        delete agg_server;
    }

    void setUp() {
    }

    void tearDown() {
    }

    void test_dds_for_suitability_pass() {
        string dds1 = string(TEST_SRC_DIR) + "/input-files/sequence_1.dds";
        string dds2 = string(TEST_SRC_DIR) + "/input-files/sequence_2.dds";
        auto_ptr<DDS> dds = build_dds_with_containers(dds1,dds2);

        // should pass the suitability test
        CPPUNIT_ASSERT(agg_server->test_dds_for_suitability(dds.get()));
    }

    void test_dds_for_suitability_fail_type() {
        string dds1 = string(TEST_SRC_DIR) + "/input-files/sequence_1.dds";
        string dds2 = string(TEST_SRC_DIR) + "/input-files/sequence_1_1.dds";
        auto_ptr<DDS> dds = build_dds_with_containers(dds1,dds2);

        // should fail the suitability test
        CPPUNIT_ASSERT(!agg_server->test_dds_for_suitability(dds.get()));
    }

    void test_dds_for_suitability_fail_name() {
        string dds1 = string(TEST_SRC_DIR) + "/input-files/sequence_1.dds";
        string dds2 = string(TEST_SRC_DIR) + "/input-files/sequence_1_2.dds";
        auto_ptr<DDS> dds = build_dds_with_containers(dds1,dds2);

        CPPUNIT_ASSERT(!agg_server->test_dds_for_suitability(dds.get()));
    }

    void test_dds_for_suitability_pass_name() {
        string dds1 = string(TEST_SRC_DIR) + "/input-files/sequence_1.dds";
        string dds2 = string(TEST_SRC_DIR) + "/input-files/sequence_1_2.dds";
        auto_ptr<DDS> dds = build_dds_with_containers(dds1,dds2);

        CPPUNIT_ASSERT(agg_server->test_dds_for_suitability(dds.get(), false /* names_must_match */));
    }

    void test_dds_for_suitability_fail_number() {
        string dds1 = string(TEST_SRC_DIR) + "/input-files/sequence_1.dds";
        string dds2 = string(TEST_SRC_DIR) + "/input-files/sequence_1_3.dds";
        auto_ptr<DDS> dds = build_dds_with_containers(dds1,dds2);

        CPPUNIT_ASSERT(!agg_server->test_dds_for_suitability(dds.get()));
    }

    void test_dds_for_suitability_fail_not_simple() {
        string dds1 = string(TEST_SRC_DIR) + "/input-files/nested_sequence_1.dds";
        string dds2 = string(TEST_SRC_DIR) + "/input-files/nested_sequence_2.dds";
        auto_ptr<DDS> dds = build_dds_with_containers(dds1,dds2);

        // should fail; the dds is not a table of simple types (contains a
        // nested sequence)
        CPPUNIT_ASSERT(!agg_server->test_dds_for_suitability(dds.get()));
    }

    // This tests building a DDS for the aggregation
    void test_build_new_dds_no_data() {
        string dds1 = string(TEST_SRC_DIR) + "/input-files/sequence_1.dds";
        string dds2 = string(TEST_SRC_DIR) + "/input-files/sequence_2.dds";
        auto_ptr<DDS> dds = build_dds_with_containers(dds1,dds2);

        auto_ptr<DDS> agg_dds = agg_server->build_new_dds(dds.get());

        DBG(cerr << "agg_dds: ");
        DBG(agg_dds->print(cerr));
        DBG(cerr << endl);

        // Should have one variable, a sequence.
        CPPUNIT_ASSERT(agg_dds->var_end() - agg_dds->var_begin() == 1);
        CPPUNIT_ASSERT(agg_dds->get_var_index(0)->type() == dods_sequence_c);

        // The sequence should have three columns
        Sequence *s = static_cast<Sequence*>(agg_dds->get_var_index(0));
        CPPUNIT_ASSERT(s->element_count() == 3); // from the DDSs we used

        // There should be 0 rows
        SequenceValues sv = s->value();
        CPPUNIT_ASSERT(sv.size() == 0);

        DBG(cerr << "agg_dds.s: ");
        DBG(s->print_val_by_rows(cerr));
        DBG(cerr << endl);
    }

    void test_intern_all_data() {
        string dds1 = string(TEST_SRC_DIR) + "/input-files/sequence_1.dds";
        auto_ptr<DataDDS> dds(new DataDDS(new TestTypeFactory));

        dds->parse(dds1);
        dds->mark_all(true);            // Without this values are not 'sent'
        dds->tag_nested_sequences();    // Without this TestSequence won't load values

        // Check first that we've built what we expect
        CPPUNIT_ASSERT(dds->var_end() - dds->var_begin() == 1);
        CPPUNIT_ASSERT(dds->get_var_index(0)->type() == dods_sequence_c);
        // The sequence should have three columns
        Sequence *s = static_cast<Sequence*>(dds->get_var_index(0));
        CPPUNIT_ASSERT(s->element_count() == 3); // from the DDSs we used
        // There should be 0 rows
        CPPUNIT_ASSERT(s->value().size() == 0);

        // Now load it with values
        ConstraintEvaluator evaluator;
        agg_server->intern_all_data(dds.get(), evaluator);

        // Is it correct?
        CPPUNIT_ASSERT(dds->var_end() - dds->var_begin() == 1);
        CPPUNIT_ASSERT(dds->get_var_index(0)->type() == dods_sequence_c);
        // The sequence should have three columns
        s = static_cast<Sequence*>(dds->get_var_index(0));

        DBG(cerr << "dds.s: ");
        DBG(s->print_val_by_rows(cerr));
        DBG(cerr << endl);

        CPPUNIT_ASSERT(s->element_count() == 3); // from the DDSs we used
        // There should be 4 rows
        CPPUNIT_ASSERT(s->value().size() == 4);

        Int32 *i = dynamic_cast<Int32*>(s->value().at(3)->at(0));
        CPPUNIT_ASSERT(i != 0);
        CPPUNIT_ASSERT(i->value() == 123456789);

        Float32 *f = dynamic_cast<Float32*>(s->value().at(3)->at(2));
        CPPUNIT_ASSERT(f != 0);
        CPPUNIT_ASSERT(f->value() == (float)99.999);
    }

    void test_intern_all_data_with_cont() {
        string dds1 = string(TEST_SRC_DIR) + "/input-files/sequence_1.dds";
        string dds2 = string(TEST_SRC_DIR) + "/input-files/sequence_2.dds";
        auto_ptr<DataDDS> dds = build_data_dds_with_containers(dds1,dds2);

        dds->mark_all(true);            // Without this values are not 'sent'
        dds->tag_nested_sequences();    // Without this TestSequence won't load values

        // Now load it with values
        ConstraintEvaluator evaluator;
        agg_server->intern_all_data(dds.get(), evaluator);

        // Is it correct?
        CPPUNIT_ASSERT(dds->var_end() - dds->var_begin() == 2);
        CPPUNIT_ASSERT(dds->get_var_index(0)->type() == dods_structure_c);
        // Each sequence should have three columns
        for (int i = 0; i < 2; ++i) {
            DBG(cerr << "checking Sequence #" << i << endl);
            Structure *c = static_cast<Structure*>(dds->get_var_index(i));
            Sequence *s = static_cast<Sequence*>(c->get_var_index(0));

            DBG(cerr << "dds.s: ");
            DBG(s->print_val_by_rows(cerr));
            DBG(cerr << endl);

            CPPUNIT_ASSERT(s->element_count() == 3); // from the DDSs we used
            // There should be 4 rows
            CPPUNIT_ASSERT(s->value().size() == 4);

            Int32 *i32 = dynamic_cast<Int32*>(s->value().at(3)->at(0));
            CPPUNIT_ASSERT(i32 != 0);
            CPPUNIT_ASSERT(i32->value() == 123456789);

            Float32 *f = dynamic_cast<Float32*>(s->value().at(3)->at(2));
            CPPUNIT_ASSERT(f != 0);
            CPPUNIT_ASSERT(f->value() == (float )99.999);
        }
    }

    void test_build_new_dds_with_data() {
        string dds1 = string(TEST_SRC_DIR) + "/input-files/sequence_1.dds";
        string dds2 = string(TEST_SRC_DIR) + "/input-files/sequence_2.dds";
        auto_ptr<DataDDS> dds = build_data_dds_with_containers(dds1,dds2);

        dds->mark_all(true);            // Without this values are not 'sent'
        dds->tag_nested_sequences();    // Without this TestSequence won't load values

        ConstraintEvaluator evaluator;
        agg_server->intern_all_data(dds.get(), evaluator);

        DBG(cerr << "DDS with containers: ");
        DBG(dds->print(cerr));
        DBG(cerr << endl);

        auto_ptr<DataDDS> agg_dds = agg_server->build_new_dds(dds.get());

        DBG(cerr << "Aggregated DDS: ");
        DBG(agg_dds->print(cerr));
        DBG(cerr << endl);

        // Should have one variable, a sequence.
        CPPUNIT_ASSERT(agg_dds->var_end() - agg_dds->var_begin() == 1);
        CPPUNIT_ASSERT(agg_dds->get_var_index(0)->type() == dods_sequence_c);

        // The sequence should have three columns
        Sequence *s = static_cast<Sequence*>(agg_dds->get_var_index(0));
        CPPUNIT_ASSERT(s->element_count() == 3); // from the DDSs we used

        // There should be 0 rows
        SequenceValues sv = s->value();
        CPPUNIT_ASSERT(sv.size() == 8);

        DBG(cerr << "agg_dds.s: ");
        DBG(s->print_val_by_rows(cerr));
        DBG(cerr << endl);
    }

    CPPUNIT_TEST_SUITE( SequenceAggregationServerTest );

    CPPUNIT_TEST(test_dds_for_suitability_pass);
    CPPUNIT_TEST(test_dds_for_suitability_fail_type);
    CPPUNIT_TEST(test_dds_for_suitability_fail_name);
    CPPUNIT_TEST(test_dds_for_suitability_pass_name);
    CPPUNIT_TEST(test_dds_for_suitability_fail_number);
    CPPUNIT_TEST(test_dds_for_suitability_fail_not_simple);

    CPPUNIT_TEST(test_build_new_dds_no_data);

    CPPUNIT_TEST(test_intern_all_data);
    CPPUNIT_TEST(test_intern_all_data_with_cont);

    CPPUNIT_TEST(test_build_new_dds_with_data);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(SequenceAggregationServerTest);

int main(int argc, char*argv[]) {
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "d");
    int option_char;
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
            test = string("SequenceAggregationServerTest::") + argv[i++];

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
