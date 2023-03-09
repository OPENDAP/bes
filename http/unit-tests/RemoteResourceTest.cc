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
#include <thread>
#include <future>
#include <chrono>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <sys/stat.h>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include "BESError.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESCatalogList.h"
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
#if 0
    static void show_file(const string &filename) {
        ifstream t(filename.c_str());

        if (t.is_open()) {
            string file_content((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
            t.close();
            cerr << endl << "#############################################################################" << endl;
            cerr << "file: " << filename << endl;
            cerr << ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl;
            cerr << file_content << endl;
            cerr << "#############################################################################" << endl;
        }
        else {
            CPPUNIT_FAIL(prolog + "FAILED TO OPEN FILE: " + filename);
        }
    }

    string get_data_file_url(const string &name) const {
        string data_file = BESUtil::assemblePath(TEST_DATA_DIR, name);
        DBG(cerr << prolog << "data_file: " << data_file << endl);
        DBG2(show_file(data_file));

        string data_file_url = "file://" + data_file;
        DBG(cerr << prolog << "data_file_url: " << data_file_url << endl);
        return data_file_url;
    }



    /**
     * @brief Copy the source file to a system determined temporary file and set the rvp tmp_file to the temp file name.
     * @param src The source file to copy
     * @param tmp_file The temporary file created.
     */
    void copy_to_temp(const string &src, string &tmp_file) {
        ifstream src_is(src);
        if (!src_is.is_open()) {
            throw BESInternalError("Failed to open source file: " + src, __FILE__, __LINE__);
        }
        DBG(cerr << prolog << "ifstream opened" << endl);

        const auto pointer = tmpnam(nullptr);
        ofstream tmp_os(pointer);
        if (!tmp_os.is_open()) {
            stringstream msg;
            msg << "Failed to open temp file: " << pointer << endl;
            throw BESInternalError(msg.str(), __FILE__, __LINE__);
        }

        char buf[4096];
        do {
            src_is.read(buf, 4096);
            tmp_os.write(buf, src_is.gcount());
        } while (src_is.gcount() > 0);
        tmp_file = pointer;
    }

    /**
     * @brief Compare two text files as string values.
     * @param file_a
     * @param file_b
     * @return True the files match, False otherwise.
     */
    static bool compare(const string &file_a, const string &file_b) {
        string a_str = get_file_as_string(file_a);
        string b_str = get_file_as_string(file_b);
        return a_str == b_str;
    }
#endif

public:
    // Called once before everything gets tested
    RemoteResourceTest() = default;

    // Called at the end of the test
    ~RemoteResourceTest() override = default;

    // Called before each test
    void setUp() override {
        TheBESKeys::ConfigFile = string(TEST_BUILD_DIR) + "/bes.conf";
        if (bes_debug) BESDebug::SetUp("cerr,rr,bes,http,curl");

        if (access(RemoteResource::d_temp_file_dir.c_str(), F_OK) != 0) {
            DBG(cerr << prolog << "Creating temp file dir: " << RemoteResource::d_temp_file_dir << endl);
            if (mkdir(RemoteResource::d_temp_file_dir.c_str(), 0777) != 0) {
                throw BESInternalError("Failed to create temp file dir: " + RemoteResource::d_temp_file_dir + ": "
                                        + strerror(errno), __FILE__, __LINE__);
            }
        }
    }

    void tearDown() override {
        if (access(RemoteResource::d_temp_file_dir.c_str(), F_OK) != 0) {
            DBG(cerr << prolog << "Creating temp file dir: " << RemoteResource::d_temp_file_dir << endl);
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

    // A multithreaded test of retrieveResource().
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

#if 0
    /**
     * tests the is_cache_resource_expired() function
     * create a temp file and sets the expired time to 1 sec
     * allows temp file to expire and checks if the expiration is noticed
     */
    void is_cached_resource_expired_test() {
        DBG(cerr << "|--------------------------------------------------|" << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        try {
            std::shared_ptr<http::url> target_url(new http::url("http://google.com"));
            RemoteResource rhr(target_url, "foobar", 1);
            DBG(cerr << prolog << "remoteResource rhr: created, expires_interval: " << rhr.d_expires_interval << endl);

            rhr.d_filename = d_temp_file;
            DBG(cerr << prolog << "d_resourceCacheFilename: " << d_temp_file << endl);

            // Get a pointer to the singleton cache instance for this process.
            HttpCache *cache = HttpCache::get_instance();
            if (!cache) {
                ostringstream oss;
                oss << prolog << "FAILED to get local cache. ";
                oss << "Unable to proceed with request for " << d_temp_file;
                oss << " The server MUST have a valid HTTP cache configuration to operate." << endl;
                CPPUNIT_FAIL(oss.str());
            }
            if (!cache->get_exclusive_lock(d_temp_file, rhr.d_fd)) {
                CPPUNIT_FAIL(prolog + "Failed to acquire exclusive lock on: " + d_temp_file);
            }
            rhr.d_initialized = true;

            sleep(2);

            bool refresh = rhr.cached_resource_is_expired();
            DBG(cerr << prolog << "is_cached_resource_expired() called, refresh: " << refresh << endl);

            CPPUNIT_ASSERT(refresh);
        }
        catch (BESError &besE) {
            stringstream msg;
            msg << endl << prolog << "Caught BESError! message: " << besE.get_verbose_message();
            msg << " type: " << besE.get_bes_error_type() << endl;
            DBG(cerr << msg.str());
            CPPUNIT_FAIL(msg.str());
        }
        DBG(cerr << prolog << "END" << endl);
    }

    /**
     * Test of the RemoteResource content filtering method.
     */
    void filter_test() {
        DBG(cerr << prolog << "BEGIN" << endl);

        string source_file = BESUtil::pathConcat(TEST_DATA_DIR, "filter_test_source.xml");
        DBG(cerr << prolog << "source_file: " << source_file << endl);

        string baseline_file = BESUtil::pathConcat(TEST_DATA_DIR, "filter_test_source.xml_baseline");
        DBG(cerr << prolog << "baseline_file: " << baseline_file << endl);

        string tmp_file;
        try {
            copy_to_temp(source_file, tmp_file);
            DBG(cerr << prolog << "temp_file: " << tmp_file << endl);

            std::map<std::string, std::string> filter;
            filter.insert(pair<string, string>("OPeNDAP_DMRpp_DATA_ACCESS_URL", "file://original_file_ref"));
            filter.insert(pair<string, string>("OPeNDAP_DMRpp_MISSING_DATA_ACCESS_URL", "file://missing_file_ref"));

            RemoteResource foo;
            foo.d_filename = tmp_file;
            foo.filter_url(filter);

            bool result_matched = compare(tmp_file, baseline_file);
            stringstream info_msg;
            info_msg << prolog << "The filtered file: " << tmp_file
                     << (result_matched ? " MATCHED " : " DID NOT MATCH ")
                     << "the baseline file: " << baseline_file << endl;
            DBG(cerr << info_msg.str());
            CPPUNIT_ASSERT_MESSAGE(info_msg.str(), result_matched);
        }
        catch (const BESError &be) {
            stringstream msg;
            msg << prolog << "Caught BESError. Message: " << be.get_verbose_message() << " ";
            msg << be.get_file() << " " << be.get_line() << endl;
            DBG(cerr << msg.str());
            CPPUNIT_FAIL(msg.str());
        }
        // By unlinking here we only are doing it if the test is successful. This allows for forensic on broke tests.
        if (!tmp_file.empty()) {
            unlink(tmp_file.c_str());
            DBG(cerr << prolog << "unlink call on: " << tmp_file << endl);
        }
        DBG(cerr << prolog << "END" << endl);
    }

    /**
     * Test of the RemoteResource content filtering method.
     */
    void filter_test_more_focus() {
        DBG(cerr << prolog << "BEGIN" << endl);

        string source_file = BESUtil::pathConcat(TEST_DATA_DIR, "filter_test_02_source.xml");
        DBG(cerr << prolog << "source_file: " << source_file << endl);

        string baseline_file = BESUtil::pathConcat(TEST_DATA_DIR, "filter_test_02_source.xml.baseline");
        DBG(cerr << prolog << "baseline_file: " << baseline_file << endl);

        string tmp_file;
        try {
            copy_to_temp(source_file, tmp_file);
            DBG(cerr << prolog << "temp_file: " << tmp_file << endl);
            string href = "href=\"";
            string trusted_url_hack = "\" dmrpp:trust=\"true\"";

            string data_access_url_key = "href=\"OPeNDAP_DMRpp_DATA_ACCESS_URL\"";
            DBG(cerr << prolog << "                   data_access_url_key: " << data_access_url_key << endl);

            string data_access_url_with_trusted_attr_str = "href=\"file://original_file_ref\" dmrpp=\"trust\"";
            DBG(cerr << prolog << " data_access_url_with_trusted_attr_str: " << data_access_url_with_trusted_attr_str
                     << endl);

            string missing_data_access_url_key = "href=\"OPeNDAP_DMRpp_MISSING_DATA_ACCESS_URL\"";
            DBG(cerr << prolog << "           missing_data_access_url_key: " << missing_data_access_url_key << endl);

            string missing_data_url_with_trusted_attr_str = "href=\"file://missing_file_ref\' dmrpp:trust=\"true\"";
            DBG(cerr << prolog << "missing_data_url_with_trusted_attr_str: " << missing_data_url_with_trusted_attr_str
                     << endl);


            std::map<std::string, std::string> filter;
            filter.insert(pair<string, string>(data_access_url_key, data_access_url_with_trusted_attr_str));
            filter.insert(pair<string, string>(missing_data_access_url_key, missing_data_url_with_trusted_attr_str));

            RemoteResource foo;
            foo.d_filename = tmp_file;
            foo.filter_url(filter);

            bool result_matched = compare(tmp_file, baseline_file);
            stringstream info_msg;
            info_msg << prolog << "The filtered file: " << tmp_file
                     << (result_matched ? " MATCHED " : " DID NOT MATCH ")
                     << "the baseline file: " << baseline_file << endl;
            DBG(cerr << info_msg.str());
            CPPUNIT_ASSERT_MESSAGE(info_msg.str(), result_matched);
        }
        catch (const BESError &be) {
            stringstream msg;
            msg << prolog << "Caught BESError. Message: " << be.get_verbose_message() << " ";
            msg << be.get_file() << " " << be.get_line() << endl;
            DBG(cerr << msg.str());
            CPPUNIT_FAIL(msg.str());
        }
        // By unlinking here we only are doing it if the test is successful. This allows for forensic on broke tests.
        if (!tmp_file.empty()) {
            unlink(tmp_file.c_str());
            DBG(cerr << prolog << "unlink call on: " << tmp_file << endl);
        }
        DBG(cerr << prolog << "END" << endl);
    }
#endif

    CPPUNIT_TEST_SUITE(RemoteResourceTest);
#if 0
        CPPUNIT_TEST(is_cached_resource_expired_test);
        CPPUNIT_TEST(filter_test);
        CPPUNIT_TEST(filter_test_more_focus);
#endif
        CPPUNIT_TEST(get_http_url_test);
        CPPUNIT_TEST(get_http_url_test_mt);
        CPPUNIT_TEST(get_http_url_test_mt_test_return_content);
        CPPUNIT_TEST(get_file_url_test);

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
