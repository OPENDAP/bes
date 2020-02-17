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
#include "../curl_utils.h"

using namespace libdap;

static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)

namespace dmrpp {

    string uid;
    string pw;

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
    /*
    size_t ngap_write_data(void *buffer, size_t size, size_t nmemb, void *data) {
        size_t nbytes = size * nmemb;
        //cerr << "ngap_write_data() bytes: " << nbytes << "  size: " << size << "  nmemb: " << nmemb << " buffer: " << buffer << "  data: " << data << endl;
        memcpy(data,buffer,nbytes);
        return nbytes;
    }
    */

    class NgapCredentialsTest : public CppUnit::TestFixture {
    private:
        string weak_config;
        char d_errbuf[CURL_ERROR_SIZE]; ///< raw error message info from libcurl





        bool evaluate_curl_response(CURL *eh) {
            long http_code = 0;
            CURLcode res = curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &http_code);
            if (CURLE_OK != res) {
                throw BESInternalError(
                        string("Error getting HTTP response code: ").append(curl::error_message(res, (char *) "")),
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

        void read_data(CURL *c_handle) {

            unsigned int tries = 0;
            unsigned int retry_limit = 3;
            useconds_t retry_time = 1000;
            bool success;
            CURLcode curl_code;

            string url = "URL assignment failed.";
            char *urlp = NULL;
            curl_easy_getinfo(c_handle, CURLINFO_EFFECTIVE_URL, &urlp);
            if(!urlp)
                throw BESInternalError(url,__FILE__,__LINE__);

            url = urlp;


            do {
                d_errbuf[0] = NULL;
                curl_code = curl_easy_perform(c_handle);
                ++tries;

                if (CURLE_OK != curl_code) {
                    throw BESInternalError(
                            string("read_data() - ERROR! Message: ").append(curl::error_message(curl_code, d_errbuf)),
                            __FILE__, __LINE__);
                }

                success = evaluate_curl_response(c_handle);
                if(debug) cout << curl::probe_easy_handle(c_handle) << endl;

                if (!success) {
                    if (tries == retry_limit) {
                        throw BESInternalError(
                                string("Data transfer error: Number of re-tries to S3 exceeded: ").append(
                                        curl::error_message(curl_code, d_errbuf)), __FILE__, __LINE__);
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

            curl_global_init(CURL_GLOBAL_ALL);

        }

        // Called after each test
        void tearDown() {
            curl_global_cleanup();
        }

#if 0
        /**
         *
         * @param target_url
         * @return
         */

        CURL *set_up_curl_handle(const string &target_url, const string &cookies_file, char *response_buff) {
            CURL *d_handle;     ///< The libcurl handle object.
            d_handle = curl_easy_init();
            CPPUNIT_ASSERT(d_handle);

            CURLcode res;
            // Target URL ----------------------------------------------------------------------------------------------
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_URL, target_url.c_str())))
                throw BESInternalError(string("HTTP Error setting URL: ").append(curl::error_message(res, d_errbuf)),
                                       __FILE__, __LINE__);

            // Pass all data to the 'write_data' function --------------------------------------------------------------
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_WRITEFUNCTION, dmrpp::ngap_write_data)))
                throw BESInternalError(string("CURL Error: ").append(curl::error_message(res, d_errbuf)),
                        __FILE__, __LINE__);

            // Pass this to write_data as the fourth argument ----------------------------------------------------------
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_WRITEDATA, reinterpret_cast<void *>(response_buff))))
                throw BESInternalError(
                        string("CURL Error setting chunk as data buffer: ").append(curl::error_message(res, d_errbuf)),
                        __FILE__, __LINE__);

            // Follow redirects ----------------------------------------------------------------------------------------
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_FOLLOWLOCATION, 1L)))
                throw BESInternalError(string("Error setting CURLOPT_FOLLOWLOCATION: ").append(
                        curl::error_message(res, d_errbuf)),
                                       __FILE__, __LINE__);


            // Use cookies ----------------------------------------------------------------------------------------
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_COOKIEFILE, cookies_file.c_str())))
                throw BESInternalError(string("Error setting CURLOPT_COOKIEFILE to '").append(cookies_file).append("' msg: ").append(
                        curl::error_message(res, d_errbuf)),
                                       __FILE__, __LINE__);

            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_COOKIEJAR, cookies_file.c_str())))
                throw BESInternalError(string("Error setting CURLOPT_COOKIEJAR: ").append(
                        curl::error_message(res, d_errbuf)),
                                       __FILE__, __LINE__);


            // Auth ----------------------------------------------------------------------------------------------------
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_HTTPAUTH, (long)CURLAUTH_ANY)))
                throw BESInternalError(string("Error setting CURLOPT_HTTPAUTH to CURLAUTH_ANY msg: ").append(
                        curl::error_message(res, d_errbuf)),
                                       __FILE__, __LINE__);

#if 0
            if(debug) cout << "uid: " << uid << endl;
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_USERNAME, uid.c_str())))
                throw BESInternalError(string("Error setting CURLOPT_USERNAME: ").append(curl_error_msg(res, d_errbuf)),
                                       __FILE__, __LINE__);

            if(debug) cout << "pw: " << pw << endl;
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_PASSWORD, pw.c_str())))
                throw BESInternalError(string("Error setting CURLOPT_PASSWORD: ").append(curl_error_msg(res, d_errbuf)),
                                       __FILE__, __LINE__);

#else
            // turn on .netrc ------------------------------------------------------------------------------------------
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_NETRC, CURL_NETRC_OPTIONAL)))
                throw BESInternalError(string("Error setting CURLOPT_NETRC to CURL_NETRC_OPTIONAL: ").append(
                        curl::error_message(res, d_errbuf)),
                                       __FILE__, __LINE__);


#endif

            // Error Buffer --------------------------------------------------------------------------------------------
            if (CURLE_OK != (res = curl_easy_setopt(d_handle, CURLOPT_ERRORBUFFER, d_errbuf)))
                throw BESInternalError(string("CURL Error: ").append(curl_easy_strerror(res)), __FILE__, __LINE__);


            return d_handle;
        }
#endif

        void get_s3_creds() {
            if(debug) cout << endl;
            string distribution_api_endpoint = "https://d33imu0z1ajyhj.cloudfront.net/s3credentials";
            string fnoc1_dds = "http://test.opendap.org/opendap/data/nc/fnoc1.nc.dds";

            string local_fnoc1="http://localhost:8080/opendap/data/nc/fnoc1.nc.dds";
            string cookies = "/Users/ndp/OPeNDAP/hyrax/bes/modules/dmrpp_module/unit-tests/ursCookies";

            string target_url = distribution_api_endpoint;
            if(debug) cout << "Target URL: " << target_url<< endl;

            CURL *c_handle = NULL;
            char response_buf[1024 * 1024];
            try {
                c_handle = curl::set_up_easy_handle(target_url, cookies, response_buf);
                read_data(c_handle);
                string response(response_buf);
                cout << response << endl;
                curl_easy_cleanup(c_handle);
            }
            catch (BESError e) {
                cerr << "Caught BESError. Message: " << e.get_message() << "  ";
                cerr << "[" << e.get_file() << ":" << e.get_line() << "]" << endl;
                CPPUNIT_ASSERT(false);
            }


        }



    CPPUNIT_TEST_SUITE(NgapCredentialsTest);

            CPPUNIT_TEST(get_s3_creds);

        CPPUNIT_TEST_SUITE_END();

    };

    CPPUNIT_TEST_SUITE_REGISTRATION(NgapCredentialsTest);

    static string cm_config;

} // namespace dmrpp

int main(int argc, char *argv[]) {
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());
    string cm_cnf = "";
    dmrpp::uid = "";
    dmrpp::pw = "";

    GetOpt getopt(argc, argv, "c:dbu:p:");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
            case 'c':
                cm_cnf = getopt.optarg;
                break;
            case 'u':
                dmrpp::uid = getopt.optarg;
                break;
            case 'p':
                dmrpp::pw = getopt.optarg;
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
