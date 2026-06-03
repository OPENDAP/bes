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

#include <iostream>
#include <memory>
#include <sstream>

#include <unistd.h>

#include "AWS_SDK.h"
#include "BESCatalogList.h"
#include "BESContextManager.h"
#include "BESDebug.h"
#include "BESError.h"
#include "BESUtil.h"
#include "TheBESKeys.h"

#include "EffectiveUrl.h"
#include "HttpNames.h"
#include "SignedUrlCache.h"
#include "url_impl.h"

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
        BESContextManager::TheManager()->set_context(EDL_UID_KEY, "test_user");

        // Reset to same starting point every time
        // (It's a singleton so resetting it is important for testing determinism)
        SignedUrlCache *theCache = SignedUrlCache::TheCache();
        theCache->d_enabled = -1;

        // ...and clear the caches
        theCache->d_presigned_s3_urls_cache.clear();
        theCache->d_href_to_tea_endpoint_cache.clear();
        theCache->d_href_to_s3_uri_cache.clear();
        theCache->d_tea_endpoint_sts_credentials_cache.clear();

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

        SignedUrlCache::TheCache()->cache_presigned_s3_url(input_url->str(), output_url);
        auto result = SignedUrlCache::TheCache()->get_presigned_s3_url(input_url);
        CPPUNIT_ASSERT_MESSAGE("Cached url should be retrievable",
                               result != nullptr && result->str() == output_url->str());

        std::string non_http_key("foo");
        SignedUrlCache::TheCache()->cache_presigned_s3_url(non_http_key, output_url);

        auto result2 = SignedUrlCache::TheCache()->get_presigned_s3_url(make_shared<http::url>(non_http_key));
        CPPUNIT_ASSERT_MESSAGE("Non-url key returns nullptr", result2 == nullptr);
    }

    void is_cache_enabled_test() {
        CPPUNIT_ASSERT_MESSAGE("Cache should not yet be initialized to enabled or disabled",
                               SignedUrlCache::TheCache()->d_enabled == -1);

        SignedUrlCache::TheCache()->is_enabled();
        CPPUNIT_ASSERT_MESSAGE("Checking if the cache is enabled should initialize it to enabled or disabled, based on "
                               "bes.conf and if the application is running from a supported aws region",
                               SignedUrlCache::TheCache()->d_enabled != -1);
    }

    void is_cache_supported_within_current_aws_region_test() {
        // Force region to be supported, for the sake of testing
        bes::AWS_SDK aws_sdk;
        SignedUrlCache::TheCache()->d_aws_region_in_which_direct_copy_is_supported =
            aws_sdk.get_aws_region_of_running_application();

        CPPUNIT_ASSERT_MESSAGE("SignedUrlCache is in supported region when the current default aws region is "
                               "equivalent to the supported region",
                               SignedUrlCache::TheCache()->is_cache_supported_within_current_aws_region());
        TheBESKeys::TheKeys()->set_key(HTTP_CACHE_EFFECTIVE_URLS_KEY, "true");
        SignedUrlCache::TheCache()->d_enabled = -1; // ...reset the cache enabled state...
        CPPUNIT_ASSERT_MESSAGE("When the current aws region is supported and the cache is allowed in the "
                               "configuration, the cache is enabled",
                               SignedUrlCache::TheCache()->is_enabled());

        TheBESKeys::TheKeys()->set_key(HTTP_CACHE_EFFECTIVE_URLS_KEY, "false");
        SignedUrlCache::TheCache()->d_enabled = -1; // ...reset the cache enabled state...
        CPPUNIT_ASSERT_MESSAGE("When the current aws region is supported but the cache is disallowed in the "
                               "configuration, the cache is not enabled",
                               !SignedUrlCache::TheCache()->is_enabled());

        SignedUrlCache::TheCache()->d_aws_region_in_which_direct_copy_is_supported = "foo";
        TheBESKeys::TheKeys()->set_key(HTTP_CACHE_EFFECTIVE_URLS_KEY, "true");
        SignedUrlCache::TheCache()->d_enabled = -1; // ...reset the cache enabled state...
        CPPUNIT_ASSERT_MESSAGE("SignedUrlCache is never enabled when not in its supported region",
                               !SignedUrlCache::TheCache()->is_cache_supported_within_current_aws_region());
        CPPUNIT_ASSERT_MESSAGE("SignedUrlCache is not enabled when not in its supported region, even if the cache is "
                               "enabled in the configuration",
                               !SignedUrlCache::TheCache()->is_enabled());

        TheBESKeys::TheKeys()->set_key(HTTP_CACHE_EFFECTIVE_URLS_KEY, "false");
        SignedUrlCache::TheCache()->d_enabled = -1; // ...reset the cache enabled state...
        CPPUNIT_ASSERT_MESSAGE("SignedUrlCache is not enabled when not in its supported region, when the cache is "
                               "disabled in the configuration",
                               !SignedUrlCache::TheCache()->is_enabled());
    }

    void cache_enabled_disabled_test() {
        DBG(cerr << prolog << "SignedUrlCache::TheCache()->is_enabled(): "
                 << (SignedUrlCache::TheCache()->is_enabled() ? "true" : "false") << endl);
        CPPUNIT_ASSERT_MESSAGE("Cache is disabled", !SignedUrlCache::TheCache()->is_enabled());

        auto input_url = make_shared<http::url>("http://started_here.com");
        auto output_url = make_shared<http::EffectiveUrl>("http://started_here.com?signed-now");

        SignedUrlCache::TheCache()->cache_presigned_s3_url(input_url->str(), output_url);

        CPPUNIT_ASSERT_MESSAGE("Cache contains single item",
                               SignedUrlCache::TheCache()->d_presigned_s3_urls_cache.size() == 1);

        // When the cache is disabled, we return a nullptr---always.
        // (In comparison, the EffectiveUrlCache creates an EffectiveUrl around the raw input url)
        auto result_when_disabled = SignedUrlCache::TheCache()->get_presigned_s3_url(input_url);
        CPPUNIT_ASSERT_MESSAGE("When cache is disabled, nullptr is returned", result_when_disabled == nullptr);

        // ...if we now enable the cache is enabled, we return the previously cached value
        SignedUrlCache::TheCache()->d_enabled = true;
        CPPUNIT_ASSERT_MESSAGE("Cache is enabled", SignedUrlCache::TheCache()->is_enabled());

        auto result_when_enabled = SignedUrlCache::TheCache()->get_presigned_s3_url(input_url);
        CPPUNIT_ASSERT_MESSAGE("When cache is re-enabled, value is returned",
                               result_when_enabled != nullptr && result_when_enabled->str() == output_url->str());
    }

    void dump_test() {
        SignedUrlCache *theCache = SignedUrlCache::TheCache();

        // Add values to each type of subcache
        SignedUrlCache::TheCache()->cache_presigned_s3_url("www.once.com",
                                                           make_shared<http::EffectiveUrl>("http://www.upon.com"));
        SignedUrlCache::TheCache()->cache_presigned_s3_url("www.a.com",
                                                           make_shared<http::EffectiveUrl>("http://www.time.com"));

        auto value =
            make_shared<SignedUrlCache::S3AccessKeyTuple>("a man", "a plan", "a canal", "3035-07-16 02:20:33+00:00");
        theCache->d_tea_endpoint_sts_credentials_cache.insert(
            pair<string, shared_ptr<SignedUrlCache::S3AccessKeyTuple>>("palindrome", value));

        theCache->d_href_to_tea_endpoint_cache.insert(pair<string, string>("foo", "whee"));
        theCache->d_href_to_s3_uri_cache.insert(pair<string, string>("yee", "haw"));

        // Check to make sure dump includes them
        auto strm = std::ostringstream();
        theCache->dump(strm);
        // Remove start of string to skip address that varies
        auto result = strm.str().substr(49);
        std::string expected_str =
            string("presigned url list:") + "\n        www.a.com:test_user --> http://www.time.com" +
            "\n        www.once.com:test_user --> http://www.upon.com" +
            "\n    href-to-s3credentials list:" + "\n        foo --> whee" +
            "\n    href-to-s3url list:" + "\n        yee --> haw" +
            "\n    s3 credentials list:" + "\n        palindrome --> Expires: 3035-07-16 02:20:33+00:00\n";
        CPPUNIT_ASSERT_MESSAGE("The dump should contain:\n" + expected_str + "\n\nbut did not; INSTEAD was\n" + result,
                               result.find(expected_str) != std::string::npos);
    }

    void is_timestamp_after_now_test() {
        std::string str_old("1980-07-16 18:40:58+00:00");
        CPPUNIT_ASSERT_MESSAGE("Ancient timestamp is before now", !SignedUrlCache::is_timestamp_after_now(str_old));

        std::string str_future("2135-07-16 02:20:33+00:00");
        CPPUNIT_ASSERT_MESSAGE("Future timestamp is after now", SignedUrlCache::is_timestamp_after_now(str_future));

        std::string str_invalid("invalid timestamp woo hooray huzzah");
        CPPUNIT_ASSERT_MESSAGE("Invalid timestamp is not after now",
                               !SignedUrlCache::is_timestamp_after_now(str_invalid));
    }

    void retrieve_cached_s3credentials_test() {
        std::string credentials_endpoint_url = "i_am_an_s3credentials_endpoint";
        std::string cache_key = credentials_endpoint_url + ":test_user";
        auto result_not_in_cache =
            SignedUrlCache::TheCache()->retrieve_cached_sts_credentials(credentials_endpoint_url);
        CPPUNIT_ASSERT_MESSAGE("Cache miss should return null", result_not_in_cache == nullptr);

        auto value =
            make_shared<SignedUrlCache::S3AccessKeyTuple>("a man", "a plan", "a canal", "2135-07-16 02:20:33+00:00");
        SignedUrlCache::TheCache()->d_tea_endpoint_sts_credentials_cache.insert(
            pair<string, shared_ptr<SignedUrlCache::S3AccessKeyTuple>>(cache_key, value));
        auto result_in_cache = SignedUrlCache::TheCache()->retrieve_cached_sts_credentials(credentials_endpoint_url);
        CPPUNIT_ASSERT_MESSAGE("Cache hit should successfully retrieve result", result_in_cache == value);
    }

    void retrieve_cached_s3credentials_expired_credentials_test() {
        std::string credentials_endpoint_url = "i_am_an_s3credentials_endpoint";
        std::string cache_key = credentials_endpoint_url + ":test_user";
        std::string expiration_time("1980-07-16 18:40:58+00:00");
        auto value = make_shared<SignedUrlCache::S3AccessKeyTuple>("https://www.foo", "https://www.bar",
                                                                   "https://www.bat", expiration_time);
        SignedUrlCache::TheCache()->d_tea_endpoint_sts_credentials_cache.insert(
            pair<string, shared_ptr<SignedUrlCache::S3AccessKeyTuple>>(cache_key, value));

        auto result = SignedUrlCache::TheCache()->retrieve_cached_sts_credentials(credentials_endpoint_url);
        CPPUNIT_ASSERT_MESSAGE("Cached expired result should not be retrieved", result == nullptr);
        CPPUNIT_ASSERT_MESSAGE("Expired result should have been removed from cache",
                               SignedUrlCache::TheCache()->d_tea_endpoint_sts_credentials_cache.empty());
    }

    void extract_sts_credentials_from_json_response_test() {

        std::string access_key("i_am_an_access_key_id");
        std::string valid_response(string("{\n\"accessKeyId\": \"") + access_key + "\",\n" +
                                   "\"secretAccessKey\": \"i_am_a_secret_access_key\",\n" +
                                   "\"sessionToken\": \"i_am_a_fake_token\",\n" +
                                   "\"expiration\": \"3025-09-30 18:40:58+00:00\"\n" + "} ");
        auto result = SignedUrlCache::extract_sts_credentials_from_json_response(valid_response);
        CPPUNIT_ASSERT_MESSAGE("Valid json should not return nullptr", result != nullptr);
        CPPUNIT_ASSERT_MESSAGE("Access key should be returned as first value", access_key == get<0>(*result));

        CPPUNIT_ASSERT_MESSAGE("Empty string should return nullptr",
                               SignedUrlCache::extract_sts_credentials_from_json_response("") == nullptr);
        CPPUNIT_ASSERT_MESSAGE("Invalid json should return nullptr",
                               SignedUrlCache::extract_sts_credentials_from_json_response("{foo}") == nullptr);

        std::string invalid_response(string("{\n\"accessKeyId\": \"") + access_key + "\",\n" +
                                     "\"secretAccessKey\": \"i_am_a_secret_access_key\",\n" +
                                     "\"sessionToken\": \"i_am_a_fake_token\",\n" + "} ");
        CPPUNIT_ASSERT_MESSAGE("Response missing field should return nullptr",
                               SignedUrlCache::extract_sts_credentials_from_json_response(invalid_response) == nullptr);

        std::string invalid_response_contents(
            string("{\n\"accessKeyId\": \"") + access_key + "\",\n" +
            "\"secretAccessKey\": [3, 4, 5],\n" + // Oh no! An array instead of a string!! Horrors!
            "\"sessionToken\": \"i_am_a_fake_token\",\n" + "\"expiration\": \"3025-09-30 18:40:58+00:00\"\n" + "} ");
        CPPUNIT_ASSERT_MESSAGE("Field with non-string response should return nullptr",
                               SignedUrlCache::extract_sts_credentials_from_json_response(invalid_response_contents) ==
                                   nullptr);
    }

    void extract_sts_credentials_from_json_response_test_nsidc() {

        std::ifstream file((string)TEST_SRC_DIR + "/testdata/mock_s3credentials_nsidc.json");
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string json_str = buffer.str();

        auto result = SignedUrlCache::extract_sts_credentials_from_json_response(json_str);
        CPPUNIT_ASSERT_MESSAGE("Valid json should not return nullptr", result != nullptr);

        auto access_key = get<0>(*result);
        std::string expected_access_key = "NOTaREALtokenObviously";
        CPPUNIT_ASSERT_MESSAGE("Access key should be " + expected_access_key + " was " + access_key,
                               expected_access_key == access_key);

        auto secret_access_key = get<1>(*result);
        std::string expected_secret_access_key = "sGNLIiXUuAlsoNotRealWhee339N";
        CPPUNIT_ASSERT_MESSAGE("Secret access key should be " + expected_secret_access_key + " was " +
                                   secret_access_key,
                               expected_secret_access_key == secret_access_key);

        auto token = get<2>(*result);
        std::string expected_token =
            "IQStillNotRealButKeepingExpectedCharactersInR/+XzThisBitHasSomeStuffyj/cgLDbAlsoThisssssss4y5K4BplC5xczsI/"
            "RWheeLookAtAllThisSecretq+StuffThatIsInHere+ed2/5nButAtLeastItLooksLikeAnOriginalResponse/Or+Something/"
            "Like+ThatBecauseI+Started+WithARealResponseAndThenSwitchedStuffAround/ButThereAreThisManySlashes/"
            "and+Also++plusSigns/forsomereason+and/"
            "luckilythosehandlejustfine+thankYouJson+hereAreSomeEqualsForGoodMeasure==";
        CPPUNIT_ASSERT_MESSAGE("Token should be " + expected_token + " was " + token, expected_token == token);

        auto expiration = get<3>(*result);
        std::string expected_expiration = "2026-05-22 16:48:37+00:00";
        CPPUNIT_ASSERT_MESSAGE("Expiration should be " + expected_expiration + " was " + expiration,
                               expected_expiration == expiration);
    }

    void cache_signed_url_components_test() {
        SignedUrlCache *theCache = SignedUrlCache::TheCache();

        theCache->cache_prerequisites_for_url_signing("", "foo", "bar");
        CPPUNIT_ASSERT_MESSAGE("Empty key_href_url results in no caching",
                               theCache->d_href_to_s3_uri_cache.empty() &&
                                   theCache->d_href_to_tea_endpoint_cache.empty());

        theCache->cache_prerequisites_for_url_signing("foo", "", "bar");
        CPPUNIT_ASSERT_MESSAGE("Empty s3_uri results in no caching",
                               theCache->d_href_to_s3_uri_cache.empty() &&
                                   theCache->d_href_to_tea_endpoint_cache.empty());

        theCache->cache_prerequisites_for_url_signing("foo", "bar", "");
        CPPUNIT_ASSERT_MESSAGE("Empty tea_endpoint_url results in no caching",
                               theCache->d_href_to_s3_uri_cache.empty() &&
                                   theCache->d_href_to_tea_endpoint_cache.empty());

        theCache->cache_prerequisites_for_url_signing("foo", "bar", "bat");
        CPPUNIT_ASSERT_MESSAGE("s3_uri should be cached", theCache->d_href_to_s3_uri_cache["foo"] == "bar");
        CPPUNIT_ASSERT_MESSAGE("tea_endpoint_url should be cached",
                               theCache->d_href_to_tea_endpoint_cache["foo"] == "bat");
    }

    void retrieve_cached_signed_url_components_test() {
        SignedUrlCache *theCache = SignedUrlCache::TheCache();

        theCache->cache_prerequisites_for_url_signing("two fish", "red fish", "blue fish");
        auto retrieved = theCache->retrieve_cached_prerequisites_for_url_signing("two fish");
        auto expected = std::pair<std::string, std::string>("red fish", "blue fish");
        CPPUNIT_ASSERT_MESSAGE("Cached components should be retrieved", expected == retrieved);

        std::string key("little_bo_peep");
        theCache->d_href_to_s3_uri_cache.insert(pair<string, string>(key, "goat"));
        auto retrieved2 = theCache->retrieve_cached_prerequisites_for_url_signing(key);
        auto empty_pair = std::pair<std::string, std::string>("", "");
        CPPUNIT_ASSERT_MESSAGE("If both urls were not cached, no response is returned", retrieved2 == empty_pair);
    }

    void append_edl_username_to_key_test() {
        SignedUrlCache *theCache = SignedUrlCache::TheCache();

        std::string expected = "foo_url:test_user";
        auto key = theCache->append_edl_username_to_key("foo_url");
        CPPUNIT_ASSERT_MESSAGE("EDL user is appended to key; expected " + expected + " got " + key, key == expected);

        BESContextManager::TheManager()->unset_context(EDL_UID_KEY);
        auto key_no_uid = theCache->append_edl_username_to_key("foo_url");
        CPPUNIT_ASSERT_MESSAGE("When EDL user is empty, key should be identity; expected foo_url got " + key_no_uid,
                               key_no_uid == "foo_url");
    }

    void cache_sts_credentials_from_tea_endpoint_test() {
        SignedUrlCache *theCache = SignedUrlCache::TheCache();

        CPPUNIT_ASSERT_MESSAGE("Credentials cache is empty", theCache->d_tea_endpoint_sts_credentials_cache.empty());

        auto result_bad = theCache->cache_sts_credentials_from_tea_endpoint("http://badurl");
        CPPUNIT_ASSERT_MESSAGE("After attempting fetch from invalid url, credentials cache is still empty",
                               theCache->d_tea_endpoint_sts_credentials_cache.empty());
        CPPUNIT_ASSERT_MESSAGE("Fetch from invalid url returns nullptr", result_bad == nullptr);

        // Note: we do not test a successful endpoint here, as that would require either
        // relying on TEA or setting up a test url with fake credentials,
        // both of which are brittle in their own ways.
        // The inputs/outputs for good and bad retrieval are covered by other tests
    }

    void sign_s3_uri_with_sts_credentials_test() {
        SignedUrlCache *theCache = SignedUrlCache::TheCache();
        auto s3_access_key_tuple =
            make_shared<SignedUrlCache::S3AccessKeyTuple>("a man", "a plan", "a canal", "2126-07-16 18:40:58+04:00");

        CPPUNIT_ASSERT_MESSAGE("Empty uri does not return a signed url but does not throw error",
                               !theCache->sign_s3_uri_with_sts_credentials("", s3_access_key_tuple));
        CPPUNIT_ASSERT_MESSAGE("Invalid s3 uri does not return a signed url but does not throw error",
                               !theCache->sign_s3_uri_with_sts_credentials("foo", s3_access_key_tuple));
        auto output = theCache->sign_s3_uri_with_sts_credentials("s3://bucket/key-that-is-somehow-leading-to-failure",
                                                                 s3_access_key_tuple);

        // Test intermediate reasons that this has failed...
        CPPUNIT_ASSERT_MESSAGE("Token hasn't expired...",
                               theCache->num_seconds_until_expiration(get<3>(*s3_access_key_tuple)) > 0);

        CPPUNIT_ASSERT_MESSAGE("Valid object should return a signed url, regardless of validity of credentials",
                               output != nullptr && !output->str().empty());

        CPPUNIT_ASSERT_MESSAGE("Missing credentials does not return a signed url but does not error",
                               !theCache->sign_s3_uri_with_sts_credentials("s3://bucket/key", nullptr));
    }

    void get_presigned_s3_url_test() {
        SignedUrlCache *theCache = SignedUrlCache::TheCache();
        // Force region to be supported, for the sake of testing
        bes::AWS_SDK aws_sdk;
        SignedUrlCache::TheCache()->d_aws_region_in_which_direct_copy_is_supported =
            aws_sdk.get_aws_region_of_running_application();
        std::shared_ptr<http::url> test_url = make_shared<http::url>("https://www.this-is-a-test.com");

        CPPUNIT_ASSERT_MESSAGE("When cache is disabled, return nullptr output without throwing error",
                               !theCache->get_presigned_s3_url(test_url));

        // The cache is disabled in bes.conf, so we need to turn it on.
        SignedUrlCache::TheCache()->d_enabled = true;

        CPPUNIT_ASSERT_MESSAGE("For invalid input, return nullptr output without throwing error",
                               !theCache->get_presigned_s3_url(nullptr));

        CPPUNIT_ASSERT_MESSAGE(
            "When no credentials available for the input, return nullptr output without throwing error",
            !theCache->get_presigned_s3_url(test_url));

        // Let's fake some pre-cached s3 credentials to test a GOOD response
        theCache->d_href_to_s3_uri_cache.insert(pair<string, string>(test_url->str(), "s3://foo/bar"));
        theCache->d_href_to_tea_endpoint_cache.insert(pair<string, string>(test_url->str(), "fake-tea-endpoint-name"));
        auto fake_sts_creds =
            make_shared<SignedUrlCache::S3AccessKeyTuple>("a man", "a plan", "a canal", "2126-07-16 18:40:58+04:00");
        theCache->d_tea_endpoint_sts_credentials_cache.insert(
            pair<string, shared_ptr<SignedUrlCache::S3AccessKeyTuple>>("fake-tea-endpoint-name:test_user",
                                                                       fake_sts_creds));
        CPPUNIT_ASSERT_MESSAGE("Signed URL cache should be empty before any valid responses",
                               theCache->d_presigned_s3_urls_cache.size() == 0);

        auto result = theCache->get_presigned_s3_url(test_url);
        CPPUNIT_ASSERT_MESSAGE("When credentials available for a url, return signed url",
                               result != nullptr && !result->str().empty());
        CPPUNIT_ASSERT_MESSAGE("Generated signed url should have been cached",
                               theCache->d_presigned_s3_urls_cache.size() == 1);
        CPPUNIT_ASSERT_MESSAGE("When url has been precached, return it",
                               theCache->get_presigned_s3_url(test_url)->str() == result->str());
        CPPUNIT_ASSERT_MESSAGE("Generated signed url should have been cached",
                               theCache->d_presigned_s3_urls_cache.size() == 1);

        auto result_for_signed_input = theCache->get_presigned_s3_url(result);
        CPPUNIT_ASSERT_MESSAGE("When input url is already signed, return it",
                               result_for_signed_input != nullptr && result_for_signed_input->str() == result->str());

        // Update request in the cache to be expired, so that we can see that the updated url is cached in its place
        auto new_key = make_shared<http::url>("https://www.this-is-a-test.com");
        std::string cached_presigned_url =
            "https://foo.s3.us-east-1.amazonaws.com/"
            "bar?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=a%20man%2F20260514%2Fus-east-1%2Fs3%2Faws4_request&"
            "X-Amz-Date=20260514T221009Z&X-Amz-Expires=31846277424&X-Amz-Security-Token=a%20canal&X-Amz-SignedHeaders="
            "host&X-Amz-Signature=99d4cae916dc174f8f36e1e5ddb881f19ff32f84e24e4021deefc8d783c1e40f";
        theCache->d_presigned_s3_urls_cache[new_key->str() + ":test_user"] =
            make_shared<http::EffectiveUrl>(cached_presigned_url);
        CPPUNIT_ASSERT_MESSAGE("When signed url has been precached, return cached value",
                               theCache->get_presigned_s3_url(new_key)->str() == cached_presigned_url);
        std::string expired_cached_url =
            "https://foo.s3.us-east-1.amazonaws.com/"
            "bar?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=a%20man%2F20260514%2Fus-east-1%2Fs3%2Faws4_request&"
            "X-Amz-Date=20200621T161744Z&X-Amz-Expires=86400&X-Amz-Security-Token=a%20canal&X-Amz-SignedHeaders="
            "host&X-Amz-Signature=99d4cae916dc174f8f36e1e5ddb881f19ff32f84e24e4021deefc8d783c1e40f";
        theCache->d_presigned_s3_urls_cache[new_key->str() + ":test_user"] =
            make_shared<http::EffectiveUrl>(expired_cached_url);
        auto regenerated_url = theCache->get_presigned_s3_url(new_key);
        CPPUNIT_ASSERT_MESSAGE("When cached url has expired, regenerate it on request " + regenerated_url->str(),
                               regenerated_url->str() != expired_cached_url);

        // Set up valid credentials for a bad request url, so that we can see that the request still fails
        std::shared_ptr<http::url> bad_request_key(new http::url("not-a-url"));
        theCache->d_href_to_s3_uri_cache.insert(pair<string, string>(bad_request_key->str(), "s3://foo/bar"));
        theCache->d_href_to_tea_endpoint_cache.insert(
            pair<string, string>(bad_request_key->str(), "fake-tea-endpoint-name-2"));
        theCache->d_tea_endpoint_sts_credentials_cache.insert(
            pair<string, shared_ptr<SignedUrlCache::S3AccessKeyTuple>>("fake-tea-endpoint-name-2", fake_sts_creds));
        CPPUNIT_ASSERT_MESSAGE(
            "When credentials available for input that is not a url, return nullptr output without throwing error",
            !theCache->get_presigned_s3_url(bad_request_key));
    }

    void split_s3_url_test() {
        auto out1 = SignedUrlCache::split_s3_uri("s3://foo/bar");
        auto expected1 = std::pair<std::string, std::string>("foo", "bar");
        CPPUNIT_ASSERT_MESSAGE("Valid input is split into `" + get<0>(expected1) + "," + get<1>(expected1) +
                                   "`, got `" + get<0>(out1) + "," + get<1>(out1) + "`",
                               expected1 == out1);

        auto expected_empty = std::pair<std::string, std::string>("", "");
        auto out2 = SignedUrlCache::split_s3_uri("foo/bar");
        CPPUNIT_ASSERT_MESSAGE("Missing s3:// prefix should return empty strings; returned `" + get<0>(out2) + "," +
                                   get<1>(out2) + "`",
                               expected_empty == out2);
        auto out3 = SignedUrlCache::split_s3_uri("x");
        CPPUNIT_ASSERT_MESSAGE("Missing s3:// prefix should return empty strings; returned `" + get<0>(out3) + "," +
                                   get<1>(out3) + "`",
                               expected_empty == out2);

        auto out4 = SignedUrlCache::split_s3_uri("s3://foo/bar/wheeyay.txt");
        auto expected4 = std::pair<std::string, std::string>("foo", "bar/wheeyay.txt");
        CPPUNIT_ASSERT_MESSAGE("Valid input is split into `" + get<0>(expected4) + "," + get<1>(expected4) +
                                   "`, got `" + get<0>(out4) + "," + get<1>(out4) + "`",
                               expected4 == out4);
    }

    std::chrono::system_clock::time_point parse_as_time_point(const std::string &datetime_string) {
        std::tm timestamp_time = {};
        strptime(datetime_string.c_str(), "%F %T%z", &timestamp_time);
        return std::chrono::system_clock::from_time_t(std::mktime(&timestamp_time));
    }

    void num_seconds_until_expiration_test() {
        std::string current_time_str = "2025-04-01 09:30:00+0400";
        std::string current_time_aws_str = "2025-04-01 09:30:00+04:00";
        std::chrono::system_clock::time_point current_time = parse_as_time_point(current_time_str);

        auto out = SignedUrlCache::num_seconds_until_expiration(current_time_aws_str, current_time);
        CPPUNIT_ASSERT_MESSAGE("Expiration time `now` should return 0s` for `" + current_time_aws_str + "`: `" +
                                   to_string(out) + "`",
                               out == 0);

        auto time_plus_five_seconds_aws_str = "2025-04-01 09:30:05+04:00";
        auto out1 = SignedUrlCache::num_seconds_until_expiration(time_plus_five_seconds_aws_str, current_time);
        CPPUNIT_ASSERT_MESSAGE("Expiration date should be 5s from now `" + to_string(out1) + "`", out1 == 5);

        auto time_minus_five_seconds_aws_str = "2025-04-01 09:29:55+04:00";
        auto out2 = SignedUrlCache::num_seconds_until_expiration(time_minus_five_seconds_aws_str, current_time);
        CPPUNIT_ASSERT_MESSAGE("Expiration date in the past should return 0s `" + to_string(out2) + "`", out2 == 0);

        std::string expiration_time_past("1980-07-16 18:40:58+04:00");
        auto out3 = SignedUrlCache::num_seconds_until_expiration(expiration_time_past);
        CPPUNIT_ASSERT_MESSAGE("Expiration time in past should return 0s `" + to_string(out3) + "`", out3 == 0);

        std::string expiration_time_future("2126-07-16 18:40:58+04:00");
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
    CPPUNIT_TEST(is_cache_enabled_test);
    CPPUNIT_TEST(is_cache_supported_within_current_aws_region_test);
    CPPUNIT_TEST(cache_enabled_disabled_test);
    CPPUNIT_TEST(dump_test);

    // Test behavior specific to SignedUrlCache:
    CPPUNIT_TEST(is_timestamp_after_now_test);
    CPPUNIT_TEST(retrieve_cached_s3credentials_test);
    CPPUNIT_TEST(retrieve_cached_s3credentials_expired_credentials_test);
    CPPUNIT_TEST(extract_sts_credentials_from_json_response_test);
    CPPUNIT_TEST(extract_sts_credentials_from_json_response_test_nsidc);

    CPPUNIT_TEST(cache_signed_url_components_test);
    CPPUNIT_TEST(retrieve_cached_signed_url_components_test);
    CPPUNIT_TEST(append_edl_username_to_key_test);
    CPPUNIT_TEST(cache_sts_credentials_from_tea_endpoint_test);

    // ...and, specifically, the signing itself:
    CPPUNIT_TEST(sign_s3_uri_with_sts_credentials_test);
    CPPUNIT_TEST(get_presigned_s3_url_test);

    // Last but not least, test those helper functions
    CPPUNIT_TEST(split_s3_url_test);
    CPPUNIT_TEST(num_seconds_until_expiration_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(SignedUrlCacheTest);

} // namespace bes

int main(int argc, char *argv[]) { return bes_run_tests<bes::SignedUrlCacheTest>(argc, argv, "cerr,bes,http") ? 0 : 1; }