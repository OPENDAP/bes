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
#include <string.h>

#include "BESError.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESStopWatch.h"
#include "BESCatalogList.h"
#include "TheBESKeys.h"
#include "BESContextManager.h"
#include "url_impl.h"
#include "AccessCredentials.h"
#include "CurlUtils.h"
#include "CredentialsManager.h"
#include "BESForbiddenError.h"
#include "EffectiveUrlCache.h"

#include "test_config.h"

// Maybe the common testing code in modules should be moved up one level? jhrg 11/3/22
#include "modules/common/run_tests_cppunit.h"

using namespace std;

#define prolog std::string("# CurlUtilsTest::").append(__func__).append("() - ")

namespace http {

class CurlUtilsTest : public CppUnit::TestFixture {

public:
    string d_data_dir = TEST_DATA_DIR;

    // Called once before everything gets tested
    CurlUtilsTest() = default;

    // Called at the end of the test
    ~CurlUtilsTest() override = default;

    // Called before each test
    void setUp() override {
        DBG( cerr << endl);
        DBG( cerr << "setUp() - BEGIN" << endl);
        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR, "bes.conf");
        DBG( cerr << "setUp() - Using BES configuration: " << bes_conf << endl);
        DBG2( show_file(bes_conf));
        TheBESKeys::ConfigFile = bes_conf;

        DBG( cerr << "setUp() - END" << endl);
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

        auto cookie_file = curl::get_cookie_filename();
        std::remove(cookie_file.c_str());

    }

/*##################################################################################################*/
/* TESTS BEGIN */

    void is_retryable_test() {
        if (debug) cerr << prolog << "BEGIN" << endl;
        bool isRetryable;

        try {
            string url = "http://test.opendap.org/data/httpd_catalog/READTHIS";
            isRetryable = curl::is_retryable(url);
            if (debug) cerr << prolog << "is_retryable('" << url << "'): " << (isRetryable ? "true" : "false") << endl;
            CPPUNIT_ASSERT(isRetryable);

            url = "https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20031229v7.nc?A-userid=hyrax&X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=ASIASF4N-AWS-Creds-00808%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20200808T032623Z&X-Amz-Expires=86400&X-Amz-Security-Token=FwoGZXIvYXdzE-AWS-Sec-Token-MWRLIZGYvDx1ONzd0ffK8VtxO8JP7thrGIQ%3D%3D&X-Amz-SignedHeaders=host&X-Amz-Signature=260a7c4dd4-AWS-SIGGY-0c7a39ee899";
            isRetryable = curl::is_retryable(url);
            if (debug) cerr << prolog << "is_retryable('" << url << "'): " << (isRetryable ? "true" : "false") << endl;
            CPPUNIT_ASSERT(!isRetryable);

            url = "https://d1jecqxxv88lkr.cloudfront.net/ghrcwuat-protected/rss_demo/rssmif16d__7/f16_ssmis_20040107v7.nc";
            isRetryable = curl::is_retryable(url);
            if (debug) cerr << prolog << "is_retryable('" << url << "'): " << (isRetryable ? "true" : "false") << endl;
            CPPUNIT_ASSERT(isRetryable);

            url = "https://data.ghrc.uat.earthdata.nasa.gov/login?code=8800da07f823dfce312ee85e44c9e89efdf6bd9d776b1cb8666029ba2c8d257e&state=%2Fghrcwuat%2Dprotected%2Frss_demo%2Frssmif16d__7%2Ff16_ssmis_20040107v7%2Enc";
            isRetryable = curl::is_retryable(url);
            if (debug) cerr << prolog << "is_retryable('" << url << "'): " << (isRetryable ? "true" : "false") << endl;
            CPPUNIT_ASSERT(!isRetryable);

            url = "https://data.ghrc.uat.earthdata.nasa.gov/login?code=46196589bfe26c4c298e1a74646b99005d20a022cabff6434a550283defa8153&state=%2Fghrcwuat%2Dprotected%2Frss_demo%2Frssmif16d__7%2Ff16_ssmis_20040115v7%2Enc";
            isRetryable = curl::is_retryable(url);
            if (debug) cerr << prolog << "is_retryable('" << url << "'): " << (isRetryable ? "true" : "false") << endl;
            CPPUNIT_ASSERT(!isRetryable);
        }
        catch (const BESError &be) {
            stringstream msg;
            msg << prolog << "ERROR! Caught BESError. Message: " << be.get_message() << endl;
            CPPUNIT_FAIL(msg.str());

        }
        catch (const std::exception &se) {
            stringstream msg;
            msg << "CAUGHT std::exception message: " << se.what() << endl;
            cerr << msg.str();
            CPPUNIT_FAIL(msg.str());
        }
        if (debug) cerr << prolog << "END" << endl;
    }

    void filter_effective_url_test() {
        string url = "https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20031229v7.nc?A-userid=hyrax&X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=ASIASF4N-AWS-Creds-00808%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20200808T032623Z&X-Amz-Expires=86400&X-Amz-Security-Token=Foo&X-Amz-SignedHeaders=host&X-Amz-Signature=...";
        string filtered_url = "https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20031229v7.nc?A-userid=hyrax";
        CPPUNIT_ASSERT_MESSAGE("The URL should have the AWS security tokens removed",
                               filtered_url == curl::filter_aws_url(url));
    }

    void filter_effective_url_token_first_test() {
        string url = "https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20031229v7.nc?X-Amz-Security-Token=Foo&A-userid=hyrax&X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=ASIASF4N-AWS-Creds-00808%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20200808T032623Z&X-Amz-Expires=86400&X-Amz-SignedHeaders=host&X-Amz-Signature=...";
        string filtered_url = "https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20031229v7.nc";
        CPPUNIT_ASSERT_MESSAGE("The URL should have the AWS security tokens removed",
                               filtered_url == curl::filter_aws_url(url));
    }

    void filter_effective_url_no_qs_test() {
        string url = "https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20031229v7.nc";
        CPPUNIT_ASSERT_MESSAGE("The URL has no query string and should not be changed",
                               url == curl::filter_aws_url(url));
    }


    void retrieve_effective_url_test() {
        if (debug) cerr << prolog << "BEGIN" << endl;
        shared_ptr<http::url> trusted_target_url(new http::url("http://test.opendap.org/opendap", true));
        shared_ptr<http::url> target_url(new http::url("http://test.opendap.org/opendap", false));
        string expected_url = "http://test.opendap.org/opendap/";

        try {
            if (debug) cerr << prolog << "   target_url: " << target_url->str() << endl;

            auto effective_url = curl::retrieve_effective_url(target_url);

            if (debug) cerr << prolog << "effective_url: " << effective_url->str() << endl;
            if (debug) cerr << prolog << " expected_url: " << expected_url << endl;
            CPPUNIT_ASSERT(effective_url->str() == expected_url);

            if (debug)
                cerr << prolog << "   target_url is " << (target_url->is_trusted() ? "" : "NOT ") << "trusted." << endl;
            if (debug)
                cerr << prolog << "effective_url is " << (effective_url->is_trusted() ? "" : "NOT ") << "trusted."
                     << endl;
            CPPUNIT_ASSERT(effective_url->is_trusted() == target_url->is_trusted());


            effective_url = curl::retrieve_effective_url(trusted_target_url);

            if (debug) cerr << prolog << "effective_url: " << effective_url->str() << endl;
            if (debug) cerr << prolog << " expected_url: " << expected_url << endl;
            CPPUNIT_ASSERT(effective_url->str() == expected_url);

            if (debug)
                cerr << prolog << "   target_url is " << (trusted_target_url->is_trusted() ? "" : "NOT ") << "trusted."
                     << endl;
            if (debug)
                cerr << prolog << "effective_url is " << (effective_url->is_trusted() ? "" : "NOT ") << "trusted."
                     << endl;
            CPPUNIT_ASSERT(effective_url->is_trusted() == trusted_target_url->is_trusted());


        }
        catch (const BESError &be) {
            stringstream msg;
            msg << "Caught BESError! Message: " << be.get_message() << " file: " << be.get_file() << " line: "
                << be.get_line() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch (const std::exception &se) {
            stringstream msg;
            msg << "CAUGHT std::exception message: " << se.what() << endl;
            cerr << msg.str();
            CPPUNIT_FAIL(msg.str());
        }
        if (debug) cerr << prolog << "END" << endl;
    }

    void add_edl_auth_headers_test() {
        if (debug) cerr << prolog << "BEGIN" << endl;
        curl_slist *hdrs = NULL;
        curl_slist *temp = NULL;
        string tokens[] = {"big_bucky_ball", "itsa_authy_token_time", "echo_my_smo:kin_token"};
        BESContextManager::TheManager()->set_context(EDL_UID_KEY, tokens[0]);
        BESContextManager::TheManager()->set_context(EDL_AUTH_TOKEN_KEY, tokens[1]);
        BESContextManager::TheManager()->set_context(EDL_ECHO_TOKEN_KEY, tokens[2]);

        try {
            hdrs = curl::add_edl_auth_headers(hdrs);
            temp = hdrs;
            size_t index = 0;
            while (temp) {
                string value(temp->data);
                if (debug) cerr << prolog << "header: " << value << endl;
                size_t found = value.find(tokens[index]);
                CPPUNIT_ASSERT(found != string::npos);
                temp = temp->next;
                index++;
            }
        }
        catch (const BESError &be) {
            stringstream msg;
            msg << "Caught BESError! Message: " << be.get_message() << " file: " << be.get_file() << " line: "
                << be.get_line() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch (const std::exception &se) {
            stringstream msg;
            msg << "CAUGHT std::exception message: " << se.what() << endl;
            cerr << msg.str();
            CPPUNIT_FAIL(msg.str());
        }
        // The BESContexts are 'unset' in tearDown(). They break some later
        // tests, causing BESForbiddenErrors to become BESSyntaxUserErrors. jhrg 11/3/22
        if (debug) cerr << prolog << "END" << endl;
    }

    // A case where signing works
    void sign_s3_url_test_1() {
        shared_ptr<http::url> target_url(new http::url("http://test.opendap.org/opendap", false));
        AccessCredentials ac;
        ac.add(AccessCredentials::ID_KEY, "foo");
        ac.add(AccessCredentials::KEY_KEY, "secret");
        ac.add(AccessCredentials::REGION_KEY, "oz-1");
        ac.add(AccessCredentials::URL_KEY, "http://test.opendap.org");

        // TODO See if the following unique_ptr really does not leak memory. jhrg 11/3//22
        std::unique_ptr<curl_slist, void (*)(curl_slist *)> headers2(new curl_slist(), &curl_slist_free_all);

        CPPUNIT_ASSERT_MESSAGE("Before calling sign_s3_url, headers should be empty", headers2->next == nullptr);
        curl_slist *new_headers = curl::sign_s3_url(target_url, &ac, headers2.get());

        CPPUNIT_ASSERT_MESSAGE("Afterward, it should have three headers", new_headers->next != nullptr);
        // skip the first element since the data will be NULL given that we passed in
        // an empty list.
        new_headers = new_headers->next;
        string h = new_headers->data;
        DBG(cerr << "new_headers->data: " << h << endl);
        CPPUNIT_ASSERT_MESSAGE("Expected Authorization: AWS4-HMAC-SHA256 Credential=foo/...",
                               h.find("Authorization: AWS4-HMAC-SHA256 Credential=foo/") != string::npos);

        new_headers = new_headers->next;
        h = new_headers->data;
        DBG(cerr << "new_headers->data: " << h << endl);
        CPPUNIT_ASSERT_MESSAGE("Expected x-amz-content-sha256: e3b0c4...",
                               h.find("x-amz-content-sha256: e3b0c4") != string::npos);

        new_headers = new_headers->next;
        h = new_headers->data;
        DBG(cerr << "new_headers->data: " << h << endl);
        CPPUNIT_ASSERT_MESSAGE("Expected x-amz-date:...", h.find("x-amz-date:") != string::npos);

        new_headers = new_headers->next;
        CPPUNIT_ASSERT_MESSAGE("There should only be three elements in the list", new_headers == nullptr);
    }

    // We have credentials, but the target url doesn't match the URL_KEY
    void sign_s3_url_test_2() {
        shared_ptr<http::url> target_url(new http::url("http://test.opendap.org/opendap", false));
        AccessCredentials ac;
        ac.add(AccessCredentials::ID_KEY, "foo");
        ac.add(AccessCredentials::KEY_KEY, "secret");
        ac.add(AccessCredentials::REGION_KEY, "oz-1");
        ac.add(AccessCredentials::URL_KEY, "http://never.org");
        auto headers = new curl_slist{};
        try {
            CPPUNIT_ASSERT_MESSAGE("Before calling sign_s3_url, headers should be empty", headers->next == nullptr);
            const curl_slist *new_headers = curl::sign_s3_url(target_url, &ac, headers);

            CPPUNIT_ASSERT_MESSAGE("For this test, there should be nothing", new_headers->next != nullptr);
        }
        catch (...) {
            curl_slist_free_all(headers);
            throw;
        }

        curl_slist_free_all(headers);
    }

    // The credentials are empty
    void sign_s3_url_test_3() {
        shared_ptr<http::url> target_url(new http::url("http://test.opendap.org/opendap", false));
        AccessCredentials ac;
        auto headers = new curl_slist{};
        try {
            CPPUNIT_ASSERT_MESSAGE("Before calling sign_s3_url, headers should be empty", headers->next == nullptr);
            const curl_slist *new_headers = curl::sign_s3_url(target_url, &ac, headers);

            CPPUNIT_ASSERT_MESSAGE("For this test, there should be nothing", new_headers->next != nullptr);
        }
        catch (...) {
            curl_slist_free_all(headers);
            throw;
        }

        curl_slist_free_all(headers);
    }

    // Test the http_get() function that extends as needed a vector<char>
    void http_get_test_vector_char() {
        const string url = "http://test.opendap.org/opendap.conf";
        vector<char> buf;
        curl::http_get(url, buf);

        DBG(cerr << "buf.data() = " << string(buf.data()) << endl);
        CPPUNIT_ASSERT_MESSAGE("Should be able to find <Proxy *>", string(buf.data()).find("<Proxy *>") == 0);
        CPPUNIT_ASSERT_MESSAGE("Should be able to find ProxyPassReverse...",
                               string(buf.data()).find("ProxyPassReverse /dap ajp://localhost:8009/opendap") !=
                               string::npos);
        DBG(cerr << "buf.size() = " << buf.size() << endl);
        CPPUNIT_ASSERT_MESSAGE("Size should be 288", buf.size() == 288);
    }

    // Test the http_get() function that extends as needed a C++ std::string
    void http_get_test_string() {
        const string url = "http://test.opendap.org/opendap.conf";
        string str;
        curl::http_get(url, str);

        DBG(cerr << "str.data() = " << string(str.data()) << endl);
        CPPUNIT_ASSERT_MESSAGE("Should be able to find <Proxy *>", string(str.data()).find("<Proxy *>") == 0);
        CPPUNIT_ASSERT_MESSAGE("Should be able to find ProxyPassReverse...",
                               string(str.data()).find("ProxyPassReverse /dap ajp://localhost:8009/opendap") !=
                               string::npos);
        DBG(cerr << "str.size() = " << str.size() << endl);
        CPPUNIT_ASSERT_MESSAGE("Size should be 288", str.size() == 288);
    }

    // Test the http_get() function that extends as needed a vector<char>.
    // This what happens if the vector already holds data - it should be
    // retained.
    void http_get_test_vector_char_appended() {
        const string url = "http://test.opendap.org/opendap.conf";
        vector<char> buf;
        const string twimc = "To whom it may concern:";
        buf.resize(twimc.size());
        memcpy(buf.data(), twimc.c_str(), twimc.size());
        curl::http_get(url, buf);

        DBG(cerr << "buf.data() = " << string(buf.data()) << endl);
        CPPUNIT_ASSERT_MESSAGE("Should be able to find <Proxy *>",
                               string(buf.data()).find("<Proxy *>") == twimc.size());
        CPPUNIT_ASSERT_MESSAGE("Should be able to find ProxyPassReverse...",
                               string(buf.data()).find("ProxyPassReverse /dap ajp://localhost:8009/opendap") !=
                               string::npos);

        DBG(cerr << "twimc.size() = " << twimc.size() << endl);
        DBG(cerr << "buf.size() = " << buf.size() << endl);
        CPPUNIT_ASSERT_MESSAGE("Size should be 288", buf.size() == 288 + twimc.size());
    }
    void http_get_test_string_appended() {
        const string url = "http://test.opendap.org/opendap.conf";
        vector<char> str;
        const string twimc = "To whom it may concern:";
        str.resize(twimc.size());
        memcpy(str.data(), twimc.c_str(), twimc.size());
        curl::http_get(url, str);

        DBG(cerr << "str.data() = " << string(str.data()) << endl);
        CPPUNIT_ASSERT_MESSAGE("Should be able to find <Proxy *>",
                               string(str.data()).find("<Proxy *>") == twimc.size());
        CPPUNIT_ASSERT_MESSAGE("Should be able to find ProxyPassReverse...",
                               string(str.data()).find("ProxyPassReverse /dap ajp://localhost:8009/opendap") !=
                               string::npos);

        DBG(cerr << "twimc.size() = " << twimc.size() << endl);
        DBG(cerr << "str.size() = " << str.size() << endl);
        CPPUNIT_ASSERT_MESSAGE("Size should be 288", str.size() == 288 + twimc.size());
    }

    // This test is to an S3 bucket and must be signed. Use the ENV_CRED
    // option of CredentialsManager. The environment variables are:
    //
    // When a bes.conf file includes the key "CredentialsManager.config" and the value
    // is "ENV_CREDS", the CredentialsManager will use the four env variables CMAC_ID, ...,
    // for the Id, Secret key, etc., needed for S3 URL signing. The four evn vars are:
    // CMAC_ID
    // CMAC_ACCESS_KEY
    // CMAC_REGION
    // CMAC_URL
    //
    // This test will read from the cloudydap bucket we own.

    void http_get_test_4() {
        const string url = "https://fail.nowhere.com/README";
        vector<char> buf;
        curl::http_get(url, buf);

        CPPUNIT_FAIL("Should have thrown an exception.");
    }

    // This test will fail with a BESForbidden exception
    void http_get_test_5() {
        const string url = "https://s3.us-east-1.amazonaws.com/cloudydap/samples/README";
        vector<char> buf;
        curl::http_get(url, buf);

        CPPUNIT_FAIL("Should have thrown an exception.");
    }

    // This test will also fail with a BESForbidden exception
    void http_get_test_6() {
        setenv("CMAC_URL", "https://s3.us-east-1", 1);
        setenv("CMAC_REGION", "us-east-1", 1);
        const string url = "https://s3.us-east-1.amazonaws.com/cloudydap/samples/README";
        vector<char> buf;
        curl::http_get(url, buf);

        CPPUNIT_FAIL("Should have thrown an exception.");
    }

    void http_get_test_7() {
        setenv("CMAC_URL", "https://s3.us-east-1", 1);
        setenv("CMAC_REGION", "us-east-1", 1);
        // If the ID and Secret key are set in the shell/environment where this test
        // is run, then this should work. Never ever set those values in any file.
        // The Keys are: CMAC_ID, CMAC_ACCESS_KEY.
        if (getenv("CMAC_ID") && getenv("CMAC_ACCESS_KEY")) {
            try {
                auto cm = http::CredentialsManager::theCM();
                cm->load_credentials();

                const string url = "https://s3.us-east-1.amazonaws.com/cloudydap/samples/README";
                vector<char> buf;
                curl::http_get(url, buf);
                DBG(cerr << "buf.data() = " << string(buf.data()) << endl);
                CPPUNIT_ASSERT_MESSAGE("Should be able to find 'Test data''",
                                       string(buf.data()).find("Test data") == 0);
                CPPUNIT_ASSERT_MESSAGE("Should be able to find 'Do not edit.''",
                                       string(buf.data()).find("Do not edit.")
                                       != string::npos);

                DBG(cerr << "buf.size() = " << buf.size() << endl);
                CPPUNIT_ASSERT_MESSAGE("Size should be 94", buf.size() == 94);
            }
            catch(const BESError &e) {
                CPPUNIT_FAIL(string("Did not sign the URL correctly. ").append(e.get_verbose_message()));
            }
        }
        else {
            CPPUNIT_ASSERT("Credentials are not set, so the test passes by default.");
        }
    }

    // Tests expected redirect location.
    void get_redirect_url_00() {

        string source_url_str("http://test.opendap.org/opendap");
        string baseline_str("http://test.opendap.org/opendap/"); // note trailing slash

        DBG( cerr << prolog << "  source_url_str: " << source_url_str << "\n");

        shared_ptr<http::url> source_url(new http::url(source_url_str.c_str(), true));

        string redirect_url_str;
        auto http_status = curl::get_redirect_url(source_url, redirect_url_str);

        DBG( cerr << prolog << "     http_status: " << http_status << "\n");
        DBG( cerr << prolog << "    baseline_str: " << baseline_str << "\n");
        DBG( cerr << prolog << "redirect_url_str: " << redirect_url_str << "\n");
        CPPUNIT_ASSERT( !redirect_url_str.empty() );
        CPPUNIT_ASSERT( redirect_url_str == baseline_str );

    }

    // Tests no redirect location.
    void get_redirect_url_01() {

        string source_url_str("http://test.opendap.org/opendap/");

        DBG( cerr << prolog << "  source_url_str: " << source_url_str << "\n");
        shared_ptr<http::url> source_url(new http::url(source_url_str.c_str(), true));

        string redirect_url_str;
        try {
            auto http_status = curl::get_redirect_url(source_url,redirect_url_str);
            DBG( cerr << prolog << "     http_status: " << http_status << "\n");
            DBG( cerr << prolog << "redirect_url_str: " << redirect_url_str << "\n");
            CPPUNIT_FAIL("A BESInternalError should have been thrown.");
        }
        catch(BESInternalError bie){
            DBG(cerr << prolog << "curl::get_redirect_url() was redirected to the EDL login endpoint.\n");
            DBG(cerr << prolog << "Caught expected BESInternalError. Message:\n" << bie.get_verbose_message() << "\n");
        }

    }

    // Tests TEA, no auth
    void get_redirect_url_02() {

        string source_url_str("https://data.ornldaac.earthdata.nasa.gov/protected/daymet"
                              "/Daymet_Daily_V4R1/data/daymet_v4_daily_hi_prcp_2022.nc");

        DBG( cerr << prolog << "  source_url_str: " << source_url_str << "\n");

        string baseline("https://urs.earthdata.nasa.gov/oauth/authorize");
        DBG( cerr << prolog << "        baseline: " << baseline << "\n");

        shared_ptr<http::url> source_url(new http::url(source_url_str.c_str(), true));

        string redirect_url_str;
        try {
            auto http_status = curl::get_redirect_url(source_url, redirect_url_str);
            DBG( cerr << prolog << "     http_status: " << http_status << "\n");
            DBG( cerr << prolog << "redirect_url_str: " << redirect_url_str << "\n");
            CPPUNIT_FAIL("A BESInternalError should have been thrown.");
        }
        catch(BESInternalError bie){
            DBG(cerr << prolog << "curl::get_redirect_url() was redirected to the EDL login endpoint.\n");
            DBG(cerr << prolog << "Caught expected BESInternalError. Message:\n" << bie.get_verbose_message() << "\n");
        }

        // does the redirect_url_str start with the baseline??
        CPPUNIT_ASSERT(redirect_url_str.rfind(baseline, 0) == 0);


        //CPPUNIT_ASSERT( redirect_url_str.empty() );

    }

    // Tests TEA, good auth
    void get_redirect_url_03() {

        string source_url_str("https://data.ornldaac.earthdata.nasa.gov/protected/daymet"
                              "/Daymet_Daily_V4R1/data/daymet_v4_daily_hi_prcp_2022.nc");
        shared_ptr<http::url> source_url(new http::url(source_url_str.c_str(), true));

        DBG( cerr << prolog << "      source_url: " << source_url->str() << "\n");

        string baseline("https://d3o6w55j8uz1ro.cloudfront.net");
        DBG( cerr << prolog << "        baseline: " << baseline << "\n");

        auto edl_user = getenv("edl_user");
        auto edl_token_type = getenv("edl_token_type");
        auto edl_token = getenv("edl_token");
        if(edl_user && edl_token_type && edl_token){
            string auth_token(edl_token_type);
            auth_token.append(" ").append(edl_token);
            string tokens[] = {edl_user,
                               auth_token,
                               edl_token};
            BESContextManager::TheManager()->set_context(EDL_UID_KEY, tokens[0]);
            BESContextManager::TheManager()->set_context(EDL_AUTH_TOKEN_KEY, tokens[1]);
            BESContextManager::TheManager()->set_context(EDL_ECHO_TOKEN_KEY, tokens[2]);

            string redirect_url_str;

            auto http_status = curl::get_redirect_url(source_url, redirect_url_str);
            DBG( cerr << prolog << "     http_status: " << http_status << "\n");
            DBG( cerr << prolog << "redirect_url_str: " << redirect_url_str << "\n");

            // does the redirect_url_str start with the baseline??
            CPPUNIT_ASSERT(redirect_url_str.rfind(baseline, 0) == 0);

        }
        else {
            DBG( cerr << prolog << "Incomplete EDL authentication credentials. Status:\n" <<
                      "        edl_user: " << (edl_user?edl_user:"<missing>") << "\n" <<
                      "  edl_token_type: " << (edl_token_type?edl_token_type:"<missing>") << "\n" <<
                      "       edl_token: " << (edl_token?edl_token:"<missing>") << "\n"
            );
        }

    }
    // Tests TEA, bad auth
    void get_redirect_url_04() {

        string source_url_str("https://data.ornldaac.earthdata.nasa.gov/protected/daymet"
                              "/Daymet_Daily_V4R1/data/daymet_v4_daily_hi_prcp_2022.nc");
        shared_ptr<http::url> source_url(new http::url(source_url_str.c_str(), true));

        DBG( cerr << prolog << "      source_url: " << source_url->str() << "\n");

        string baseline("https://d3o6w55j8uz1ro.cloudfront.net");
        DBG( cerr << prolog << "        baseline: " << baseline << "\n");

        auto edl_user = "hard-times-charlie";
        auto edl_token_type = "Bearer";
        auto edl_token = "this-is-so-not-a-edl-valid-token";
        if(edl_user && edl_token_type && edl_token){
            string auth_token(edl_token_type);
            auth_token.append(" ").append(edl_token);
            string tokens[] = {edl_user,
                               auth_token,
                               edl_token};
            BESContextManager::TheManager()->set_context(EDL_UID_KEY, tokens[0]);
            BESContextManager::TheManager()->set_context(EDL_AUTH_TOKEN_KEY, tokens[1]);
            BESContextManager::TheManager()->set_context(EDL_ECHO_TOKEN_KEY, tokens[2]);

            string redirect_url_str;

            try {
                BESStopWatch sw;
                sw.start(prolog);
                auto http_status = curl::get_redirect_url(source_url, redirect_url_str);
                CPPUNIT_FAIL("The call to curl::get_redirect_url() should have thrown a "
                             "BESInternalError! http_status: " + to_string(http_status));
            }
            catch(BESInternalError bie){
                DBG(cerr << prolog << "curl::get_redirect_url() was redirected to the EDL login endpoint.\n");
                DBG(cerr << prolog << "Caught expected BESInternalError. Message:\n" << bie.get_verbose_message() << "\n");
            }

        }
        else {
            DBG( cerr << prolog << "Incomplete EDL authentication credentials. Status:\n" <<
                      "        edl_user: " << (edl_user?edl_user:"<missing>") << "\n" <<
                      "  edl_token_type: " << (edl_token_type?edl_token_type:"<missing>") << "\n" <<
                      "       edl_token: " << (edl_token?edl_token:"<missing>") << "\n"
            );
        }

    }

    void time_redirect_url_and_effective_url() {

        DBG( cout << endl);

        string source_url_str("https://data.ornldaac.earthdata.nasa.gov/protected/daymet"
                              "/Daymet_Daily_V4R1/data/daymet_v4_daily_hi_prcp_2022.nc");
        shared_ptr<http::url> source_url(new http::url(source_url_str.c_str(), true));

        DBG( cerr << prolog << "      source_url: " << source_url->str() << "\n");

        string baseline("https://d3o6w55j8uz1ro.cloudfront.net");
        DBG( cerr << prolog << "        baseline: " << baseline << "\n");

        auto edl_user = getenv("edl_user");
        auto edl_token_type = getenv("edl_token_type");
        auto edl_token = getenv("edl_token");
        if(edl_user && edl_token_type && edl_token){
            string auth_token(edl_token_type);
            auth_token.append(" ").append(edl_token);
            string tokens[] = {edl_user,
                               auth_token,
                               edl_token};
            BESContextManager::TheManager()->set_context(EDL_UID_KEY, tokens[0]);
            BESContextManager::TheManager()->set_context(EDL_AUTH_TOKEN_KEY, tokens[1]);
            BESContextManager::TheManager()->set_context(EDL_ECHO_TOKEN_KEY, tokens[2]);
            BESDebug::SetUp("cerr,moogoo");
            // The results...
            string redirect_url_str;
            shared_ptr<EffectiveUrl> effective_url;

            // @TODO The first request always takes and oddly long time, we should profile this to see why.
            {
                // We warm up the test by making a first request - this always takes much longer than any
                // subsequent request. Like 3,303,337 us for first and vs 641,921 us for second.
                BESStopWatch sw;
                DBG( sw.start("WARMUP - CurlUtilsTest calling curl::retrieve_effective_url()") );
                effective_url = curl::retrieve_effective_url(source_url);
                DBG(cerr << prolog << "   effective_url: " << effective_url->str() << "\n");
                // does the effective_url start with the baseline??
                CPPUNIT_ASSERT(effective_url->str().rfind(baseline, 0) == 0);
            }

            unsigned int reps = 10;
            for (int i=0; i<reps ;i++)
            {
                {
                    BESStopWatch sw;
                    DBG( sw.start("CurlUtilsTest calling curl::retrieve_effective_url() - " + to_string(i)) );
                    effective_url = curl::retrieve_effective_url(source_url);
                    DBG(cerr << prolog << "   effective_url: " << effective_url->str() << "\n");
                    // does the effective_url start with the baseline??
                    CPPUNIT_ASSERT(effective_url->str().rfind(baseline, 0) == 0);
                }
                {

                    BESStopWatch sw;
                    DBG( sw.start("CurlUtilsTest calling curl::get_redirect_url() - " + to_string(i)) );
                    auto http_status = curl::get_redirect_url(source_url, redirect_url_str);
                    DBG(cerr << prolog << "     http_status: " << http_status << "\n");
                    DBG(cerr << prolog << "redirect_url_str: " << redirect_url_str << "\n");
                    // does the redirect_url_str start with the baseline??
                    CPPUNIT_ASSERT(redirect_url_str.rfind(baseline, 0) == 0);
                }
            }
        }
        else {
            DBG( cerr << prolog << "Incomplete EDL authentication credentials.\n");
            DBG( cerr << prolog << "        edl_user: " << (edl_user?edl_user:"<missing>") << "\n");
            DBG( cerr << prolog << "  edl_token_type: " << (edl_token_type?edl_token_type:"<missing>") << "\n");
            DBG( cerr << prolog << "       edl_token: " << (edl_token?edl_token:"<missing>") << "\n");
        }
    }

/* TESTS END */
/*##################################################################################################*/

    CPPUNIT_TEST_SUITE(CurlUtilsTest);


        CPPUNIT_TEST(get_redirect_url_00);
        CPPUNIT_TEST(get_redirect_url_01);
        CPPUNIT_TEST(get_redirect_url_02);
        CPPUNIT_TEST(get_redirect_url_03);
        CPPUNIT_TEST(get_redirect_url_04);
        CPPUNIT_TEST(time_redirect_url_and_effective_url);


    CPPUNIT_TEST(is_retryable_test);
    CPPUNIT_TEST(retrieve_effective_url_test);
    CPPUNIT_TEST(add_edl_auth_headers_test);

    CPPUNIT_TEST(filter_effective_url_test);
    CPPUNIT_TEST(filter_effective_url_token_first_test);
    CPPUNIT_TEST(filter_effective_url_no_qs_test);

    CPPUNIT_TEST(sign_s3_url_test_1);
    CPPUNIT_TEST(sign_s3_url_test_2);
    CPPUNIT_TEST(sign_s3_url_test_3);

    CPPUNIT_TEST(http_get_test_vector_char);
    CPPUNIT_TEST(http_get_test_string);
    CPPUNIT_TEST(http_get_test_vector_char_appended);
    CPPUNIT_TEST(http_get_test_string_appended);

    CPPUNIT_TEST_EXCEPTION(http_get_test_4, BESInternalError);
    CPPUNIT_TEST_EXCEPTION(http_get_test_5, BESForbiddenError);
    CPPUNIT_TEST_EXCEPTION(http_get_test_6, BESForbiddenError);

    CPPUNIT_TEST(http_get_test_7);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(CurlUtilsTest);

} // namespace http

int main(int argc, char *argv[]) {
    return bes_run_tests<http::CurlUtilsTest>(argc, argv, "cerr,bes,http,curl,curl:timing") ? 0 : 1;
}
