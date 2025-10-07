
#include "config.h"

// TODO Remove if not used. jhrg 8/3/24
/**
 * @brief Perform an HTTP HEAD request.
 * @param target_url
 * @param tries Default is 3.
 * @param wait_time_us Default is 1'000'000 (1 second).
 * @return True if the HEAD request was successful, false otherwise.
 */
bool http_head(const string &target_url, int tries, unsigned long wait_time_us) {
    curl_slist *request_headers = nullptr;

    try {
        // TODO Factor this from here and http_get() below. jhrg 8/3/24
        // Add the authorization headers
        request_headers = add_edl_auth_headers(request_headers);

        auto url = std::make_shared<http::url>(target_url);
        request_headers = sign_url_for_s3_if_possible(url, request_headers);

#if DEVELOPER
        AccessCredentials *credentials = CredentialsManager::theCM()->get(url);
        if (credentials) {
            INFO_LOG(prolog << "Looking for EDL Token for URL: " << target_url << '\n');
            string edl_token = credentials->get("edl_token");
            if (!edl_token.empty()) {
                INFO_LOG(prolog << "Using EDL Token for URL: " << target_url << '\n');
                request_headers = curl::append_http_header(request_headers, "Authorization", edl_token);
            }
        }
#endif

        CURL *ceh = curl::init(target_url, request_headers, nullptr);
        if (!ceh)
            throw BESInternalError("Failed to acquire cURL Easy Handle!", __FILE__, __LINE__);

        vector<char> error_buffer(CURL_ERROR_SIZE, (char) 0);
        set_error_buffer(ceh, error_buffer.data());

        curl_easy_setopt(ceh, CURLOPT_NOBODY, 1L);

        int attempts = 0;
        bool status = false;
        do {
            CURLcode curl_code = curl_easy_perform(ceh);
            long http_code = 0;
            if (curl_code == CURLE_OK)
                http_code = get_http_code(ceh);
            if (curl_code == CURLE_OK && http_code == 200) {
                status = true;
                break;
            } else if (curl_code == CURLE_OK && http_code == 500) {
                usleep(wait_time_us);
                attempts++;
            } else if (curl_code != CURLE_OK || http_code != 200) {
                ERROR_LOG(string("Problem with HEAD request, HTTP response: ") + to_string(http_code) + '\n');
                status = false;
                break;
            }
        } while (attempts++ < tries);

        if (attempts >= tries) {
            ERROR_LOG(string("HEAD request, failed after ") + to_string(tries) + " attempts.\n");
            status = false;
        }

        unset_error_buffer(ceh);
        curl_easy_cleanup(ceh);
        curl_slist_free_all(request_headers);
        return status;
    }
    catch (...) {
        curl_slist_free_all(request_headers);
        throw;
    }
}

These are CurlUtilsTest.cc snippets:

#if 0
    CPPUNIT_TEST(http_head_test);
    CPPUNIT_TEST(http_head_test_404);

    CPPUNIT_TEST(http_get_test_string);
#endif


#if 0
void http_head_test() {
        DBG(cerr << prolog << "BEGIN\n");
        const string url = "http://test.opendap.org/opendap.conf";
        DBG(cerr << prolog << "Retrieving: " << url << "\n");
        bool result = curl::http_head(url);
        CPPUNIT_ASSERT_MESSAGE("The HEAD request should have succeeded.", result);
        DBG(cerr << prolog << "END\n");
    }

    void http_head_test_404() {
        DBG(cerr << prolog << "BEGIN\n");
        const string url = "http://test.opendap.org/not_there";
        DBG(cerr << prolog << "Retrieving: " << url << "\n");
        bool result = curl::http_head(url);
        CPPUNIT_ASSERT_MESSAGE("The HEAD request should have failed.", !result);
        DBG(cerr << prolog << "END\n");
    }

    // Test the http_get() function that extends as needed a C++ std::string
    void http_get_test_string() {
        DBG(cerr << prolog << "BEGIN\n");
        const string url = "http://test.opendap.org/opendap.conf";
        string str;
        DBG(cerr << prolog << "Retrieving: " << url << "\n");
        curl::http_get(url, str);
        DBG(cerr << prolog << "Response Body:\n" << str << "\n");

        DBG(cerr << "str.data() = " << string(str.data()) << "\n");
        CPPUNIT_ASSERT_MESSAGE("Should be able to find <Proxy *>", string(str.data()).find("<Proxy *>") == 0);
        CPPUNIT_ASSERT_MESSAGE("Should be able to find ProxyPassReverse...",
                               string(str.data()).find("ProxyPassReverse /dap ajp://localhost:8009/opendap") !=
                               string::npos);
        DBG(cerr << "str.size() = " << str.size() << "\n");
        CPPUNIT_ASSERT_MESSAGE("Size should be 288", str.size() == 288);
        DBG(cerr << prolog << "END\n");
    }
#endif
