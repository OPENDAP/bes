// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES component of the Hyrax Data Server.

// Copyright (c) 2018 OPeNDAP, Inc.
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
#include <iostream>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <unistd.h>

#include "BESError.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESCatalogList.h"
#include "TheBESKeys.h"
#include "BESContextManager.h"

#include "HttpNames.h"
#include "url_impl.h"
#include "EffectiveUrl.h"
#include "SignedUrlCache.h"

#include "test_config.h"

using namespace std;

static bool debug = false;
static bool Debug = false;
static bool bes_debug = false;
static bool ngap_tests = false;
static std::string token;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)
#define prolog std::string("SignedUrlCacheTest::").append(__func__).append("() - ")

namespace http {

class SignedUrlCacheTest : public CppUnit::TestFixture {
private:
    string d_data_dir = TEST_DATA_DIR;

    void show_file(string filename) {
        ifstream t(filename.c_str());

        if (t.is_open()) {
            string file_content((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
            t.close();
            cerr << endl << "#############################################################################" << endl;
            cerr << "file: " << filename << endl;
            cerr << ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl;
            cerr << file_content << endl;
            cerr << "#############################################################################" << endl;
        } else {
            cerr << "FAILED TO OPEN FILE: " << filename << endl;
        }
    }

public:
    // Called once before everything gets tested
    SignedUrlCacheTest() = default;

    // Called at the end of the tests
    ~SignedUrlCacheTest() override = default;

    // Called before each test
    void setUp() override {
        DBG(cerr << endl);
        DBG(cerr << prolog << "BEGIN" << endl);
        DBG(cerr << prolog << "data_dir: " << d_data_dir << endl);
        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR, "bes.conf");
        DBG(cerr << prolog << "Using BES configuration: " << bes_conf << endl);
        if (Debug) show_file(bes_conf);
        TheBESKeys::ConfigFile = bes_conf;

        if (bes_debug) BESDebug::SetUp("cerr,bes,euc,http,curl");

        // Reset to same starting point every time 
        // (It's a singleton so resetting it is important for test determinism)
        SignedUrlCache *theCache = SignedUrlCache::TheCache();
        theCache->d_enabled = -1;

        // ...and clear the caches
        theCache->d_signed_urls.clear();
        theCache->d_href_to_s3credentials_cache.clear();
        theCache->d_href_to_s3_cache.clear();
        theCache->d_s3credentials_cache.clear();

        if (!token.empty()) {
            DBG(cerr << "Setting BESContext " << EDL_AUTH_TOKEN_KEY << " to: '" << token << "'" << endl);
            BESContextManager::TheManager()->set_context(EDL_AUTH_TOKEN_KEY, token);
        }
        DBG(cerr << prolog << "END" << endl);
    }

/*##################################################################################################*/
/* TESTS BEGIN */

    void get_cached_signed_url_test() {
        // The cache is disabled in bes.conf, so we need to turn it on.
        SignedUrlCache::TheCache()->d_enabled = true;

        auto input_url = make_shared<http::url>("http://started_here.com");
        auto output_url = make_shared<http::EffectiveUrl>("http://started_here.com?signed-now");

        SignedUrlCache::TheCache()->d_signed_urls.insert(
            pair<string, shared_ptr<http::EffectiveUrl>>(input_url->str(), output_url));
        auto result = SignedUrlCache::TheCache()->get_signed_url(input_url);
        CPPUNIT_ASSERT(result->str() == output_url->str());

        std::string non_http_key("foo");
        SignedUrlCache::TheCache()->d_signed_urls.insert(
            pair<string, shared_ptr<http::EffectiveUrl>>(non_http_key, output_url));
        auto result2 = SignedUrlCache::TheCache()->get_signed_url(make_shared<http::url>(non_http_key));
        CPPUNIT_ASSERT_MESSAGE("Non-url key returns nullptr", result2 == nullptr);
    }

    void is_cache_disabled_test() {
        DBG(cerr << prolog << "SignedUrlCache::TheCache()->is_enabled(): "
                 << (SignedUrlCache::TheCache()->is_enabled() ? "true" : "false") << endl);
        CPPUNIT_ASSERT(!SignedUrlCache::TheCache()->is_enabled());

        auto input_url = make_shared<http::url>("http://started_here.com");
        auto output_url = make_shared<http::EffectiveUrl>("http://started_here.com?signed-now");

        SignedUrlCache::TheCache()->d_signed_urls.insert(
            pair<string, shared_ptr<http::EffectiveUrl>>(input_url->str(), output_url));
        CPPUNIT_ASSERT(SignedUrlCache::TheCache()->d_signed_urls.size() == 1);

        // When the cache is disabled, we return a nullptr---always. 
        // (In comparison, the EffectiveUrlCache creates an EffectiveUrl around the raw input url)
        auto result_when_disabled = SignedUrlCache::TheCache()->get_signed_url(input_url);
        CPPUNIT_ASSERT(result_when_disabled == nullptr);

        // ...if we now enable the cache is enabled, we return the previously cached value
        SignedUrlCache::TheCache()->d_enabled = true;
        CPPUNIT_ASSERT(SignedUrlCache::TheCache()->is_enabled());

        auto result_when_enabled = SignedUrlCache::TheCache()->get_signed_url(input_url);
        CPPUNIT_ASSERT(result_when_enabled->str() == output_url->str());
    }

    void set_skip_regex_test() {
        DBG(cerr << prolog << "BEGIN" << endl);
        try {
            // The cache is disabled in bes.conf, so we need to turn it on.
            SignedUrlCache::TheCache()->d_enabled = true;

            // This one does not add the URL or even check it because it _should_ be matching the skip regex
            // in the bes.conf
            auto src_url = make_shared<http::url>("https://foobar.com/opendap/data/nc/fnoc1.nc?dap4.ce=u;v");
            auto result_url = SignedUrlCache::TheCache()->get_signed_url(src_url);
            CPPUNIT_ASSERT(SignedUrlCache::TheCache()->d_signed_urls.empty());
            CPPUNIT_ASSERT(result_url == nullptr);

            // Similarly, skipped even when that url has been previously 
            // added to the cache somehow
            auto output_url = make_shared<http::EffectiveUrl>("http://started_here.com?signed-now");
            SignedUrlCache::TheCache()->d_signed_urls.insert(
                pair<string, shared_ptr<http::EffectiveUrl>>(src_url->str(), output_url));
            CPPUNIT_ASSERT(SignedUrlCache::TheCache()->d_signed_urls.size() == 1);
            auto result_url2 = SignedUrlCache::TheCache()->get_signed_url(src_url);
            CPPUNIT_ASSERT(result_url2 == nullptr);
        }
        catch (const BESError &be) {
            stringstream msg;
            msg << prolog << "ERROR! Caught BESError. Message: " << be.get_message() << endl;
            CPPUNIT_FAIL(msg.str());
        }
    }

    void dump_test() {
        // Add values to each type of subcache
        SignedUrlCache::TheCache()->d_signed_urls.insert(
                pair<string, shared_ptr<http::EffectiveUrl>>("www.foo.com", make_shared<http::EffectiveUrl>("http://www.bar.com")));

        // Check to make sure dump includes them
        auto strm = std::ostringstream();
        SignedUrlCache::TheCache()->dump(strm);
        // Remove start of string to skip address that varies
        auto result = strm.str().substr(49);
        std::string expected_str = string("d_skip_regex: ") +
            "\n    signed url list:" +
            "\n        www.foo.com --> http://www.bar.com";
        CPPUNIT_ASSERT_MESSAGE("The dump should be `" + expected_str + "`; was `" + result + "`", expected_str == result);
    }

    void is_timestamp_after_now_test() {
        std::string str_old("1980-07-16 18:40:58+00:00");
        CPPUNIT_ASSERT_MESSAGE("Ancient timestamp is before now", !SignedUrlCache::is_timestamp_after_now(str_old));

        std::string str_future("3035-07-16 02:20:33+00:00");
        CPPUNIT_ASSERT_MESSAGE("Future timestamp is after now", SignedUrlCache::is_timestamp_after_now(str_future));

        std::string str_invalid("invalid timestamp woo hooray huzzah");
        CPPUNIT_ASSERT_MESSAGE("Invalid timestamp is not after now", !SignedUrlCache::is_timestamp_after_now(str_invalid));
    }

    void retrieve_cached_s3credentials_test() {
        std::string key("i_am_a_key");
        auto result_not_in_cache = SignedUrlCache::TheCache()->retrieve_cached_s3credentials(key);
        CPPUNIT_ASSERT_MESSAGE("Cache miss should return null", result_not_in_cache == nullptr);

        auto value = make_shared<SignedUrlCache::S3AccessKeyTuple>("a man", "a plan", "a canal", "3035-07-16 02:20:33+00:00");
        SignedUrlCache::TheCache()->d_s3credentials_cache.insert(pair<string, shared_ptr<SignedUrlCache::S3AccessKeyTuple>>(key, value));
        auto result_in_cache = SignedUrlCache::TheCache()->retrieve_cached_s3credentials(key);
        CPPUNIT_ASSERT_MESSAGE("Cache hit should successfully retrieve result", result_in_cache == value);
    }

    void retrieve_cached_s3credentials_expired_credentials_test() {
        std::string key("i_am_a_key");
        std::string expiration_time("1980-07-16 18:40:58+00:00");
        auto value = make_shared<SignedUrlCache::S3AccessKeyTuple>("https://www.foo", "https://www.bar", "https://www.bat", expiration_time);
        SignedUrlCache::TheCache()->d_s3credentials_cache.insert(pair<string, shared_ptr<SignedUrlCache::S3AccessKeyTuple>>(key, value));

        auto result = SignedUrlCache::TheCache()->retrieve_cached_s3credentials(key);
        CPPUNIT_ASSERT_MESSAGE("Cached expired result should not be retrieved", result == nullptr);
        CPPUNIT_ASSERT_MESSAGE("Expired result should have been removed from cache", SignedUrlCache::TheCache()->d_s3credentials_cache.empty());
    }

    void extract_s3_credentials_from_response_json_test() {

        std::string access_key("i_am_an_access_key_id");
        std::string valid_response(
            string("{\n\"accessKeyId\": \"") + access_key + "\",\n" +
            "\"secretAccessKey\": \"i_am_a_secret_access_key\",\n" +
            "\"sessionToken\": \"i_am_a_fake_token\",\n" +
            "\"expiration\": \"3025-09-30 18:40:58+00:00\"\n" +
            "} ");
        auto result = SignedUrlCache::extract_s3_credentials_from_response_json(valid_response);
        CPPUNIT_ASSERT_MESSAGE("Valid json should not return nullptr", result != nullptr);
        CPPUNIT_ASSERT_MESSAGE("Access key should be returned as first value", access_key == get<0>(*result));

        CPPUNIT_ASSERT_MESSAGE("Empty string should return nullptr", SignedUrlCache::extract_s3_credentials_from_response_json("") == nullptr);
        CPPUNIT_ASSERT_MESSAGE("Invalid json should return nullptr", SignedUrlCache::extract_s3_credentials_from_response_json("{foo}") == nullptr);

        std::string invalid_response(
            string("{\n\"accessKeyId\": \"") + access_key + "\",\n" +
            "\"secretAccessKey\": \"i_am_a_secret_access_key\",\n" +
            "\"sessionToken\": \"i_am_a_fake_token\",\n" +
            "} ");
        CPPUNIT_ASSERT_MESSAGE("Response missing field should return nullptr", SignedUrlCache::extract_s3_credentials_from_response_json(invalid_response) == nullptr);

        std::string invalid_response_contents(
            string("{\n\"accessKeyId\": \"") + access_key + "\",\n" +
            "\"secretAccessKey\": [3, 4, 5],\n" + // Oh no! An array instead of a string!! Horrors!
            "\"sessionToken\": \"i_am_a_fake_token\",\n" +
            "\"expiration\": \"3025-09-30 18:40:58+00:00\"\n" +
            "} ");
        CPPUNIT_ASSERT_MESSAGE("Field with non-string response should return nullptr", SignedUrlCache::extract_s3_credentials_from_response_json(invalid_response_contents) == nullptr);
    }

    void cache_signed_url_components_test() {

        SignedUrlCache *theCache = SignedUrlCache::TheCache();

        theCache->cache_signed_url_components("", "foo", "bar");
        CPPUNIT_ASSERT_MESSAGE("Empty key_href_url results in no caching", theCache->d_href_to_s3_cache.empty() && theCache->d_href_to_s3credentials_cache.empty());

        theCache->cache_signed_url_components("foo", "", "bar");
        CPPUNIT_ASSERT_MESSAGE("Empty s3_url results in no caching", theCache->d_href_to_s3_cache.empty() && theCache->d_href_to_s3credentials_cache.empty());

        theCache->cache_signed_url_components("foo", "bar", "");
        CPPUNIT_ASSERT_MESSAGE("Empty s3credentials_url results in no caching", theCache->d_href_to_s3_cache.empty() && theCache->d_href_to_s3credentials_cache.empty());

        theCache->cache_signed_url_components("foo", "bar", "bat");
        CPPUNIT_ASSERT_MESSAGE("s3_url should be cached", theCache->d_href_to_s3_cache["foo"] == "bar");
        CPPUNIT_ASSERT_MESSAGE("s3credentials_url should be cached", theCache->d_href_to_s3credentials_cache["foo"] == "bat");
    }

/*
    void cache_test_00() {
        DBG(cerr << prolog << "BEGIN" << endl);
        //string source_url;
        //string value;
        try {
            // The cache is disabled in bes.conf, so we need to turn it on.
            SignedUrlCache::TheCache()->d_enabled = true;

#if 0
            string src_url_00 = "https://d1jecqxxv88lkr.cloudfront.net/ghrcwuat-protected/rss_demo/rssmif16d__7/f1"
                                "6_ssmis_20040107v7.nc";
            string eurl_str = "https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/rss_demo/rssmif16d__7/f16_ssm"
                              "is_20031229v7.nc?A-userid=hyrax&X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=A"
                              "SIASF4N-AWS-Creds-00808%2Fus-west-2%2Fs3%2Faws4_request&X-Amz-Date=20200808T032623Z"
                              "&X-Amz-Expires=86400&X-Amz-Security-Token=FwoGZXIvYXdzE-AWS-Sec-Token-MWRLIZGYvDx1O"
                              "Nzd0ffK8VtxO8JP7thrGIQ%3D%3D&X-Amz-SignedHeaders=host&X-Amz-Signature=260a7c4dd4-AW"
                              "S-SIGGY-0c7a39ee899";
            auto effective_url_00 = shared_ptr<http::EffectiveUrl>(new http::EffectiveUrl(eurl_str));
            SignedUrlCache::TheCache()->d_signed_urls.insert(pair<string, shared_ptr<http::EffectiveUrl>>(src_url_00, effective_url_00));
            CPPUNIT_ASSERT(SignedUrlCache::TheCache()->d_signed_urls.size() == 1);
#endif

            string src_url_01 = "http://test.opendap.org/data/httpd_catalog/READTHIS";
            string eurl_str = "https://test.opendap.org/data/httpd_catalog/READTHIS";
            auto effective_url_01 = shared_ptr<http::EffectiveUrl>(new http::EffectiveUrl(eurl_str));
            SignedUrlCache::TheCache()->d_signed_urls.insert(
                    pair<string, shared_ptr<http::EffectiveUrl>>(src_url_01, effective_url_01));
            CPPUNIT_ASSERT(SignedUrlCache::TheCache()->d_signed_urls.size() == 1);

            // This one actually does the thing
            eurl_str = "http://test.opendap.org/opendap/";
            auto expected_url_02 = std::unique_ptr<EffectiveUrl>(new http::EffectiveUrl(eurl_str));

            shared_ptr<http::url> src_url_02(new http::url("http://test.opendap.org/opendap"));
            DBG(cerr << prolog << "Retrieving effective URL for: " << src_url_02->str() << endl);
            auto result_url = SignedUrlCache::TheCache()->get_signed_url(src_url_02);
            CPPUNIT_ASSERT(SignedUrlCache::TheCache()->d_signed_urls.size() == 2);

            DBG(cerr << prolog << "SignedUrlCache::TheCache()->get_signed_url() returned: " << result_url
                     << endl);
            CPPUNIT_ASSERT(result_url->str() == expected_url_02->str());
        }
        catch (const BESError &be) {
            stringstream msg;
            msg << prolog << "ERROR! Caught BESError. Message: " << be.get_message() << endl;
            CPPUNIT_FAIL(msg.str());

        }
        DBG(cerr << prolog << "END" << endl);
    }


    void cache_test_01() {
        DBG(cerr << prolog << "BEGIN" << endl);
        string source_url;
        string value;
        string result_url;
        try {
            // The cache is disabled in bes.conf so we need to turn it on.
            SignedUrlCache::TheCache()->d_enabled = true;

            std::map<std::string, http::EffectiveUrl *> d_signed_urls;
            source_url = "http://someURL";

            http::EffectiveUrl first_eu("http://someOtherUrl");
            d_signed_urls[source_url] = &first_eu;
            DBG(cerr << prolog << "source_url: " << source_url << endl);
            DBG(cerr << prolog << "first_eu: " << first_eu.str() << endl);

            CPPUNIT_ASSERT(d_signed_urls[source_url] == &first_eu);

            http::EffectiveUrl second_eu("http://someMoreUrlLovin");
            d_signed_urls[source_url] = &second_eu;
            DBG(cerr << prolog << "source_url: " << source_url << endl);
            DBG(cerr << prolog << "second_eu: " << second_eu.str() << endl);

            CPPUNIT_ASSERT(d_signed_urls[source_url] == &second_eu);
        }
        catch (const BESError &be) {
            stringstream msg;
            msg << prolog << "ERROR! Caught BESError. Message: " << be.get_message() << endl;
            CPPUNIT_FAIL(msg.str());

        }
        DBG(cerr << prolog << "END" << endl);
    }

    void euc_ghrc_tea_url_test() {
        if (!ngap_tests) {
            DBG(cerr << prolog << "SKIPPING." << endl);
            return;
        }
        DBG(cerr << prolog << "BEGIN" << endl);
        string source_url;
        string value;
        try {
            // The cache is disabled in bes.conf so we need to turn it on.
            SignedUrlCache::TheCache()->d_enabled = true;

            shared_ptr<http::url> thing1(new http::url(
                    "https://d1jecqxxv88lkr.cloudfront.net/ghrcwuat-protected/rss_demo/rssmif16d__7/f16_ssmis_20031026v7.nc"));
            string thing1_out_of_region_effective_url_prefix = "https://d1jecqxxv88lkr.cloudfront.net/s3";
            string thing1_in_region_effective_url_prefix = "https://ghrcwuat-protected.s3.us-west-2.amazonaws.com/";

            DBG(cerr << prolog << "Retrieving signed URL for: " << thing1->str() << endl);
            auto result_url = SignedUrlCache::TheCache()->get_signed_url(thing1);
            CPPUNIT_ASSERT(SignedUrlCache::TheCache()->d_signed_urls.size() == 1);

            DBG(cerr << prolog << "SignedUrlCache::TheCache()->get_signed_url() returned: " << result_url
                     << endl);
            CPPUNIT_ASSERT(result_url->str().rfind(thing1_in_region_effective_url_prefix, 0) == 0
                           || result_url->str().rfind(thing1_out_of_region_effective_url_prefix, 0) == 0);

            // result_url = SignedUrlCache::TheCache()->get_signed_url(thing1);
        }
        catch (const BESError &be) {
            stringstream msg;
            msg << prolog << "ERROR! Caught BESError. Message: " << be.get_message() << endl;
            CPPUNIT_FAIL(msg.str());

        }
        DBG(cerr << prolog << "END" << endl);
    }

    void euc_harmony_url_test() {
        if (!ngap_tests) {
            DBG(cerr << prolog << "SKIPPING." << endl);
            return;
        }
        DBG(cerr << prolog << "BEGIN" << endl);
        string source_url;
        string value;
        try {
            // The cache is disabled in bes.conf, so we need to turn it on.
            SignedUrlCache::TheCache()->d_enabled = true;

            shared_ptr<http::url> thing1(
                    new http::url("https://harmony.uat.earthdata.nasa.gov/service-results/harmony-uat-staging/public/"
                                  "sds/staged/ATL03_20200714235814_03000802_003_01.h5"));
            string thing1_out_of_region_effective_url_prefix = "https://djpip0737hawz.cloudfront.net/s3";
            string thing1_in_region_effective_url_prefix = "https://harmony-uat-staging.s3.us-west-2.amazonaws.com/public/";

            DBG(cerr << prolog << "Retrieving signed URL for: " << thing1->str() << endl);
            auto result_url = SignedUrlCache::TheCache()->get_signed_url(thing1);
            CPPUNIT_ASSERT(SignedUrlCache::TheCache()->d_signed_urls.size() == 1);

            DBG(cerr << prolog << "SignedUrlCache::TheCache()->get_signed_url() returned: " << result_url
                     << endl);

            CPPUNIT_ASSERT(result_url->str().rfind(thing1_in_region_effective_url_prefix, 0) == 0
                           || result_url->str().rfind(thing1_out_of_region_effective_url_prefix, 0) == 0);

            // TODO ??? result_url = SignedUrlCache::TheCache()->get_signed_url(thing1);
        }
        catch (const BESError &be) {
            stringstream msg;
            msg << prolog << "ERROR! Caught BESError. Message: " << be.get_message() << endl;
            CPPUNIT_FAIL(msg.str());

        }
        DBG(cerr << prolog << "END" << endl);
    }

    void trusted_url_test_01() {
        DBG(cerr << prolog << "BEGIN" << endl);
        string url_str = "http://test.opendap.org/data/nothing_is_here.html";
        string result_url_str = "http://test.opendap.org/data/httpd_catalog/READTHIS";
        shared_ptr<http::url> trusted_src_url(new http::url(url_str, true));
        shared_ptr<http::url> untrusted_src_url(new http::url(url_str, false));

        DBG(cerr << prolog << "Retrieving signed URL for: " << trusted_src_url->str() << endl);
        try {
            // The cache is disabled in bes.conf so we need to turn it on.
            SignedUrlCache::TheCache()->d_enabled = true;

            shared_ptr<http::url> result_url;

            result_url = SignedUrlCache::TheCache()->get_signed_url(untrusted_src_url);
            DBG(cerr << prolog << "source_url: " << untrusted_src_url->str() << " is "
                     << (untrusted_src_url->is_trusted() ? "" : "NOT ") << "trusted." << endl);
            DBG(cerr << prolog << "result_url: " << result_url->str() << " is "
                     << (result_url->is_trusted() ? "" : "NOT ") << "trusted." << endl);
            DBG(cerr << prolog << "SignedUrlCache::TheCache()->d_signed_urls.size(): "
                     << SignedUrlCache::TheCache()->d_signed_urls.size() << endl);
            CPPUNIT_ASSERT(SignedUrlCache::TheCache()->d_signed_urls.size() == 1);
            CPPUNIT_ASSERT(result_url->str() == result_url_str);
            CPPUNIT_ASSERT(!result_url->is_trusted());

            result_url = SignedUrlCache::TheCache()->get_signed_url(trusted_src_url);
            DBG(cerr << prolog << "source_url: " << trusted_src_url->str() << " is "
                     << (trusted_src_url->is_trusted() ? "" : "NOT ") << "trusted." << endl);
            DBG(cerr << prolog << "result_url: " << result_url->str() << " is "
                     << (result_url->is_trusted() ? "" : "NOT ") << "trusted." << endl);
            DBG(cerr << prolog << "SignedUrlCache::TheCache()->d_signed_urls.size(): "
                     << SignedUrlCache::TheCache()->d_signed_urls.size() << endl);
            CPPUNIT_ASSERT(SignedUrlCache::TheCache()->d_signed_urls.size() == 1);
            CPPUNIT_ASSERT(result_url->str() == result_url_str);
            CPPUNIT_ASSERT(result_url->is_trusted());

            result_url = SignedUrlCache::TheCache()->get_signed_url(untrusted_src_url);
            DBG(cerr << prolog << "source_url: " << untrusted_src_url->str() << " is "
                     << (untrusted_src_url->is_trusted() ? "" : "NOT ") << "trusted." << endl);
            DBG(cerr << prolog << "result_url: " << result_url->str() << " is "
                     << (result_url->is_trusted() ? "" : "NOT ") << "trusted." << endl);
            DBG(cerr << prolog << "SignedUrlCache::TheCache()->d_signed_urls.size(): "
                     << SignedUrlCache::TheCache()->d_signed_urls.size() << endl);
            CPPUNIT_ASSERT(SignedUrlCache::TheCache()->d_signed_urls.size() == 1);
            CPPUNIT_ASSERT(result_url->str() == result_url_str);
            CPPUNIT_ASSERT(!result_url->is_trusted());

        }
        catch (const BESError &be) {
            stringstream msg;
            msg << prolog << "ERROR! Caught BESError. Message: " << be.get_message() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        DBG(cerr << prolog << "END" << endl);
    }
*/

/* TESTS END */
/*##################################################################################################*/

CPPUNIT_TEST_SUITE(SignedUrlCacheTest);

    // Test behavior analogous to that of the EffectiveUrlCache:
    CPPUNIT_TEST(get_cached_signed_url_test);
    CPPUNIT_TEST(is_cache_disabled_test);
    CPPUNIT_TEST(set_skip_regex_test);
    CPPUNIT_TEST(dump_test);

    // Test behavior specific to SignedUrlCache:
    CPPUNIT_TEST(is_timestamp_after_now_test);
    CPPUNIT_TEST(retrieve_cached_s3credentials_test);
    CPPUNIT_TEST(retrieve_cached_s3credentials_expired_credentials_test);
    CPPUNIT_TEST(extract_s3_credentials_from_response_json_test);
    CPPUNIT_TEST(cache_signed_url_components_test);
    // - get_s3credentials_from_endpoint
    // - retrieve_cached_signed_url_components

    // ...and, specifically, the signing itself: 
    // TODO-future: will add/update these tests once signing behavior is implemented!
    // - sign_url
    // - get_signed_url
    // CPPUNIT_TEST(cache_test_00);
    // CPPUNIT_TEST(cache_test_01);
    // CPPUNIT_TEST(euc_ghrc_tea_url_test);
    // CPPUNIT_TEST(euc_harmony_url_test);
    // CPPUNIT_TEST(trusted_url_test_01);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(SignedUrlCacheTest);

} // namespace httpd_catalog

int main(int argc, char *argv[]) {
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    int option_char;
    while ((option_char = getopt(argc, argv, "dbDPt:N")) != -1)
        switch (option_char) {
            case 'd':
                debug = true;  // debug is a static global
                cerr << "debug enabled" << endl;
                break;
            case 'D':
                Debug = true;  // Debug is a static global
                cerr << "Debug enabled" << endl;
                break;
            case 'b':
                bes_debug = true;  // debug is a static global
                cerr << "bes_debug enabled" << endl;
                break;
            case 'N':
                ngap_tests = true; // ngap_tests is a static global
                cerr << "NGAP Tests Enabled." << token << endl;
                break;
            case 't':
                token = optarg; // token is a static global
                cerr << "Authorization header value: " << token << endl;
                break;
            default:
                break;
        }

    argc -= optind;
    argv += optind;

    bool wasSuccessful = true;
    string test;
    if (0 == argc) {
        // run them all
        wasSuccessful = runner.run("");
    } else {
        int i = 0;
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = http::SignedUrlCacheTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
