// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>
#include <unistd.h>

#include <sys/types.h>
#include <stdio.h>

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <unistd.h>  // for stat
#include <sstream>

#include <libdap/ObjectType.h>
#include <libdap/EncodingType.h>
#include <libdap/ServerFunction.h>
#include <libdap/ServerFunctionsList.h>
#include <libdap/ConstraintEvaluator.h>
#include <libdap/DAS.h>
#include <libdap/DDS.h>
#include <libdap/Str.h>
#include <libdap/DDXParserSAX2.h>
#include <libdap/D4AsyncUtil.h>

#include <libdap/DMR.h>
#include <libdap/D4Group.h>
#include <libdap/D4ParserSax2.h>
#include <test/D4TestTypeFactory.h>
#include <libdap/util.h>
#include <libdap/mime_util.h>
#include <libdap/debug.h>

#include <test/TestTypeFactory.h>
#include <test/TestByte.h>

#include "BESRegex.h"
#include "BESDebug.h"
#include "TheBESKeys.h"
#include "BESDapResponseBuilder.h"
#include "BESDapFunctionResponseCache.h"
#include "BESStoredDapResultCache.h"

#include "BESResponseObject.h"
#include "BESDDSResponse.h"
#include "BESDataDDSResponse.h"
#include "BESDataHandlerInterface.h"
#include "RequestServiceTimer.h"

#include "test_utils.h"
#include "test_config.h"

using namespace CppUnit;
using namespace std;
using namespace libdap;

int test_variable_sleep_interval = 0;

static bool debug = false;
static bool debug_2 = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);
#undef DBG2
#define DBG2(x) do { if (debug_2) (x); } while(false);

#define BES_DATA_ROOT "BES.Data.RootDirectory"
#define BES_CATALOG_ROOT "BES.Catalog.catalog.RootDirectory"
#define BES_CANCEL_TIMEOUT_ON_SEND "BES.CancelTimeoutOnSend"

#define MODULE "dap"
#define plog std::string("ResponseBuilderTest::").append(__func__).append("() - ")

static bool parser_debug = false;

static void rb_simple_function(int, BaseType *[], DDS &, BaseType **btpp)
{
    Str *response = new Str("result");

    response->set_value("qwerty");
    *btpp = response;
    return;
}

static void parse_datadds_response(istream &in, string &prolog, vector<char> &blob)
{
    // Split the stuff in the input stream into two parts:
    // The text prolog and the binary blob
    const int line_length = 1024;
    // Read up to 'Data:'
    char line[line_length];
    while (!in.eof()) {
        in.getline(line, line_length);
        DBG(cerr << "prolog line: " << line << endl);
        if (strncmp(line, "Data:", 5) == 0) break;
        prolog += string(line);
        prolog += "\n";
    }

    // Read the blob
    streampos pos = in.tellg();
    in.seekg(0, in.end);
    unsigned int length = in.tellg() - pos;
    DBG(cerr << "blob length: " << length << endl);

    // return to byte just after 'Data:'
    in.seekg(pos, in.beg);

    // Fix for HYRAX-804: Change reserve() to resize() for the vector
    // blob. jhrg 8/3/18
    blob.resize(length);
    in.read(blob.data(), length);
}

class ResponseBuilderTest: public TestFixture {
private:
    BESDapResponseBuilder *drb, *drb3, *drb5, *drb6;
    string d_stored_result_subdir;
    DDS *test_05_dds;
    string stored_dap2_result_filename;

    AttrTable *cont_a;
    DAS *das;
    DDS *dds;
    TestTypeFactory *ttf;

    DMR *test_01_dmr;
    D4ParserSax2 *d4_parser;
    D4TestTypeFactory *d4_ttf;
    D4BaseTypeFactory *d4_btf;

    ostringstream oss;
    time_t now;
    char now_array[256];
    libdap::ServerFunction *rbSSF;

    void loadServerSideFunction()
    {
        rbSSF = new ServerFunction(
        // The name of the function as it will appear in a constraint expression
            "rbSimpleFunc",
            // The version of the function
            "1.0",
            // A brief description of the function
            "Returns a string",
            // A usage/syntax statement
            "rbSimpleFunc()",
            // A URL that points two a web page describing the function
            "http://docs.opendap.org/index.php/Hyrax:_Server_Side_Functions",
            // A URI that defines the role of the function
            "http://services.opendap.org/dap4/unit-tests/ResponseBuilderTest",
            // A pointer to the helloWorld() function
            rb_simple_function);

        libdap::ServerFunctionsList::TheList()->add_function(rbSSF);
    }

public:
    ResponseBuilderTest() :
        drb(0), drb3(0), drb5(0), drb6(0), d_stored_result_subdir("/builder_response_cache"), test_05_dds(0),
        // FIXME Cannot rely on hash name being the same on different machines. jhrg 3/4/15
        stored_dap2_result_filename(TEST_BUILD_DIR + d_stored_result_subdir + "/my_result_16877844200208667996.data_ddx"),
        cont_a(0), das(0), dds(0), test_01_dmr(0), d4_parser(0), d4_ttf(0), d4_btf(0)
    {
        now = time(0);
        ostringstream time_string;
        time_string << (int) now;
        strncpy(now_array, time_string.str().c_str(), 255);
        now_array[255] = '\0';

        loadServerSideFunction();
    }

    ~ResponseBuilderTest()
    {
        clean_cache_dir(d_stored_result_subdir);
    }

    void setUp()
    {
        DBG2(cerr << plog << "BEGIN" << endl);
        DBG2(BESDebug::SetUp("cerr,cache,dap"));

        // BESDapResponseBuilder now uses theBESKeys. jhrg 12/30/15
        TheBESKeys::ConfigFile = (string) TEST_SRC_DIR + "/input-files/test.keys"; // empty file. jhrg 10/20/15

        // Test pathname
        drb = new BESDapResponseBuilder();

        // This file has an ancillary DAS in the input-files dir.
        // drb3 is also used to test escaping stuff in URLs. 5/4/2001 jhrg
        drb3 = new BESDapResponseBuilder();
        drb3->set_dataset_name((string) TEST_SRC_DIR + "/input-files/coads.data");
        drb3->set_ce("u,x,z[0]&grid(u,\"lat<10.0\")");

        // Test escaping stuff. 5/4/2001 jhrg
        drb5 = new BESDapResponseBuilder();
        drb5->set_dataset_name("nowhere%5Bmydisk%5Dmyfile");
        drb5->set_ce("u%5B0%5D");

        // Try a server side function call.
        // loadServerSideFunction(); NB: This is called by the test's ctor
        drb6 = new BESDapResponseBuilder();
        drb6->set_dataset_name((string) TEST_SRC_DIR + "/input-files/bears.data");
        drb6->set_ce("rbSimpleFunc()");

        cont_a = new AttrTable;
        cont_a->append_attr("size", "Int32", "7");
        cont_a->append_attr("type", "String", "cars");
        das = new DAS;
        das->add_table("a", cont_a); // Ingests cont_a, no copy?

        ttf = new TestTypeFactory;
        dds = new DDS(ttf, "test");
        TestByte *a = new TestByte("a");
        dds->add_var_nocopy(a);

        dds->transfer_attributes(das);
        dds->set_dap_major(3);
        dds->set_dap_minor(2);
        dds->filename("JustAByte");

        string cid;
        DDXParser dp(ttf);
        test_05_dds = new DDS(ttf);
        dp.intern((string) TEST_SRC_DIR + "/input-files/test.05.ddx", test_05_dds, cid);
        // for these tests, set the filename to the dataset_name. ...keeps the cache names short
        test_05_dds->filename(test_05_dds->get_dataset_name());
        // cid == http://dods.coas.oregonstate.edu:8080/dods/dts/test.01.blob
        DBG2(cerr << plog << "DDS Name: " << test_05_dds->get_dataset_name() << endl);
        DBG2(cerr << plog << "Intern CID: " << cid << endl);

        d4_parser = new D4ParserSax2();
        d4_ttf = new D4TestTypeFactory();
        d4_btf = new D4BaseTypeFactory();

        test_01_dmr = new DMR(d4_ttf);

        string dmr_filename = (string) TEST_SRC_DIR + "/input-files/test_01.dmr";
        DBG2(cerr << plog << "Parsing DMR file " << dmr_filename << endl);
        d4_parser->intern(read_test_baseline(dmr_filename), test_01_dmr, parser_debug);
        DBG2(cerr << plog << "Parsed DMR from file " << dmr_filename << endl);

        TheBESKeys::TheKeys()->set_key(BESDapFunctionResponseCache::PATH_KEY,
            (string) TEST_BUILD_DIR + "/response_cache");
        TheBESKeys::TheKeys()->set_key(BESDapFunctionResponseCache::PREFIX_KEY, "dap_response");
        TheBESKeys::TheKeys()->set_key(BESDapFunctionResponseCache::SIZE_KEY, "100");

        // Starting TheTimer with '0' disables bes-timeout.
        RequestServiceTimer::TheTimer()->start(std::chrono::seconds{0});
        // Starting TheTimer with a positive value sets bes-timeout to that value in seconds.

        TheBESKeys::TheKeys()->set_key(BES_CANCEL_TIMEOUT_ON_SEND, "false");
        // Set TheKeys() BES.CancelTimeoutOnSend ==> true to override bes-timeout set in TheTimer
        // use sleep(#) in a test to simulate a delay to trip bes-timeout in the Transform.

        DBG2(cerr << plog << "setUp() - END" << endl);
    }

    void tearDown()
    {
        DBG2(cerr << plog << "BEGIN" << endl);

        delete drb;
        drb = 0;
        delete drb3;
        drb3 = 0;
        delete drb5;
        drb5 = 0;
        delete drb6;
        drb6 = 0;

        delete das;
        das = 0;
        delete dds;
        dds = 0;
        delete ttf;
        ttf = 0;

        delete test_05_dds;
        test_05_dds = 0;
        delete d4_parser;
        d4_parser = 0;
        delete d4_ttf;
        d4_ttf = 0;
        delete d4_btf;
        d4_btf = 0;
        delete test_01_dmr;
        test_01_dmr = 0;
        DBG2(cerr << plog << "END" << endl);
    }

    bool re_match(BESRegex &r, const string &s)
    {
        DBG(cerr << plog << "s.size(): " << s.size() << endl);
        int pos = r.match(s.c_str(), s.size());
        DBG(cerr << plog << "r.match(s): " << pos << endl);
        return pos > 0 && static_cast<unsigned>(pos) == s.size();
    }

    bool re_match_binary(BESRegex &r, const string &s)
    {
        DBG(cerr << plog << "s.size(): " << s.size() << endl);
        int pos = r.match(s.c_str(), s.size());
        DBG(cerr << plog << "r.match(s): " << pos << endl);
        return pos > 0;
    }

    inline bool file_exists(const std::string& name)
    {
        struct stat buffer;
        return (stat(name.c_str(), &buffer) == 0);
    }

    void send_das_test()
    {
        DBG(cerr << endl << plog << "BEGIN" << endl);
        try {
#if HAVE_WORKING_REGEX
            string baseline = read_test_baseline((string) TEST_SRC_DIR + "/input-files/send_das_baseline_C++11_regex.txt");
#else
            string baseline = read_test_baseline((string) TEST_SRC_DIR + "/input-files/send_das_baseline.txt");
#endif

            DBG(cerr << plog << "---- start baseline ----" << endl << baseline << "---- end baseline ----" << endl);
            BESRegex r1(baseline.c_str());

            drb->send_das(oss, *das);

            DBG(cerr << plog << "DAS: " << oss.str() << endl);

            CPPUNIT_ASSERT(re_match(r1, oss.str()));
            oss.str("");
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        DBG(cerr << plog << "END" << endl);
    }

    void send_dds_test()
    {
        DBG(cerr << endl << plog << "BEGIN" << endl);
        try {
#if HAVE_WORKING_REGEX
            string baseline = read_test_baseline((string) TEST_SRC_DIR + "/input-files/send_dds_baseline_C++11_regex.txt");
#else
            string baseline = read_test_baseline((string) TEST_SRC_DIR + "/input-files/send_dds_baseline.txt");
#endif
            DBG(cerr << plog << "---- start baseline ----" << endl << baseline << "---- end baseline ----" << endl);
            BESRegex r1(baseline.c_str());

            ConstraintEvaluator ce;

            drb->send_dds(oss, &dds, ce);

            DBG(cerr << plog << "DDS: " << oss.str() << endl);

            CPPUNIT_ASSERT(re_match(r1, oss.str()));
            oss.str("");
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        DBG(cerr << plog << "END" << endl);
    }

    void send_ddx_test()
    {
        DBG(cerr << endl << plog << "BEGIN" << endl);

        string baseline = read_test_baseline((string) TEST_SRC_DIR + "/input-files/response_builder_send_ddx_test.xml");
        BESRegex r1(baseline.c_str());
        ConstraintEvaluator ce;

        try {
            drb->send_ddx(oss, &dds, ce);

            DBG(cerr << plog << "DDX: " << oss.str() << endl);

            CPPUNIT_ASSERT(re_match(r1, baseline));
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error: " + e.get_error_message());
        }
        DBG(cerr << plog << "END" << endl);
    }

#ifdef DAP2_STORED_RESULTS
    void store_dap2_result_test()
    {

        DBG(cerr << "store_dap2_result_test() - Configuring BES Keys." << endl);

        TheBESKeys::ConfigFile = (string) TEST_SRC_DIR + "/input-files/test.keys";
        TheBESKeys::TheKeys()->set_key(BES_CATALOG_ROOT, (string) TEST_SRC_DIR);
        TheBESKeys::TheKeys()->set_key(BESStoredDapResultCache::SUBDIR_KEY, d_stored_result_subdir);
        TheBESKeys::TheKeys()->set_key(BESStoredDapResultCache::PREFIX_KEY, "my_result_");
        TheBESKeys::TheKeys()->set_key(BESStoredDapResultCache::SIZE_KEY, "1100");
        TheBESKeys::TheKeys()->set_key(D4AsyncUtil::STYLESHEET_REFERENCE_KEY,
            "http://localhost:8080/opendap/xsl/asynResponse.xsl");

        ConstraintEvaluator ce;

        // Set this to be a stored result request
        drb->set_store_result("http://localhost:8080/opendap/");
        // Make the async_accepted string be empty to indicate that it was not set by a "client"
        drb->set_async_accepted("");

        DBG(
            cerr << "store_dap2_result_test() - Checking stored result request where async_accpeted is NOT set."
            << endl);

        string baseline = read_test_baseline(
            (string) TEST_SRC_DIR + "/input-files/response_builder_store_dap2_data_async_required.xml");
        try {
            oss.str("");
            drb->send_dap2_data(oss, &test_05_dds, ce, false);

            string candidateResponseDoc = oss.str();

            DBG(cerr << "store_dap2_result_test() - Server Response Document: " << endl << candidateResponseDoc << endl);
            DBG(cerr << "store_dap2_result_test() - Baseline Document: " << endl << baseline << endl);
            CPPUNIT_ASSERT(candidateResponseDoc == baseline);

            CPPUNIT_ASSERT(!file_exists(stored_dap2_result_filename));

        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error: " + e.get_error_message());
        }

        DBG(
            cerr
            << "store_dap2_result_test() - Checking stored result request where client indicates that async is accepted (async_accepted IS set)."
            << endl);
        // Make the async_accpeted string be the string "0" to indicate that client doesn't care how long it takes...
        drb->set_async_accepted("0");
        DBG(cerr << "store_dap2_result_test() - async_accepted is set to: " << drb->get_async_accepted() << endl);

        baseline = read_test_baseline(
            (string) TEST_SRC_DIR + "/input-files/response_builder_store_dap2_data_async_accepted.xml");

        try {
            oss.str("");
            drb->send_dap2_data(oss, &test_05_dds, ce, false);

            string candidateResponseDoc = oss.str();
            DBG(cerr << "store_dap2_result_test() - Server Response Document: " << endl << candidateResponseDoc << endl);
            DBG(cerr << "store_dap2_result_test() - Baseline Document: " << endl << baseline << endl);
            // FIXME These XML docs contain the cached filename, which uses a machine
            // dependent hash value
            //CPPUNIT_ASSERT(candidateResponseDoc == baseline);

            TestTypeFactory ttf;
            // Force read from the cache file
            DBG(cerr << "store_dap2_result_test() - Reading stored DAP2 dataset." << endl);
            // FIXME This filename (stored_dap2_...) was built above in the ctor but uses a
            // hash code that is machine dependent. jhrg 3/4/15
            DDS *cache_dds = BESStoredDapResultCache::get_instance()->get_cached_dap2_data_ddx(
                stored_dap2_result_filename, &ttf, "test.05");
            DBG(cerr << "store_dap2_result_test() - Stored DAP2 dataset read." << endl);
            CPPUNIT_ASSERT(cache_dds);

            // There are nine variables in test.05.ddx
            CPPUNIT_ASSERT(cache_dds->var_end() - cache_dds->var_begin() == 9);

            ostringstream oss;
            DDS::Vars_iter i = cache_dds->var_begin();
            while (i != cache_dds->var_end()) {
                DBG(cerr << "Variable " << (*i)->name() << endl);
                // this will incrementally add the string rep of values to 'oss'
                (*i)->print_val(oss, "", false /*print declaration */);
                DBG(cerr << "Value " << oss.str() << endl);
                ++i;
            }

            // In this regex the value of <number> in the DAP2 Str variable (Silly test string: <number>)
            // is a any single digit. The *Test classes implement a counter and return strings where
            // <number> is 1, 2, ..., and running several of the tests here in a row will get a range of
            // values for <number>.
            BESRegex regex(
                "2551234567894026531840320006400099.99999.999\"Silly test string: [0-9]\"\"http://dcz.gso.uri.edu/avhrr-archive/archive.html\"");
            CPPUNIT_ASSERT(re_match(regex, oss.str()));
            delete cache_dds;
            cache_dds = 0;

        }
        catch (Error &e) {
            CPPUNIT_FAIL("ERROR: " + e.get_error_message());
        }

        BESStoredDapResultCache *sdrc = BESStoredDapResultCache::get_instance();
        DBG(cerr << "Retrieved BESStoredDapResultCache instance:  " << sdrc << endl);
        sdrc->delete_instance();
        DBG(cerr << "Deleted BESStoredDapResultCache instance:  " << sdrc << endl);

        TheBESKeys::TheKeys()->set_key(BES_CATALOG_ROOT, "");
        TheBESKeys::TheKeys()->set_key(BESStoredDapResultCache::SUBDIR_KEY, "");
        TheBESKeys::TheKeys()->set_key(BESStoredDapResultCache::PREFIX_KEY, "");
        TheBESKeys::TheKeys()->set_key(BESStoredDapResultCache::SIZE_KEY, "");
        TheBESKeys::TheKeys()->set_key(D4AsyncUtil::STYLESHEET_REFERENCE_KEY, "");
    }
#endif

    void store_dap4_result_test()
    {
        DBG(cerr << endl << plog << "BEGIN" << endl);

        TheBESKeys::ConfigFile = (string) TEST_SRC_DIR + "/input-files/test.keys";
        TheBESKeys::TheKeys()->set_key(BES_CATALOG_ROOT, (string) TEST_SRC_DIR);
        ConstraintEvaluator ce;

        // Set this to be a stored result request
        drb->set_store_result("http://localhost:8080/opendap/");
        drb->set_async_accepted("0");
        DBG(
            cerr
                << plog << "Checking stored result request where the result cache is not configured."
                << endl);

        string baseline_file = (string) TEST_SRC_DIR + "/input-files/response_builder_store_result_not_available.xml";
        string baseline = read_test_baseline(baseline_file);
        DBG(cerr << plog << "Response baseline read from " << baseline_file << endl);

        try {
            oss.str("");
            test_01_dmr->root()->set_send_p(true);
            drb->store_dap4_result(oss, *test_01_dmr);

            string candidateResponseDoc = oss.str();

            DBG(cerr << plog << "Server Response Document: " << endl << candidateResponseDoc << endl);
            DBG(cerr << plog << "Baseline Document: " << endl << baseline << endl);
            CPPUNIT_ASSERT(candidateResponseDoc == baseline);

            // FIXME Test to make sure no stored object file is created.

        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error: " + e.get_error_message());
        }

        // Configure the StoredResultCache

        TheBESKeys::TheKeys()->set_key(DAP_STORED_RESULTS_CACHE_SUBDIR_KEY, d_stored_result_subdir);
        TheBESKeys::TheKeys()->set_key(DAP_STORED_RESULTS_CACHE_PREFIX_KEY, "my_result_");
        TheBESKeys::TheKeys()->set_key(DAP_STORED_RESULTS_CACHE_SIZE_KEY, "1100");
        TheBESKeys::TheKeys()->set_key(D4AsyncUtil::STYLESHEET_REFERENCE_KEY,
            "http://localhost:8080/opendap/xsl/asynResponse.xsl");
        DBG(cerr << plog << "BES Keys configured." << endl);

        // Set this to be a stored result request
        drb->set_store_result("http://localhost:8080/opendap/");
        drb->set_async_accepted("");

        DBG(
            cerr << plog << "Checking stored result request where async_accpeted is NOT set."
                << endl);

        baseline_file = (string) TEST_SRC_DIR + "/input-files/response_builder_store_dap4_data_async_required.xml";
        baseline = read_test_baseline(baseline_file);
        DBG(cerr << plog << "Response baseline read from " << baseline_file << endl);

        try {
            oss.str("");
            test_01_dmr->root()->set_send_p(true);
            drb->store_dap4_result(oss, *test_01_dmr);

            string candidateResponseDoc = oss.str();

            DBG(cerr << plog << "Server Response Document: " << endl << candidateResponseDoc << endl);
            DBG(cerr << plog << "Baseline Document: " << endl << baseline << endl);
            CPPUNIT_ASSERT(candidateResponseDoc == baseline);

            // FIXME Test to make sure no stored object file is created.

        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error: " + e.get_error_message());
        }

        DBG(
            cerr
                << plog << "Checking stored result request where client indicates that async is accepted (async_accepted IS set)."
                << endl);

        drb->set_async_accepted("0");
        DBG(cerr << plog << "async_accepted is set to: " << drb->get_async_accepted() << endl);

        baseline_file = (string) TEST_SRC_DIR + "/input-files/response_builder_store_dap4_data_async_accepted.xml";
        baseline = read_test_baseline(baseline_file);
        DBG(cerr << plog << "Response baseline read from " << baseline_file << endl);

        try {
            oss.str("");
            drb->store_dap4_result(oss, *test_01_dmr);

            string candidateResponseDoc = oss.str();
            DBG(cerr << plog << "Server Response Document: " << endl << candidateResponseDoc << endl);
            DBG(cerr << plog << "Baseline Document: " << endl << baseline << endl);
            CPPUNIT_ASSERT(candidateResponseDoc == baseline);

            // Check out stored result file and make sure we can read and parse and inspect it.

            string stored_object_baseline_file = (string) TEST_SRC_DIR
                + "/input-files/response_builder_store_dap4_data.dap";
            DBG(
                cerr << plog << "Stored Object Baseline File: " << endl
                    << stored_object_baseline_file << endl);
            baseline = read_test_baseline(stored_object_baseline_file);

            string stored_object_response_file = (string) TEST_BUILD_DIR + d_stored_result_subdir
                + "/my_result_9619561608535196802.dap";
            DBG(
                cerr << plog << "Stored Object Response File: " << endl
                    << stored_object_response_file << endl);

            BESStoredDapResultCache *cache = BESStoredDapResultCache::get_instance();

            DMR *cached_data = cache->get_cached_dap4_data(stored_object_response_file, d4_btf, "test.01");

            DBG(cerr << plog << "Stored DAP4 dataset has been read." << endl);

            int response_element_count = cached_data->root()->element_count(true);
            DBG(cerr << plog << "response_element_count: " << response_element_count << endl);

            CPPUNIT_ASSERT(cached_data);
            // There are nine variables in test.05.ddx
            CPPUNIT_ASSERT(response_element_count == 11);

            ostringstream oss;
            Constructor::Vars_iter i = cached_data->root()->var_begin();
            while (i != cached_data->root()->var_end()) {
                DBG(cerr << "Variable " << (*i)->name() << endl);
                // this will incrementally add thr string rep of values to 'oss'
                (*i)->print_val(oss, "", false /*print declaration */);
                DBG(
                    cerr << plog << "response_value (" << oss.str().size() << " chars): " << endl << oss.str() << endl
                        << endl);
                ++i;
            }

            DBG(
                cerr << plog << "baseline ( " << baseline.size() << " chars): " << endl
                    << baseline << endl);

            CPPUNIT_ASSERT(baseline == oss.str());

        }
        catch (Error &e) {
            CPPUNIT_FAIL("Error: " + e.get_error_message());
        }
        
        TheBESKeys::TheKeys()->set_key(BES_CATALOG_ROOT, "");
        TheBESKeys::TheKeys()->set_key(DAP_STORED_RESULTS_CACHE_SUBDIR_KEY, "");
        TheBESKeys::TheKeys()->set_key(DAP_STORED_RESULTS_CACHE_PREFIX_KEY, "");
        TheBESKeys::TheKeys()->set_key(DAP_STORED_RESULTS_CACHE_SIZE_KEY, "");
        TheBESKeys::TheKeys()->set_key(D4AsyncUtil::STYLESHEET_REFERENCE_KEY, "");
    }

    void escape_code_test()
    {
        DBG(cerr << endl << plog << "BEGIN" << endl);
        // These should NOT be escaped.
        DBG(cerr << plog << drb3->get_dataset_name() << endl);
        DBG(cerr << plog << drb3->get_ce() << endl);

        CPPUNIT_ASSERT(drb3->get_dataset_name() == (string)TEST_SRC_DIR + "/input-files/coads.data");
        CPPUNIT_ASSERT(drb3->get_ce() == "u,x,z[0]&grid(u,\"lat<10.0\")");

        // The ResponseBuilder instance is feed escaped values; they should be
        // unescaped by the ctor and the mutators. 5/4/2001 jhrg

        DBG(cerr << plog << drb5->get_dataset_name() << endl);
        DBG(cerr << plog << drb5->get_ce() << endl);

        CPPUNIT_ASSERT(drb5->get_dataset_name() == "nowhere[mydisk]myfile");
        CPPUNIT_ASSERT(drb5->get_ce() == "u[0]");

        drb5->set_ce("u%5B0%5D");
        CPPUNIT_ASSERT(drb5->get_ce() == "u[0]");

        drb5->set_ce("Grid%20u%5B0%5D");
        CPPUNIT_ASSERT(drb5->get_ce() == "Grid%20u[0]");
        DBG(cerr << plog << "END" << endl);
    }

    void invoke_server_side_function_test()
    {
        DBG(cerr << endl << plog << "BEGIN" << endl);

        try {
#if HAVE_WORKING_REGEX
            string baseline = read_test_baseline((string) TEST_SRC_DIR + "/input-files/simple_function_baseline_C++11_regex.txt");
#else
            string baseline = read_test_baseline((string) TEST_SRC_DIR + "/input-files/simple_function_baseline.txt");
#endif
            BESRegex r1(baseline.c_str());

            DBG(cerr << plog << "---- start baseline ----" << endl << baseline << "---- end baseline ----" << endl);

            // This is set in the SetUp() method. jhrg 10/20/15
            // drb6->set_ce("rbSimpleFunc()");

            ConstraintEvaluator ce;
            DBG(cerr << plog << "Calling BESDapResponseBuilder.send_dap2_data()" << endl);
            drb6->send_dap2_data(oss, &dds, ce);

            DBG(cerr << plog << "---- start result ----" << endl << oss.str() << "---- end result ----" << endl);

            string prolog;
            vector<char> blob;
            istringstream iss(oss.str());
            parse_datadds_response(iss, prolog, blob);

            DBG(cerr << plog << "prolog: " << prolog << endl);

            CPPUNIT_ASSERT(re_match(r1, prolog));

            // This block of code was going to test if the binary data
            // in the response document matches some sequence of bytes
            // in a baseline file. it's not working and likely not that
            // important - the function under test returns a string and
            // it's clearly present in the output when instrumentation is
            // on. Return to this when there's time. 5/20/13 jhrg
#if 0
            ifstream blob_baseline_in(((string)TEST_SRC_DIR + "/input-files/blob_baseline.bin").c_str());

            blob_baseline_in.seekg(0, blob_baseline_in.end);
            unsigned int blob_baseline_length = blob_baseline_in.tellg();

            DBG(cerr << "blob_baseline length: " << blob_baseline_length << endl);
            DBG(cerr << "blob size: " << blob.size() << endl);

            CPPUNIT_ASSERT(blob_baseline_length == blob.size());

            blob_baseline_in.seekg(0, blob_baseline_in.beg);

            char blob_baseline[blob_baseline_length];
            blob_baseline_in.read(blob_baseline, blob_baseline_length);
            blob_baseline_in.close();
            for (int i = 0; i < blob_baseline_length; ++i) {
                DBG(cerr << "bb[" << i << "]: " << blob_baseline[i] << endl);
                DBG(cerr << "blob[" << i << "]: " << blob_baseline[i] << endl);
            }
#endif
        }
        catch (Error &e) {
            CPPUNIT_FAIL("Caught libdap::Error!! Message: " + e.get_error_message());
        }
        catch (BESError &e) {
            CPPUNIT_FAIL("Caught BESError!! Message: " + e.get_message() + " (" + e.get_file() + ")");
        }

        DBG(cerr << plog << "END" << endl);
    }


    void dummy_test(){
        DBG(cerr << endl << plog << "BEGIN" << endl);
        DBG(cerr << plog << "NOTHING WILL BE DONE." << endl);
        DBG(cerr << plog << "END" << endl);
    }

    CPPUNIT_TEST_SUITE( ResponseBuilderTest );

#if 0
        // These tests are not working because they rely on regex code that
        // is not portable across C++ versions. 1/31/23 jhrg
        CPPUNIT_TEST(send_das_test);
        CPPUNIT_TEST(send_dds_test);
#endif
        CPPUNIT_TEST(send_ddx_test);

        CPPUNIT_TEST(escape_code_test);
#if 0
        // Same regex issue as above. 1/31/23 jhrg
        CPPUNIT_TEST(invoke_server_side_function_test);
#endif
        CPPUNIT_TEST(dummy_test);

#if 0
        // FIXME These tests have baselines that rely on hash values that are
        // machine dependent. jhrg 3/4/15
        CPPUNIT_TEST(store_dap2_result_test);
        CPPUNIT_TEST(store_dap4_result_test);
#endif
    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ResponseBuilderTest);

int main(int argc, char*argv[])
{
    int option_char;
    while ((option_char = getopt(argc, argv, "dDh")) != -1)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'D':
            debug_2 = 1;
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: ResponseBuilderTest has the following tests:" << endl;
            const std::vector<Test*> &tests = ResponseBuilderTest::suite()->getTests();
            unsigned int prefix_len = ResponseBuilderTest::suite()->getName().append("::").size();
            for (std::vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                cerr << (*i)->getName().replace(0, prefix_len, "") << endl;
            }
            break;
        }
        default:
            break;
        }

    argc -= optind;
    argv += optind;

    // clean out the response_cache dir

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    string test = "";
    if (0 == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        int i = 0;
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = ResponseBuilderTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
