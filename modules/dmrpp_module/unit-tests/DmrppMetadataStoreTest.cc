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

#include <unistd.h>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <Array.h>
#include <Byte.h>
#include <DAS.h>
#include <DDS.h>
#include <DDXParserSAX2.h>
#include <DMR.h>
#include <D4ParserSax2.h>

#include <GetOpt.h>
#include <GNURegex.h>
#include <util.h>
#include <debug.h>

#include <BaseTypeFactory.h>
#include <D4BaseTypeFactory.h>

#include "BESError.h"
#include "TheBESKeys.h"
#include "BESDebug.h"

#include "DmrppMetadataStore.h"

#if 1
#include "DMRpp.h"
#include "DmrppTypeFactory.h"
#include "DmrppParserSax2.h"
#endif

#include "read_test_baseline.h"
#include "test_config.h"

static bool debug = false;
static bool bes_debug = false;
static bool clean = true;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

#define DEBUG_KEY "dmrpp_store,cache"

using namespace CppUnit;
using namespace std;
using namespace libdap;
using namespace dmrpp;

namespace bes {

// Move this into the class when we goto C++-11
static const string c_mds_prefix = "mds_"; // used when cleaning the cache, etc.
static const string c_mds_name = "/mds";
static const string c_mds_baselines = string(TEST_SRC_DIR) + "/baselines";

class DmrppMetadataStoreTest: public TestFixture {
private:
    DMR *d_test_dmr;
    D4BaseTypeFactory d_d4f;
    DmrppTypeFactory d_dmrpp_f;

    string d_mds_dir;
    DmrppMetadataStore *d_mds;

    /**
     * SetUp()-like method to build a DMR for testing.
     */
    void init_dmr_and_mds()
    {
        try {
            // Stock code to get the d_test_dds and d_mds objects used by many
            // of the tests.
            d_mds = DmrppMetadataStore::get_instance(d_mds_dir, c_mds_prefix, 1000);
            DBG(cerr << "Retrieved DmrppMetadataStore object: " << d_mds << endl);

            // Get a DMR to cache.
            string file_name = string(TEST_SRC_DIR).append("/input-files/test_01.dmr");

            d_test_dmr = new DMR(&d_d4f);
            D4ParserSax2 dp;
            DBG(cerr << "DMR file to be parsed: " << file_name << endl);
            fstream in(file_name.c_str(), ios::in|ios::binary);
            dp.intern(in, d_test_dmr);

            DBG(cerr << "DMR Name: " << d_test_dmr->name() << endl);
            CPPUNIT_ASSERT(d_test_dmr);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }
    }

#if 1
    void init_dmrpp_and_mds()
    {
        try {
            // Stock code to get the d_test_dds and d_mds objects used by many
            // of the tests.
            d_mds = DmrppMetadataStore::get_instance(d_mds_dir, c_mds_prefix, 1000);
            DBG(cerr << "Retrieved DmrppMetadataStore object: " << d_mds << endl);

            // Get a DMRpp to cache.
            string file_name = string(TEST_SRC_DIR).append("/input-files/chunked_fourD.h5.dmrpp");

            d_test_dmr = new DMRpp(&d_dmrpp_f);
            DmrppParserSax2 dp;
            DBG(cerr << "DMRpp file to be parsed: " << file_name << endl);
            fstream in(file_name.c_str(), ios::in|ios::binary);
            dp.intern(in, d_test_dmr);

            DBG(cerr << "DMRpp Name: " << d_test_dmr->name() << endl);
            CPPUNIT_ASSERT(d_test_dmr);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }
    }
#endif

public:
    DmrppMetadataStoreTest() :
        d_test_dmr(0), d_mds_dir(string(TEST_BUILD_DIR).append(c_mds_name)), d_mds(0)
    {
    }

    ~DmrppMetadataStoreTest()
    {
    }

    void setUp()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        if (bes_debug) BESDebug::SetUp(string("cerr,").append(DEBUG_KEY));

        if (clean) clean_cache_dir(c_mds_name);

        // Contains BES Log parameters but not cache names
        TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");

        DBG(cerr << __func__ << " - END" << endl);
    }

    void tearDown()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        delete d_test_dmr; d_test_dmr = 0;

        d_mds->delete_instance();

        if (clean) clean_cache_dir(d_mds_dir);

        DBG(cerr << __func__ << " - END" << endl);
    }

    void ctor_test_1()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            // The call to get_instance should fail since the directory is named,
            // but does not exist and cannot be made.
            d_mds = DmrppMetadataStore::get_instance("/new", c_mds_prefix, 1000);
            DBG(cerr << "retrieved DmrppMetadataStore instance: " << d_mds << endl);
            CPPUNIT_FAIL("get_instance() Should not return when the non-existent directory cannot be created");
        }
        catch (BESError &e) {
            CPPUNIT_ASSERT(!e.get_message().empty());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }

    void ctor_test_2()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            // This one should work and will make the directory
            d_mds = DmrppMetadataStore::get_instance(d_mds_dir, c_mds_prefix, 1000);
            DBG(cerr << "retrieved DmrppMetadataStore instance: " << d_mds << endl);
            CPPUNIT_ASSERT(d_mds);
            CPPUNIT_ASSERT(!d_mds->is_unlimited());

            d_mds->update_and_purge("no_name"); //cheesy test - use -b to read BES debug info
        }
        catch (BESError &e) {
            CPPUNIT_FAIL("Caught exception: " + e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }

    void ctor_test_3()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            // This one should work and will make the directory
            d_mds = DmrppMetadataStore::get_instance(d_mds_dir, c_mds_prefix, 0);
            DBG(cerr << "retrieved DmrppMetadataStore instance: " << d_mds << endl);
            CPPUNIT_ASSERT(d_mds);
            CPPUNIT_ASSERT(d_mds->is_unlimited());

            d_mds->update_and_purge("no_name");
        }
        catch (BESError &e) {
            CPPUNIT_FAIL("Caught exception: " + e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }

    void cache_a_dmr_response()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            init_dmr_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            DmrppMetadataStore::StreamDMR write_the_dmr_response(d_test_dmr);
            bool stored = d_mds->store_dap_response(write_the_dmr_response,
                d_test_dmr->name() + ".dmr_r", d_test_dmr->name(), "DMR");

            CPPUNIT_ASSERT(stored);

            // Now check the file
            string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "test_array_4.dmr_r";
            DBG(cerr << "Reading baseline: " << baseline_name << endl);
            CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

            string test_05_dmr_baseline = read_test_baseline(baseline_name);

            string response_name = d_mds_dir + "/" + c_mds_prefix + "test_array_4.dmr_r";
            DBG(cerr << "Reading response: " << response_name << endl);
            CPPUNIT_ASSERT(access(response_name.c_str(), R_OK) == 0);

            string stored_response = read_test_baseline(response_name);

            CPPUNIT_ASSERT(stored_response == test_05_dmr_baseline);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }

#if 1
    void cache_a_dmrpp_response()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            init_dmrpp_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            DmrppMetadataStore::StreamDMRpp write_the_dmrpp_response(d_test_dmr);
            bool stored = d_mds->store_dap_response(write_the_dmrpp_response, d_test_dmr->name() + ".dmrpp_r", d_test_dmr->name(), "DMRpp");

            CPPUNIT_ASSERT(stored);

            // Now check the file
            string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "chunked_fourD.h5.dmrpp_r";
            DBG(cerr << "Reading baseline: " << baseline_name << endl);
            CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

            string chunked_4d_dmrpp_baseline = read_test_baseline(baseline_name);

            string response_name = d_mds_dir + "/" + c_mds_prefix + "chunked_fourD.h5.dmrpp_r";
            DBG(cerr << "Reading response: " << response_name << endl);
            CPPUNIT_ASSERT(access(response_name.c_str(), R_OK) == 0);

            string stored_response = read_test_baseline(response_name);

            CPPUNIT_ASSERT(stored_response == chunked_4d_dmrpp_baseline);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }
#endif

#if 0
    void add_response_test()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            init_dds_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            bool stored = d_mds->add_responses(d_test_dds, d_test_dds->get_dataset_name());

            CPPUNIT_ASSERT(stored);

            // look for the files
            string dds_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dds->get_dataset_name().append("dds_r")), false /*mangle*/);
            DBG(cerr << __func__ << " - dds_cache_name: " << dds_cache_name << endl);
            CPPUNIT_ASSERT(access(dds_cache_name.c_str(), R_OK) == 0);

            string das_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dds->get_dataset_name().append("das_r")), false /*mangle*/);
            DBG(cerr << __func__ << " - das_cache_name: " << das_cache_name << endl);
            CPPUNIT_ASSERT(access(das_cache_name.c_str(), R_OK) == 0);

            /// Previously the MDS built all three metadata responses using
            /// either the DDS or DMR. Now it only does that when SYMETRIC_ADD_RESPONSES
            /// is defined. When that is not defined (the default) the DDS
            /// builds only the DAP2 responses and the DMR builds only the
            /// DAP4 responses. jhrg 3/20/18
#if SYMETRIC_ADD_RESPONSES
            string dmr_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dds->get_dataset_name().append("dmr_r")), false /*mangle*/);
            DBG(cerr << __func__ << " - dmr_cache_name: " << das_cache_name << endl);
            CPPUNIT_ASSERT(access(dmr_cache_name.c_str(), R_OK) == 0);
#endif
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }
#endif

#if 0
    void get_dmr_response_test() {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            init_dmr_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            bool stored = d_mds->add_responses(d_test_dmr, d_test_dmr->name());

            CPPUNIT_ASSERT(stored);

            // Now lets read the object from the cache
            ostringstream oss;
            d_mds->get_dmr_response(d_test_dmr->name(), oss);
            DBG(cerr << "DMR response: " << endl << oss.str() << endl);

            string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "test_01.dmr_r";
            DBG(cerr << "Reading baseline: " << baseline_name << endl);
            CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

            string test_05_dmr_baseline = read_test_baseline(baseline_name);

            CPPUNIT_ASSERT(test_05_dmr_baseline == oss.str());
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }

#endif

#if 0
    void remove_object_test()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            init_dds_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            bool stored = d_mds->add_responses(d_test_dds, d_test_dds->get_dataset_name());

            CPPUNIT_ASSERT(stored);

            // look for the files
            string dds_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dds->get_dataset_name().append("dds_r")), false /*mangle*/);
            DBG(cerr << __func__ << " - dds_cache_name: " << dds_cache_name << endl);
            CPPUNIT_ASSERT(access(dds_cache_name.c_str(), R_OK) == 0);

            string das_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dds->get_dataset_name().append("das_r")), false /*mangle*/);
            DBG(cerr << __func__ << " - das_cache_name: " << das_cache_name << endl);
            CPPUNIT_ASSERT(access(das_cache_name.c_str(), R_OK) == 0);

#if SYMETRIC_ADD_RESPONSES
            string dmr_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dds->get_dataset_name().append("dmr_r")), false /*mangle*/);
            DBG(cerr << __func__ << " - dmr_cache_name: " << dmr_cache_name << endl);
            CPPUNIT_ASSERT(access(dmr_cache_name.c_str(), R_OK) == 0);
#endif

            bool removed = d_mds->remove_responses(d_test_dds->get_dataset_name());
            CPPUNIT_ASSERT(removed);

            CPPUNIT_ASSERT(access(dds_cache_name.c_str(), R_OK) != 0);
            CPPUNIT_ASSERT(access(das_cache_name.c_str(), R_OK) != 0);
#if SYMETRIC_ADD_RESPONSES
            CPPUNIT_ASSERT(access(dmr_cache_name.c_str(), R_OK) != 0);
#endif
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }
#endif

#if 0
    void cache_a_dmr_response_dmr()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            init_dmr_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            DmrppMetadataStore::StreamDMR write_the_dmr_response(d_test_dmr);
            bool stored = d_mds->store_dap_response(write_the_dmr_response,
                d_test_dmr->name() + ".dmr_r", d_test_dmr->name(), "DMR");

            CPPUNIT_ASSERT(stored);

            // Now check the file
            string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "test_array_4.dmr_r";
            DBG(cerr << "Reading baseline: " << baseline_name << endl);
            CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

            string test_01_dmr_baseline = read_test_baseline(baseline_name);

            string response_name = d_mds_dir + "/" + c_mds_prefix + "test_array_4.dmr_r";
            // read_test_baseline() just reads stuff from a file - it will work for the response, too.
            DBG(cerr << "Reading response: " << response_name << endl);
            CPPUNIT_ASSERT(access(response_name.c_str(), R_OK) == 0);

            string stored_response = read_test_baseline(response_name);

            CPPUNIT_ASSERT(stored_response == test_01_dmr_baseline);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }

    void add_response_dmr_test()
    {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            init_dmr_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            bool stored = d_mds->add_responses(d_test_dmr, d_test_dmr->name());

            CPPUNIT_ASSERT(stored);

#if SYMETRIC_ADD_RESPONSES
            // look for the files
            string dds_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dmr->name().append("dds_r")), false /*mangle*/);
            DBG(cerr << __func__ << " - dds_cache_name: " << dds_cache_name << endl);
            CPPUNIT_ASSERT(access(dds_cache_name.c_str(), R_OK) == 0);

            string das_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dmr->name().append("das_r")), false /*mangle*/);
            DBG(cerr << __func__ << " - das_cache_name: " << das_cache_name << endl);
            CPPUNIT_ASSERT(access(das_cache_name.c_str(), R_OK) == 0);
#endif
            string dmr_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dmr->name().append("dmr_r")), false /*mangle*/);
            DBG(cerr << __func__ << " - dmr_cache_name: " << dmr_cache_name << endl);
            CPPUNIT_ASSERT(access(dmr_cache_name.c_str(), R_OK) == 0);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }

    void get_dmr_object_test() {
        DBG(cerr << __func__ << " - BEGIN" << endl);

        try {
            init_dmr_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            bool stored = d_mds->add_responses(d_test_dmr, d_test_dmr->name());

            CPPUNIT_ASSERT(stored);

            DMR *dmr = d_mds->get_dmr_object(d_test_dmr->name());

            CPPUNIT_ASSERT(dmr);

            DBG(cerr << "DMR: " << dmr->name() << endl);

            ostringstream oss;
            XMLWriter writer;
            dmr->print_dap4(writer);
            oss << writer.get_doc();

            string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "test_01.dmr_r";
            DBG(cerr << "Reading baseline: " << baseline_name << endl);
            CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

            string test_01_dmr_baseline = read_test_baseline(baseline_name);

            CPPUNIT_ASSERT(test_01_dmr_baseline == oss.str());
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch(Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }

        DBG(cerr << __func__ << " - END" << endl);
    }
#endif


    CPPUNIT_TEST_SUITE( DmrppMetadataStoreTest );

    CPPUNIT_TEST(ctor_test_1);
    CPPUNIT_TEST(ctor_test_2);
    CPPUNIT_TEST(ctor_test_3);

    CPPUNIT_TEST(cache_a_dmr_response);
    CPPUNIT_TEST(cache_a_dmrpp_response);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DmrppMetadataStoreTest);

} // namespace bes

int main(int argc, char*argv[])
{

    GetOpt getopt(argc, argv, "dbkh");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'b':
            bes_debug = true;  // bes_debug is a static global
            cerr << "##### BES DEBUG is ON" << endl;
            break;
        case 'k':   // -k turns off cleaning the metadata store dir
            clean = false;
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: DmrppMetadataStoreTest has the following tests:" << endl;
            const std::vector<Test*> &tests = bes::DmrppMetadataStoreTest::suite()->getTests();
            unsigned int prefix_len = bes::DmrppMetadataStoreTest::suite()->getName().append("::").length();
            for (std::vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                cerr << (*i)->getName().replace(0, prefix_len, "") << endl;
            }
            break;
        }
        default:
            break;
        }

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

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
            test = bes::DmrppMetadataStoreTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);

            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
