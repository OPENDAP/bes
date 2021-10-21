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

#include "BESInternalError.h"
#include "BESDebug.h"

#include "DMZ.h"

#include "read_test_baseline.h"
#include "test_config.h"

using namespace std;
using namespace libdap;
using namespace bes;

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

public:
    // Called once before everything gets tested
    DMZTest() : d_dmz(nullptr) { }

    // Called at the end of the test
    ~DMZTest() = default;

    // Called before each test
    void setUp()
    {
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

    void test_process_dataset() {
        try {
            d_dmz = new DMZ(chunked_fourD_dmrpp);
            DMR dmr;
            d_dmz->process_dataset(dmr, d_dmz->d_xml_doc.first_node());
            DBG(cerr << "dmr.dap_version(): " << dmr.dap_version() << endl);
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

    void test_build_thin_dmr() {
        try {
            d_dmz = new DMZ(chunked_fourD_dmrpp);
            DMR local_dmr;
            d_dmz->build_thin_dmr(local_dmr);
            CPPUNIT_ASSERT("woo hoo");
        }
        catch (BESInternalError &e) {
            CPPUNIT_FAIL("DMZ ctor should not succeed with an empty path");
        }
    }

CPPUNIT_TEST_SUITE( DMZTest );

    CPPUNIT_TEST(test_DMZ_ctor_1);
    CPPUNIT_TEST(test_DMZ_ctor_2);
    CPPUNIT_TEST(test_DMZ_ctor_3);
    CPPUNIT_TEST(test_DMZ_ctor_4);

    CPPUNIT_TEST(test_process_dataset);
    CPPUNIT_TEST(test_build_thin_dmr);

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
