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

#include "awsv4.h"

#include "test_config.h"

using namespace libdap;

static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)

#define SECRET_KEY "wJalrXUtnFEMI/K7MDENG+bPxRfiCYEXAMPLEKEY"
#define KEY_ID "AKIDEXAMPLE"


namespace dmrpp {

    class awsv4_test: public CppUnit::TestFixture {
    private:
        string cm_config;
        string weak_config;

        string fileToString(const string &fn)
        {
            ifstream is;
            is.open(fn.c_str(), ios::binary);
            if (!is || is.eof()) return "";

            // get length of file:
            is.seekg(0, ios::end);
            int length = is.tellg();

            // back to start
            is.seekg(0, ios::beg);

            // allocate memory:
            vector<char> buffer(length + 1);

            // read data as a block:
            is.read(&buffer[0], length);
            is.close();
            buffer[length] = '\0';

            return string(&buffer[0]);
        }

    public:
        // Called once before everything gets tested
        awsv4_test()
        {
        }

        // Called at the end of the test
        ~awsv4_test()
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


        void show_baseline(string name, string value){
            cerr << "# BEGIN " << name << " -------------------------------------------------" << endl;
            cerr << value << endl;
            cerr << "# END " << name << " -------------------------------------------------" << endl << endl;
        }


        void run_test(string test_name){
            string test_file_base = string(TEST_SRC_DIR).append("/awsv4/")
                    .append(test_name).append("/")
                    .append(test_name);

            //file-name.req—the web request to be signed.
            string web_request_baseline;
            web_request_baseline = fileToString(test_file_base + ".req");
            if(debug) show_baseline("web_request_baseline", web_request_baseline);

            //file-name.creq—the resulting canonical request.
            string canonical_request_baseline;
            canonical_request_baseline = fileToString(test_file_base + ".creq");
            if(debug) show_baseline("canonical_request_baseline", canonical_request_baseline);

            //file-name.sts—the resulting string to sign.
            string string_to_sign_baseline;
            string_to_sign_baseline = fileToString(test_file_base + ".sts");
            if(debug) show_baseline("string_to_sign_baseline", string_to_sign_baseline);

            //file-name.sreq— the signed request.
            string signed_request_baseline;
            signed_request_baseline = fileToString(test_file_base + ".sreq");
            if(debug) show_baseline("signed_request_baseline", signed_request_baseline);


            //file-name.authz—the Authorization header.
            string auth_header_baseline;
            auth_header_baseline = fileToString(test_file_base + ".authz");
            if(debug) show_baseline("auth_header_baseline", auth_header_baseline);


            // AWSv4 examples are based on a request dat/time of:
            // define REQUEST_DATE "2015 08 30 T 12 36 00Z"
            //
            // Set timezone to GMT0
            setenv("TZ", "GMT0", true);

            // Populate time struct
            struct tm t_info;
            t_info.tm_year   = 115; // Years since 1900
            t_info.tm_mon    = 7;   // August
            t_info.tm_mday   = 30;
            t_info.tm_hour   = 12;   // 1200 GMT
            t_info.tm_min    = 36;
            t_info.tm_sec    = 0;

            // Get the time value
            const std::time_t request_time = mktime(&t_info);
            if(debug) cerr << "request_time: " << request_time << endl;


            std::string auth_header =
                    AWSV4::compute_awsv4_signature(
                            "https://example.amazonaws.com/",
                            request_time,
                            KEY_ID,
                            SECRET_KEY,
                            "us-east-1",
                            "service",
                            debug);

            if(debug) show_baseline("auth_header", auth_header);

            CPPUNIT_ASSERT(auth_header == auth_header_baseline);

        }

        void get_vanilla_query() {
            run_test("get-vanilla-query");
        }




    CPPUNIT_TEST_SUITE( awsv4_test );

            CPPUNIT_TEST(get_vanilla_query);

    CPPUNIT_TEST_SUITE_END();

    };

    CPPUNIT_TEST_SUITE_REGISTRATION(awsv4_test);

} // namespace dmrpp

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "dD");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
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
            string test = dmrpp::awsv4_test::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
