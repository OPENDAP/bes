// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2018 OPeNDAP, Inc.
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

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include <unistd.h>

#include "ServerAdministrator.h"
#include <BESDebug.h>
#include <BESError.h>
#include <BESUtil.h>
#include <TheBESKeys.h>

#include "test_config.h"
#include "test_utils.h"

static bool debug = false;
static bool bes_debug = false;

using namespace std;
using namespace CppUnit;
using namespace bes;

#define prolog std::string("ServerAdministratorTest::").append(__func__).append("() - ")

class ServerAdministratorTest : public CppUnit::TestFixture {

public:
    // Called once before everything gets tested
    ServerAdministratorTest() = default;

    // Called at the end of the test
    ~ServerAdministratorTest() = default;

    // Called before each test
    void setUp() {
        if (debug)
            cerr << endl;

        if (debug)
            cerr << prolog << "BEGIN" << endl;
        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR, "bes.conf");
        if (debug)
            cerr << prolog << "Using BES configuration: " << bes_conf << endl;

        TheBESKeys::ConfigFile = bes_conf;

        if (bes_debug)
            BESDebug::SetUp(string("cerr,bes"));
    }

    // Called after each test
    void tearDown() { TheBESKeys::ConfigFile = ""; }

    CPPUNIT_TEST_SUITE(ServerAdministratorTest);

    CPPUNIT_TEST(xml_dmup_test);
    CPPUNIT_TEST(json_dmup_test);
    CPPUNIT_TEST(get_test);

    CPPUNIT_TEST_SUITE_END();

    void xml_dmup_test() {
        try {
            ServerAdministrator admin;
            string xml_result = admin.xdump();
            string xml_baseline =
                "<ServerAdministrator organization=\"Company/Institution Name\" street=\"Street Address\" "
                "city=\"City\" region=\"State\" country=\"USA\" postalcode=\"12345\" telephone=\"+1.800.555.1212\" "
                "email=\"admin.email.address@your.domain.name\" website=\"http://www.your.domain.name\" />";

            if (debug) {
                cerr << "xml_baseline: " << xml_baseline << endl;
                cerr << "  xml_result: " << xml_result << endl;
            }
            CPPUNIT_ASSERT(xml_result == xml_baseline);
        } catch (BESError &e) {
            cerr << __func__ << "() - Error: " << e.get_verbose_message() << endl;
            CPPUNIT_ASSERT(false);
        }
    }

    void json_dmup_test() {

        try {
            ServerAdministrator admin;
            string json_result = admin.jdump();
            string json_baseline =
                "{\"ServerAdministrator\":{\"organization\": \"Company/Institution Name\", \"street\": \"Street "
                "Address\", \"city\": \"City\", \"region\": \"State\", \"country\": \"USA\", \"postalcode\": "
                "\"12345\", \"telephone\": \"+1.800.555.1212\", \"email\": \"admin.email.address@your.domain.name\", "
                "\"website\": \"http://www.your.domain.name\" }}";
            if (debug) {
                cerr << "json_baseline: " << json_baseline << endl;
                cerr << "  json_result: " << json_result << endl;
            }
            CPPUNIT_ASSERT(json_result == json_baseline);
        } catch (BESError &e) {
            cerr << __func__ << "() - Error: " << e.get_verbose_message() << endl;
            CPPUNIT_ASSERT(false);
        }
    }

    void get_test() {

        try {
            ServerAdministrator admin;
            string baseline, result;
            baseline = "City";
            result = admin.get("CiTy");
            if (debug) {
                cerr << "baseline: " << baseline << endl;
                cerr << "  result: " << result << endl;
            }
            CPPUNIT_ASSERT(result == baseline);

            result = admin.get("CiTy");
            if (debug) {
                cerr << "baseline: " << baseline << endl;
                cerr << "  result: " << result << endl;
            }
            CPPUNIT_ASSERT(result == baseline);

            result = admin.get("city");
            if (debug) {
                cerr << "baseline: " << baseline << endl;
                cerr << "  result: " << result << endl;
            }
            CPPUNIT_ASSERT(result == baseline);

            baseline = "admin.email.address@your.domain.name";
            result = admin.get("email");
            if (debug) {
                cerr << "baseline: " << baseline << endl;
                cerr << "  result: " << result << endl;
            }
            CPPUNIT_ASSERT(result == baseline);

            result = admin.get_email();
            if (debug) {
                cerr << "baseline: " << baseline << endl;
                cerr << "  result: " << result << endl;
            }
            CPPUNIT_ASSERT(result == baseline);

            baseline = "Street Address";
            result = admin.get("STREET");
            if (debug) {
                cerr << "baseline: " << baseline << endl;
                cerr << "  result: " << result << endl;
            }
            CPPUNIT_ASSERT(result == baseline);

            result = admin.get_street();
            if (debug) {
                cerr << "baseline: " << baseline << endl;
                cerr << "  result: " << result << endl;
            }
            CPPUNIT_ASSERT(result == baseline);

        } catch (BESError &e) {
            cerr << __func__ << "() - Error: " << e.get_verbose_message() << endl;
            CPPUNIT_ASSERT(false);
        }
    }
};

// BindTest

CPPUNIT_TEST_SUITE_REGISTRATION(ServerAdministratorTest);

int main(int argc, char *argv[]) {
    int option_char;
    while ((option_char = getopt(argc, argv, "bdh")) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1; // debug is a static global
            break;

        case 'b': {
            bes_debug = 1; // debug is a static global
            break;
        }
        case 'h': { // help - show test names
            cerr << "Usage: ServerAdministratorTest has the following tests:" << endl;
            const std::vector<Test *> &tests = ServerAdministratorTest::suite()->getTests();
            unsigned int prefix_len = ServerAdministratorTest::suite()->getName().append("::").size();
            for (std::vector<Test *>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                cerr << (*i)->getName().replace(0, prefix_len, "") << endl;
            }
            break;
        }
        default:
            break;
        }

    argc -= optind;
    argv += optind;

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    string test = "";
    if (0 == argc) {
        // run them all
        wasSuccessful = runner.run("");
    } else {
        int i = 0;
        while (i < argc) {
            if (debug)
                cerr << "Running " << argv[i] << endl;
            test = ServerAdministratorTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
