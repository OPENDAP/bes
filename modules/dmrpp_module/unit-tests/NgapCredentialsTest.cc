// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2020 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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

#include <stdlib.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <GetOpt.h>
#include <util.h>
#include <debug.h>

#include "BESContextManager.h"
#include "BESError.h"
#include "BESDebug.h"
#include "TheBESKeys.h"

#include "CredentialsManager.h"

#include "test_config.h"
#include "AccessCredentials.h"

using namespace libdap;

static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)

namespace dmrpp {

    class NgapCredentialsTest: public CppUnit::TestFixture {
    private:
        string weak_config;

    public:
        static string cm_config;

        // Called once before everything gets tested
        NgapCredentialsTest()
        {
        }

        // Called at the end of the test
        ~NgapCredentialsTest()
        {
        }

        // Called before each test
        void setUp()
        {
            if(debug) cout << endl ;
            if (bes_debug) BESDebug::SetUp("cerr,dmrpp");

            TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");
            cm_config = string(TEST_BUILD_DIR).append("/credentials.conf");
            weak_config = string(TEST_SRC_DIR).append("/input-files/weak.conf");

        }

        // Called after each test
        void tearDown()
        {
        }



        void get_s3_creds() {

        }



    CPPUNIT_TEST_SUITE( NgapCredentialsTest );
        static string cm_config;

            ;

            CPPUNIT_TEST(get_s3_creds);

        CPPUNIT_TEST_SUITE_END();

    };

    CPPUNIT_TEST_SUITE_REGISTRATION(NgapCredentialsTest);

} // namespace dmrpp

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());
    GetOpt getopt(argc, argv, "c:dD");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
            case 'c':
                dmrpp::NgapCredentialsTest::cm_config = getopt.optarg;
                break;
            case 'd':
                debug = true;  // debug is a static global
                break;
            case 'b':
                debug = true;  // debug is a static global
                bes_debug = true;  // debug is a static global
                break;
            default:
                break;
        }

    bool wasSuccessful = true;
    int i = getopt.optind;
    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            string test = dmrpp::NgapCredentialsTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
