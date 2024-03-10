// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES component of the Hyrax Data Server.

// Copyright (c) 2018,2023 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>,
// James Gallagher <jgallagher@opendap.org>
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
#include <thread>
#include <future>
#include <chrono>
#include <vector>

#include <cstdlib>
#include <cerrno>
#include <unistd.h>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include "BESError.h"
#include "BESInternalError.h"
#include "BESDebug.h"
#include "TheBESKeys.h"

#include "RemoteResource.h"
#include "test_config.h"

using namespace std;

bool debug = false;
bool Debug = false;
bool bes_debug = false;
bool purge_cache = false;
bool ngap_tests = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false)
#undef DBG2
#define DBG2(x) do { if (Debug) (x); } while(false)

#define prolog std::string("RemoteResourceTest::").append(__func__).append("() - ")

namespace http {

class RemoteResourceTest : public CppUnit::TestFixture {
private:

    static string get_file_as_string(const string &filename) {
        ifstream t(filename);

        if (t.is_open()) {
            string file_content((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
            t.close();
            DBG2(cerr << endl << "#############################################################################"
                      << endl);
            DBG2(cerr << "file: " << filename << endl);
            DBG2(cerr << ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl);
            DBG2(cerr << file_content << endl);
            DBG2(cerr << "#############################################################################" << endl);
            return file_content;
        }
        else {
            CPPUNIT_FAIL(prolog + "FAILED TO OPEN FILE: " + filename);
        }
        return "";
    }

    static long long file_size(const string &filename) {
        ifstream in_file(filename, ios::binary);
        in_file.seekg(0, ios::end);
        return in_file.tellg();
    }

public:
    // Called once before everything gets tested
    RemoteResourceTest() = default;

    // Called at the end of the test
    ~RemoteResourceTest() override = default;

    // Called before each test
    void setUp() override {
        debug=true;
        TheBESKeys::ConfigFile = string(TEST_BUILD_DIR) + "/bes.conf";
        if (bes_debug) BESDebug::SetUp("cerr,rr,bes,http,curl");

        // Force re-init for every test. We need this because the temp file
        // dir is removed in the tearDown() method.
        RemoteResource::d_temp_file_dir = "";
    }

    void tearDown() override {
        DBG(cerr << prolog << "RemoteResource::d_temp_file_dir: " << RemoteResource::d_temp_file_dir << endl);
        if (access(RemoteResource::d_temp_file_dir.c_str(), F_OK) == 0) {
            DBG(cerr << prolog << "Removing temp file dir: " << RemoteResource::d_temp_file_dir << endl);
            if (system(("rm -rf " + RemoteResource::d_temp_file_dir).c_str()) != 0) {
                throw BESInternalError("Failed to remove temp file dir: " + RemoteResource::d_temp_file_dir + ": "
                                       + strerror(errno), __FILE__, __LINE__);
            }
        }
    }

/*##################################################################################################*/
/* TESTS BEGIN */

    void get_http_url_test() {
        DBG(cerr << "|--------------------------------------------------|" << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        string url = "http://test.opendap.org/data/httpd_catalog/READTHIS";
        DBG(cerr << prolog << "url: " << url << endl);
        http::RemoteResource rhr(make_shared<http::url>(url));
        try {
            rhr.retrieve_resource();
            string filename = rhr.get_filename();
            DBG(cerr << prolog << "filename: " << filename << endl);
            string expected_content("This is a test. If this was not a test you would have known the answer.\n");
            DBG(cerr << prolog << "expected_content string: " << expected_content << endl);
            string content = get_file_as_string(filename);
            DBG(cerr << prolog << "retrieved content: " << content << endl);
            CPPUNIT_ASSERT(content == expected_content);
        }
        catch (const BESError &e) {
            CPPUNIT_FAIL("Caught BESError! message: " + e.get_verbose_message());
        }
        DBG(cerr << prolog << "END" << endl);
    }

    // A multi-threaded test of retrieveResource().
    void get_http_url_test_mt() {
        DBG(cerr << "|--------------------------------------------------|" << endl);

         vector<string> urls = {"http://test.opendap.org/data/httpd_catalog/READTHIS",
                                "http://test.opendap.org/opendap/data/nc/fnoc1.nc.das",
                                "http://test.opendap.org/opendap/data/nc/fnoc1.nc.dds",
                                "http://test.opendap.org/opendap/data/nc/fnoc1.nc.dmr",
                                "http://test.opendap.org/opendap/data/nc/fnoc1.nc.dap",
                                "http://test.opendap.org/opendap/data/nc/fnoc1.nc.dods"};
        try {
            std::vector<std::future<string>> futures;

            for (auto &url : urls) {
                futures.emplace_back(std::async(std::launch::async, [](const string &url) -> string {
                    http::RemoteResource rhr(make_shared<http::url>(url));
                    DBG(cerr << "Start RR" << endl);
                    rhr.retrieve_resource();
                    DBG(cerr << "End RR" << endl);
                    CPPUNIT_ASSERT_MESSAGE("Retrieved resource should not be empty", file_size(rhr.get_filename()) > 0);
                    return {rhr.get_filename() + ", " + to_string(file_size(rhr.get_filename()))};
                }, url));
            }

            DBG(cerr << "I'm doing my own work!" << endl);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            DBG(cerr << "I'm done with my own work!" << endl);

            DBG(cerr << "Start querying" << endl);
            for (auto &future: futures) {
                CPPUNIT_ASSERT_MESSAGE("Invalid future", future.valid());
                string result;
                CPPUNIT_ASSERT_NO_THROW_MESSAGE("Expected a return value, not throw an exception", result = future.get());
                DBG(cerr << result << endl);
            }
        }
        catch (const BESError &e) {
            CPPUNIT_FAIL("Caught BESError! message: " + e.get_verbose_message());
        }
        DBG(cerr << prolog << "END" << endl);
    }

    void get_http_url_test_mt_test_return_content() {
        DBG(cerr << "|--------------------------------------------------|" << endl);

        vector<string> urls = {"http://test.opendap.org/data/httpd_catalog/READTHIS",
                               "http://test.opendap.org/data/httpd_catalog/READTHIS",
                               "http://test.opendap.org/data/httpd_catalog/READTHIS",
                               "http://test.opendap.org/data/httpd_catalog/READTHIS",
                               "http://test.opendap.org/data/httpd_catalog/READTHIS",
                               "http://test.opendap.org/data/httpd_catalog/READTHIS"};
        try {
            std::vector<std::future<string>> futures;

            for (auto &url : urls) {
                futures.emplace_back(std::async(std::launch::async, [](const string &url) -> string {
                    http::RemoteResource rhr(make_shared<http::url>(url));
                    DBG(cerr << "Start RR" << endl);
                    rhr.retrieve_resource();
                    DBG(cerr << "End RR" << endl);
                    CPPUNIT_ASSERT_MESSAGE("Retrieved resource should not be empty", file_size(rhr.get_filename()) > 0);
                    DBG(cerr << prolog << "filename: " << rhr.get_filename() << endl);
                    string expected_content("This is a test. If this was not a test you would have known the answer.\n");
                    DBG(cerr << prolog << "expected content: " << expected_content << endl);
                    string content = get_file_as_string(rhr.get_filename());
                    DBG(cerr << prolog << "retrieved content: " << content << endl);
                    CPPUNIT_ASSERT(content == expected_content);
                    return {rhr.get_filename() + ", " + to_string(file_size(rhr.get_filename()))};
                }, url));
            }

            DBG(cerr << "I'm doing my own work!" << endl);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            DBG(cerr << "I'm done with my own work!" << endl);

            DBG(cerr << "Start querying" << endl);
            for (auto &future: futures) {
                CPPUNIT_ASSERT_MESSAGE("Invalid future", future.valid());
                string result;
                CPPUNIT_ASSERT_NO_THROW_MESSAGE("Expected a return value, not throw an exception", result = future.get());
                DBG(cerr << result << endl);
            }
        }
        catch (const BESError &e) {
            CPPUNIT_FAIL("Caught BESError! message: " + e.get_verbose_message());
        }
        DBG(cerr << prolog << "END" << endl);
    }

    void get_file_url_test() {
        DBG(cerr << "|--------------------------------------------------|" << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        string data_file_url = string("file://") + TEST_DATA_DIR + "/test_file";
        auto url_ptr = make_shared<http::url>(data_file_url);
        http::RemoteResource rhr(url_ptr);
        try {
            rhr.retrieve_resource();
            string cache_filename = rhr.get_filename();
            DBG(cerr << prolog << "cache_filename: " << cache_filename << endl);

            string expected("This a TEST. Move Along...\n");
            string retrieved = get_file_as_string(cache_filename);
            DBG(cerr << prolog << "expected_content: '" << expected << "'" << endl);
            DBG(cerr << prolog << "retrieved_content: '" << retrieved << "'" << endl);
            CPPUNIT_ASSERT(retrieved == expected);
        }
        catch (const BESError &e) {
            CPPUNIT_FAIL("Caught BESError! message: " + e.get_verbose_message());
        }
        DBG(cerr << prolog << "END" << endl);
    }

    void set_delete_temp_file_default_ctor_test() {
        // Http.RemoteResource.TmpFile.Delete
        http::RemoteResource rhr;
        CPPUNIT_ASSERT_EQUAL_MESSAGE("The default value for d_delete_file should be true",
                                     true, rhr.d_delete_file);
    }

    void set_delete_temp_file_URL_default_test() {
        // Http.RemoteResource.TmpFile.Delete
        auto http_url = make_shared<http::url>("http://test.opendap.org/data/httpd_catalog/READTHIS");
        http::RemoteResource rhr(http_url);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("The default value for d_delete_file should be true for a http:// URL.",
                                     true, rhr.d_delete_file);
    }

    void set_delete_temp_file_FILE_default_test() {
        // Http.RemoteResource.TmpFile.Delete
        // Because d_delete_file is set to false for a file:// url, we need to use a http:// url for this test.
        auto http_url = make_shared<http::url>(string("file://") + TEST_DATA_DIR + "/test_file");
        http::RemoteResource rhr(http_url);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("The default value for d_delete_file should be false for a file:// URL.",
                                     false, rhr.d_delete_file);
    }

    void set_delete_temp_file_URL_test() {
        // Http.RemoteResource.TmpFile.Delete
        TheBESKeys::TheKeys()->set_key("Http.RemoteResource.TmpFile.Delete", "false");
        auto http_url = make_shared<http::url>("http://test.opendap.org/data/httpd_catalog/READTHIS");
        http::RemoteResource rhr(http_url);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("The value for d_delete_file should be false since Http.RemoteResource.TmpFile.Delete is false.",
                                     false, rhr.d_delete_file);
    }

    // Test that when the Http.RemoteResource.TmpFile.Delete key is set to true, and
    // the URL is a file:// URL, the value of d_delete_file is still false. We should
    // never delete the stuff reference by a file URL.
    void set_delete_temp_file_set_with_file_test() {
        // Http.RemoteResource.TmpFile.Delete
        TheBESKeys::TheKeys()->set_key("Http.RemoteResource.TmpFile.Delete", "true");
        auto http_url = make_shared<http::url>(string("file://") + TEST_DATA_DIR + "/test_file");
        http::RemoteResource rhr(http_url);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("The value for d_delete_file should be false for a file:// URL no matter what",
                                     false, rhr.d_delete_file);
    }

CPPUNIT_TEST_SUITE(RemoteResourceTest);

        CPPUNIT_TEST(get_http_url_test);
        CPPUNIT_TEST(get_http_url_test_mt);
        CPPUNIT_TEST(get_http_url_test_mt_test_return_content);
        CPPUNIT_TEST(get_file_url_test);

        CPPUNIT_TEST(set_delete_temp_file_default_ctor_test);
        CPPUNIT_TEST(set_delete_temp_file_URL_default_test);
        CPPUNIT_TEST(set_delete_temp_file_FILE_default_test);
        CPPUNIT_TEST(set_delete_temp_file_URL_test);
        CPPUNIT_TEST(set_delete_temp_file_set_with_file_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(RemoteResourceTest);

} // namespace httpd_catalog

int main(int argc, char *argv[]) {
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    int option_char;
    while ((option_char = getopt(argc, argv, "dbDPN")) != -1)
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
            case 'P':
                purge_cache = true;  // debug is a static global
                cerr << "purge_cache enabled" << endl;
                break;
            default:
                break;
        }

    argc -= optind;
    argv += optind;

    bool wasSuccessful = true;
    if (0 == argc) {
        wasSuccessful = runner.run("");         // run them all
    }
    else {
        for (int i = 0; i < argc; ++i) {
            DBG(cerr << "Running " << argv[i] << endl);
            string test = http::RemoteResourceTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
