// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES component of the Hyrax Data Server.

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

#include "config.h"

#include <memory>
#include <cstring>
#include <iostream>
#include <sstream>
#include <regex>

#include <curl/curl.h>
#include <curl/easy.h>

#include "BESError.h"
#include "BESXMLInfo.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESStopWatch.h"
#include "BESCatalogList.h"
#include "TheBESKeys.h"
#include "BESContextManager.h"
#include "url_impl.h"
#include "AccessCredentials.h"
#include "CredentialsManager.h"
#include "BESForbiddenError.h"
#include "BESSyntaxUserError.h"
#include "BESDataHandlerInterface.h"

#include "CurlUtils.h"
#include "HttpError.h"

#include "test_config.h"

// Maybe the common testing code in modules should be moved up one level? jhrg 11/3/22
#include "modules/common/run_tests_cppunit.h"

using namespace std;

#define prolog std::string("HttpErrorTest::").append(__func__).append("() - ")

namespace http {

class HttpErrorTest : public CppUnit::TestFixture {

public:
    string d_data_dir = TEST_DATA_DIR;

    // Called once before everything gets tested
    HttpErrorTest() = default;

    // Called at the end of the test
    ~HttpErrorTest() override = default;

    // Called before each test
    void setUp() override {
        DBG(cerr << "\n");
        DBG(cerr << "#-----------------------------------------------------------------\n");
        DBG(cerr << "setUp() - BEGIN\n");
        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR, "bes.conf");
        DBG(cerr << "setUp() - Using BES configuration: " << bes_conf << "\n");
        DBG2(show_file(bes_conf));
        TheBESKeys::ConfigFile = bes_conf;

        DBG(cerr << "setUp() - END\n");
    }

    // Called after each test
    void tearDown() override {
        // These are set in add_edl_auth_headers_test() and not 'unsetting' them
        // causes other odd behavior in subsequent tests (Forbidden exceptions
        // become SyntaxUser ones). Adding the unset operations here ensures they
        // happen even if exceptions are thrown by the add_edl...() test.
        BESContextManager::TheManager()->unset_context(EDL_UID_KEY);
        BESContextManager::TheManager()->unset_context(EDL_AUTH_TOKEN_KEY);
        BESContextManager::TheManager()->unset_context(EDL_ECHO_TOKEN_KEY);

        // We have to remove the cookie file between test invocations.
        // Not doing so can cause the previous test's login success
        // to propagate to the next test. Which is a problem when testing
        // behaviors related to authentication success/failure.
        auto cookie_file = curl::get_cookie_filename();
        ifstream f(cookie_file.c_str());
        if (f.good()) {
            int retval = std::remove(cookie_file.c_str());
            if (retval != 0 && debug) {
                stringstream msg;
                msg << "Failed to delete cookie file: '" << cookie_file << "' ";
                msg << "Message: " << strerror(errno);
                DBG(cerr << prolog << msg.str() << "\n");
            }
        }
        DBG(cerr << "\n");
    }

/*##################################################################################################*/
/* TESTS BEGIN */


    void test_with_headers_and_body() {
        try {

            {
                /*
                      HttpError(const std::string msg,
                      const CURLcode code,
                      const unsigned int http_status,
                      const std::string origin_url,
                      const std::string redirect_url,
                      const std::vector<std::string> response_headers,
                      const std::string response_body,
                      const std::string file,
                      const int line):

                 */
                string msg("Test Error");
                string origin("http://someserver.somewhere.org");
                string redirect("https://someserver.somewhere.org/");

                vector<string> resp_hdrs;
                resp_hdrs.emplace_back("location: earth");
                resp_hdrs.emplace_back("server: celeste");
                resp_hdrs.emplace_back("captain: Aubrey");

                string body("</h1>A nautical novel in 20 parts.</h1>");

                throw http::HttpError(msg, CURLE_OK, 302, origin, redirect, resp_hdrs, body, __FILE__, __LINE__);
            }
        }
        catch (http::HttpError he) {
            DBG(cerr << prolog << he.dump() << "\n");
            throw;
        }
    }


    void test_no_headers_no_body() {
        try {
            {

                /*
                    HttpError(const std::string msg,
                            const CURLcode code,
                            const unsigned int http_status,
                            const std::string origin_url,
                            const std::string redirect_url,
                            const std::string file,
                            const int line):
                */
                string msg("Test Error");
                string origin("http://someserver.somewhere.org");
                string redirect("https://someserver.somewhere.org/");
                throw http::HttpError(msg, CURLE_OK, 302, origin, redirect, __FILE__, __LINE__);
            }
        }
        catch (http::HttpError he) {
            DBG(cerr << prolog << he.dump() << "\n");
            throw;
        }
    }



    void test_BESInfo_1() {
        try {
            {

                /*
                    HttpError(const std::string msg,
                            const CURLcode code,
                            const unsigned int http_status,
                            const std::string origin_url,
                            const std::string redirect_url,
                            const std::string file,
                            const int line):
                */
                string baseline = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n"
                                  "<response xmlns=\"http://xml.opendap.org/ns/bes/1.0#\">\n"
                                  "    <test_BESInfo_1>\n"
                                  "        <BESError>\n"
                                  "            <Type>7</Type>\n"
                                  "            <Message>Test Error</Message>\n"
                                  "            <Administrator>harold.arlen@mountain_apple.com</Administrator>\n"
                                  "            <curl_code>0</curl_code>\n"
                                  "            <http_status>302</http_status>\n"
                                  "            <origin_url>http://someserver.somewhere.org</origin_url>\n"
                                  "            <redirect_url>https://someserver.somewhere.org/</redirect_url>\n"
                                  "            <response_headers/>\n"
                                  "            <response_body/>\n"
                                  "            <Location>\n"
                                  "                <File>HttpErrorTest.cc</File>\n"
                                  "                <Line />\n"
                                  "            </Location>\n"
                                  "        </BESError>\n"
                                  "    </test_BESInfo_1>\n"
                                  "</response>\n";

                string msg("Test Error");
                string origin("http://someserver.somewhere.org");
                string redirect("https://someserver.somewhere.org/");
                auto http_error = http::HttpError(msg, CURLE_OK, 302, origin, redirect, __FILE__, __LINE__);
                //throw http_error;
                BESDataHandlerInterface dhi;
                BESXMLInfo bi;
                bi.begin_response("test_BESInfo_1", dhi);
                bi.add_exception(http_error, "harold.arlen@mountain_apple.com");
                bi.end_response();

                stringstream rss;
                bi.print(rss);
                regex rx("<Line>\\d+<\\/Line>");
                string result = std::regex_replace (rss.str(),rx,"<Line />");

                DBG( cerr << "\n");
                DBG( cerr << prolog << "baseline: \n\n" << baseline << "\n");
                DBG( cerr << prolog << "result: \n\n" << result);
                CPPUNIT_ASSERT( result == baseline);

            }
        }
        catch (http::HttpError he) {
            DBG(cerr << prolog << he.dump() << "\n");
            throw;
        }
    }

    void test_BESInfo_2() {
        DBG( cerr << "\n");

        try {
            /*
                  HttpError(const std::string msg,
                  const CURLcode code,
                  const unsigned int http_status,
                  const std::string origin_url,
                  const std::string redirect_url,
                  const std::vector<std::string> response_headers,
                  const std::string response_body,
                  const std::string file,
                  const int line):

             */
            string baseline("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n"
                            "<response xmlns=\"http://xml.opendap.org/ns/bes/1.0#\">\n"
                            "    <test_BESInfo_2>\n"
                            "        <BESError>\n"
                            "            <Type>7</Type>\n"
                            "            <Message>Test Error</Message>\n"
                            "            <Administrator>harold.arlen@mountain_apple.com</Administrator>\n"
                            "            <curl_code>0</curl_code>\n"
                            "            <http_status>302</http_status>\n"
                            "            <origin_url>http://someserver.somewhere.org</origin_url>\n"
                            "            <redirect_url>https://someserver.somewhere.org/</redirect_url>\n"
                            "            <response_headers>\n"
                            "                <header>location: earth</header>\n"
                            "                <header>server: celeste</header>\n"
                            "                <header>captain: Aubrey</header>\n"
                            "            </response_headers>\n"
                            "            <response_body>&lt;/h1&gt;A nautical novel in 20 parts.&lt;/h1&gt;</response_body>\n"
                            "            <Location>\n"
                            "                <File>HttpErrorTest.cc</File>\n"
                            "                <Line />\n"
                            "            </Location>\n"
                            "        </BESError>\n"
                            "    </test_BESInfo_2>\n"
                            "</response>\n");

            string msg("Test Error");
            string origin("http://someserver.somewhere.org");
            string redirect("https://someserver.somewhere.org/");

            vector<string> resp_hdrs;
            resp_hdrs.emplace_back("location: earth");
            resp_hdrs.emplace_back("server: celeste");
            resp_hdrs.emplace_back("captain: Aubrey");

            string body("</h1>A nautical novel in 20 parts.</h1>");

            auto http_error = http::HttpError(msg, CURLE_OK, 302, origin, redirect, resp_hdrs, body, __FILE__,
                                              __LINE__);
            BESDataHandlerInterface dhi;
            BESXMLInfo bi;
            bi.begin_response("test_BESInfo_2", dhi);
            bi.add_exception(http_error, "harold.arlen@mountain_apple.com");
            bi.end_response();

            stringstream rss;
            bi.print(rss);
            regex rx("<Line>\\d+<\\/Line>");
            string result = std::regex_replace (rss.str(),rx,"<Line />");

            DBG( cerr << "\n");
            DBG( cerr << prolog << "baseline: \n\n" << baseline << "\n");
            DBG( cerr << prolog << "result: \n\n" << result);
            CPPUNIT_ASSERT( result == baseline);
        }
        catch (http::HttpError he) {
            DBG(cerr << prolog << he.dump() << "\n");
            throw;
        }
    }




/* TESTS END */
/*##################################################################################################*/

    CPPUNIT_TEST_SUITE(HttpErrorTest);

    CPPUNIT_TEST_EXCEPTION(test_with_headers_and_body, http::HttpError);
    CPPUNIT_TEST_EXCEPTION(test_no_headers_no_body, http::HttpError);
    CPPUNIT_TEST(test_BESInfo_1);
    CPPUNIT_TEST(test_BESInfo_2);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(HttpErrorTest);

} // namespace http

int main(int argc, char *argv[]) {
    return bes_run_tests<http::HttpErrorTest>(argc, argv, "cerr,bes,http,curl,curl:timing") ? 0 : 1;
}
