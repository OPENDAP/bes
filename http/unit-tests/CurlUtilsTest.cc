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

#include "BESError.h"
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
#include "CurlUtils.h"
#include "HttpError.h"

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
        DBG(cerr << "\n");
        DBG(cerr << prolog << "#-----------------------------------------------------------------\n");
        DBG(cerr << prolog << "BEGIN\n");

        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR, "bes.conf");
        DBG(cerr << prolog << "Using BES configuration: " << bes_conf << "\n");
        DBG2(show_file(bes_conf));
        TheBESKeys::ConfigFile = bes_conf;

        DBG(cerr << prolog << "END\n");
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

    void test_is_url_signed_for_s3_WithSignedUrl() {
        // Test a URL that contains all the required S3 signature parameters
        std::string signedUrl = "https://example.com/myfile?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=some-credential&X-Amz-Signature=abc123";
        CPPUNIT_ASSERT(curl::is_url_signed_for_s3(signedUrl));
    }

    void test_is_url_signed_for_s3_with_one_key_only() {
        // Test a URL that contains all the required S3 signature parameters
        std::string signedUrl = "https://example.com/myfile?X-Amz-Algorithm=AWS4-HMAC-SHA256";
        CPPUNIT_ASSERT(!curl::is_url_signed_for_s3(signedUrl));
    }

    void test_is_url_signed_for_s3_WithoutSignedUrl() {
        // Test a URL that does not contain any S3 signature parameters
        std::string unsignedUrl = "https://example.com/myfile";
        CPPUNIT_ASSERT(!curl::is_url_signed_for_s3(unsignedUrl));
    }

    void test_is_url_signed_for_s3_EmptyUrl() {
        // Test an empty URL string
        std::string emptyUrl = "";
        CPPUNIT_ASSERT(!curl::is_url_signed_for_s3(emptyUrl));
    }

    void test_is_url_signed_for_s3_PartialMatch() {
        // Test a URL with similar parameters but not exact S3 signature keys
        std::string partialMatchUrl = "https://example.com/myfile?X-Amz-InvalidParam=test";
        CPPUNIT_ASSERT(!curl::is_url_signed_for_s3(partialMatchUrl));
    }

    // This does not test if the buffer (arg #2) is corrupted, If that's the case error_message()
    // response is not reliable. jhrg 11/22/24
    void test_error_message_with_buffer()
    {
        // Test with a non-empty error buffer
        CPPUNIT_ASSERT_EQUAL(
                std::string(
                "cURL_error_buffer: Test error buffer, cURL_message: Couldn't resolve host name (code: 6)\n"),
                curl::error_message(CURLE_COULDNT_RESOLVE_HOST, "Test error buffer")
        );
    }
    void test_error_message_without_buffer()
    {
        // Test with an empty error buffer
        CPPUNIT_ASSERT_EQUAL(
                std::string("cURL_message: Couldn't resolve host name (code: 6)\n"),
                curl::error_message(CURLE_COULDNT_RESOLVE_HOST, nullptr)
        );
    }

    void is_retryable_test() {
        DBG(cerr << prolog << "BEGIN\n");
        bool isRetryable;

        try {
            string url = "http://test.opendap.org/data/httpd_catalog/READTHIS";
            isRetryable = curl::is_retryable(url);
            DBG(cerr << prolog << "is_retryable('" << url << "'): " << (isRetryable ? "true" : "false") << "\n");
            CPPUNIT_ASSERT(isRetryable);

            url = "https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20031229v7.nc?A-userid=hyrax&X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=ASIASF4N-AWS-Creds-00808%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20200808T032623Z&X-Amz-Expires=86400&X-Amz-Security-Token=FwoGZXIvYXdzE-AWS-Sec-Token-MWRLIZGYvDx1ONzd0ffK8VtxO8JP7thrGIQ%3D%3D&X-Amz-SignedHeaders=host&X-Amz-Signature=260a7c4dd4-AWS-SIGGY-0c7a39ee899";
            isRetryable = curl::is_retryable(url);
            DBG(cerr << prolog << "is_retryable('" << url << "'): " << (isRetryable ? "true" : "false") << "\n");
            CPPUNIT_ASSERT(!isRetryable);

            url = "https://d1jecqxxv88lkr.cloudfront.net/ghrcwuat-protected/rss_demo/rssmif16d__7/f16_ssmis_20040107v7.nc";
            isRetryable = curl::is_retryable(url);
            DBG(cerr << prolog << "is_retryable('" << url << "'): " << (isRetryable ? "true" : "false") << "\n");
            CPPUNIT_ASSERT(isRetryable);

            url = "https://data.ghrc.uat.earthdata.nasa.gov/login?code=8800da07f823dfce312ee85e44c9e89efdf6bd9d776b1cb8666029ba2c8d257e&state=%2Fghrcwuat%2Dprotected%2Frss_demo%2Frssmif16d__7%2Ff16_ssmis_20040107v7%2Enc";
            isRetryable = curl::is_retryable(url);
            DBG(cerr << prolog << "is_retryable('" << url << "'): " << (isRetryable ? "true" : "false") << "\n");
            CPPUNIT_ASSERT(!isRetryable);

            url = "https://data.ghrc.uat.earthdata.nasa.gov/login?code=46196589bfe26c4c298e1a74646b99005d20a022cabff6434a550283defa8153&state=%2Fghrcwuat%2Dprotected%2Frss_demo%2Frssmif16d__7%2Ff16_ssmis_20040115v7%2Enc";
            isRetryable = curl::is_retryable(url);
            DBG(cerr << prolog << "is_retryable('" << url << "'): " << (isRetryable ? "true" : "false") << "\n");
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
        DBG(cerr << prolog << "END\n");
    }

    void filter_effective_url_test() {
        DBG(cerr << prolog << "BEGIN\n");
        string url = "https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20031229v7.nc?A-userid=hyrax&X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=ASIASF4N-AWS-Creds-00808%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20200808T032623Z&X-Amz-Expires=86400&X-Amz-Security-Token=Foo&X-Amz-SignedHeaders=host&X-Amz-Signature=...";
        string filtered_url = "https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20031229v7.nc?A-userid=hyrax";
        CPPUNIT_ASSERT_MESSAGE("The URL should have the AWS security tokens removed",
                               filtered_url == curl::filter_aws_url(url));
        DBG(cerr << prolog << "END\n");
    }

    void filter_effective_url_token_first_test() {
        DBG(cerr << prolog << "BEGIN\n");
        string url = "https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20031229v7.nc?X-Amz-Security-Token=Foo&A-userid=hyrax&X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=ASIASF4N-AWS-Creds-00808%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20200808T032623Z&X-Amz-Expires=86400&X-Amz-SignedHeaders=host&X-Amz-Signature=...";
        string filtered_url = "https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20031229v7.nc";
        CPPUNIT_ASSERT_MESSAGE("The URL should have the AWS security tokens removed",
                               filtered_url == curl::filter_aws_url(url));
        DBG(cerr << prolog << "END\n");
    }

    void filter_effective_url_no_qs_test() {
        DBG(cerr << prolog << "BEGIN\n");
        string url = "https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssmis_20031229v7.nc";
        CPPUNIT_ASSERT_MESSAGE("The URL has no query string and should not be changed",
                               url == curl::filter_aws_url(url));
        DBG(cerr << prolog << "END\n");
    }


    void retrieve_effective_url_test() {
        DBG(cerr << prolog << "BEGIN\n");
        shared_ptr<http::url> trusted_target_url(new http::url("http://test.opendap.org/opendap", true));
        shared_ptr<http::url> target_url(new http::url("http://test.opendap.org/opendap", false));
        string expected_url = "http://test.opendap.org/opendap/";

        try {
            DBG(cerr << prolog << "   target_url: " << target_url->str() << "\n");

            auto effective_url = curl::get_redirect_url(target_url);

            DBG(cerr << prolog << "effective_url: " << effective_url->str() << "\n");
            DBG(cerr << prolog << " expected_url: " << expected_url << "\n");
            CPPUNIT_ASSERT(effective_url->str() == expected_url);

            DBG(cerr << prolog << "   target_url is " << (target_url->is_trusted() ? "" : "NOT ") << "trusted."
                     << "\n");
            DBG(cerr << prolog << "effective_url is " << (effective_url->is_trusted() ? "" : "NOT ") << "trusted."
                     << "\n");
            CPPUNIT_ASSERT(effective_url->is_trusted() == target_url->is_trusted());


            effective_url = curl::get_redirect_url(trusted_target_url);

            DBG(cerr << prolog << "effective_url: " << effective_url->str() << "\n");
            DBG(cerr << prolog << " expected_url: " << expected_url << "\n");
            CPPUNIT_ASSERT(effective_url->str() == expected_url);

            DBG(cerr << prolog << "   target_url is " << (trusted_target_url->is_trusted() ? "" : "NOT ") << "trusted."
                     << "\n");
            DBG(cerr << prolog << "effective_url is " << (effective_url->is_trusted() ? "" : "NOT ") << "trusted."
                     << "\n");
            CPPUNIT_ASSERT(effective_url->is_trusted() == trusted_target_url->is_trusted());


        }
        catch (const BESError &be) {
            stringstream msg;
            msg << "Caught BESError! Message: " << be.get_message() << " file: " << be.get_file() << " line: "
                << be.get_line() << "\n";
            CPPUNIT_FAIL(msg.str());
        }
        catch (const std::exception &se) {
            stringstream msg;
            msg << "CAUGHT std::exception message: " << se.what() << "\n";
            cerr << msg.str();
            CPPUNIT_FAIL(msg.str());
        }
        DBG(cerr << prolog << "END\n");
    }

    void add_edl_auth_headers_test() {
        DBG(cerr << prolog << "BEGIN\n");
        curl_slist *hdrs = nullptr;
        curl_slist *sl_iter;
        string tokens[] = {"big_bucky_ball", "itsa_authy_token_time", "echo_my_smo:kin_token"};
        BESContextManager::TheManager()->set_context(EDL_UID_KEY, tokens[0]);
        BESContextManager::TheManager()->set_context(EDL_AUTH_TOKEN_KEY, tokens[1]);
        BESContextManager::TheManager()->set_context(EDL_ECHO_TOKEN_KEY, tokens[2]);

        try {
            hdrs = curl::add_edl_auth_headers(hdrs);
            sl_iter = hdrs;
            size_t index = 0;
            while (sl_iter) {
                string value(sl_iter->data);
                DBG(cerr << prolog << "header: " << value << "\n");
                size_t found = value.find(tokens[index]);
                CPPUNIT_ASSERT(found != string::npos);
                sl_iter = sl_iter->next;
                index++;
            }
            curl_slist_free_all(hdrs);
        }
        catch (const BESError &be) {
            curl_slist_free_all(hdrs);

            stringstream msg;
            msg << "Caught BESError! Message: " << be.get_message() << " file: " << be.get_file() << " line: "
                << be.get_line() << "\n";
            CPPUNIT_FAIL(msg.str());
        }
        catch (const std::exception &se) {
            curl_slist_free_all(hdrs);

            stringstream msg;
            msg << "CAUGHT std::exception message: " << se.what() << "\n";
            cerr << msg.str();
            CPPUNIT_FAIL(msg.str());
        }
        // The BESContexts are 'unset' in tearDown(). They break some later
        // tests, causing BESForbiddenErrors to become BESSyntaxUserErrors. jhrg 11/3/22
        DBG(cerr << prolog << "END\n");
    }

    // A case where signing works
    void sign_s3_url_test_1() {
        DBG(cerr << prolog << "BEGIN\n");
        string baseline;
        shared_ptr<http::url> target_url(new http::url("http://test.opendap.org/opendap", false));
        AccessCredentials ac;
        ac.add(AccessCredentials::ID_KEY, "foo");
        ac.add(AccessCredentials::KEY_KEY, "secret");
        ac.add(AccessCredentials::REGION_KEY, "oz-1");
        ac.add(AccessCredentials::URL_KEY, "http://test.opendap.org");

        vector<string> baselines = {
                "Authorization: AWS4-HMAC-SHA256 Credential=foo/",
                "x-amz-content-sha256: e3b0c4",
                "x-amz-date:"
        };

        curl_slist *request_headers = nullptr;
        DBG(cerr << prolog << "request_headers: " << (void *) request_headers << "\n");
        request_headers = curl::sign_s3_url(target_url, &ac, request_headers);
        DBG(cerr << prolog << "request_headers: " << (void *) request_headers << "\n");
        CPPUNIT_ASSERT_MESSAGE("The request headers should be not null.", request_headers != nullptr);

        auto request_hdr_itr = request_headers;
        try {
            size_t i = 0;
            size_t hc = 0;
            while (request_hdr_itr != nullptr || i < baselines.size()) {
                string hdr;
                if (i < baselines.size()) {
                    baseline = baselines[i];
                    DBG(cerr << prolog << "            baselines[" << i << "]: " << baseline << "\n");
                }
                if (request_hdr_itr) {
                    request_hdr_itr = request_hdr_itr->next;
                    //CPPUNIT_ASSERT_MESSAGE("The request headers should be not null", request_headers != nullptr);
                    if (request_hdr_itr) {
                        hdr = request_hdr_itr->data;
                        DBG(cerr << prolog << "request_headers->data[" << i << "]: " << hdr << "\n");
                        hc++;
                    }
                }
                if (request_hdr_itr != nullptr && i < baselines.size()) {
                    CPPUNIT_ASSERT_MESSAGE("Expected " + baseline, hdr.find(baseline) != string::npos);
                }
                i++;
            }
            CPPUNIT_ASSERT_MESSAGE(
                    "Header count and baselines should match. baselines: " + to_string(baselines.size()) +
                    " headers: " + to_string(hc), hc == baselines.size());
            curl_slist_free_all(request_headers);
        }
        catch (...) {
            curl_slist_free_all(request_headers);
        }
        DBG(cerr << prolog << "END\n");
    }

    // We have credentials, but the target url doesn't match the URL_KEY
    void sign_s3_url_test_2() {
        DBG(cerr << prolog << "BEGIN\n");
        shared_ptr<http::url> target_url(new http::url("http://test.opendap.org/opendap", false));
        AccessCredentials ac;
        ac.add(AccessCredentials::ID_KEY, "foo");
        ac.add(AccessCredentials::KEY_KEY, "secret");
        ac.add(AccessCredentials::REGION_KEY, "oz-1");
        ac.add(AccessCredentials::URL_KEY, "http://never.org");
        curl_slist *headers = nullptr;
        try {
            CPPUNIT_ASSERT_MESSAGE("Before calling sign_s3_url, headers should be nullptr", headers == nullptr);
            headers = curl::sign_s3_url(target_url, &ac, nullptr);
            DBG(cerr << prolog << "hdr_itr: " << (void *) headers << "\n");

            CPPUNIT_ASSERT_MESSAGE("For this test, there should be nothing", headers->next != nullptr);

            curl_slist_free_all(headers);

        }
        catch (...) {
            curl_slist_free_all(headers);
            throw;
        }
        DBG(cerr << prolog << "END\n");
    }

    // The credentials are empty
    void sign_s3_url_test_3() {
        DBG(cerr << prolog << "BEGIN\n");
        shared_ptr<http::url> target_url(new http::url("http://test.opendap.org/opendap", false));
        AccessCredentials ac;
        curl_slist *headers = nullptr;

        try {
            //CPPUNIT_ASSERT_MESSAGE("Before calling sign_s3_url, headers should be empty", headers->next == nullptr);
            headers = curl::sign_s3_url(target_url, &ac, nullptr);
            CPPUNIT_ASSERT_MESSAGE("For this test, there should be nothing", headers->next != nullptr);
            curl_slist_free_all(headers);
        }
        catch (...) {
            curl_slist_free_all(headers);
            throw;
        }
        DBG(cerr << prolog << "END\n");
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

    // This test will fail with a HttpError exception
    void http_get_test_4() {
        DBG(cerr << prolog << "BEGIN\n");
        const string url = "https://fail.nowhere.com/README";
        string buf;
        DBG(cerr << prolog << "Retrieving " << url << "\n");
        curl::http_get(url, buf);

        CPPUNIT_FAIL("Should have thrown an exception.");
    }

    // This test will fail with a HttpError exception
    void http_get_test_5() {
        DBG(cerr << prolog << "BEGIN\n");
        const string url = "https://s3.us-east-1.amazonaws.com/cloudydap/samples/README";
        string buf;
        DBG(cerr << prolog << "Retrieving " << url << "\n");
        curl::http_get(url, buf);

        CPPUNIT_FAIL("Should have thrown an exception.");
    }

    // This test will also fail with a HttpError exception
    void http_get_test_6() {
        DBG(cerr << prolog << "BEGIN\n");
        setenv("CMAC_URL", "https://s3.us-east-1", 1);
        setenv("CMAC_REGION", "us-east-1", 1);
        const string url = "https://s3.us-east-1.amazonaws.com/cloudydap/samples/README";
        string buf;
        DBG(cerr << prolog << "Retrieving " << url << "\n");
        curl::http_get(url, buf);

        CPPUNIT_FAIL("Should have thrown an exception.");
    }

    void http_get_test_7() {
        DBG(cerr << prolog << "BEGIN\n");
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
                string buf;
                DBG(cerr << prolog << "Retrieving " << url << "\n");
                curl::http_get(url, buf);
                DBG(cerr << "buf.data() = " << string(buf.data()) << "\n");
                CPPUNIT_ASSERT_MESSAGE("Should be able to find 'Test data''",
                                       string(buf.data()).find("Test data") == 0);
                CPPUNIT_ASSERT_MESSAGE("Should be able to find 'Do not edit.''",
                                       string(buf.data()).find("Do not edit.")
                                       != string::npos);

                DBG(cerr << "buf.size() = " << buf.size() << "\n");
                CPPUNIT_ASSERT_MESSAGE("Size should be 94", buf.size() == 94);
            }
            catch (const BESError &e) {
                CPPUNIT_FAIL(string("Did not sign the URL correctly. ").append(e.get_verbose_message()));
            }
        } else {
            DBG(cerr << prolog << "SKIPPING (Credentials are not set.)\n");
        }
        DBG(cerr << prolog << "END\n");
    }

    /**
     * Tests an expected redirect location
     */
    void get_redirect_url_test_expected_redirect() {
        DBG(cerr << prolog << "BEGIN\n");
        string source_url_str("http://test.opendap.org/opendap");
        string baseline_str("http://test.opendap.org/opendap/"); // note trailing slash

        DBG(cerr << prolog << "  source_url_str: " << source_url_str << "\n");

        shared_ptr<http::url> source_url(new http::url(source_url_str.c_str(), true));

        auto redirect_url = curl::get_redirect_url(source_url);

        DBG(cerr << prolog << "    baseline_str: " << baseline_str << "\n");
        DBG(cerr << prolog << "redirect_url: " << redirect_url->str() << "\n");
        CPPUNIT_ASSERT(!redirect_url->str().empty());
        CPPUNIT_ASSERT(redirect_url->str() == baseline_str);
        DBG(cerr << prolog << "END\n");
    }

    /**
     * Tests a no redirect location, i.e. one that returns an HTTP code 200 (OK)
     * and a response body of some sort.
     */
    void get_redirect_url_unexpected_ok() {
        DBG(cerr << prolog << "BEGIN\n");
        string source_url_str("http://test.opendap.org/opendap/hyrax/version");

        DBG(cerr << prolog << "  source_url_str: " << source_url_str << "\n");
        shared_ptr<http::url> source_url(new http::url(source_url_str, true));

        try {
            auto redirect_url = curl::get_redirect_url(source_url);
            DBG(cerr << prolog << "redirect_url: " << redirect_url->str() << "\n");
            CPPUNIT_FAIL("An HttpError should have been thrown.");
        }
        catch (HttpError &bie) {
            DBG(cerr << prolog << "curl::get_redirect_url() was NOT redirected. This is an expected error.\n");
            DBG(cerr << prolog << "Caught expected HttpError. \nError Message:\n\n" << bie.get_verbose_message()
                     << "\n");
        }
        DBG(cerr << prolog << "END\n");
    }

    /**
     *  Tests TEA redirect, no auth credentials
     */
    void get_redirect_url_test_tea_no_creds() {
        DBG(cerr << prolog << "BEGIN\n");
        string source_url_str("https://data.ornldaac.earthdata.nasa.gov/protected/daymet"
                              "/Daymet_Daily_V4R1/data/daymet_v4_daily_hi_prcp_2022.nc");

        DBG(cerr << prolog << "  source_url_str: " << source_url_str << "\n");

        string baseline("https://urs.earthdata.nasa.gov/oauth/authorize");
        DBG(cerr << prolog << "        baseline: " << baseline << "\n");

        shared_ptr<http::url> source_url(new http::url(source_url_str, true));

        try {
            auto redirect_url = curl::get_redirect_url(source_url);
            DBG(cerr << prolog << "redirect_url: " << redirect_url->str() << "\n");
            CPPUNIT_FAIL("A BESSyntaxUserError should have been thrown.");
        }
        catch (BESSyntaxUserError &bie) {
            DBG(cerr << prolog
                     << "curl::get_redirect_url() was redirected to the EDL login endpoint. This is an expected error.\n");
            DBG(cerr << prolog << "Caught expected BESSyntaxUserError.\nError Message:\n\n" << bie.get_verbose_message()
                     << "\n");
        }
        DBG(cerr << prolog << "END\n");
    }

    /**
     * Tests TEA, with good auth credentials
     */
    void get_redirect_url_test_tea_good_auth() {
        DBG(cerr << prolog << "BEGIN\n");
        string source_url_str("https://data.ornldaac.earthdata.nasa.gov/protected/daymet"
                              "/Daymet_Daily_V4R1/data/daymet_v4_daily_hi_prcp_2022.nc");
        shared_ptr<http::url> source_url(new http::url(source_url_str, true));

        DBG(cerr << prolog << "      source_url: " << source_url->str() << "\n");

        string baseline("https://d3o6w55j8uz1ro.cloudfront.net");
        DBG(cerr << prolog << "        baseline: " << baseline << "\n");

        auto edl_user = getenv("edl_user");
        auto edl_token_type = getenv("edl_token_type");
        auto edl_token = getenv("edl_token");
        if (edl_user && edl_token_type && edl_token) {
            string auth_token(edl_token_type);
            auth_token.append(" ").append(edl_token);
            string tokens[] = {edl_user,
                               auth_token,
                               edl_token};
            BESContextManager::TheManager()->set_context(EDL_UID_KEY, tokens[0]);
            BESContextManager::TheManager()->set_context(EDL_AUTH_TOKEN_KEY, tokens[1]);
            BESContextManager::TheManager()->set_context(EDL_ECHO_TOKEN_KEY, tokens[2]);

            auto redirect_url = curl::get_redirect_url(source_url);
            DBG(cerr << prolog << "redirect_url: " << redirect_url->str() << "\n");

            // does the redirect_url_str start with the baseline??
            CPPUNIT_ASSERT(redirect_url->str().rfind(baseline, 0) == 0);

        } else {
            DBG(cerr << prolog << "SKIPPING TEST - Incomplete EDL authentication credentials. Status:\n" <<
                     "        edl_user: " << (edl_user ? edl_user : "<missing>") << "\n" <<
                     "  edl_token_type: " << (edl_token_type ? edl_token_type : "<missing>") << "\n" <<
                     "       edl_token: " << (edl_token ? edl_token : "<missing>") << "\n"
            );
        }
        DBG(cerr << prolog << "END\n");
    }

    /**
     * Tests TEA, with BAD auth credentials
     */
    void get_redirect_url_test_tea_bad_auth() {
        DBG(cerr << prolog << "BEGIN\n");
        string source_url_str("https://data.ornldaac.earthdata.nasa.gov/protected/daymet"
                              "/Daymet_Daily_V4R1/data/daymet_v4_daily_hi_prcp_2022.nc");
        shared_ptr<http::url> source_url(new http::url(source_url_str.c_str(), true));

        DBG(cerr << prolog << "      source_url: " << source_url->str() << "\n");

        string baseline("https://d3o6w55j8uz1ro.cloudfront.net");
        DBG(cerr << prolog << "        baseline: " << baseline << "\n");

        auto edl_user = "hard-times-charlie";
        auto edl_token_type = "Bearer";
        auto edl_token = "this-is-so-not-a-edl-valid-token";
        if (edl_user && edl_token_type && edl_token) {
            string auth_token(edl_token_type);
            auth_token.append(" ").append(edl_token);
            string tokens[] = {edl_user,
                               auth_token,
                               edl_token};
            BESContextManager::TheManager()->set_context(EDL_UID_KEY, tokens[0]);
            BESContextManager::TheManager()->set_context(EDL_AUTH_TOKEN_KEY, tokens[1]);
            BESContextManager::TheManager()->set_context(EDL_ECHO_TOKEN_KEY, tokens[2]);

            try {
                BESStopWatch sw;
                sw.start(prolog);
                auto redirect_url = curl::get_redirect_url(source_url);
                CPPUNIT_FAIL("The call to curl::get_redirect_url() should have thrown a "
                             "BESSyntaxUserError!");
            }
            catch (BESSyntaxUserError &bie) {
                DBG(cerr << prolog << "curl::get_redirect_url() was redirected to the EDL login endpoint.\n");
                DBG(cerr << prolog << "Caught expected BESSyntaxUserError. Message:\n" << bie.get_verbose_message()
                         << "\n");
            }

        } else {
            DBG(cerr << prolog << "Incomplete EDL authentication credentials. Status:\n" <<
                     "        edl_user: " << (edl_user ? edl_user : "<missing>") << "\n" <<
                     "  edl_token_type: " << (edl_token_type ? edl_token_type : "<missing>") << "\n" <<
                     "       edl_token: " << (edl_token ? edl_token : "<missing>") << "\n"
            );
        }
        DBG(cerr << prolog << "END\n");
    }

    /**
     * Runs a timing comparison test on the new get_redirect_url(), which does not follow redirects,
     * and the old get_effective_utl(), which does follow redirects.
     */
    void time_redirect_url_and_effective_url() {
        DBG(cerr << prolog << "BEGIN\n");

        string source_url_str("https://data.ornldaac.earthdata.nasa.gov/protected/daymet"
                              "/Daymet_Daily_V4R1/data/daymet_v4_daily_hi_prcp_2022.nc");
        shared_ptr<http::url> source_url(new http::url(source_url_str.c_str(), true));

        DBG(cerr << prolog << "      source_url: " << source_url->str() << "\n");

        string baseline("https://d3o6w55j8uz1ro.cloudfront.net");
        DBG(cerr << prolog << "        baseline: " << baseline << "\n");

        auto edl_user = getenv("edl_user");
        auto edl_token_type = getenv("edl_token_type");
        auto edl_token = getenv("edl_token");
        if (edl_user && edl_token_type && edl_token) {
            string auth_token(edl_token_type);
            auth_token.append(" ").append(edl_token);
            string tokens[] = {edl_user,
                               auth_token,
                               edl_token};
            BESContextManager::TheManager()->set_context(EDL_UID_KEY, tokens[0]);
            BESContextManager::TheManager()->set_context(EDL_AUTH_TOKEN_KEY, tokens[1]);
            BESContextManager::TheManager()->set_context(EDL_ECHO_TOKEN_KEY, tokens[2]);

            // What matters here is that we assign cerr to the BESDebug output stream
            DBG(BESDebug::SetUp("cerr,DUMMY_KEY"));

            // The results...
            shared_ptr<EffectiveUrl> redirect_url;
            shared_ptr<EffectiveUrl> effective_url;

            // @TODO The first request always takes an oddly long time, we should profile this to see why.
            {
                // We warm up the test by making a first request - this always takes much longer than any
                // subsequent request. Like 3,303,337 us for first and vs 641,921 us for second.
                BESStopWatch sw;
                DBG(sw.start("WARMUP - CurlUtilsTest calling curl::retrieve_effective_url()"));
                effective_url = curl::get_redirect_url(source_url);
                DBG(cerr << prolog << "   effective_url: " << effective_url->str() << "\n");
                // does the effective_url start with the baseline??
                CPPUNIT_ASSERT(effective_url->str().rfind(baseline, 0) == 0);
            }
            unsigned int reps = 2;
            for (unsigned int i = 0; i < reps; i++) {
                {
                    BESStopWatch sw;
                    DBG(sw.start("CurlUtilsTest calling curl::retrieve_effective_url() - " + to_string(i)));
                    effective_url = curl::get_redirect_url(source_url);
                    DBG(cerr << prolog << "   effective_url: " << effective_url->str() << "\n");
                    // does the effective_url start with the baseline??
                    CPPUNIT_ASSERT(effective_url->str().rfind(baseline, 0) == 0);
                }
                {
                    BESStopWatch sw;
                    DBG(sw.start("CurlUtilsTest calling curl::get_redirect_url() - " + to_string(i)));
                    redirect_url = curl::get_redirect_url(source_url);
                    DBG(cerr << prolog << "redirect_url: " << redirect_url->str() << "\n");
                    // does the redirect_url_str start with the baseline??
                    CPPUNIT_ASSERT(redirect_url->str().rfind(baseline, 0) == 0);
                }
            }
        } else {
            DBG(cerr << prolog << "Incomplete EDL authentication credentials.\n");
            DBG(cerr << prolog << "        edl_user: " << (edl_user ? edl_user : "<missing>") << "\n");
            DBG(cerr << prolog << "  edl_token_type: " << (edl_token_type ? edl_token_type : "<missing>") << "\n");
            DBG(cerr << prolog << "       edl_token: " << (edl_token ? edl_token : "<missing>") << "\n");
        }
        DBG(cerr << prolog << "END\n");
    }

    void how_big() {
        DBG(cerr << prolog << "BEGIN\n");
        DBG(cerr << prolog << "        char: " << (sizeof(char) * 8) << " bits\n");
        DBG(cerr << prolog << "       short: " << (sizeof(short) * 8) << " bits\n");
        DBG(cerr << prolog << "         int: " << (sizeof(int) * 8) << " bits\n");
        DBG(cerr << prolog << "unsigned int: " << (sizeof(unsigned int) * 8) << " bits\n");
        DBG(cerr << prolog << "        long: " << (sizeof(long) * 8) << " bits\n");
        DBG(cerr << prolog << "   long long: " << (sizeof(long long) * 8) << " bits\n");
        DBG(cerr << prolog << "END\n");
        CPPUNIT_ASSERT("Sizes are sizes");
    }

    CPPUNIT_TEST_SUITE(CurlUtilsTest);

        CPPUNIT_TEST(test_is_url_signed_for_s3_WithSignedUrl);
        CPPUNIT_TEST(test_is_url_signed_for_s3_with_one_key_only);
        CPPUNIT_TEST(test_is_url_signed_for_s3_WithoutSignedUrl);
        CPPUNIT_TEST(test_is_url_signed_for_s3_EmptyUrl);
        CPPUNIT_TEST(test_is_url_signed_for_s3_PartialMatch);

        CPPUNIT_TEST(test_error_message_with_buffer);
        CPPUNIT_TEST(test_error_message_without_buffer);

        CPPUNIT_TEST(how_big);

        CPPUNIT_TEST(get_redirect_url_test_expected_redirect);
        CPPUNIT_TEST(get_redirect_url_unexpected_ok);
        CPPUNIT_TEST(get_redirect_url_test_tea_no_creds);
        CPPUNIT_TEST(get_redirect_url_test_tea_good_auth);
        CPPUNIT_TEST(get_redirect_url_test_tea_bad_auth);
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

        CPPUNIT_TEST_EXCEPTION(http_get_test_4, HttpError);
        CPPUNIT_TEST_EXCEPTION(http_get_test_5, HttpError);
        CPPUNIT_TEST_EXCEPTION(http_get_test_6, HttpError);

        CPPUNIT_TEST(http_get_test_7);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(CurlUtilsTest);

} // namespace http

int main(int argc, char *argv[]) {
    return bes_run_tests<http::CurlUtilsTest>(argc, argv, "cerr,bes,http,curl,curl:timing") ? 0 : 1;
}
