// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2021 OPeNDAP, Inc.
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

#include <memory>
#include <exception>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <libdap/debug.h>
#include <libdap/DMR.h>
#include <libdap/D4Group.h>
#include <libdap/Array.h>
#include <libdap/XMLWriter.h>

#include "url_impl.h"
#include "TheBESKeys.h"
#include "BESInternalError.h"
#include "BESDebug.h"

#include "DMZ.h"
#include "DmrppTypeFactory.h"

#include "read_test_baseline.h"
#include "test_config.h"

using namespace std;
using namespace libdap;
using namespace bes;
using namespace rapidxml;

static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)
#define prolog std::string("DMZTest::").append(__func__).append("() - ")

namespace dmrpp {

class DMZTest: public CppUnit::TestFixture {
private:
    DMZ *d_dmz;

    const string chunked_fourD_dmrpp = string(TEST_SRC_DIR).append("/input-files/chunked_fourD.h5.dmrpp");
    const string broken_dmrpp = string(TEST_SRC_DIR).append("/input-files/broken_elements.dmrpp");
    const string grid_2_2d_dmrpp = string(TEST_SRC_DIR).append("/input-files/grid_2_2d.h5.dmrpp");
    const string coads_climatology_dmrpp = string(TEST_SRC_DIR).append("/input-files/coads_climatology.dmrpp");
    const string test_array_6_1_dmrpp = string(TEST_SRC_DIR).append("/input-files/test_array_6.1.xml");
    const string test_simple_6_dmrpp = string(TEST_SRC_DIR).append("/input-files/test_simple_6.xml");

public:
    // Called once before everything gets tested
    DMZTest() : d_dmz(nullptr) { }

    // Called at the end of the test
    ~DMZTest() = default;

    // Called before each test
    void setUp()
    {
        TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");
        if (debug) cerr << endl;
        if (bes_debug) BESDebug::SetUp("cerr,dmz");
    }

    // Called after each test
    void tearDown()
    {
        delete d_dmz;
    }

    void test_DMZ_ctor_1() {
        d_dmz = new DMZ(chunked_fourD_dmrpp);
        CPPUNIT_ASSERT(d_dmz);
        DBG(cerr << "d_dmz->d_xml_text.size(): " << d_dmz->d_xml_text.size() << endl);
        CPPUNIT_ASSERT(d_dmz->d_xml_text.size() > 0);
        DBG(cerr << "d_dmz->d_xml_doc.first_node()->name(): " << d_dmz->d_xml_doc.first_node()->name() << endl);
        CPPUNIT_ASSERT(strcmp(d_dmz->d_xml_doc.first_node()->name(), "Dataset") == 0);
    }

    void test_DMZ_ctor_2() {
        try {
            d_dmz = new DMZ("no-such-file");    // Should return could not open
            CPPUNIT_FAIL("DMZ ctor should not succeed with bad path.");
        }
        catch (BESInternalError &e) {
            CPPUNIT_ASSERT("Caught BESInternalError with xml pathname fail");
        }
    }

    void test_DMZ_ctor_3() {
        try {
            d_dmz = new DMZ(string(TEST_SRC_DIR).append("/input-files/empty-file.txt"));    // Should return could not open
            CPPUNIT_FAIL("DMZ ctor should not succeed with empty file.");
        }
        catch (BESInternalError &e) {
            CPPUNIT_ASSERT("Caught BESInternalError with xml pathname fail");
        }
    }


    void test_DMZ_ctor_4() {
        try {
            d_dmz = new DMZ(""); // zero length
            CPPUNIT_FAIL("DMZ ctor should not succeed with an empty path");
        }
        catch (BESInternalError &e) {
            CPPUNIT_ASSERT("Caught BESInternalError with xml pathname fail");
        }
    }

    void test_process_dataset_1() {
        try {
            d_dmz = new DMZ(chunked_fourD_dmrpp);
            DMR dmr;
            d_dmz->process_dataset(&dmr, d_dmz->d_xml_doc.first_node());
            DBG(cerr << "dmr.dap_version(): " << dmr.dap_version() << endl);
            CPPUNIT_ASSERT(dmr.dap_version() == "4.0");
            CPPUNIT_ASSERT(dmr.dmr_version() == "1.0");
            CPPUNIT_ASSERT(dmr.get_namespace() == "http://xml.opendap.org/ns/DAP/4.0#");
            CPPUNIT_ASSERT(dmr.name() == "chunked_fourD.h5");
            DBG(cerr << "d_dmz->d_dataset_elem_href->str(): " << d_dmz->d_dataset_elem_href->str() << endl);
            CPPUNIT_ASSERT(d_dmz->d_dataset_elem_href->str() == string("file://") + TEST_DMRPP_CATALOG + "/data/dmrpp/chunked_fourD.h5");
        }
        catch (BESInternalError &e) {
            CPPUNIT_FAIL("Caught BESInternalError " + e.get_verbose_message());
        }
        catch (BESError &e) {
            CPPUNIT_FAIL("Caught BESError " + e.get_verbose_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL("Caught std::exception " + string(e.what()));
        }
        catch (...) {
            CPPUNIT_FAIL("Caught ? ");
        }
    }

    void test_process_dataset_2() {
        try {
            d_dmz = new DMZ(broken_dmrpp);
            DmrppTypeFactory factory;
            DMR dmr(&factory);
            d_dmz->process_dataset(&dmr, d_dmz->d_xml_doc.first_node());
            CPPUNIT_FAIL("DMZ ctor should fail when the Dataset element lacks a name attribute.");
        }
        catch (BESInternalError &e) {
            CPPUNIT_ASSERT("Caught BESInternalError with missing Dataset name attribute");
        }
    }

    void test_build_thin_dmr_1() {
        try {
            d_dmz = new DMZ(chunked_fourD_dmrpp);
            DmrppTypeFactory factory;
            DMR dmr(&factory);
            d_dmz->build_thin_dmr(&dmr);

            XMLWriter xml;
            dmr.print_dap4(xml);
            DBG(cerr << "DMR: " << xml.get_doc() << endl);

            // Look for one var, with four dims each size 40
            D4Group *root = dmr.root();
            CPPUNIT_ASSERT(root->var_end() - root->var_begin() == 1);
            BaseType *btp = *(root->var_begin());
            CPPUNIT_ASSERT(btp->type() == dods_array_c && btp->var()->type() == dods_float32_c);
            CPPUNIT_ASSERT(btp->name() == "d_16_chunks");
            Array *array = dynamic_cast<Array*>(btp);
            CPPUNIT_ASSERT(array->dim_end() - array->dim_begin() == 4);
            CPPUNIT_ASSERT(array->dimension_size(array->dim_begin()) == 40);
            CPPUNIT_ASSERT(array->dimension_size(array->dim_begin()+1) == 40);
            CPPUNIT_ASSERT(array->dimension_size(array->dim_begin()+2) == 40);
            CPPUNIT_ASSERT(array->dimension_size(array->dim_begin()+3) == 40);
        }
        catch (BESInternalError &e) {
            DBG(cerr << "BESInternalError: " << e.get_verbose_message() << endl);
            CPPUNIT_FAIL("build_thin_dmr should not throw");
        }
    }

    void test_build_thin_dmr_2() {
        try {
            d_dmz = new DMZ(grid_2_2d_dmrpp);
            DmrppTypeFactory factory;
            DMR dmr(&factory);
            d_dmz->build_thin_dmr(&dmr);

            XMLWriter xml;
            dmr.print_dap4(xml);
            DBG(cerr << "DMR: " << xml.get_doc() << endl);

            // Look for one var, with four dims each size 40
            D4Group *root = dmr.root();
            DBG(cerr << "vars:" << root->var_end() - root->var_begin() << endl);
            CPPUNIT_ASSERT(root->var_end() - root->var_begin() == 0);
            CPPUNIT_ASSERT(root->grp_end() - root->grp_begin() == 2);
            BaseType *btp = root->find_var("/HDFEOS/GRIDS/GeoGrid2/Data Fields/temperature");

            CPPUNIT_ASSERT(btp->type() == dods_array_c && btp->var()->type() == dods_float32_c);
            DBG(cerr << "btp->FQN(): " << btp->FQN() << endl);
            CPPUNIT_ASSERT(btp->FQN() == "/HDFEOS/GRIDS/GeoGrid2/Data Fields/temperature");

            Array *array = dynamic_cast<Array*>(btp);
            CPPUNIT_ASSERT(array->dim_end() - array->dim_begin() == 2);
            CPPUNIT_ASSERT(array->dimension_size(array->dim_begin()) == 4);
            CPPUNIT_ASSERT(array->dimension_size(array->dim_begin()+1) == 8);
        }
        catch (BESInternalError &e) {
            DBG(cerr << "BESInternalError: " << e.get_verbose_message() << endl);
            CPPUNIT_FAIL("build_thin_dmr should not throw");
        }
    }

    void test_build_thin_dmr_3() {
        try {
            d_dmz = new DMZ(coads_climatology_dmrpp);
            DmrppTypeFactory factory;
            DMR dmr(&factory);
            d_dmz->build_thin_dmr(&dmr);

            XMLWriter xml;
            dmr.print_dap4(xml);
            DBG(cerr << "DMR: " << xml.get_doc() << endl);

            // Look for one var, with four dims each size 40
            D4Group *root = dmr.root();
            DBG(cerr << "vars:" << root->var_end() - root->var_begin() << endl);
            CPPUNIT_ASSERT(root->var_end() - root->var_begin() == 7);

            BaseType *btp = root->find_var("/SST");
            CPPUNIT_ASSERT(btp->type() == dods_array_c && btp->var()->type() == dods_float32_c);
            DBG(cerr << "btp->FQN(): " << btp->FQN() << endl);
            CPPUNIT_ASSERT(btp->FQN() == "/SST");

            Array *array = dynamic_cast<Array*>(btp);
            CPPUNIT_ASSERT(array->dim_end() - array->dim_begin() == 3);
            CPPUNIT_ASSERT(array->dimension_size(array->dim_begin()) == 12);
            CPPUNIT_ASSERT(array->dimension_size(array->dim_begin()+1) == 90);
            CPPUNIT_ASSERT(array->dimension_size(array->dim_begin()+2) == 180);
        }
        catch (BESInternalError &e) {
            DBG(cerr << "BESInternalError: " << e.get_verbose_message() << endl);
            CPPUNIT_FAIL("build_thin_dmr should not throw");
        }
    }

    void test_build_thin_dmr_4() {
        try {
            d_dmz = new DMZ(test_simple_6_dmrpp);
            DmrppTypeFactory factory;
            DMR dmr(&factory);
            d_dmz->build_thin_dmr(&dmr);

            XMLWriter xml;
            dmr.print_dap4(xml);
            DBG(cerr << "DMR: " << xml.get_doc() << endl);

            // Look for one var, with four dims each size 40
            D4Group *root = dmr.root();
            DBG(cerr << "vars:" << root->var_end() - root->var_begin() << endl);
            CPPUNIT_ASSERT(root->var_end() - root->var_begin() == 1);

            BaseType *btp = root->find_var("/s");
            CPPUNIT_ASSERT(btp->type() == dods_structure_c);
            DBG(cerr << "btp->FQN(): " << btp->FQN() << endl);
            CPPUNIT_ASSERT(btp->FQN() == "/s");
            auto ctor = dynamic_cast<Constructor*>(btp);
            CPPUNIT_ASSERT(ctor);
            DBG(cerr << "struct vars:" << ctor->var_end() - ctor->var_begin() << endl);
            CPPUNIT_ASSERT(ctor->var_end() - ctor->var_begin() == 2);
        }
        catch (BESInternalError &e) {
            DBG(cerr << "BESInternalError: " << e.get_verbose_message() << endl);
            CPPUNIT_FAIL("build_thin_dmr should not throw");
        }
    }

    void test_build_thin_dmr_5() {
        try {
            d_dmz = new DMZ(test_array_6_1_dmrpp);
            DmrppTypeFactory factory;
            DMR dmr(&factory);
            d_dmz->build_thin_dmr(&dmr);

            XMLWriter xml;
            dmr.print_dap4(xml);
            DBG(cerr << "DMR: " << xml.get_doc() << endl);

            // Look for one var, with four dims each size 40
            D4Group *root = dmr.root();
            DBG(cerr << "vars:" << root->var_end() - root->var_begin() << endl);
            CPPUNIT_ASSERT(root->var_end() - root->var_begin() == 1);

            BaseType *btp = root->find_var("/a");
            CPPUNIT_ASSERT(btp);
            CPPUNIT_ASSERT(btp->type() == dods_array_c && btp->var()->type() == dods_structure_c);
            DBG(cerr << "btp->FQN(): " << btp->FQN() << endl);
            CPPUNIT_ASSERT(btp->FQN() == "/a");

            Array *array = dynamic_cast<Array*>(btp);
            CPPUNIT_ASSERT(array->dim_end() - array->dim_begin() == 2);
            CPPUNIT_ASSERT(array->dimension_size(array->dim_begin()) == 3);
            CPPUNIT_ASSERT(array->dimension_size(array->dim_begin()+1) == 4);

            btp = root->find_var("/a.i");
            CPPUNIT_ASSERT(btp);
            CPPUNIT_ASSERT(btp->type() == dods_array_c && btp->var()->type() == dods_int32_c);
            DBG(cerr << "btp->FQN(): " << btp->FQN() << endl);
            CPPUNIT_ASSERT(btp->FQN() == "/a.i");

            array = dynamic_cast<Array*>(btp);
            CPPUNIT_ASSERT(array->dim_end() - array->dim_begin() == 2);
            CPPUNIT_ASSERT(array->dimension_size(array->dim_begin()) == 3);
            CPPUNIT_ASSERT(array->dimension_size(array->dim_begin()+1) == 6);
        }
        catch (BESInternalError &e) {
            CPPUNIT_FAIL("Caught BESInternalError: " + e.get_verbose_message());
        }
        catch (BESError &e) {
            CPPUNIT_FAIL("Caught BESError: " + e.get_verbose_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL("Caught std::exception: " + string(e.what()));
        }
        catch (...) {
            CPPUNIT_FAIL("Caught ... WTF");
        }
    }

#if 0
    void test_build_xml_path_to_variable_1() {
        d_dmz = new DMZ(chunked_fourD_dmrpp);
        DmrppTypeFactory factory;
        DMR dmr(&factory);
        d_dmz->build_thin_dmr(&dmr);
        BaseType *btp = *(dmr.root()->var_begin());
        string xml_path = d_dmz->build_xml_path_to_variable(btp);
        DBG(cerr << "build_xml_path_to_variable(): " << xml_path << endl);
    }
#endif

    void test_get_variable_xml_node_1() {
        d_dmz = new DMZ(chunked_fourD_dmrpp);
        DmrppTypeFactory factory;
        DMR dmr(&factory);
        d_dmz->build_thin_dmr(&dmr);
        BaseType *btp = *(dmr.root()->var_begin());
        xml_node<> *node = d_dmz->get_variable_xml_node(btp);
        CPPUNIT_ASSERT(node);
        DBG(cerr << "get_variable_xml_node(): " << node->name() << ", " << node->first_attribute()->value() << endl);
        CPPUNIT_ASSERT(btp->type() == dods_array_c && node->name() == btp->var()->type_name());
        CPPUNIT_ASSERT(node->first_attribute()->value() == btp->name());
    }

    void test_get_variable_xml_node_2() {
        d_dmz = new DMZ(grid_2_2d_dmrpp);
        DmrppTypeFactory factory;
        DMR dmr(&factory);
        d_dmz->build_thin_dmr(&dmr);

        BaseType *btp = dmr.root()->find_var("/HDFEOS/GRIDS/GeoGrid2/Data Fields/temperature");
        xml_node<> *node = d_dmz->get_variable_xml_node(btp);
        CPPUNIT_ASSERT(node);
        DBG(cerr << "get_variable_xml_node(): " << node->name() << ", " << node->first_attribute()->value() << endl);
        CPPUNIT_ASSERT(btp->type() == dods_array_c && node->name() == btp->var()->type_name());
        CPPUNIT_ASSERT(node->first_attribute()->value() == btp->name());
    }

    void test_get_variable_xml_node_3() {
        d_dmz = new DMZ(coads_climatology_dmrpp);
        DmrppTypeFactory factory;
        DMR dmr(&factory);
        d_dmz->build_thin_dmr(&dmr);

        BaseType *btp = dmr.root()->find_var("/TIME");
        xml_node<> *node = d_dmz->get_variable_xml_node(btp);
        CPPUNIT_ASSERT(node);
        DBG(cerr << "get_variable_xml_node(): " << node->name() << ", " << node->first_attribute()->value() << endl);
        CPPUNIT_ASSERT(btp->type() == dods_array_c && node->name() == btp->var()->type_name());
        CPPUNIT_ASSERT(node->first_attribute()->value() == btp->name());
    }

    void test_get_variable_xml_node_4() {
        d_dmz = new DMZ(test_array_6_1_dmrpp);
        DmrppTypeFactory factory;
        DMR dmr(&factory);
        d_dmz->build_thin_dmr(&dmr);

        BaseType *btp = dmr.root()->find_var("/a.j");
        CPPUNIT_ASSERT(btp);
        xml_node<> *node = d_dmz->get_variable_xml_node(btp);
        CPPUNIT_ASSERT(node);
        DBG(cerr << "get_variable_xml_node(): " << node->name() << ", " << node->first_attribute()->value() << endl);
        CPPUNIT_ASSERT(btp->type() == dods_array_c && node->name() == btp->var()->type_name());
        CPPUNIT_ASSERT(node->first_attribute()->value() == btp->name());
    }
    void test_load_attributes_1() {
        try {
            d_dmz = new DMZ(chunked_fourD_dmrpp);
            DmrppTypeFactory factory;
            DMR dmr(&factory);
            d_dmz->build_thin_dmr(&dmr);

            XMLWriter xml;
            dmr.print_dap4(xml);
            DBG(cerr << "DMR: " << xml.get_doc() << endl);

            // Given a thin DMR, load in the attributes of the first variable
            BaseType *btp = *(dmr.root()->var_begin());
            d_dmz->load_attributes(btp);
            dmr.print_dap4(xml);
            DBG(cerr << "DMR with attributes: " << xml.get_doc() << endl);
        }
        catch (BESInternalError &e) {
            CPPUNIT_FAIL("Caught BESInternalError " + e.get_verbose_message());
        }
        catch (BESError &e) {
            CPPUNIT_FAIL("Caught BESError " + e.get_verbose_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL("Caught std::exception " + string(e.what()));
        }
        catch (...) {
            CPPUNIT_FAIL("Caught ? ");
        }
    }

    CPPUNIT_TEST_SUITE( DMZTest );

    CPPUNIT_TEST(test_DMZ_ctor_1);
    CPPUNIT_TEST(test_DMZ_ctor_2);
    CPPUNIT_TEST(test_DMZ_ctor_3);
    CPPUNIT_TEST(test_DMZ_ctor_4);

    CPPUNIT_TEST(test_process_dataset_1);
    CPPUNIT_TEST(test_process_dataset_2);

    CPPUNIT_TEST(test_build_thin_dmr_1);
    CPPUNIT_TEST(test_build_thin_dmr_2);
    CPPUNIT_TEST(test_build_thin_dmr_3);
    CPPUNIT_TEST(test_build_thin_dmr_4);
    CPPUNIT_TEST(test_build_thin_dmr_5);

    CPPUNIT_TEST(test_get_variable_xml_node_1);
    CPPUNIT_TEST(test_get_variable_xml_node_2);
    CPPUNIT_TEST(test_get_variable_xml_node_3);
    CPPUNIT_TEST_FAIL(test_get_variable_xml_node_4);

    CPPUNIT_TEST(test_load_attributes_1);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DMZTest);

} // namespace dmrpp

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    int option_char;
    while ((option_char = getopt(argc, argv, "dD")) != -1)
        switch (option_char) {
            case 'd':
                debug = true;  // debug is a static global
                break;
            case 'D':
                debug = true;  // debug is a static global
                bes_debug = true;  // debug is a static global
                break;
            default:
                break;
        }

    argc -= optind;
    argv += optind;

    bool wasSuccessful = true;
    string test = "";
    if (0 == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        int i = 0;
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = dmrpp::DMZTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
