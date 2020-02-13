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
#include <unistd.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <GetOpt.h>
#include <util.h>
#include <debug.h>

#include <curl/curl.h>
#include "xml2json/include/xml2json.hpp"

#include "xml2json/include/rapidjson/document.h"
#include "xml2json/include/rapidjson/writer.h"

#if HAVE_CURL_MULTI_H
#include <curl/multi.h>
#endif

#include "BESContextManager.h"
#include "BESError.h"
#include "BESInternalError.h"
#include "BESDebug.h"
#include "TheBESKeys.h"

#include "test_config.h"

using namespace libdap;

static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)

namespace dmrpp {


    /*
    * @brief Callback passed to libcurl to handle reading a single byte.
    *
    * This callback assumes that the size of the data is small enough
    * that all of the bytes will be either read at once or that a local
            * temporary buffer can be used to build up the values.
    *
    * @param buffer Data from libcurl
    * @param size Number of bytes
    * @param nmemb Total size of data in this call is 'size * nmemb'
    * @param data Pointer to this
    * @return The number of bytes read
    */
    size_t ngap_write_data(void *buffer, size_t size, size_t nmemb, void *data) {
        size_t nbytes = size * nmemb;

        // Peek into the bytes read and look for an error from the object store.
        // Error messages always start off with '<?xml' so only check for one if we have more than
        // four characters in 'buffer.' jhrg 12/17/19
        if (nbytes > 4) {
            string peek(reinterpret_cast<const char *>(buffer), 5);
            if (peek == "<?xml") {
                // At this point we no longer care about great performance - error msg readability
                // is more important. jhrg 12/30/19
                string xml_message = reinterpret_cast<const char *>(buffer);
                xml_message.erase(xml_message.find_last_not_of("\t\n\v\f\r 0") + 1);
                // Decode the AWS XML error message. In some cases this will fail because pub keys,
                // which maybe in this error text, may have < or > chars in them. the XML parser
                // will be sad if that happens. jhrg 12/30/19
                try {
                    string json_message = xml2json(xml_message.c_str());
                    BESDEBUG("dmrpp", "AWS S3 Access Error:" << json_message << endl);

                    rapidjson::Document d;
                    d.Parse(json_message.c_str());
                    rapidjson::Value &s = d["Error"]["Message"];
                    // We might want to get the "Code" from the "Error" if these text messages
                    // are not good enough. But the "Code" is not really suitable for normal humans...
                    // jhrg 12/31/19

                    throw BESInternalError(string("Error accessing remote. Message: ").append(s.GetString()), __FILE__,
                                           __LINE__);
                }
                catch (BESInternalError) {
                    // re-throw BESSyntaxUserError - added for the future if we make BESError a child
                    // of std::exception as it should be. jhrg 12/30/19
                    throw;
                }
                catch (std::exception &e) {
                    BESDEBUG("dmrpp", " Access Error:" << xml_message << endl);
                    throw BESInternalError(string("Error accessing remote. Message."), __FILE__, __LINE__);
                }
            }
        }
    }

    class NgapCredentialsTest : public CppUnit::TestFixture {
    private:
        string weak_config;
        char d_errbuf[CURL_ERROR_SIZE]; ///< raw error message info from libcurl


        string curl_error_msg(CURLcode res, char *errbuf) {
            ostringstream oss;
            size_t len = strlen(errbuf);
            if (len) {
                oss << errbuf;
                oss << " (code: " << (int) res << ")";
            } else {
                oss << curl_easy_strerror(res) << "(result: " << res << ")";
            }

            return oss.str();
        }

    public:
        string cm_config;

        // Called once before everything gets tested
        NgapCredentialsTest() {
        }

        // Called at the end of the test
        ~NgapCredentialsTest() {
        }

        // Called before each test
        void setUp() {
            if (debug) cout << endl;
            if (bes_debug) BESDebug::SetUp("cerr,dmrpp");

            TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");
            cm_config = string(TEST_BUILD_DIR).append("/credentials.conf");
            weak_config = string(TEST_SRC_DIR).append("/input-files/weak.conf");

        }

        // Called after each test
        void tearDown() {
        }


        /**
         *
         * @param target_url
         * @return
         */
        CURL *set_up_curl_handle(string target_url, char *response_buff) {
            CURL *d_handle;     ///< The libcurl handle object.
            d_handle = curl_easy_init();
            CPPUNIT_ASSERT(d_handle);

            CURLcode res;

            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_ERRORBUFFER, d_errbuf)))
                throw BESInternalError(string("CURL Error: ").append(curl_easy_strerror(res)), __FILE__, __LINE__);

            // Pass all data to the 'write_data' function
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_WRITEFUNCTION, dmrpp::ngap_write_data)))
                throw BESInternalError(string("CURL Error: ").append(curl_error_msg(res, d_errbuf)), __FILE__,
                                       __LINE__);

            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_URL, target_url.c_str())))
                throw BESInternalError(string("HTTP Error setting URL: ").append(curl_error_msg(res, d_errbuf)),
                                       __FILE__, __LINE__);

#if 0
            // get the offset to offset + size bytes
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_RANGE, chunk->get_curl_range_arg_string().c_str())))
                throw BESInternalError(string("HTTP Error setting Range: ").append(curl_error_msg(res, handle->d_errbuf)), __FILE__,
                                       __LINE__);
#endif
            // Pass this to write_data as the fourth argument
            if (CURLE_OK !=
                (res = curl_easy_setopt(d_handle, CURLOPT_WRITEDATA, reinterpret_cast<void *>(response_buff))))
                throw BESInternalError(
                        string("CURL Error setting chunk as data buffer: ").append(curl_error_msg(res, d_errbuf)),
                        __FILE__, __LINE__);
#if 0
            // store the easy_handle so that we can call release_handle in multi_handle::read_data()
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_PRIVATE, reinterpret_cast<void*>(handle))))
                throw BESInternalError(string("CURL Error setting easy_handle as private data: ").append(curl_error_msg(res, handle->d_errbuf)), __FILE__,
                                       __LINE__);
#endif

        }

        bool evaluate_curl_response(CURL *eh) {
            long http_code = 0;
            CURLcode res = curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &http_code);
            if (CURLE_OK != res) {
                throw BESInternalError(
                        string("Error getting HTTP response code: ").append(curl_error_msg(res, (char *) "")),
                        __FILE__, __LINE__);
            }

            // Newer Apache servers return 206 for range requests. jhrg 8/8/18
            switch (http_code) {
                case 200: // OK
                case 206: // Partial content - this is to be expected since we use range gets
                    // cases 201-205 are things we should probably reject, unless we add more
                    // comprehensive HTTP/S processing here. jhrg 8/8/18
                    return true;

                case 500: // Internal server error
                case 503: // Service Unavailable
                case 504: // Gateway Timeout
                    return false;

                default: {
                    ostringstream oss;
                    oss << "HTTP status error: Expected an OK status, but got: ";
                    oss << http_code;
                    throw BESInternalError(oss.str(), __FILE__, __LINE__);
                }
            }
        }

        void get_s3_creds() {
            string distribution_api_endpoint = "https://d33imu0z1ajyhj.cloudfront.net/s3credentials";
            string fnoc1_dds = "http://test.opendap.org/opendap/data/nc/fnoc1.nc.dds";

            string target_url = fnoc1_dds;

            char response_buf[1024 * 1024];
            try {
                CURL *c_handle = set_up_curl_handle(target_url, response_buf);
                read_data(c_handle, target_url);
                string response(response_buf);
                cout << response << endl;
            }
            catch (BESError e) {
                cerr << "Caught BESError. Message: " << e.get_message() << "  ";
                cerr << "[" << e.get_file() << ":" << e.get_line() << "]" << endl;
                CPPUNIT_ASSERT(false);
            }


        }

        void read_data(CURL *c_handle, string url) {

            unsigned int tries = 0;
            unsigned int retry_limit = 3;
            useconds_t retry_time = 1000;
            bool success;

            do {
                d_errbuf[0] = NULL;
                CURLcode curl_code = curl_easy_perform(c_handle);
                ++tries;

                if (CURLE_OK != curl_code) {
                    throw BESInternalError(
                            string("read_data() - ERROR! Message: ").append(curl_error_msg(curl_code, d_errbuf)),
                            __FILE__, __LINE__);
                }

                success = evaluate_curl_response(c_handle);

                if (!success) {
                    if (tries == retry_limit) {
                        throw BESInternalError(
                                string("Data transfer error: Number of re-tries to S3 exceeded: ").append(
                                        curl_error_msg(curl_code, d_errbuf)), __FILE__, __LINE__);
                    } else {
                        BESDEBUG("dmrpp",
                                 "HTTP transfer 500 error, will retry (trial " << tries << " for: " << url << ").");
                        usleep(retry_time);
                        retry_time *= 2;
                    }
                }
#if 0
                curl_slist_free_all(d_headers);
                d_headers = 0;
#endif
            } while (!success);
        }


    CPPUNIT_TEST_SUITE(NgapCredentialsTest);
            static string cm_config;

            ;

            CPPUNIT_TEST(get_s3_creds);

        CPPUNIT_TEST_SUITE_END();

    };

    CPPUNIT_TEST_SUITE_REGISTRATION(NgapCredentialsTest);


} // namespace dmrpp

int main(int argc, char *argv[]) {
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());
    string cm_cnf = "";
    GetOpt getopt(argc, argv, "c:db");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
            case 'c':
                cm_cnf = getopt.optarg;
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
    } else {
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            string test = dmrpp::NgapCredentialsTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
