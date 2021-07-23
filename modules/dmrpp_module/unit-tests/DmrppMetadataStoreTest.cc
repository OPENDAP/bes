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

#include <unistd.h>
#include <GNURegex.h>
#include <util.h>
#include <debug.h>

#include <BaseTypeFactory.h>
#include <D4BaseTypeFactory.h>

#include "BESError.h"
#include "TheBESKeys.h"
#include "BESDebug.h"

#include "DmrppMetadataStore.h"

#include "DMRpp.h"
#include "DmrppTypeFactory.h"
#include "DmrppParserSax2.h"

#include "read_test_baseline.h"
#include "test_config.h"

static bool debug = false;
static bool bes_debug = false;
static bool clean = true;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

#define DEBUG_KEY "dmrpp_store,cache,dmrpp:parser"

using namespace CppUnit;
using namespace std;
using namespace libdap;
using namespace dmrpp;

#define prolog std::string("DmrppMetadataStoreTest::").append(__func__).append("() - ")

namespace bes {

// Move this into the class when we goto C++-11
static const string c_mds_prefix = "mds_"; // used when cleaning the cache, etc.
static const string c_mds_name = "/mds";
static const string c_mds_baselines = string(TEST_SRC_DIR) + "/baselines";

class DmrppMetadataStoreTest: public TestFixture {
private:
    DMR *d_test_dmr;
    shared_ptr<http::url> d_test_dmr_url;
    D4BaseTypeFactory d_d4f;
    DmrppTypeFactory d_dmrpp_factory;

    string d_mds_dir;
    DmrppMetadataStore *d_mds;
    /**
     * SetUp()-like method to build a DMR for testing.
     */
    void init_dmr_and_mds()
    {
        DBG( cerr << prolog << "BEGIN" << endl );
        try {
            // Stock code to get the d_test_dds and d_mds objects used by many
            // of the tests.
            d_mds = DmrppMetadataStore::get_instance(d_mds_dir, c_mds_prefix, 1000);
            DBG(cerr << prolog << "Retrieved DmrppMetadataStore object: " << d_mds << endl);

            // Get a DMR to cache.
            string file_name = string(TEST_SRC_DIR).append("/input-files/test_01.dmr");

            d_test_dmr = new DMR(&d_d4f);
            D4ParserSax2 dp;
            DBG(cerr << prolog << "DMR file to be parsed: " << file_name << endl);
            fstream in(file_name.c_str(), ios::in|ios::binary);
            dp.intern(in, d_test_dmr);

            DBG(cerr << prolog << "DMR Name: " << d_test_dmr->name() << endl);
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
        DBG( cerr << prolog << "BEGIN" << endl );
    }

    void init_dmrpp_and_mds()
    {
        DBG( cerr << prolog << "BEGIN" << endl );
        try {
            // Stock code to get the d_test_dds and d_mds objects used by many
            // of the tests.
            d_mds = DmrppMetadataStore::get_instance(d_mds_dir, c_mds_prefix, 1000);
            DBG(cerr << prolog << "Retrieved DmrppMetadataStore object: " << d_mds << endl);

            // Get a DMRpp to cache.
            string file_name = string(TEST_SRC_DIR).append("/input-files/chunked_fourD.h5.dmrpp");
            string test_dmr_url_str = FILE_PROTOCOL;
            test_dmr_url_str += "This/Is/a/Test.nc";
            d_test_dmr_url = shared_ptr<http::url>(new http::url(test_dmr_url_str));

            DMRpp *dmrpp = new DMRpp(&d_dmrpp_factory);
            dmrpp->set_href(d_test_dmr_url->str());

            d_test_dmr = dmrpp;
            DmrppParserSax2 dp;
            DBG(cerr << prolog << "DMRpp file to be parsed: " << file_name << endl);
            fstream in(file_name.c_str(), ios::in|ios::binary);
            dp.intern(in, d_test_dmr);

            DBG(cerr << prolog << "DMRpp Name: " << d_test_dmr->name() << endl);
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
        DBG( cerr << prolog << "END" << endl );
    }

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
        DBG( cerr << endl );
        DBG(cerr << prolog <<  "BEGIN" << endl);

        if (bes_debug) BESDebug::SetUp(string("cerr,").append(DEBUG_KEY));

        if (clean) clean_cache_dir(c_mds_name);

        // Contains BES Log parameters but not cache names
        TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");

        DBG(cerr << prolog <<  "END" << endl);
    }

    void tearDown()
    {
        DBG(cerr << prolog <<  "BEGIN" << endl);

        delete d_test_dmr; d_test_dmr = 0;

        d_mds->delete_instance();

        if (clean) clean_cache_dir(d_mds_dir);

        DBG(cerr << prolog <<  "END" << endl);
        DBG( cerr << endl );
    }

    void ctor_test_1()
    {
        DBG(cerr << prolog <<  "BEGIN" << endl);

        try {
            // The call to get_instance should fail since the directory is named,
            // but does not exist and cannot be made.
        		// Check to see if the test is being run by the super user or built in root.
        	    if (access("/", W_OK) != 0) {
				d_mds = DmrppMetadataStore::get_instance("/new", c_mds_prefix, 1000);
				DBG(cerr << prolog << "retrieved DmrppMetadataStore instance: " << d_mds << endl);
				CPPUNIT_FAIL("get_instance() Should not return when the non-existent directory cannot be created");
        	    }
        }
        catch (BESError &e) {
            CPPUNIT_ASSERT(!e.get_message().empty());
        }

        DBG(cerr << prolog <<  "END" << endl);
    }

    void ctor_test_2()
    {
        DBG(cerr << prolog <<  "BEGIN" << endl);

        try {
            // This one should work and will make the directory
            d_mds = DmrppMetadataStore::get_instance(d_mds_dir, c_mds_prefix, 1000);
            DBG(cerr << prolog << "retrieved DmrppMetadataStore instance: " << d_mds << endl);
            CPPUNIT_ASSERT(d_mds);
            CPPUNIT_ASSERT(!d_mds->is_unlimited());

            d_mds->update_and_purge("no_name"); //cheesy test - use -b to read BES debug info
        }
        catch (BESError &e) {
            CPPUNIT_FAIL("Caught exception: " + e.get_message());
        }

        DBG(cerr << prolog <<  "END" << endl);
    }

    void ctor_test_3()
    {
        DBG(cerr << prolog <<  "BEGIN" << endl);

        try {
            // This one should work and will make the directory
            d_mds = DmrppMetadataStore::get_instance(d_mds_dir, c_mds_prefix, 0);
            DBG(cerr << prolog << "retrieved DmrppMetadataStore instance: " << d_mds << endl);
            CPPUNIT_ASSERT(d_mds);
            CPPUNIT_ASSERT(d_mds->is_unlimited());

            d_mds->update_and_purge("no_name");
        }
        catch (BESError &e) {
            CPPUNIT_FAIL("Caught exception: " + e.get_message());
        }

        DBG(cerr << prolog <<  "END" << endl);
    }

    void cache_a_dmr_response()
    {
        DBG(cerr << prolog <<  "BEGIN" << endl);

        try {
            init_dmr_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            DmrppMetadataStore::StreamDMR write_the_dmr_response(d_test_dmr);
            bool stored = d_mds->store_dap_response(write_the_dmr_response,
                d_test_dmr->name() + ".dmr_r", d_test_dmr->name(), "DMR");

            CPPUNIT_ASSERT(stored);

            // Now check the file
            string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "test_array_4.dmr_r";
            DBG(cerr << prolog << "Reading baseline: " << baseline_name << endl);
            CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

            string test_05_dmr_baseline = read_test_baseline(baseline_name);

            string response_name = d_mds_dir + "/" + c_mds_prefix + "test_array_4.dmr_r";
            DBG(cerr << prolog << "Reading response: " << response_name << endl);
            CPPUNIT_ASSERT(access(response_name.c_str(), R_OK) == 0);

            string stored_response = read_test_baseline(response_name);

            CPPUNIT_ASSERT(stored_response == test_05_dmr_baseline);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << prolog <<  "END" << endl);
    }

    void cache_a_dmrpp_response()
    {
        DBG(cerr << prolog <<  "BEGIN" << endl);

        try {
            init_dmrpp_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            DmrppMetadataStore::StreamDMRpp write_the_dmrpp_response(d_test_dmr);
            bool stored = d_mds->store_dap_response(write_the_dmrpp_response, d_test_dmr->name() + ".dmrpp_r", d_test_dmr->name(), "DMRpp");

            CPPUNIT_ASSERT(stored);

            // Now check the file
            string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "chunked_fourD.h5.dmrpp_r";
            DBG(cerr << prolog << "Reading baseline: " << baseline_name << endl);
            CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

            string chunked_4d_dmrpp_baseline = read_test_baseline(baseline_name);

            string response_name = d_mds_dir + "/" + c_mds_prefix + "chunked_fourD.h5.dmrpp_r";
            DBG(cerr << prolog << "Reading response: " << response_name << endl);
            CPPUNIT_ASSERT(access(response_name.c_str(), R_OK) == 0);

            string stored_response = read_test_baseline(response_name);

            CPPUNIT_ASSERT(stored_response == chunked_4d_dmrpp_baseline);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << prolog <<  "END" << endl);
    }

    void add_response_test()
    {
        DBG(cerr << prolog <<  "BEGIN" << endl);

        try {
            init_dmr_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            bool stored = d_mds->add_responses(d_test_dmr, d_test_dmr->name());

            CPPUNIT_ASSERT(stored);

            string dmr_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dmr->name().append("dmr_r")), false /*mangle*/);
            DBG(cerr << prolog <<  " - dmr_cache_name: " << dmr_cache_name << endl);
            CPPUNIT_ASSERT(access(dmr_cache_name.c_str(), R_OK) == 0);

            // There should be no DMR++ in the MDS
            string dmrpp_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dmr->name().append("dmrpp_r")), false /*mangle*/);
            DBG(cerr << prolog <<  " - dmrpp_cache_name: " << dmrpp_cache_name << endl);
            CPPUNIT_ASSERT(access(dmrpp_cache_name.c_str(), R_OK) != 0);

        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << prolog <<  "END" << endl);
    }

    // Make a DMR++. add_responses() should add both the DMR and DMR++
    void add_response_test_2()
    {
        DBG(cerr << prolog <<  "BEGIN" << endl);

        try {
            init_dmrpp_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            bool stored = d_mds->add_responses(d_test_dmr, d_test_dmr->name());

            CPPUNIT_ASSERT(stored);

            string dmr_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dmr->name().append("dmr_r")), false /*mangle*/);
            DBG(cerr << prolog <<  " - dmr_cache_name: " << dmr_cache_name << endl);
            CPPUNIT_ASSERT(access(dmr_cache_name.c_str(), R_OK) == 0);

            string dmrpp_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dmr->name().append("dmrpp_r")), false /*mangle*/);
            DBG(cerr << prolog <<  " - dmrpp_cache_name: " << dmrpp_cache_name << endl);
            CPPUNIT_ASSERT(access(dmrpp_cache_name.c_str(), R_OK) == 0);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << prolog <<  "END" << endl);
    }

    void is_dmrpp_available_test()
    {
        DBG(cerr << prolog <<  "BEGIN" << endl);

        try {
            init_dmrpp_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            bool stored = d_mds->add_responses(d_test_dmr, d_test_dmr->name());

            CPPUNIT_ASSERT(stored);

            GlobalMetadataStore::MDSReadLock dmr = d_mds->is_dmr_available(d_test_dmr->name());
            CPPUNIT_ASSERT(dmr());

            DmrppMetadataStore::MDSReadLock dmrpp = d_mds->is_dmrpp_available(d_test_dmr->name());
            CPPUNIT_ASSERT(dmrpp());
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << prolog <<  "END" << endl);

    }

    void write_dmr_response_test() {
        DBG(cerr << prolog <<  "BEGIN" << endl);

        try {
            init_dmr_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            bool stored = d_mds->add_responses(d_test_dmr, d_test_dmr->name());

            CPPUNIT_ASSERT(stored);

            // Now lets read the object from the cache
            ostringstream oss;
            d_mds->write_dmr_response(d_test_dmr->name(), oss);
            DBG(cerr << prolog << "DMR response: " << endl << oss.str() << endl);

            string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "test_array_4.dmr_r";
            DBG(cerr << prolog << "Reading baseline: " << baseline_name << endl);
            CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

            string test_array_4_dmr_baseline = read_test_baseline(baseline_name);

            CPPUNIT_ASSERT(test_array_4_dmr_baseline == oss.str());
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << prolog <<  "END" << endl);
    }

    void write_dmrpp_response_test() {
        DBG(cerr << prolog <<  "BEGIN" << endl);

        try {
            init_dmrpp_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            bool stored = d_mds->add_responses(d_test_dmr, d_test_dmr->name());

            CPPUNIT_ASSERT(stored);

            // Now lets read the object from the cache
            ostringstream oss;
            d_mds->write_dmrpp_response(d_test_dmr->name(), oss);
            DBG(cerr << prolog << "DMR++ response: " << endl << oss.str() << endl);

            string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "chunked_fourD.h5.dmrpp_r";
            DBG(cerr << prolog << "Reading baseline: " << baseline_name << endl);
            CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

            string test_05_dmr_baseline = read_test_baseline(baseline_name);

            CPPUNIT_ASSERT(test_05_dmr_baseline == oss.str());
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << prolog <<  "END" << endl);
    }

    void remove_object_test()
    {
        DBG(cerr << prolog <<  "BEGIN" << endl);

        try {
            init_dmrpp_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            bool stored = d_mds->add_responses(d_test_dmr, d_test_dmr->name());

            CPPUNIT_ASSERT(stored);

            // look for the files
            string dmr_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dmr->name().append("dmr_r")), false /*mangle*/);
            DBG(cerr << prolog <<  "dmr_cache_name: " << dmr_cache_name << endl);
            CPPUNIT_ASSERT(access(dmr_cache_name.c_str(), R_OK) == 0);

            string dmrpp_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dmr->name().append("dmrpp_r")), false /*mangle*/);
            DBG(cerr << prolog <<  "dmrpp_cache_name: " << dmrpp_cache_name << endl);
            CPPUNIT_ASSERT(access(dmrpp_cache_name.c_str(), R_OK) == 0);

            bool removed = d_mds->remove_responses(d_test_dmr->name());
            CPPUNIT_ASSERT(removed);

            CPPUNIT_ASSERT(access(dmr_cache_name.c_str(), R_OK) != 0);
            CPPUNIT_ASSERT(access(dmrpp_cache_name.c_str(), R_OK) != 0);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << prolog <<  "END" << endl);
    }

    void get_dmr_object_test() {
        DBG(cerr << prolog <<  "BEGIN" << endl);

        try {
            init_dmr_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            bool stored = d_mds->add_responses(d_test_dmr, d_test_dmr->name());

            CPPUNIT_ASSERT(stored);

            DMR *dmr = d_mds->get_dmr_object(d_test_dmr->name());

            CPPUNIT_ASSERT(dmr);

            DBG(cerr << prolog << "DMR: " << dmr->name() << endl);

            ostringstream oss;
            XMLWriter writer;
            dmr->print_dap4(writer);
            oss << writer.get_doc();

            string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "test_array_4.dmr_r";
            DBG(cerr << prolog << "Reading baseline: " << baseline_name << endl);
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

        DBG(cerr << prolog <<  "END" << endl);
    }

    void get_dmrpp_object_test() {
        DBG(cerr << prolog <<  "BEGIN" << endl);

        try {
            init_dmrpp_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            bool stored = d_mds->add_responses(d_test_dmr, d_test_dmr->name());
            DBG(cerr << prolog <<  "Stored: " << (stored?"true":"false") << endl);
            CPPUNIT_ASSERT(stored);

            DMRpp *dmrpp = d_mds->get_dmrpp_object(d_test_dmr->name());
            DBG(cerr << prolog <<  "Got dmr++: " << (dmrpp?dmrpp->name():"NULL") << endl);
            CPPUNIT_ASSERT(dmrpp);

            ostringstream oss;
            XMLWriter writer;
            dmrpp->print_dmrpp(writer, d_test_dmr_url->str());
            oss << writer.get_doc();

            string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "chunked_fourD.h5.dmrpp_r";
            DBG(cerr << prolog << "Reading baseline: " << baseline_name << endl);
            CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

            string chunked_fourD_dmrpp_baseline = read_test_baseline(baseline_name);

            CPPUNIT_ASSERT(chunked_fourD_dmrpp_baseline == oss.str());
        }
        catch (BESError &e) {
            stringstream msg;
            msg << "Caught BESError! message: " << e.get_verbose_message() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }

        DBG(cerr << prolog <<  "END" << endl);
    }

    void use_dmrpp_response_test()
    {
        DBG(cerr << prolog <<  "BEGIN" << endl);

        try {
            init_dmrpp_and_mds();

            // Store it - this will work if the the code is cleaning the cache.
            bool stored = d_mds->add_responses(d_test_dmr, d_test_dmr->name());

            CPPUNIT_ASSERT(stored);

            string dmr_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dmr->name().append("dmr_r")), false /*mangle*/);
            DBG(cerr << prolog <<  " - dmr_cache_name: " << dmr_cache_name << endl);
            CPPUNIT_ASSERT(access(dmr_cache_name.c_str(), R_OK) == 0);

            string dmrpp_cache_name = d_mds->get_cache_file_name(d_mds->get_hash(d_test_dmr->name().append("dmrpp_r")), false /*mangle*/);
            DBG(cerr << prolog <<  " - dmrpp_cache_name: " << dmrpp_cache_name << endl);
            CPPUNIT_ASSERT(access(dmrpp_cache_name.c_str(), R_OK) == 0);

            // So the DMR++ response is in the cache and an instance of DmrppMetadataStore can
            // access it. What about am instance of GlobalMetadataStore?

            // This will use the same parameters as the Dmrpp M S.
            GlobalMetadataStore *gms = GlobalMetadataStore::get_instance(d_mds_dir, c_mds_prefix, 1000);

            string gms_dmr_cache_name = gms->get_cache_file_name(gms->get_hash(d_test_dmr->name().append("dmr_r")), false /*mangle*/);
            DBG(cerr << prolog <<  " - gms_dmr_cache_name: " << gms_dmr_cache_name << endl);
            CPPUNIT_ASSERT(access(gms_dmr_cache_name.c_str(), R_OK) == 0);

            string gms_dmrpp_cache_name = gms->get_cache_file_name(gms->get_hash(d_test_dmr->name().append("dmrpp_r")), false /*mangle*/);
            DBG(cerr << prolog <<  " - gms_dmrpp_cache_name: " << gms_dmrpp_cache_name << endl);
            CPPUNIT_ASSERT(access(gms_dmrpp_cache_name.c_str(), R_OK) == 0);

            // Now lets read the object from the cache
            ostringstream oss;
            gms->write_dmrpp_response(d_test_dmr->name(), oss);
            DBG(cerr << prolog << "DMR++ response: " << endl << oss.str() << endl);

            string baseline_name = c_mds_baselines + "/" + c_mds_prefix + "chunked_fourD.h5.dmrpp_r";
            DBG(cerr << prolog << "Reading baseline: " << baseline_name << endl);
            CPPUNIT_ASSERT(access(baseline_name.c_str(), R_OK) == 0);

            string chunked_fourD_dmrpp_baseline = read_test_baseline(baseline_name);

            CPPUNIT_ASSERT(chunked_fourD_dmrpp_baseline == oss.str());
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }

        DBG(cerr << prolog <<  "END" << endl);
    }


    CPPUNIT_TEST_SUITE( DmrppMetadataStoreTest );

    CPPUNIT_TEST(ctor_test_1);
    CPPUNIT_TEST(ctor_test_2);
    CPPUNIT_TEST(ctor_test_3);

    CPPUNIT_TEST(cache_a_dmr_response);
    CPPUNIT_TEST(cache_a_dmrpp_response);

    CPPUNIT_TEST(write_dmr_response_test);
    CPPUNIT_TEST(add_response_test);
    CPPUNIT_TEST(add_response_test_2);

    CPPUNIT_TEST(is_dmrpp_available_test);
    CPPUNIT_TEST(write_dmrpp_response_test);

    CPPUNIT_TEST(remove_object_test);

    CPPUNIT_TEST(get_dmr_object_test);
    CPPUNIT_TEST(get_dmrpp_object_test);

    CPPUNIT_TEST(use_dmrpp_response_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DmrppMetadataStoreTest);

} // namespace bes

int main(int argc, char*argv[])
{
    int option_char;
    while ((option_char = getopt(argc, argv, "dbkh")) != -1)
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

    argc -= optind;
    argv += optind;

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
            test = bes::DmrppMetadataStoreTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);

            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
