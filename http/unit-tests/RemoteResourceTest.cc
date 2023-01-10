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
#include <cstring>
#include <unistd.h>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <libdap/util.h>

#include "BESError.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESCatalogList.h"
#include "TheBESKeys.h"
#include "BESContextManager.h"

#include "RemoteResource.h"
#include "HttpNames.h"
#include "HttpCache.h"
#include "url_impl.h"

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

    /**
     * purges the http cache for temp files created in tests
     */
    void purge_http_cache() {
        DBG2(cerr << prolog << "Purging cache!" << endl);
        string cache_dir = TheBESKeys::TheKeys()->read_string_key(HTTP_CACHE_DIR_KEY, "");
        string cache_prefix = TheBESKeys::TheKeys()->read_string_key(HTTP_CACHE_PREFIX_KEY, "");

        if (!(cache_dir.empty() && cache_prefix.empty())) {
            DBG2(cerr << prolog << HTTP_CACHE_DIR_KEY << ": " << cache_dir << endl);
            DBG2(cerr << prolog << "Purging " << cache_dir << " of files with prefix: " << cache_prefix << endl);
            string sys_cmd = "mkdir -p " + cache_dir;
            DBG2(cerr << "Running system command: " << sys_cmd << endl);
            system(sys_cmd.c_str());

            sys_cmd = "exec rm -rf " + BESUtil::assemblePath(cache_dir, cache_prefix);
            sys_cmd = sys_cmd.append("*");
            DBG2(cerr << "Running system command: " << sys_cmd << endl);
            system(sys_cmd.c_str());
            DBG2(cerr << prolog << "The HTTP cache has been purged." << endl);
        }
    }

    void show_file(const string &filename) {
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
            cerr << "FAILED TO OPEN FILE: " << filename << endl;
        }
    }

    string get_file_as_string(const string &filename) {
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
            stringstream msg;
            msg << prolog << "FAILED TO OPEN FILE: " << filename << endl;
            CPPUNIT_FAIL(msg.str());
        }
        return "";
    }

    string get_data_file_url(string name) {
        string data_file = BESUtil::assemblePath(d_data_dir, name);
        DBG(cerr << prolog << "data_file: " << data_file << endl);
        DBG2(show_file(data_file));

        string data_file_url = "file://" + data_file;
        DBG(cerr << prolog << "data_file_url: " << data_file_url << endl);
        return data_file_url;
    }

    /**
     * @brief Copy the source file to a system determined tempory file and set the rvp tmp_file to the temp file name.
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
    bool compare(const string &file_a, const string &file_b) {
        string a_str = get_file_as_string(file_a);
        string b_str = get_file_as_string(file_b);
        return a_str == b_str;
    }

public:
    string d_data_dir = TEST_DATA_DIR;
    string d_temp_file;

    // Called once before everything gets tested
    RemoteResourceTest() = default;

    // Called at the end of the test
    ~RemoteResourceTest() override = default;

    // Called before each test
    void setUp() override {
        DBG2(cerr << endl << prolog << "BEGIN" << endl);
        if (debug && !Debug) cerr << endl;
        DBG(cerr << prolog << "data_dir: " << d_data_dir << endl);
        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR, "bes.conf");
        DBG2(cerr << prolog << "Using BES configuration: " << bes_conf << endl);
        if (bes_debug) show_file(bes_conf);
        TheBESKeys::ConfigFile = bes_conf;

        if (bes_debug) BESDebug::SetUp("cerr,rr,bes,http,curl");

        if (purge_cache) {
            purge_http_cache();
        }

        string tmp_file_name(tmpnam(nullptr));
        DBG(cerr << prolog << "tmp_file_name: " << tmp_file_name << endl);
        {
            ofstream ofs(tmp_file_name);
            if (!ofs.is_open()) {
                CPPUNIT_FAIL("Failed to open temporary file: " + tmp_file_name);
            }
            ofs << "This is the temp file." << endl;
        }
        d_temp_file = tmp_file_name;

        DBG2(cerr << "setUp() - END" << endl);
    }

    // Called after each test
    void tearDown() override {
        if (!d_temp_file.empty())
            unlink(d_temp_file.c_str());

        string temp_file_hdrs = d_temp_file + ".hdrs";
        unlink(temp_file_hdrs.c_str());
    }

/*##################################################################################################*/
/* TESTS BEGIN */

    void get_http_url_test() {
        DBG(cerr << "|--------------------------------------------------|" << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        string url = "http://test.opendap.org/data/httpd_catalog/READTHIS";
        DBG(cerr << prolog << "url: " << url << endl);
        std::shared_ptr<http::url> url_ptr(new http::url(url));
        http::RemoteResource rhr(url_ptr);
        try {
            rhr.retrieveResource();
            string cache_filename = rhr.getCacheFileName();
            DBG(cerr << prolog << "cache_filename: " << cache_filename << endl);
            string expected_content("This is a test. If this was not a test you would have known the answer.\n");
            DBG(cerr << prolog << "expected_content string: " << expected_content << endl);
            string content = get_file_as_string(cache_filename);
            DBG(cerr << prolog << "retrieved content: " << content << endl);
            CPPUNIT_ASSERT(content == expected_content);
        }
        catch (BESError &besE) {
            cerr << "Caught BESError! message: " << besE.get_verbose_message() << " type: " << besE.get_bes_error_type()
                 << endl;
            CPPUNIT_ASSERT(false);
        }
        DBG(cerr << prolog << "END" << endl);
    }

    // A multithreaded test of retrieveResource().
    void get_http_url_test_mt() {
        DBG(cerr << "|--------------------------------------------------|" << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        string url = "http://test.opendap.org/data/httpd_catalog/READTHIS";
        DBG(cerr << prolog << "url: " << url << endl);
        std::shared_ptr<http::url> url_ptr(new http::url(url));

        try {
            std::vector<std::future<string>> futures;

            for (size_t i = 0; i < 3; ++i) {
                futures.emplace_back(std::async(std::launch::async, [url_ptr]() {
                    http::RemoteResource rhr(url_ptr);
                    std::cout << "Start RR" << std::endl;
                    rhr.retrieveResource();
                    std::cout << "End RR" << std::endl;
                    return rhr.getCacheFileName();
                }));
            }

            std::cout << "I'm doing my own work!" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "I'm done with my own work!" << std::endl;

            std::cout << "Start querying" << std::endl;
            for (auto &future: futures) {
                std::cout << future.get() << std::endl;
            }
        }
        catch (BESError &besE) {
            cerr << "Caught BESError! message: " << besE.get_verbose_message() << " type: " << besE.get_bes_error_type()
                 << endl;
            CPPUNIT_ASSERT(false);
        }
        DBG(cerr << prolog << "END" << endl);
    }

#if 0
    rhr.retrieveResource();
    string cache_filename = rhr.getCacheFileName();
    DBG(cerr << prolog << "cache_filename: " << cache_filename << endl);
    string expected_content("This is a test. If this was not a test you would have known the answer.\n");
    DBG(cerr << prolog << "expected_content string: " << expected_content << endl);
    string content = get_file_as_string(cache_filename);
    DBG(cerr << prolog << "retrieved content: " << content << endl);
    CPPUNIT_ASSERT( content == expected_content );
#endif

    void get_file_url_test() {
        DBG(cerr << "|--------------------------------------------------|" << endl);
        DBG(cerr << prolog << "BEGIN" << endl);

        string data_file_url = get_data_file_url("test_file"); // "file:// + TEST_DATA_DIR + /test_file"
        std::shared_ptr<http::url> url_ptr(new http::url(data_file_url));
        http::RemoteResource rhr(url_ptr);
        try {
            rhr.retrieveResource();
            string cache_filename = rhr.getCacheFileName();
            DBG(cerr << prolog << "cache_filename: " << cache_filename << endl);

            string expected("This a TEST. Move Along...\n");
            string retrieved = get_file_as_string(cache_filename);
            DBG(cerr << prolog << "expected_content: '" << expected << "'" << endl);
            DBG(cerr << prolog << "retrieved_content: '" << retrieved << "'" << endl);
            CPPUNIT_ASSERT(retrieved == expected);
        }
        catch (BESError &besE) {
            stringstream msg;
            msg << prolog << "Caught BESError! message: '" << besE.get_message() << "' bes_error_type: "
                << besE.get_bes_error_type() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        DBG(cerr << prolog << "END" << endl);
    }

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

            rhr.d_resourceCacheFileName = d_temp_file;
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

        string source_file = BESUtil::pathConcat(d_data_dir, "filter_test_source.xml");
        DBG(cerr << prolog << "source_file: " << source_file << endl);

        string baseline_file = BESUtil::pathConcat(d_data_dir, "filter_test_source.xml_baseline");
        DBG(cerr << prolog << "baseline_file: " << baseline_file << endl);

        string tmp_file;
        try {
            copy_to_temp(source_file, tmp_file);
            DBG(cerr << prolog << "temp_file: " << tmp_file << endl);

            std::map<std::string, std::string> filter;
            filter.insert(pair<string, string>("OPeNDAP_DMRpp_DATA_ACCESS_URL", "file://original_file_ref"));
            filter.insert(pair<string, string>("OPeNDAP_DMRpp_MISSING_DATA_ACCESS_URL", "file://missing_file_ref"));

            RemoteResource foo;
            foo.d_resourceCacheFileName = tmp_file;
            foo.filter_retrieved_resource(filter);

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

        string source_file = BESUtil::pathConcat(d_data_dir, "filter_test_02_source.xml");
        DBG(cerr << prolog << "source_file: " << source_file << endl);

        string baseline_file = BESUtil::pathConcat(d_data_dir, "filter_test_02_source.xml.baseline");
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
            foo.d_resourceCacheFileName = tmp_file;
            foo.filter_retrieved_resource(filter);

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


/* TESTS END */
/*##################################################################################################*/

    CPPUNIT_TEST_SUITE(RemoteResourceTest);

        CPPUNIT_TEST(is_cached_resource_expired_test);
        CPPUNIT_TEST(filter_test);
        CPPUNIT_TEST(filter_test_more_focus);
        CPPUNIT_TEST(get_http_url_test);
#if 0
        // FIXME Fix RemoteResource so this test passes. jhrg 1/9/23
        CPPUNIT_TEST(get_http_url_test_mt);
#endif
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
