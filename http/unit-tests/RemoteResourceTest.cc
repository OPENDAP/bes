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

#include <memory>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <unistd.h>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <GetOpt.h>
#include <util.h>
#include <HttpCache.h>

#include "BESError.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESCatalogList.h"
#include "TheBESKeys.h"
#include "BESContextManager.h"

#include "RemoteResource.h"
#include "HttpNames.h"
#include "HttpCache.h"

#include "test_config.h"

using namespace std;

static bool debug = false;
static bool Debug = false;
static bool bes_debug = false;
static bool purge_cache = false;
static bool ngap_tests = false;
static std::string token;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)
#define prolog std::string("RemoteResourceTest::").append(__func__).append("() - ")

namespace http {

class RemoteResourceTest: public CppUnit::TestFixture {
private:

    /**
     *
     */
    void show_file(string filename)
    {
        ifstream t(filename.c_str());

        if (t.is_open()) {
            string file_content((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
            t.close();
            cerr << endl << "#############################################################################" << endl;
            cerr << "file: " << filename << endl;
            cerr <<         ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl;
            cerr << file_content << endl;
            cerr << "#############################################################################" << endl;
        }
        else {
            cerr << "FAILED TO OPEN FILE: " << filename << endl;
        }
    }

    std::string get_file_as_string(string filename)
    {
        ifstream t(filename.c_str());

        if (t.is_open()) {
            string file_content((istreambuf_iterator<char>(t)), istreambuf_iterator<char>());
            t.close();
            if(Debug) cerr << endl << "#############################################################################" << endl;
            if(Debug) cerr << "file: " << filename << endl;
            if(Debug) cerr <<         ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << endl;
            if(Debug) cerr << file_content << endl;
            if(Debug) cerr << "#############################################################################" << endl;
            return file_content;
        }
        else {
            stringstream msg;
            msg << prolog << "FAILED TO OPEN FILE: " << filename << endl;
            CPPUNIT_FAIL( msg.str());
        }
        return "";
    }

    /**
     *
     */
    string get_data_file_url(string name){
        string data_file = BESUtil::assemblePath(d_data_dir,name);
        if(debug) cerr << prolog << "data_file: " << data_file << endl;
        if(Debug) show_file(data_file);

        string data_file_url = "file://" + data_file;
        if(debug) cerr << prolog << "data_file_url: " << data_file_url << endl;
        return data_file_url;
    }

    /**
     * @brief Copy the source file to a system determined tempory file and set the rvp tmp_file to the temp file name.
     * @param src The source file to copy
     * @param tmp_file The temporary file created.
     */
    void copy_to_temp(const string &src, string &tmp_file){
        ifstream src_is(src);
        if(!src_is.is_open()){
            throw BESInternalError("Failed to open source file: "+src,__FILE__,__LINE__);
        }
        if(debug) cerr << prolog << "ifstream opened" << endl;

        char *pointer = tmpnam(nullptr);
        ofstream tmp_os(pointer);
        if(!tmp_os.is_open()){
            stringstream msg;
            msg << "Failed to open temp file: " << pointer << endl;
            throw BESInternalError(msg.str(),__FILE__,__LINE__);
        }
        char buf[4096];

        do {
            src_is.read(&buf[0], 4096);
            tmp_os.write(&buf[0], src_is.gcount());
        }while (src_is.gcount() > 0);
        tmp_file=pointer;
    }

    /**
     * @brief Compare two text files as string values.
     * @param file_a
     * @param file_b
     * @return True the files match, False otherwise.
     */
    bool compare(const string &file_a, const string &file_b){
        string a_str = get_file_as_string(file_a);
        string b_str = get_file_as_string(file_b);
        return a_str == b_str;
    }


public:
    string d_data_dir;

    // Called once before everything gets tested
    RemoteResourceTest()
    {
        d_data_dir = TEST_DATA_DIR;;
    }

    // Called at the end of the test
    ~RemoteResourceTest()
    {
    }

    // Called before each test
    void setUp()
    {
        if(Debug) cerr << endl << prolog << "BEGIN" << endl;
        if(debug && !Debug) cerr << endl;
        if(debug) cerr << prolog << "data_dir: " << d_data_dir << endl;
        string bes_conf = BESUtil::assemblePath(TEST_BUILD_DIR,"bes.conf");
        if(Debug) cerr << prolog << "Using BES configuration: " << bes_conf << endl;
        if (bes_debug) show_file(bes_conf);
        TheBESKeys::ConfigFile = bes_conf;

        if (bes_debug) BESDebug::SetUp("cerr,rr,bes,http");

        if(!token.empty()){
            if(debug) cerr << "Setting BESContext " << EDL_AUTH_TOKEN_KEY<< " to: '"<< token << "'" << endl;
            BESContextManager::TheManager()->set_context(EDL_AUTH_TOKEN_KEY,token);
        }

        if(purge_cache){
            cache_purge();
        }


        if(Debug) cerr << "setUp() - END" << endl;
    }

    // Called after each test
    void tearDown()
    {
    }

    void cache_purge(){
        if(Debug) cerr << prolog << "Purging cache!" << endl;
        string cache_dir;
        bool found;
        TheBESKeys::TheKeys()->get_value(HTTP_CACHE_DIR_KEY,cache_dir,found);
        if(found){
            if(Debug) cerr << prolog << HTTP_CACHE_DIR_KEY << ": " <<  cache_dir << endl;
            if(Debug) cerr << prolog << "Purging " << cache_dir << endl;
            string sys_cmd = "mkdir -p "+ cache_dir;
            system(sys_cmd.c_str());
            sys_cmd = "exec rm -rf "+ BESUtil::assemblePath(cache_dir,"/*");
            system(sys_cmd.c_str());
            if(Debug) cerr << prolog << cache_dir  << " has been purged." << endl;
        }
    }

/*##################################################################################################*/
/* TESTS BEGIN */

    /**
     *
     */
    void load_hdrs_from_file_test(){
        if(debug) cerr << "|--------------------------------------------------|" << endl;
        if(debug) cerr << prolog << "BEGIN" << endl;

        string url = "file://";
        url += BESUtil::pathConcat(d_data_dir,"load_hdrs_from_file_test_file.txt");
        if(debug) cerr << prolog << "url: " << url << endl;
        RemoteResource rhr(url);
        if(debug) cerr << prolog << "rhr created" << endl;
        try {
            rhr.load_hdrs_from_file();
            if(debug) cerr << prolog << "loaded hdrs from file" << endl;
            vector<string> *hdrs = rhr.getResponseHeaders();
            if(debug) cerr << prolog << "hdrs retrieved" << endl;

            for(size_t i=0; i<hdrs->size() && debug ; i++){
                cerr << prolog << "hdr["<< i << "]: " << (*hdrs)[i] << endl;
            }

            string header_1 = (*hdrs)[0];
            string header_2 = (*hdrs)[1];
            string header_3 = (*hdrs)[2];
            string header_4 = (*hdrs)[3];

            string expected_header_1 = "This ...";
            string expected_header_2 = "Is ...";
            string expected_header_3 = "A ...";
            string expected_header_4 = "TEST";

            if(debug) cerr << prolog << "Expected Header: " << expected_header_1 << "   <--/-->     Retrieved Header: " << header_1 << endl;
            if(debug) cerr << prolog << "Expected Header: " << expected_header_2 << "   <--/-->     Retrieved Header: " << header_2 << endl;
            if(debug) cerr << prolog << "Expected Header: " << expected_header_3 << "   <--/-->     Retrieved Header: " << header_3 << endl;
            if(debug) cerr << prolog << "Expected Header: " << expected_header_4 << "   <--/-->     Retrieved Header: " << header_4 << endl;
            CPPUNIT_ASSERT( header_1.compare(expected_header_1) && header_2.compare(expected_header_2) && header_3.compare(expected_header_3) && header_4.compare(expected_header_4));

        }
        catch (BESError &besE){
            cerr << "Caught BESError! message: " << besE.get_verbose_message() << " type: " << besE.get_bes_error_type() << endl;
            CPPUNIT_ASSERT(false);
        }
        if(debug) cerr << prolog << "END" << endl;
    }

    /**
     *
     */
    void update_file_and_headers_test(){
        if(debug) cerr << "|--------------------------------------------------|" << endl;
        if(debug) cerr << prolog << "BEGIN" << endl;

        try {
            string tmp_file_name(tmpnam(nullptr));
            if (debug) cerr << prolog << "tmp_file_name: " << tmp_file_name << endl;
            {
                ofstream ofs(tmp_file_name);
                if(!ofs.is_open()){
                    throw BESInternalError("Failed to open temporary file: "+tmp_file_name,__FILE__,__LINE__);
                }
                ofs << "This is the temp file." << endl;
            }
            //unlink(pointer);

            RemoteResource rhr("http://google.com", "foobar");
            if(debug) cerr << prolog << "remoteResource rhr: created" << endl;

            rhr.d_resourceCacheFileName = tmp_file_name;
            if(debug) cerr << prolog << "d_resourceCacheFilename: " << tmp_file_name << endl;

            string source_url = "file://" + BESUtil::pathConcat(d_data_dir,"update_file_and_headers_test_file.txt");
            rhr.d_remoteResourceUrl = source_url;
            if(debug) cerr << prolog << "d_remoteResourceUrl: " << source_url << endl;
            
            // Get a pointer to the singleton cache instance for this process.
            HttpCache *cache = HttpCache::get_instance();
            if (!cache) {
                ostringstream oss;
                oss << prolog << "FAILED to get local cache. ";
                oss << "Unable to proceed with request for " << tmp_file_name;
                oss << " The server MUST have a valid HTTP cache configuration to operate." << endl;
                CPPUNIT_FAIL(oss.str());
            }
            if(!cache->get_exclusive_lock(tmp_file_name, rhr.d_fd)){
                CPPUNIT_FAIL(prolog + "Failed to acquire exclusive lock on: "+tmp_file_name);
            }
            rhr.d_initialized = true;

            rhr.update_file_and_headers();

            if(debug) cerr << prolog << "update_file_and_headers() called" << endl;

            string cache_filename = rhr.getCacheFileName();
            if(debug) cerr << prolog << "cache_filename: " << cache_filename << endl;
            string expected_content("This an updating file and headers TEST. Move Along...");
            if(debug) cerr << prolog << "expected_content: " << expected_content << endl;
            string content = get_file_as_string(cache_filename);
            if(debug) cerr << prolog << "retrieved content: " << content << endl;
            CPPUNIT_ASSERT( content == expected_content );
        }
        catch (BESError &besE){
            cerr << endl << prolog << "Caught BESError! message: " << besE.get_verbose_message() << " type: " << besE.get_bes_error_type() << endl;
            CPPUNIT_ASSERT(false);
        }
        if(debug) cerr << prolog << "END" << endl;
    }

    void get_http_url_test() {
        if(debug) cerr << "|--------------------------------------------------|" << endl;
        if(debug) cerr << prolog << "BEGIN" << endl;

        string url = "http://test.opendap.org/data/httpd_catalog/READTHIS";
        if(debug) cerr << prolog << "url: " << url << endl;
        http::RemoteResource rhr(url);
        try {
            rhr.retrieveResource();
            vector<string> *hdrs = rhr.getResponseHeaders();

            for(size_t i=0; i<hdrs->size() && debug ; i++){
                cerr << prolog << "hdr["<< i << "]: " << (*hdrs)[i] << endl;
            }
            string cache_filename = rhr.getCacheFileName();
            if(debug) cerr << prolog << "cache_filename: " << cache_filename << endl;
            string expected_content("This is a test. If this was not a test you would have known the answer.\n");
            if(debug) cerr << prolog << "expected_content string: " << expected_content << endl;
            string content = get_file_as_string(cache_filename);
            if(debug) cerr << prolog << "retrieved content: " << content << endl;
            CPPUNIT_ASSERT( content == expected_content );
        }
        catch (BESError &besE){
            cerr << "Caught BESError! message: " << besE.get_verbose_message() << " type: " << besE.get_bes_error_type() << endl;
            CPPUNIT_ASSERT(false);
        }
        if(debug) cerr << prolog << "END" << endl;
    }

    void get_ngap_ghrc_tea_url_test() {
        if(debug) cerr << "|--------------------------------------------------|" << endl;
        if(!ngap_tests){
            if(debug) cerr << prolog << "SKIPPING." << endl;
            return;
        }
        if(debug) cerr << prolog << "BEGIN" << endl;
        string url = "https://d1jecqxxv88lkr.cloudfront.net/ghrcwuat-protected/rss_demo/rssmif16d__7/f16_ssmis_20031026v7.nc.dmrpp";
        if(debug) cerr << prolog << "url: " << url << endl;
        http::RemoteResource rhr(url);
        try {
            rhr.retrieveResource();
            vector<string> *hdrs = rhr.getResponseHeaders();
            for(size_t i=0; i<hdrs->size() && debug ; i++){
                cerr << prolog << "hdr["<< i << "]: " << (*hdrs)[i] << endl;
            }
            string cache_filename = rhr.getCacheFileName();
            if(debug) cerr << prolog << "cache_filename: " << cache_filename << endl;
            //string expected_content("This is a test. If this was not a test you would have known the answer.\n");
            //if(debug) cerr << prolog << "expected_content string: " << expected_content << endl;
            string content = get_file_as_string(cache_filename);
            if(debug) cerr << prolog << "retrieved content: " << content << endl;
            // CPPUNIT_ASSERT( content == expected_content );
        }
        catch (BESError &besE){
            stringstream msg;
            msg << "Caught BESError! message: " << besE.get_verbose_message() << " type: " << besE.get_bes_error_type() << endl;
            cerr << msg.str();
            CPPUNIT_FAIL(msg.str());
        }

        if(debug) cerr << prolog << "END" << endl;
    }



    void get_ngap_harmony_url_test() {
        if(debug) cerr << "|--------------------------------------------------|" << endl;
        if(!ngap_tests){
            if(debug) cerr << prolog << "SKIPPING." << endl;
            return;
        }
        if(debug) cerr << prolog << "BEGIN" << endl;

        string url = "https://harmony.uat.earthdata.nasa.gov/service-results/harmony-uat-staging/public/"
                        "sds/staged/ATL03_20200714235814_03000802_003_01.h5.dmrpp";

        if(debug) cerr << prolog << "url: " << url << endl;
        http::RemoteResource rhr(url);
        try {
            rhr.retrieveResource();
            vector<string> *hdrs = rhr.getResponseHeaders();
            for(size_t i=0; i<hdrs->size() && debug ; i++){
                cerr << prolog << "hdr["<< i << "]: " << (*hdrs)[i] << endl;
            }
            string cache_filename = rhr.getCacheFileName();
            if(debug) cerr << prolog << "cache_filename: " << cache_filename << endl;
            string content = get_file_as_string(cache_filename);
            if(debug) cerr << prolog << "retrieved content: " << content << endl;
            // CPPUNIT_ASSERT( content == expected_content );
        }
        catch (BESError &besE){
            stringstream msg;
            msg << "Caught BESError! message: " << besE.get_verbose_message() << " type: " << besE.get_bes_error_type() << endl;
            cerr << msg.str();
            CPPUNIT_FAIL(msg.str());
        }
        if(debug) cerr << prolog << "END" << endl;
    }

    /**
     *
     */
    void get_file_url_test() {
        if(debug) cerr << "|--------------------------------------------------|" << endl;
        if(debug) cerr << prolog << "BEGIN" << endl;

        string data_file_url = get_data_file_url("test_file");
        http::RemoteResource rhr(data_file_url);
        try {
            rhr.retrieveResource();
            vector<string> *hdrs = rhr.getResponseHeaders();

            for(size_t i=0; i<hdrs->size() && debug ; i++){
                cerr << prolog << "hdr["<< i << "]: " << (*hdrs)[i] << endl;
            }
            string cache_filename = rhr.getCacheFileName();
            if(debug) cerr << prolog << "cache_filename: " << cache_filename << endl;

            string expected("This a TEST. Move Along...\n");
            string retrieved = get_file_as_string(cache_filename);
            if(debug) cerr << prolog << "expected_content: '" << expected << "'" << endl;
            if(debug) cerr << prolog << "retrieved_content: '" << retrieved << "'" << endl;
            CPPUNIT_ASSERT( retrieved == expected );
        }
        catch (BESError &besE){
            stringstream msg;
            msg << prolog << "Caught BESError! message: '" << besE.get_message() << "' bes_error_type: " << besE.get_bes_error_type() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        if(debug) cerr << prolog << "END" << endl;
    }

     /**
      *
      */
     void is_cached_resource_expired_test(){
         if(debug) cerr << "|--------------------------------------------------|" << endl;


     }

    /**
     * Test of the RemoteResource content filtering method.
     */
    void filter_test() {
        if(debug) cerr << prolog << "BEGIN" << endl;

        string source_file = BESUtil::pathConcat(d_data_dir,"filter_test_source.xml");
        if(debug) cerr << prolog << "source_file: " << source_file << endl;

        string baseline_file = BESUtil::pathConcat(d_data_dir,"filter_test_source.xml_baseline");
        if(debug) cerr << prolog << "baseline_file: " << baseline_file << endl;

        string tmp_file;
        try {
            copy_to_temp(source_file,tmp_file);
            if(debug) cerr << prolog << "temp_file: " << tmp_file << endl;

            std::map<std::string,std::string> filter;
            filter.insert(pair<string,string>("OPeNDAP_DMRpp_DATA_ACCESS_URL","file://original_file_ref"));
            filter.insert(pair<string,string>("OPeNDAP_DMRpp_MISSING_DATA_ACCESS_URL","file://missing_file_ref"));

            RemoteResource foo;
            foo.d_resourceCacheFileName = tmp_file;
            foo.filter_retrieved_resource(filter);

            bool result_matched = compare(tmp_file,baseline_file);
            stringstream info_msg;
            info_msg << prolog << "The filtered file: "<< tmp_file << (result_matched?" MATCHED ":" DID NOT MATCH ")
            << "the baseline file: " << baseline_file << endl;
            if(debug) cerr << info_msg.str();
            CPPUNIT_ASSERT_MESSAGE(info_msg.str(),result_matched);
        }
        catch(BESError be){
            stringstream msg;
            msg << prolog << "Caught BESError. Message: " << be.get_verbose_message() << " ";
            msg << be.get_file() << " " << be.get_line() << endl;
            if(debug) cerr << msg.str();
            CPPUNIT_FAIL(msg.str());
        }
        // By unlinking here we only are doing it if the test is successful. This allows for forensic on broke tests.
        if(!tmp_file.empty()){
            unlink(tmp_file.c_str());
            if(debug) cerr << prolog << "unlink call on: " << tmp_file << endl;
        }
        if(debug) cerr << prolog << "END" << endl;
    }

/* TESTS END */
/*##################################################################################################*/


    CPPUNIT_TEST_SUITE( RemoteResourceTest );

    CPPUNIT_TEST(load_hdrs_from_file_test);
    CPPUNIT_TEST(update_file_and_headers_test);
    //CPPUNIT_TEST(is_cached_resource_expired_test);
    CPPUNIT_TEST(filter_test);
    CPPUNIT_TEST(get_http_url_test);
    CPPUNIT_TEST(get_file_url_test);
    CPPUNIT_TEST(get_ngap_ghrc_tea_url_test);
    CPPUNIT_TEST(get_ngap_harmony_url_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(RemoteResourceTest);

} // namespace httpd_catalog

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "dbDPN");
    int option_char;
    while ((option_char = getopt()) != -1)
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
            ngap_tests = true;  // debug is a static global
            cerr << "NGAP Tests Enabled." << endl;
            break;
        case 'P':
            purge_cache = true;  // debug is a static global
            cerr << "purge_cache enabled" << endl;
            break;
        case 't':
            token = getopt.optarg; // token is a static global
            cerr << "Authorization header value: " << token << endl;
            break;
        default:
            break;
        }

    bool wasSuccessful = true;
    string test = "";
    int i = getopt.optind;
    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = http::RemoteResourceTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
