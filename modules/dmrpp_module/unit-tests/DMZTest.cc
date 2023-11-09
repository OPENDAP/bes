// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of bes, A C++ implementation of the OPeNDAP Data
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
#include <cstring>
#include <algorithm>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <libdap/debug.h>
#include <libdap/DMR.h>
#include <libdap/D4Group.h>
#include <libdap/D4Attributes.h>
#include <libdap/D4Dimensions.h>
#include <libdap/Array.h>
#include <libdap/Byte.h>
#include <libdap/XMLWriter.h>

#include "url_impl.h"
#include "TheBESKeys.h"
#include "BESUtil.h"
#include "BESInternalError.h"

#include "DMZ.h"
#include "Chunk.h"
#include "DmrppCommon.h"
#include "DmrppArray.h"
#include "DmrppTypeFactory.h"

#define PUGIXML_HEADER_ONLY
#include <pugixml.hpp>

#include "modules/common/run_tests_cppunit.h"
#include "read_test_baseline.h"
#include "test_config.h"

using namespace std;
using namespace libdap;
using namespace bes;

#define prolog std::string("DMZTest::").append(__func__).append("() - ")
constexpr auto bes_debug_args="cerr,dmrpp:dmz";

namespace dmrpp {

class DMZTest: public CppUnit::TestFixture {
private:
    unique_ptr<DMZ> d_dmz {nullptr};

    const string chunked_fourD_dmrpp = string(TEST_SRC_DIR).append("/input-files/chunked_fourD.h5.dmrpp");
    const string chunked_oneD_dmrpp = string(TEST_SRC_DIR).append("/input-files/chunked_oneD.h5.dmrpp");
    const string broken_dmrpp = string(TEST_SRC_DIR).append("/input-files/broken_elements.dmrpp");
    const string grid_2_2d_dmrpp = string(TEST_SRC_DIR).append("/input-files/grid_2_2d.h5.dmrpp");
    const string coads_climatology_dmrpp = string(TEST_SRC_DIR).append("/input-files/coads_climatology.dmrpp");
    const string test_array_6_1_dmrpp = string(TEST_SRC_DIR).append("/input-files/test_array_6.1.xml");
    const string test_simple_6_dmrpp = string(TEST_SRC_DIR).append("/input-files/test_simple_6.xml");
    const string vlsa_element_values_dmrpp = string(TEST_SRC_DIR).append("/input-files/vlsa_element_values.dmrpp");
    const string vlsa_base64_values_dmrpp = string(TEST_SRC_DIR).append("/input-files/vlsa_base64_values.dmrpp");

public:
    // Called once before everything gets tested
    DMZTest() = default;

    // Called at the end of the test
    ~DMZTest() override = default;

    // Called before each test
    void setUp() override
    {
        DBG( cerr << "\n" );
        TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");
    }

    // This is a 'function try block' and provides a way to abstract the many
    // exception types. jhrg 10/28/21
    static void handle_fatal_exceptions() try {
        throw;
    }
    catch (const BESInternalError &e) {
        CPPUNIT_FAIL("Caught BESInternalError " + e.get_verbose_message());
    }
    catch (const BESError &e) {
        CPPUNIT_FAIL("Caught BESError " + e.get_verbose_message());
    }
    catch (const std::exception &e) {
        CPPUNIT_FAIL("Caught std::exception " + string(e.what()));
    }
    catch (...) {
        CPPUNIT_FAIL("Caught ? ");
    }

    void test_DMZ_ctor_1() {
        d_dmz.reset(new DMZ(chunked_fourD_dmrpp));
        CPPUNIT_ASSERT(d_dmz);
        DBG(cerr << "d_dmz->d_xml_doc.document_element().name(): " << d_dmz->d_xml_doc.document_element().name() << endl);
        CPPUNIT_ASSERT(strcmp(d_dmz->d_xml_doc.document_element().name(), "Dataset") == 0);
    }

    void test_DMZ_ctor_2() {
        try {
            d_dmz.reset(new DMZ("no-such-file"));    // Should return could not open
            CPPUNIT_FAIL("DMZ ctor should not succeed with bad path.");
        }
        catch (const BESInternalError &e) {
            CPPUNIT_ASSERT("Caught BESInternalError with xml pathname fail");
        }
    }

    void test_DMZ_ctor_3() {
        try {
            d_dmz.reset(new DMZ(string(TEST_SRC_DIR).append("/input-files/empty-file.txt")));    // Should return could not open
            CPPUNIT_FAIL("DMZ ctor should not succeed with empty file.");
        }
        catch (const BESInternalError &e) {
            CPPUNIT_ASSERT("Caught BESInternalError with xml pathname fail");
        }
    }
    
    void test_DMZ_ctor_4() {
        try {
            d_dmz.reset(new DMZ("")); // zero length
            CPPUNIT_FAIL("DMZ ctor should not succeed with an empty path");
        }
        catch (const BESInternalError &e) {
            CPPUNIT_ASSERT("Caught BESInternalError with xml pathname fail");
        }
    }

    void test_parse_xml_string() {
        d_dmz = make_unique<DMZ>();
        CPPUNIT_ASSERT(d_dmz);
        const string chunked_fourD_dmrpp_string = BESUtil::file_to_string(chunked_fourD_dmrpp);
        d_dmz->parse_xml_string(chunked_fourD_dmrpp_string);
        DBG(cerr << "d_dmz->d_xml_doc.document_element().name(): " << d_dmz->d_xml_doc.document_element().name() << endl);
        CPPUNIT_ASSERT(strcmp(d_dmz->d_xml_doc.document_element().name(), "Dataset") == 0);
    }

    void test_parse_xml_string_bad_input() {
        try {
            d_dmz = make_unique<DMZ>();
            CPPUNIT_ASSERT(d_dmz);
            const string bad_input = "<?xml version='1.0' encoding='UTF-8'?> BARF";
            d_dmz->parse_xml_string(bad_input);
            CPPUNIT_FAIL("DMZ ctor should not succeed with bad string input");
        }
        catch (const BESInternalError &e) {
            CPPUNIT_ASSERT("Caught BESInternalError with bad string input");
        }
    }


// This shows how to test the processing of different XML elements w/o first
    // parsing a whole DMR++. jhrg 5/2/22
    // TODO Adopt this pattern throughout this file? jhrg 5/2/22
    void test_process_dataset_0() {
        try {
            pugi::xml_document doc;
            string source {R"(<Dataset xmlns="http://xml.opendap.org/ns/DAP/4.0#"
            xmlns:dmrpp="http://xml.opendap.org/dap/dmrpp/1.0.0#" dapVersion="4.0"
            dmrVersion="1.0" name="chunked_fourD.h5" dmrpp:href="data/dmrpp/chunked_fourD.h5"/>)"};
            pugi::xml_parse_result result = doc.load_string(source.c_str());
            if (!result) {
                DBG(cerr << "XML [" << source << "] parsed with errors" << endl);
                DBG(cerr << "Error description: " << result.description() << endl);
                CPPUNIT_FAIL("Could not parse the XML String");
            }

            d_dmz.reset(new DMZ());
            DMR dmr;
            d_dmz->process_dataset(&dmr, doc.first_child());

            DBG(cerr << "dmr.dap_version(): " << dmr.dap_version() << endl);
            CPPUNIT_ASSERT(dmr.dap_version() == "4.0");
            CPPUNIT_ASSERT(dmr.dmr_version() == "1.0");
            CPPUNIT_ASSERT(dmr.get_namespace() == "http://xml.opendap.org/ns/DAP/4.0#");
            CPPUNIT_ASSERT(dmr.name() == "chunked_fourD.h5");
            DBG(cerr << "d_dmz->d_dataset_elem_href->str(): " << d_dmz->d_dataset_elem_href->str() << endl);
            CPPUNIT_ASSERT(d_dmz->d_dataset_elem_href->str() == string("file://") + TEST_DMRPP_CATALOG + "/data/dmrpp/chunked_fourD.h5");
        }
        catch (...) {
            handle_fatal_exceptions();
        }
    }
    void test_process_dataset_1() {
        try {
            d_dmz.reset(new DMZ(chunked_fourD_dmrpp));
            DMR dmr;
            d_dmz->process_dataset(&dmr, d_dmz->d_xml_doc.first_child());
            DBG(cerr << "dmr.dap_version(): " << dmr.dap_version() << endl);
            CPPUNIT_ASSERT(dmr.dap_version() == "4.0");
            CPPUNIT_ASSERT(dmr.dmr_version() == "1.0");
            CPPUNIT_ASSERT(dmr.get_namespace() == "http://xml.opendap.org/ns/DAP/4.0#");
            CPPUNIT_ASSERT(dmr.name() == "chunked_fourD.h5");
            DBG(cerr << "d_dmz->d_dataset_elem_href->str(): " << d_dmz->d_dataset_elem_href->str() << endl);
            CPPUNIT_ASSERT(d_dmz->d_dataset_elem_href->str() == string("file://") + TEST_DMRPP_CATALOG + "/data/dmrpp/chunked_fourD.h5");
        }
        catch (...) {
            handle_fatal_exceptions();
        }
    }

    void test_process_dataset_2() {
        try {
            d_dmz.reset(new DMZ(broken_dmrpp));
            DmrppTypeFactory factory;
            DMR dmr(&factory);
            d_dmz->process_dataset(&dmr, d_dmz->d_xml_doc.first_child());
            CPPUNIT_FAIL("DMZ ctor should fail when the Dataset element lacks a name attribute.");
        }
        catch (const BESInternalError &e) {
            CPPUNIT_ASSERT("Caught BESInternalError with missing Dataset name attribute");
        }
    }

    void test_build_thin_dmr_1() {
        try {
            d_dmz.reset(new DMZ(chunked_fourD_dmrpp));
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
        catch (...) {
            handle_fatal_exceptions();
        }
    }

    void test_build_thin_dmr_2() {
        try {
            d_dmz.reset(new DMZ(grid_2_2d_dmrpp));
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
        catch (...) {
            handle_fatal_exceptions();
        }
    }

    void test_build_thin_dmr_3() {
        try {
            d_dmz.reset(new DMZ(coads_climatology_dmrpp));
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
        catch (...) {
            handle_fatal_exceptions();
        }
    }

    void test_build_thin_dmr_4() {
        try {
            d_dmz.reset(new DMZ(test_simple_6_dmrpp));
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
        catch (...) {
            handle_fatal_exceptions();
        }
    }

    void test_build_thin_dmr_5() {
        try {
            d_dmz.reset(new DMZ(test_array_6_1_dmrpp));
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
        catch (...) {
            handle_fatal_exceptions();
        }
    }

    void test_build_basetype_chain_1() {
        d_dmz.reset(new DMZ(chunked_fourD_dmrpp));
        DmrppTypeFactory factory;
        DMR dmr(&factory);
        d_dmz->build_thin_dmr(&dmr);
        BaseType *btp = dmr.root()->find_var("/d_16_chunks");

        stack<BaseType*> bt;
        d_dmz->build_basetype_chain(btp, bt);
        CPPUNIT_ASSERT(bt.size() == 1);
    }

    void test_build_basetype_chain_2() {
        d_dmz.reset(new DMZ(grid_2_2d_dmrpp));
        DmrppTypeFactory factory;
        DMR dmr(&factory);
        d_dmz->build_thin_dmr(&dmr);
        BaseType *btp = dmr.root()->find_var("/HDFEOS/GRIDS/GeoGrid2/Data Fields/temperature");

        stack<BaseType*> bt;
        d_dmz->build_basetype_chain(btp, bt);
        CPPUNIT_ASSERT(bt.size() == 5);

        DBG(cerr << "bt.top()->name(): " << bt.top()->name() << endl);
        CPPUNIT_ASSERT(bt.top()->name() == "HDFEOS");
        bt.pop(); bt.pop(); bt.pop(); bt.pop();
        DBG(cerr << "four pop() operations, then bt.top()->name(): " << bt.top()->name() << endl);
        CPPUNIT_ASSERT(bt.top()->name() == "temperature");
    }

    void test_build_basetype_chain_3() {
        d_dmz.reset(new DMZ(test_array_6_1_dmrpp));
        DmrppTypeFactory factory;
        DMR dmr(&factory);
        d_dmz->build_thin_dmr(&dmr);

        BaseType *btp = dmr.root()->find_var("/a.j");
        CPPUNIT_ASSERT(btp);

        stack<BaseType*> bt;
        d_dmz->build_basetype_chain(btp, bt);
        CPPUNIT_ASSERT(bt.size() == 3);

        DBG(cerr << "bt.top()->name(): " << bt.top()->name() << endl);
        CPPUNIT_ASSERT(bt.top()->name() == "a");
        bt.pop(); bt.pop();
        DBG(cerr << "two pop() operations, then bt.top()->name(): " << bt.top()->name() << endl);
        CPPUNIT_ASSERT(bt.top()->name() == "j");
    }

    void test_get_variable_xml_node_1() {
        d_dmz.reset(new DMZ(chunked_fourD_dmrpp));
        DmrppTypeFactory factory;
        DMR dmr(&factory);
        d_dmz->build_thin_dmr(&dmr);
        BaseType *btp = *(dmr.root()->var_begin());
        CPPUNIT_ASSERT(btp);
        pugi::xml_node node = d_dmz->get_variable_xml_node(btp);
        CPPUNIT_ASSERT(node);
        DBG(cerr << "get_variable_xml_node(): " << node.name() << ", " << node.first_attribute().value() << endl);
        CPPUNIT_ASSERT(btp->type() == dods_array_c && node.name() == btp->var()->type_name());
        CPPUNIT_ASSERT(node.first_attribute().value() == btp->name());
    }

    void test_get_variable_xml_node_2() {
        d_dmz.reset(new DMZ(grid_2_2d_dmrpp));
        DmrppTypeFactory factory;
        DMR dmr(&factory);
        d_dmz->build_thin_dmr(&dmr);

        BaseType *btp = dmr.root()->find_var("/HDFEOS/GRIDS/GeoGrid2/Data Fields/temperature");
        pugi::xml_node node = d_dmz->get_variable_xml_node(btp);
        CPPUNIT_ASSERT(node);
        DBG(cerr << "get_variable_xml_node(): " << node.name() << ", " << node.first_attribute().value() << endl);
        CPPUNIT_ASSERT(btp->type() == dods_array_c && node.name() == btp->var()->type_name());
        CPPUNIT_ASSERT(node.first_attribute().value() == btp->name());
    }

    void test_get_variable_xml_node_3() {
        d_dmz.reset(new DMZ(coads_climatology_dmrpp));
        DmrppTypeFactory factory;
        DMR dmr(&factory);
        d_dmz->build_thin_dmr(&dmr);

        BaseType *btp = dmr.root()->find_var("/TIME");
        pugi::xml_node node = d_dmz->get_variable_xml_node(btp);
        CPPUNIT_ASSERT(node);
        DBG(cerr << "get_variable_xml_node(): " << node.name() << ", " << node.first_attribute().value() << endl);
        CPPUNIT_ASSERT(btp->type() == dods_array_c && node.name() == btp->var()->type_name());
        CPPUNIT_ASSERT(node.first_attribute().value() == btp->name());
    }

    void test_get_variable_xml_node_4() {
        d_dmz.reset(new DMZ(test_array_6_1_dmrpp));
        DmrppTypeFactory factory;
        DMR dmr(&factory);
        d_dmz->build_thin_dmr(&dmr);

        BaseType *btp = dmr.root()->find_var("/a.j");
        CPPUNIT_ASSERT(btp);
        pugi::xml_node node = d_dmz->get_variable_xml_node(btp);
        CPPUNIT_ASSERT(node);
        DBG(cerr << "get_variable_xml_node(): " << node.name() << ", " << node.first_attribute().value() << endl);
        CPPUNIT_ASSERT(btp->type() == dods_array_c && node.name() == btp->var()->type_name());
        CPPUNIT_ASSERT(node.first_attribute().value() == btp->name());
    }

    void test_load_attributes_1() {
        try {
            d_dmz.reset(new DMZ(chunked_fourD_dmrpp));
            DmrppTypeFactory factory;
            DMR dmr(&factory);
            d_dmz->build_thin_dmr(&dmr);

            XMLWriter xml;
            dmr.print_dap4(xml);
            DBG(cerr << "DMR: " << xml.get_doc() << endl);

            // Given a thin DMR, load in the attributes of the first variable
            BaseType *btp = *(dmr.root()->var_begin());
            CPPUNIT_ASSERT(btp->attributes()->empty());

            d_dmz->load_attributes(btp);
            XMLWriter xml2;
            dmr.print_dap4(xml2);
            DBG(cerr << "DMR with attributes: " << xml2.get_doc() << endl);

            CPPUNIT_ASSERT(!btp->attributes()->empty());
            CPPUNIT_ASSERT(btp->attributes()->attribute_end() - btp->attributes()->attribute_begin() == 2);
        }
        catch (...) {
            handle_fatal_exceptions();
        }
    }

    void test_load_attributes_2() {
        try {
            d_dmz.reset(new DMZ(grid_2_2d_dmrpp));
            DmrppTypeFactory factory;
            DMR dmr(&factory);
            d_dmz->build_thin_dmr(&dmr);

            XMLWriter xml;
            dmr.print_dap4(xml);
            DBG(cerr << "DMR: " << xml.get_doc() << endl);

            // Given a thin DMR, load in the attributes of the first variable
            BaseType *btp = dmr.root()->find_var("/HDFEOS/GRIDS/GeoGrid2/Data Fields/temperature");
            CPPUNIT_ASSERT(btp->attributes()->empty());

            d_dmz->load_attributes(btp);
            XMLWriter xml2;
            dmr.print_dap4(xml2);
            DBG(cerr << "DMR with attributes: " << xml2.get_doc() << endl);

            CPPUNIT_ASSERT(!btp->attributes()->empty());
            CPPUNIT_ASSERT(btp->attributes()->attribute_end() - btp->attributes()->attribute_begin() == 1);
        }
        catch (...) {
            handle_fatal_exceptions();
        }
    }

    void test_load_attributes_3() {
        try {
            d_dmz.reset(new DMZ(test_array_6_1_dmrpp));
            DmrppTypeFactory factory;
            DMR dmr(&factory);
            d_dmz->build_thin_dmr(&dmr);

            XMLWriter xml;
            dmr.print_dap4(xml);
            DBG(cerr << "DMR: " << xml.get_doc() << endl);

            // Given a thin DMR, load in the attributes of the first variable
            BaseType *btp = dmr.root()->find_var("/a.j");
            CPPUNIT_ASSERT(btp->attributes()->empty());

            d_dmz->load_attributes(btp);
            XMLWriter xml2;
            dmr.print_dap4(xml2);
            DBG(cerr << "DMR with attributes: " << xml2.get_doc() << endl);

            CPPUNIT_ASSERT(!btp->attributes()->empty());
            CPPUNIT_ASSERT(btp->attributes()->attribute_end() - btp->attributes()->attribute_begin() == 1);
        }
        catch (...) {
            handle_fatal_exceptions();
        }
    }

    void test_process_cds_node_1() {
        try {
            d_dmz.reset(new DMZ(chunked_fourD_dmrpp));
            DmrppTypeFactory factory;
            DMR dmr(&factory);
            d_dmz->build_thin_dmr(&dmr);

            XMLWriter xml;
            dmr.print_dap4(xml);
            DBG(cerr << "DMR: " << xml.get_doc() << endl);

            // Given a thin DMR, load in the attributes of the first variable
            BaseType *btp = *(dmr.root()->var_begin());
            auto *dc = dynamic_cast<DmrppCommon *>(btp);
            CPPUNIT_ASSERT(dc);

            // goto the DOM tree node for this variable
            pugi::xml_node var_node = d_dmz->get_variable_xml_node(btp);
            if (var_node == nullptr)
                throw BESInternalError("Could not find location of variable in the DMR++ XML document.", __FILE__,
                                       __LINE__);

            // Chunks for this node will be held in the var_node siblings.
            int chunks_nodes = 0;
            for (auto child = var_node.child("dmrpp:chunks"); child; child = child.next_sibling()) {
                ++chunks_nodes;
                dmrpp::DMZ::process_cds_node(dc, child);
            }

            CPPUNIT_ASSERT(chunks_nodes == 1);
            vector<unsigned long long> c_sizes = dc->get_chunk_dimension_sizes();
            CPPUNIT_ASSERT(c_sizes.size() == 4);
            CPPUNIT_ASSERT(c_sizes.at(0) == 20);
            CPPUNIT_ASSERT(c_sizes.at(1) == 20);
            CPPUNIT_ASSERT(c_sizes.at(2) == 20);
            CPPUNIT_ASSERT(c_sizes.at(3) == 20);
        }
        catch (...) {
            handle_fatal_exceptions();
        }
    }

    void test_get_array_dims_1() {
        unique_ptr<libdap::Array> a(new Array("a", new libdap::Byte("a")));
        a->append_dim(10);
        a->append_dim(20);

        auto const array_dim_sizes = DMZ::get_array_dims(a.get());
        CPPUNIT_ASSERT_MESSAGE("Should have two dimensions", array_dim_sizes.size() == 2);
        CPPUNIT_ASSERT_MESSAGE("Should be 10", array_dim_sizes.at(0) == 10);
        CPPUNIT_ASSERT_MESSAGE("Should be 20", array_dim_sizes.at(1) == 20);
    }

    void test_get_array_dims_2() {
        unique_ptr<libdap::Array> a(new Array("a", new libdap::Byte("a")));
        a->append_dim(new libdap::D4Dimension("a1", 10));
        a->append_dim(new libdap::D4Dimension("a2", 20));

        auto const array_dim_sizes = DMZ::get_array_dims(a.get());
        CPPUNIT_ASSERT_MESSAGE("Should have two dimensions", array_dim_sizes.size() == 2);
        CPPUNIT_ASSERT_MESSAGE("Should be 10", array_dim_sizes.at(0) == 10);
        CPPUNIT_ASSERT_MESSAGE("Should be 20", array_dim_sizes.at(1) == 20);
    }

    void test_get_array_dims_3() {
        unique_ptr<libdap::Array> a(new Array("a", new libdap::Byte("a")));
        a->append_dim(10);

        auto const array_dim_sizes = DMZ::get_array_dims(a.get());
        CPPUNIT_ASSERT_MESSAGE("Should have two dimensions", array_dim_sizes.size() == 1);
        CPPUNIT_ASSERT_MESSAGE("Should be 10", array_dim_sizes.at(0) == 10);
    }

    void test_get_array_dims_4() {
        unique_ptr<libdap::Array> a(new Array("a", new libdap::Byte("a")));

        auto const array_dim_sizes = DMZ::get_array_dims(a.get());
        CPPUNIT_ASSERT_MESSAGE("Should have two dimensions", array_dim_sizes.size() == 0);
    }

    void test_logical_chunks_1() {
        unique_ptr<DmrppCommon> dc(new DmrppCommon);

        vector<unsigned long long> cds_values = { 20, 20, 20, 20};
        dc->set_chunk_dimension_sizes(cds_values);

        vector<unsigned long long> array_dim_sizes;
        array_dim_sizes.push_back(40);
        array_dim_sizes.push_back(40);
        array_dim_sizes.push_back(40);
        array_dim_sizes.push_back(40);

        size_t num_logical_chunks = DMZ::logical_chunks(array_dim_sizes, dc.get());

        CPPUNIT_ASSERT(num_logical_chunks == 16);
    }

    void test_logical_chunks_2() {
        unique_ptr<DmrppCommon> dc(new DmrppCommon);

        vector<unsigned long long> cds_values = { 1000 };
        dc->set_chunk_dimension_sizes(cds_values);

        vector<unsigned long long> array_dim_sizes;
        array_dim_sizes.push_back(4000);

        size_t num_logical_chunks = DMZ::logical_chunks(array_dim_sizes, dc.get());

        CPPUNIT_ASSERT(num_logical_chunks == 4);
    }

    void test_logical_chunks_3() {
        unique_ptr<DmrppCommon> dc(new DmrppCommon);

        vector<unsigned long long> cds_values = { 2, 3, 4 };
        dc->set_chunk_dimension_sizes(cds_values);

        vector<unsigned long long> array_dim_sizes;
        array_dim_sizes.push_back(4);
        array_dim_sizes.push_back(6);
        array_dim_sizes.push_back(8);

        size_t num_logical_chunks = DMZ::logical_chunks(array_dim_sizes, dc.get());

        CPPUNIT_ASSERT(num_logical_chunks == 8);
    }

    void test_logical_chunks_4() {
        unique_ptr<DmrppCommon> dc(new DmrppCommon);

        vector<unsigned long long> cds_values = { 41, 50, 53 };
        dc->set_chunk_dimension_sizes(cds_values);

        vector<unsigned long long> array_dim_sizes;
        array_dim_sizes.push_back(100);
        array_dim_sizes.push_back(50);
        array_dim_sizes.push_back(200);

        size_t num_logical_chunks = DMZ::logical_chunks(array_dim_sizes, dc.get());

        CPPUNIT_ASSERT(num_logical_chunks == 12);
    }

    /// Helper to build Chunks for test_get_chunk_map tests
    shared_ptr<Chunk> get_another_chunk(unsigned long long i1, unsigned long long i2) {
        shared_ptr<Chunk> c1(new Chunk);
        vector<unsigned long long> c1_pia{i1, i2};
        c1->set_position_in_array(c1_pia);
        return c1;
    }

    // set< vector<unsigned long long> > DMZ::get_chunk_map(const vector<shared_ptr<Chunk>> &chunks)
    void test_get_chunk_map_1() {
        vector<shared_ptr<Chunk>> vc;
        vc.push_back(get_another_chunk(0, 0));

        set< vector<unsigned long long> > chunks = DMZ::get_chunk_map(vc);

        CPPUNIT_ASSERT(!chunks.empty());
        CPPUNIT_ASSERT(chunks.size() == 1);
        vector<unsigned long long> c1_pia{0, 0};
        CPPUNIT_ASSERT(chunks.find(c1_pia) != chunks.end());
    }

    void test_get_chunk_map_2() {
        vector<shared_ptr<Chunk>> vc;
        vc.push_back(get_another_chunk(0, 0));
        vc.push_back(get_another_chunk(0, 2));
        vc.push_back(get_another_chunk(2, 2));

        set< vector<unsigned long long> > chunks = DMZ::get_chunk_map(vc);
        CPPUNIT_ASSERT(!chunks.empty());
        CPPUNIT_ASSERT(chunks.size() == 3);

        vector<unsigned long long> pia1{2, 2};
        CPPUNIT_ASSERT(chunks.find(pia1) != chunks.end());
        vector<unsigned long long> pia2{2, 0};
        CPPUNIT_ASSERT(chunks.find(pia2) == chunks.end());
    }

    // Test the case when there are no chunks in the DMR++ - the array is all fill values
    void test_process_fill_value_chunks_all_fill() {
        unique_ptr<DmrppCommon> dc(new DmrppCommon);
        // process_fill_value_chunks() uses these values and calls DmrppCommon::add_chunk()
        dc->d_fill_value_str = "17";
        dc->d_byte_order = "LE";

        set<shape> cm;
        shape cs{10000};
        shape as{40000};
        DMZ::process_fill_value_chunks(dc.get(), cm, cs, as, 10000);
        CPPUNIT_ASSERT_MESSAGE("There should be four chunks", dc->get_immutable_chunks().size() == 4);
        DBG(for_each(dc->get_immutable_chunks().begin(), dc->get_immutable_chunks().end(),
                 [](const shared_ptr<Chunk> c) { cerr << c->get_fill_value() << " "; }));

        vector<shared_ptr<Chunk>> vspc = dc->get_immutable_chunks();
        CPPUNIT_ASSERT(vspc.at(0)->get_size() == 10000);
        CPPUNIT_ASSERT(vspc.at(0)->get_uses_fill_value() == true);
        CPPUNIT_ASSERT(vspc.at(0)->get_position_in_array().at(0) == 0);

        CPPUNIT_ASSERT(vspc.at(1)->get_uses_fill_value() == true);
        CPPUNIT_ASSERT(vspc.at(1)->get_position_in_array().at(0) == 10000);

        CPPUNIT_ASSERT(vspc.at(2)->get_uses_fill_value() == true);
        CPPUNIT_ASSERT(vspc.at(2)->get_position_in_array().at(0) == 20000);

        CPPUNIT_ASSERT(vspc.at(3)->get_uses_fill_value() == true);
        CPPUNIT_ASSERT(vspc.at(3)->get_position_in_array().at(0) == 30000);
    }

    void test_process_fill_value_chunks_some_fill() {
        unique_ptr<DmrppCommon> dc(new DmrppCommon);
        // process_fill_value_chunks() uses these values and calls DmrppCommon::add_chunk()
        dc->d_fill_value_str = "17";
        dc->d_byte_order = "LE";
        // load up some chunks to simulate parsing the DMR++
        dc->add_chunk("LE", 10000, 0, "[0]");
        dc->add_chunk("LE", 10000, 20000, "[20000]");

        set<shape> cm;
        // Add two chunks
        shape c1{0};
        shape c2{20000};
        cm.insert(c1);
        cm.insert(c2);


        shape cs{10000};
        shape as{40000};
        DMZ::process_fill_value_chunks(dc.get(), cm, cs, as, 20000);
        CPPUNIT_ASSERT_MESSAGE("There should be two chunks", dc->get_immutable_chunks().size() == 4);
        DBG(for_each(dc->get_immutable_chunks().begin(), dc->get_immutable_chunks().end(),
                     [](const shared_ptr<Chunk> c) { cerr << c->get_uses_fill_value() << " "; }));

        vector<shared_ptr<Chunk>> vspc = dc->get_immutable_chunks();
        CPPUNIT_ASSERT(vspc.at(0)->get_uses_fill_value() == false);
        CPPUNIT_ASSERT(vspc.at(0)->get_position_in_array().at(0) == 0);

        CPPUNIT_ASSERT(vspc.at(1)->get_uses_fill_value() == false);
        CPPUNIT_ASSERT(vspc.at(1)->get_position_in_array().at(0) == 20000);

        CPPUNIT_ASSERT(vspc.at(2)->get_size() == 20000);    // two bytes per chunk (simulated) in this test
        CPPUNIT_ASSERT(vspc.at(2)->get_uses_fill_value() == true);
        CPPUNIT_ASSERT(vspc.at(2)->get_position_in_array().at(0) == 10000);

        CPPUNIT_ASSERT(vspc.at(3)->get_uses_fill_value() == true);
        CPPUNIT_ASSERT(vspc.at(3)->get_position_in_array().at(0) == 30000);
    }

    void test_process_fill_value_chunks_some_fill_2D() {
        unique_ptr<DmrppCommon> dc(new DmrppCommon);
        // process_fill_value_chunks() uses these values and calls DmrppCommon::add_chunk()
        dc->d_fill_value_str = "17";
        dc->d_byte_order = "LE";
        // load up some chunks to simulate parsing the DMR++
        dc->add_chunk("LE", 21, 0, "[0,0]");
        dc->add_chunk("LE", 21, 20000, "[0,14]");
        dc->add_chunk("LE", 21, 30000, "[3,7]");


        set<shape> cm;
        shape c1{0,0}; cm.insert(c1);
        shape c2{0,14}; cm.insert(c2);
        shape c3{3,7}; cm.insert(c3);

        shape cs{3, 7};
        shape as{6, 16};
        DMZ::process_fill_value_chunks(dc.get(), cm, cs, as, 21);
        CPPUNIT_ASSERT_MESSAGE("There should be four chunks", dc->get_immutable_chunks().size() == 6);
        DBG(for_each(dc->get_immutable_chunks().begin(), dc->get_immutable_chunks().end(),
                     [](const shared_ptr<Chunk> c) { cerr << c->get_uses_fill_value() << " "; }));

        vector<shared_ptr<Chunk>> vspc = dc->get_immutable_chunks();
        // These two chunks were added to simulate parsing the DMR++
        CPPUNIT_ASSERT(vspc.at(0)->get_uses_fill_value() == false);
        CPPUNIT_ASSERT(vspc.at(0)->get_position_in_array().at(0) == 0);
        CPPUNIT_ASSERT(vspc.at(0)->get_position_in_array().at(1) == 0);
        // skip second chunk
        CPPUNIT_ASSERT(vspc.at(2)->get_uses_fill_value() == false);
        CPPUNIT_ASSERT(vspc.at(2)->get_position_in_array().at(0) == 3);
        CPPUNIT_ASSERT(vspc.at(2)->get_position_in_array().at(1) == 7);

        // These two chunks were added as fill value chunks
        CPPUNIT_ASSERT(vspc.at(3)->get_uses_fill_value() == true);
        CPPUNIT_ASSERT(vspc.at(3)->get_position_in_array().at(0) == 0);
        CPPUNIT_ASSERT(vspc.at(3)->get_position_in_array().at(1) == 7);
        // skipped 3,0
        CPPUNIT_ASSERT(vspc.at(5)->get_uses_fill_value() == true);
        CPPUNIT_ASSERT(vspc.at(5)->get_position_in_array().at(0) == 3);
        CPPUNIT_ASSERT(vspc.at(5)->get_position_in_array().at(1) == 14);

        CPPUNIT_ASSERT(vspc.at(3)->get_size() == 21);
    }

    void test_process_fill_value_chunks_all_fill_2D() {
        unique_ptr<DmrppCommon> dc(new DmrppCommon);
        // process_fill_value_chunks() uses these values and calls DmrppCommon::add_chunk()
        dc->d_fill_value_str = "17";
        dc->d_byte_order = "LE";

        set<shape> cm;
        shape cs{3, 7};
        shape as{6, 16};
        DMZ::process_fill_value_chunks(dc.get(), cm, cs, as, 1);
        CPPUNIT_ASSERT_MESSAGE("There should be four chunks", dc->get_immutable_chunks().size() == 6);
        DBG(for_each(dc->get_immutable_chunks().begin(), dc->get_immutable_chunks().end(),
                     [](const shared_ptr<Chunk> c) { cerr << c->get_uses_fill_value() << " "; }));

        vector<shared_ptr<Chunk>> vspc = dc->get_immutable_chunks();
        CPPUNIT_ASSERT(vspc.at(0)->get_uses_fill_value() == true);
        CPPUNIT_ASSERT(vspc.at(0)->get_position_in_array().at(0) == 0);
        CPPUNIT_ASSERT(vspc.at(0)->get_position_in_array().at(1) == 0);

        CPPUNIT_ASSERT(vspc.at(3)->get_uses_fill_value() == true);
        CPPUNIT_ASSERT(vspc.at(3)->get_position_in_array().at(0) == 3);
        CPPUNIT_ASSERT(vspc.at(3)->get_position_in_array().at(1) == 0);

        CPPUNIT_ASSERT(vspc.at(5)->get_uses_fill_value() == true);
        CPPUNIT_ASSERT(vspc.at(5)->get_position_in_array().at(0) == 3);
        CPPUNIT_ASSERT(vspc.at(5)->get_position_in_array().at(1) == 14);
    }

    void test_load_chunks_1() {
        try {
            d_dmz.reset(new DMZ(chunked_fourD_dmrpp));
            DmrppTypeFactory factory;
            DMR dmr(&factory);
            d_dmz->build_thin_dmr(&dmr);

            XMLWriter xml;
            dmr.print_dap4(xml);
            DBG(cerr << "DMR: " << xml.get_doc() << endl);

            // Given a thin DMR, load in the attributes of the first variable
            auto *btp = *(dmr.root()->var_begin());
            CPPUNIT_ASSERT(btp);

            d_dmz->load_chunks(btp);

            auto const* dc = dynamic_cast<DmrppCommon *>(btp);
            auto c_sizes = dc->get_chunk_dimension_sizes();
            CPPUNIT_ASSERT(c_sizes.size() == 4);
            CPPUNIT_ASSERT(c_sizes.at(0) == 20);
            CPPUNIT_ASSERT(c_sizes.at(1) == 20);
            CPPUNIT_ASSERT(c_sizes.at(2) == 20);
            CPPUNIT_ASSERT(c_sizes.at(3) == 20);

            auto chunks = dc->get_immutable_chunks();
            DBG(cerr << "chunks.size(): " << chunks.size() << endl);
            CPPUNIT_ASSERT(chunks.size() == 16);
            CPPUNIT_ASSERT(chunks.at(0)->get_offset() == 4728);
            CPPUNIT_ASSERT(chunks.at(0)->get_size() == 640000);
            CPPUNIT_ASSERT(chunks.at(0)->get_position_in_array().size() == 4);
            CPPUNIT_ASSERT(chunks.at(0)->get_position_in_array().at(0) == 0);
            CPPUNIT_ASSERT(chunks.at(0)->get_position_in_array().at(3) == 0);

            CPPUNIT_ASSERT(chunks.at(15)->get_offset() == 9606776);
            CPPUNIT_ASSERT(chunks.at(15)->get_size() == 640000);
            CPPUNIT_ASSERT(chunks.at(15)->get_position_in_array().size() == 4);
            CPPUNIT_ASSERT(chunks.at(15)->get_position_in_array().at(0) == 20);
            CPPUNIT_ASSERT(chunks.at(15)->get_position_in_array().at(3) == 20);
        }
        catch (...) {
            handle_fatal_exceptions();
        }
    }

    void test_load_chunks_2() {
        try {
            d_dmz.reset(new DMZ(coads_climatology_dmrpp));
            DmrppTypeFactory factory;
            DMR dmr(&factory);
            d_dmz->build_thin_dmr(&dmr);

            XMLWriter xml;
            dmr.print_dap4(xml);
            DBG(cerr << "DMR: " << xml.get_doc() << endl);

            // Given a thin DMR, load in the attributes of the first variable
            auto *btp = dmr.root()->find_var("/TIME");
            CPPUNIT_ASSERT(btp);

            d_dmz->load_chunks(btp);

            auto const* dc = dynamic_cast<DmrppCommon *>(btp);
            auto const& c_sizes = dc->get_chunk_dimension_sizes();
            CPPUNIT_ASSERT(c_sizes.empty());

            auto chunks = dc->get_immutable_chunks();
            DBG(cerr << "chunks.size(): " << chunks.size() << endl);
            CPPUNIT_ASSERT(chunks.size() == 1);
            CPPUNIT_ASSERT(chunks.at(0)->get_offset() == 3112560);
            CPPUNIT_ASSERT(chunks.at(0)->get_size() == 96);
            CPPUNIT_ASSERT(chunks.at(0)->get_position_in_array().empty());
        }
        catch (...) {
            handle_fatal_exceptions();
        }
    }

    void test_load_all_attributes_1() {
        try {
            d_dmz.reset(new DMZ(coads_climatology_dmrpp));
            DmrppTypeFactory factory;
            DMR dmr(&factory);
            d_dmz->build_thin_dmr(&dmr);

            XMLWriter xml;
            dmr.print_dap4(xml);
            DBG(cerr << "DMR: " << xml.get_doc() << endl);

            d_dmz->load_all_attributes(&dmr);

            XMLWriter xml2;
            dmr.print_dap4(xml2);
            DBG(cerr << "DMR: " << xml2.get_doc() << endl);

            auto *attrs = dmr.root()->attributes();
            CPPUNIT_ASSERT(!attrs->empty());
            const D4Attribute *history = attrs->get("NC_GLOBAL.history");
            CPPUNIT_ASSERT(history);
            CPPUNIT_ASSERT(history->name() == "history");
            CPPUNIT_ASSERT(history->value(0).find("FERRET") != string::npos);
        }
        catch (...) {
            handle_fatal_exceptions();
        }
    }

    void test_vlsa_element_values() {
        try {
            DBG( cerr << prolog << "Using input: " << vlsa_element_values_dmrpp << "\n");

            d_dmz.reset(new DMZ(vlsa_element_values_dmrpp));
            DmrppTypeFactory factory;
            DMR dmr(&factory);
            d_dmz->build_thin_dmr(&dmr);

            XMLWriter xml;
            dmr.print_dap4(xml);
            DBG(cerr << prolog << "Using DMR document: \n" << xml.get_doc() << endl);

            auto vars = dmr.root()->variables();
            CPPUNIT_ASSERT( vars.size() == 1 );

            auto btp = vars[0];

            CPPUNIT_ASSERT(btp->type() == dods_array_c);

            auto vlsa = dynamic_cast<DmrppArray *>(btp);
            DBG(cerr << prolog << "vlsa->is_vlsa(): " <<(vlsa->is_vlsa()?"true":"false") << endl);
            CPPUNIT_ASSERT_MESSAGE("vlsa->is_vlsa() is not marked as VLSA", vlsa->is_vlsa());

            DBG(cerr << prolog << "vlsa->read_p(): " <<(vlsa->read_p()?"true":"false") << endl);
            d_dmz->load_chunks(vlsa);
            DBG(cerr << prolog << "vlsa->read_p(): " <<(vlsa->read_p()?"true":"false") << endl);
            CPPUNIT_ASSERT_MESSAGE("vlsa->read_p() is false. Should be true.", vlsa->read_p());

            auto baselines = { "Parting", "is su", "swe", ""};

            auto baseline = baselines.begin();
            vector<string> values;
            vlsa->value(values);
            uint64_t idx=0;
            for(auto value:values){
                DBG( cerr << prolog << "    value[" << idx << "]: '" << value << "'\n");
                DBG( cerr << prolog << "baselines[" << idx << "]: '" << *baseline << "'\n");
                CPPUNIT_ASSERT_MESSAGE("Value does not match baseline!", value == *baseline);
                baseline++;
                idx++;
            }

        }
        catch (...) {
            handle_fatal_exceptions();
        }
    }

    void test_vlsa_base64_values() {
        try {
            DBG( cerr << prolog << "Using input: " << vlsa_base64_values_dmrpp << "\n");

            d_dmz.reset(new DMZ(vlsa_base64_values_dmrpp));
            DmrppTypeFactory factory;
            DMR dmr(&factory);
            d_dmz->build_thin_dmr(&dmr);

            XMLWriter xml;
            dmr.print_dap4(xml);
            DBG(cerr << prolog << "Using DMR document: \n" << xml.get_doc() << endl);

            auto vars = dmr.root()->variables();
            CPPUNIT_ASSERT( vars.size() == 1 );

            auto btp = vars[0];

            CPPUNIT_ASSERT(btp->type() == dods_array_c);

            auto vlsa = dynamic_cast<DmrppArray *>(btp);
            DBG(cerr << prolog << "vlsa->is_vlsa(): " <<(vlsa->is_vlsa()?"true":"false") << endl);
            CPPUNIT_ASSERT_MESSAGE("vlsa->is_vlsa() is not marked as VLSA", vlsa->is_vlsa());

            DBG(cerr << prolog << "vlsa->read_p(): " <<(vlsa->read_p()?"true":"false") << endl);
            d_dmz->load_chunks(vlsa);
            DBG(cerr << prolog << "vlsa->read_p(): " <<(vlsa->read_p()?"true":"false") << endl);
            CPPUNIT_ASSERT_MESSAGE("vlsa->read_p() is false. Should be true.", vlsa->read_p());

            auto baselines = { "Parting", "is su", "swe", ""};

            auto baseline = baselines.begin();
            vector<string> values;
            vlsa->value(values);
            uint64_t idx=0;
            for(auto value:values){
                DBG( cerr << prolog << "    value[" << idx << "]: '" << value << "'\n");
                DBG( cerr << prolog << "baselines[" << idx << "]: '" << *baseline << "'\n");
                CPPUNIT_ASSERT_MESSAGE("Value does not match baseline!", value == *baseline);
                baseline++;
                idx++;
            }

        }
        catch (...) {
            handle_fatal_exceptions();
        }
    }

CPPUNIT_TEST_SUITE( DMZTest );

    CPPUNIT_TEST(test_vlsa_element_values);
    CPPUNIT_TEST(test_vlsa_base64_values);

    CPPUNIT_TEST(test_DMZ_ctor_1);
    CPPUNIT_TEST(test_DMZ_ctor_2);
    CPPUNIT_TEST(test_DMZ_ctor_3);
    CPPUNIT_TEST(test_DMZ_ctor_4);

    CPPUNIT_TEST(test_parse_xml_string);
    CPPUNIT_TEST(test_parse_xml_string_bad_input);

    CPPUNIT_TEST(test_process_dataset_0);
    CPPUNIT_TEST(test_process_dataset_1);
    CPPUNIT_TEST(test_process_dataset_2);

    CPPUNIT_TEST(test_build_thin_dmr_1);
    CPPUNIT_TEST(test_build_thin_dmr_2);
    CPPUNIT_TEST(test_build_thin_dmr_3);
    CPPUNIT_TEST(test_build_thin_dmr_4);
    CPPUNIT_TEST(test_build_thin_dmr_5);

    CPPUNIT_TEST(test_build_basetype_chain_1);
    CPPUNIT_TEST(test_build_basetype_chain_2);
    CPPUNIT_TEST(test_build_basetype_chain_3);

    CPPUNIT_TEST(test_get_variable_xml_node_1);
    CPPUNIT_TEST(test_get_variable_xml_node_2);
    CPPUNIT_TEST(test_get_variable_xml_node_3);
    CPPUNIT_TEST_FAIL(test_get_variable_xml_node_4);

    CPPUNIT_TEST(test_load_attributes_1);
    CPPUNIT_TEST(test_load_attributes_2);
    CPPUNIT_TEST(test_load_attributes_3);

    CPPUNIT_TEST(test_process_cds_node_1);

    CPPUNIT_TEST(test_get_array_dims_1);
    CPPUNIT_TEST(test_get_array_dims_2);
    CPPUNIT_TEST(test_get_array_dims_3);
    CPPUNIT_TEST(test_get_array_dims_4);

    CPPUNIT_TEST(test_logical_chunks_1);
    CPPUNIT_TEST(test_logical_chunks_2);
    CPPUNIT_TEST(test_logical_chunks_3);
    CPPUNIT_TEST(test_logical_chunks_4);

    CPPUNIT_TEST(test_get_chunk_map_1);
    CPPUNIT_TEST(test_get_chunk_map_2);

    CPPUNIT_TEST(test_process_fill_value_chunks_all_fill);
    CPPUNIT_TEST(test_process_fill_value_chunks_some_fill);
    CPPUNIT_TEST(test_process_fill_value_chunks_all_fill_2D);
    CPPUNIT_TEST(test_process_fill_value_chunks_some_fill_2D);

    CPPUNIT_TEST(test_load_chunks_1);
    CPPUNIT_TEST(test_load_chunks_2);

    CPPUNIT_TEST(test_load_all_attributes_1);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DMZTest);

} // namespace dmrpp

int main(int argc, char*argv[])
{

    return bes_run_tests<dmrpp::DMZTest>(argc, argv, bes_debug_args) ? 0 : 1;
}
