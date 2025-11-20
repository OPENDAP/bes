// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES component of the Hyrax Data Server.

// Copyright (c) 2025 OPeNDAP, Inc.
// Authors: Nathan Potter <ndp@opendap.org>, Hannah Robertson <hrobertson@opendap.org>
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
#include <sstream>

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

#include "modules/common/run_tests_cppunit.h"

using namespace std;

static std::string token;

#define prolog std::string("SignedUrlCacheTest::").append(__func__).append("() - ")

namespace bes {

class SignedUrlCacheTest : public CppUnit::TestFixture {
public:
    // Called once before everything gets tested
    SignedUrlCacheTest() = default;

    // Called at the end of the tests
    ~SignedUrlCacheTest() override = default;

    // Called before each test
    void setUp() override {
        DBG(cerr << endl);
        DBG(cerr << prolog << "BEGIN" << endl);
        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR, "bes.conf");
        DBG(cerr << prolog << "Using BES configuration: " << bes_conf << endl);
        TheBESKeys::ConfigFile = bes_conf;

        // Reset to same starting point every time 
        // (It's a singleton so resetting it is important for testing determinism)
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
        CPPUNIT_ASSERT_MESSAGE("Cached url should be retrievable", result->str() == output_url->str());

        std::string non_http_key("foo");
        SignedUrlCache::TheCache()->d_signed_urls.insert(
            pair<string, shared_ptr<http::EffectiveUrl>>(non_http_key, output_url));
        auto result2 = SignedUrlCache::TheCache()->get_signed_url(make_shared<http::url>(non_http_key));
        CPPUNIT_ASSERT_MESSAGE("Non-url key returns nullptr", result2 == nullptr);
    }

    void is_cache_disabled_test() {
        DBG(cerr << prolog << "SignedUrlCache::TheCache()->is_enabled(): "
                 << (SignedUrlCache::TheCache()->is_enabled() ? "true" : "false") << endl);
        CPPUNIT_ASSERT_MESSAGE("Cache is disabled", !SignedUrlCache::TheCache()->is_enabled());

        auto input_url = make_shared<http::url>("http://started_here.com");
        auto output_url = make_shared<http::EffectiveUrl>("http://started_here.com?signed-now");

        SignedUrlCache::TheCache()->d_signed_urls.insert(
            pair<string, shared_ptr<http::EffectiveUrl>>(input_url->str(), output_url));
        CPPUNIT_ASSERT_MESSAGE("Cache contains single item", SignedUrlCache::TheCache()->d_signed_urls.size() == 1);

        // When the cache is disabled, we return a nullptr---always. 
        // (In comparison, the EffectiveUrlCache creates an EffectiveUrl around the raw input url)
        auto result_when_disabled = SignedUrlCache::TheCache()->get_signed_url(input_url);
        CPPUNIT_ASSERT_MESSAGE("When cache is disabled, nullptr is returned", result_when_disabled == nullptr);

        // ...if we now enable the cache is enabled, we return the previously cached value
        SignedUrlCache::TheCache()->d_enabled = true;
        CPPUNIT_ASSERT_MESSAGE("Cache is enabled", SignedUrlCache::TheCache()->is_enabled());

        auto result_when_enabled = SignedUrlCache::TheCache()->get_signed_url(input_url);
        CPPUNIT_ASSERT_MESSAGE("When cache is re-enabled, value is returned", result_when_enabled->str() == output_url->str());
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
            CPPUNIT_ASSERT_MESSAGE("When key matches skip regex, value is not cached", SignedUrlCache::TheCache()->d_signed_urls.empty());
            CPPUNIT_ASSERT_MESSAGE("When key matches skip regex, nullptr is returned", result_url == nullptr);

            // Similarly, skipped even when that url has been previously 
            // added to the cache somehow
            auto output_url = make_shared<http::EffectiveUrl>("http://started_here.com?signed-now");
            SignedUrlCache::TheCache()->d_signed_urls.insert(
                pair<string, shared_ptr<http::EffectiveUrl>>(src_url->str(), output_url));
            auto result_url2 = SignedUrlCache::TheCache()->get_signed_url(src_url);
            CPPUNIT_ASSERT_MESSAGE("When key matches skip regex, even if it exists in the cache, the value is not returned", result_url2 == nullptr);
        }
        catch (const BESError &be) {
            stringstream msg;
            msg << prolog << "ERROR! Caught BESError. Message: " << be.get_message() << endl;
            CPPUNIT_FAIL(msg.str());
        }
    }

    void dump_test() {
        SignedUrlCache *theCache = SignedUrlCache::TheCache();

        // Add values to each type of subcache
        theCache->d_signed_urls.insert(
                pair<string, shared_ptr<http::EffectiveUrl>>("www.once.com", make_shared<http::EffectiveUrl>("http://www.upon.com")));
        theCache->d_signed_urls.insert(
                pair<string, shared_ptr<http::EffectiveUrl>>("www.a.com", make_shared<http::EffectiveUrl>("http://www.time.com")));

        auto value = make_shared<SignedUrlCache::S3AccessKeyTuple>("a man", "a plan", "a canal", "3035-07-16 02:20:33+00:00");
        theCache->d_s3credentials_cache.insert(pair<string, shared_ptr<SignedUrlCache::S3AccessKeyTuple>>("palindrome", value));

        theCache->d_href_to_s3credentials_cache.insert(pair<string, string>("foo", "whee"));
        theCache->d_href_to_s3_cache.insert(pair<string, string>("yee", "haw"));


        // Check to make sure dump includes them
        auto strm = std::ostringstream();
        theCache->dump(strm);
        // Remove start of string to skip address that varies
        auto result = strm.str().substr(49);
        std::string expected_str = string("d_skip_regex: ") +
            "\n    signed url list:" +
            // "\n        www.foo.com --> http://www.bar.com";
            "\n        www.a.com --> http://www.time.com" +
            "\n        www.once.com --> http://www.upon.com" +
            "\n    href-to-s3credentials list:" +
            "\n        foo --> whee" +
            "\n    href-to-s3url list:" +
            "\n        yee --> haw" +
            "\n    s3 credentials list:" +
            "\n        palindrome --> Expires: 3035-07-16 02:20:33+00:00\n";
        CPPUNIT_ASSERT_MESSAGE("The dump should contain:\n" + expected_str + "\n\nbut did not; INSTEAD was\n" + result, result.find(expected_str) != std::string::npos);
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

    void retrieve_cached_signed_url_components_test() {
        SignedUrlCache *theCache = SignedUrlCache::TheCache();

        theCache->cache_signed_url_components("two fish", "red fish", "blue fish");
        auto retrieved = theCache->retrieve_cached_signed_url_components("two fish");
        auto expected = std::pair<std::string, std::string>("red fish", "blue fish");
        CPPUNIT_ASSERT_MESSAGE("Cached components should be retrieved", expected == retrieved);

        std::string key("little_bo_peep");
        theCache->d_href_to_s3_cache.insert(pair<string, string>(key, "goat"));
        auto retrieved2 = theCache->retrieve_cached_signed_url_components(key);
        auto empty_pair = std::pair<std::string, std::string>("", "");
        CPPUNIT_ASSERT_MESSAGE("If both urls were not cached, no response is returned", retrieved2 == empty_pair);
    }

    void get_s3credentials_from_endpoint_test() {
        SignedUrlCache *theCache = SignedUrlCache::TheCache();
        
        CPPUNIT_ASSERT_MESSAGE("Credentials cache is empty", theCache->d_s3credentials_cache.empty());

        auto result_bad = theCache->get_s3credentials_from_endpoint("http://badurl");
        CPPUNIT_ASSERT_MESSAGE("After attempting fetch from invalid url, credentials cache is still empty", theCache->d_s3credentials_cache.empty());
        CPPUNIT_ASSERT_MESSAGE("Fetch from invalid url returns nullptr", result_bad == nullptr);

        // Note: we do not test a successful endpoint here, as that would require either
        // relying on TEA or setting up a test url with fake credentials,
        // both of which are brittle in their own ways.
        // The inputs/outputs for good and bad retrieval are covered by other tests
    }

    void split_s3_url_test() {
        auto out1 = SignedUrlCache::split_s3_url("s3://foo/bar");
        auto expected1 = std::pair<std::string,std::string>("foo", "bar");
        CPPUNIT_ASSERT_MESSAGE("Valid input is split into `" + get<0>(expected1) + "," + get<1>(expected1) + "`, got `" +                                 get<0>(out1) + "," + get<1>(out1) + "`", expected1 == out1);

        auto expected_empty = std::pair<std::string,std::string>("", "");
        auto out2 = SignedUrlCache::split_s3_url("foo/bar");
        CPPUNIT_ASSERT_MESSAGE("Missing s3:// prefix should return empty strings; returned `" + get<0>(out2) + "," + get<1>(out2) + "`", expected_empty == out2);
        auto out3 = SignedUrlCache::split_s3_url("x");
        CPPUNIT_ASSERT_MESSAGE("Missing s3:// prefix should return empty strings; returned `" + get<0>(out3) + "," + get<1>(out3) + "`", expected_empty == out2);

        auto out4 = SignedUrlCache::split_s3_url("s3://foo/bar/wheeyay.txt");
        auto expected4 = std::pair<std::string,std::string>("foo", "bar/wheeyay.txt");
        CPPUNIT_ASSERT_MESSAGE("Valid input is split into `" + get<0>(expected4) + "," + get<1>(expected4) + "`, got `" +                                 get<0>(out4) + "," + get<1>(out4) + "`", expected4 == out4);
    }

    std::chrono::system_clock::time_point parse_as_time_point(const std::string& datetime_string) {
        std::tm timestamp_time = {};
        strptime(datetime_string.c_str(), "%F %T%z", &timestamp_time);
        return std::chrono::system_clock::from_time_t(std::mktime(&timestamp_time));
    }

    void num_seconds_until_expiration_test() {
        std::string current_time_str = "2025-04-01 09:30:00+0400";
        std::string current_time_aws_str = "2025-04-01 09:30:00+04:00";
        std::chrono::system_clock::time_point current_time = parse_as_time_point(current_time_str);

        auto out = SignedUrlCache::num_seconds_until_expiration(current_time_aws_str, current_time);
        CPPUNIT_ASSERT_MESSAGE("Expiration time `now` should return 0s` for `" + current_time_aws_str + "`: `" + to_string(out) + "`", out == 0);

        auto time_plus_five_seconds_aws_str = "2025-04-01 09:30:05+04:00";
        auto out1 = SignedUrlCache::num_seconds_until_expiration(time_plus_five_seconds_aws_str, current_time);
        CPPUNIT_ASSERT_MESSAGE("Expiration date should be 5s from now `" + to_string(out1) + "`", out1 == 5);

        auto time_minus_five_seconds_aws_str = "2025-04-01 09:29:55+04:00";
        auto out2 = SignedUrlCache::num_seconds_until_expiration(time_minus_five_seconds_aws_str, current_time);
        CPPUNIT_ASSERT_MESSAGE("Expiration date in the past should return 0s `" + to_string(out2) + "`", out2 == 0);

        std::string expiration_time_past("1980-07-16 18:40:58+04:00");
        auto out3 = SignedUrlCache::num_seconds_until_expiration(expiration_time_past);
        CPPUNIT_ASSERT_MESSAGE("Expiration time in past should return 0s `" + to_string(out3) + "`", out3 == 0);

        std::string expiration_time_future("3026-07-16 18:40:58+04:00");
        auto out4 = SignedUrlCache::num_seconds_until_expiration(expiration_time_future);
        CPPUNIT_ASSERT_MESSAGE("Expiration time in future should be > 0s `" + to_string(out4) + "`", out4 > 0);

        auto out5 = SignedUrlCache::num_seconds_until_expiration("lil_date");
        CPPUNIT_ASSERT_MESSAGE("Invalid date string should return 0s `" + to_string(out5) + "`", out5 == 0);
    }

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
    CPPUNIT_TEST(retrieve_cached_signed_url_components_test);
    CPPUNIT_TEST(get_s3credentials_from_endpoint_test);

    // // ...and, specifically, the signing itself: 
    // // TODO-future: will add/update these tests once signing behavior is implemented
    // // - sign_url
    // // - get_signed_url

    // Last but not least, test those helper functions
    CPPUNIT_TEST(split_s3_url_test);
    CPPUNIT_TEST(num_seconds_until_expiration_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(SignedUrlCacheTest);

} // namespace bes

int main(int argc, char *argv[]) {
    return bes_run_tests<bes::SignedUrlCacheTest>(argc, argv, "cerr,bes,http") ? 0 : 1;
}