// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2018 OPeNDAP, Inc.
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

#include <memory>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <GetOpt.h>
#include <util.h>
#include <debug.h>

#include "BESContextManager.h"
#include "BESError.h"
#include "BESDebug.h"
#include "TheBESKeys.h"

#include "url_impl.h"
#include "Chunk.h"

#include "test_config.h"

using namespace libdap;

static bool debug = false;
static bool bes_debug = false;
static string bes_conf_file = "/bes.conf";

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)
#define prolog std::string("ChunkTest::").append(__func__).append("() - ")

namespace dmrpp {

class ChunkTest: public CppUnit::TestFixture {
private:
    Chunk d_chunk;

public:
    // Called once before everything gets tested
    ChunkTest()
    {
    }

    // Called at the end of the test
    ~ChunkTest()
    {
    }

    // Called before each test
    void setUp()
    {
        TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append(bes_conf_file);
        if (bes_debug) BESDebug::SetUp("cerr,http,curl,dmrpp");
        DBG(cerr << endl);
    }

    // Called after each test
    void tearDown()
    {
    }

    void set_position_in_array_test()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        d_chunk.set_position_in_array("[1,2,3,4]");
        vector<unsigned long long> pia = d_chunk.get_position_in_array();
        CPPUNIT_ASSERT(pia.size() == 4);
        CPPUNIT_ASSERT(pia.at(0) == 1);
        CPPUNIT_ASSERT(pia.at(1) == 2);
        CPPUNIT_ASSERT(pia.at(2) == 3);
        CPPUNIT_ASSERT(pia.at(3) == 4);
    }

    void set_position_in_array_test_2()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        d_chunk.set_position_in_array("[5]");
        vector<unsigned long long> pia = d_chunk.get_position_in_array();
        CPPUNIT_ASSERT(pia.size() == 1);
        CPPUNIT_ASSERT(pia.at(0) == 5);
    }

    void set_position_in_array_test_3()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        d_chunk.set_position_in_array("[]");
        CPPUNIT_FAIL("set_position_in_array() should throw on missing values");
    }

    void set_position_in_array_test_4()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        d_chunk.set_position_in_array("[1,2,3,4");
        CPPUNIT_FAIL("set_position_in_array() should throw on a missing bracket");
    }

    void set_position_in_array_test_5()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        d_chunk.set_position_in_array("[1,x]");
        CPPUNIT_FAIL("set_position_in_array() should throw on bad values");
    }

    // This test fails
    void set_position_in_array_test_6()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        d_chunk.set_position_in_array("[1,2,]");
        CPPUNIT_FAIL("set_position_in_array() should throw on bad values");
    }

    void add_tracking_query_param_test()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        CPPUNIT_ASSERT(d_chunk.d_query_marker.empty());
    }

    void add_tracking_query_param_test_2()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        CPPUNIT_ASSERT(S3_TRACKING_CONTEXT == "cloudydap");
    }

    void add_tracking_query_param_test_3()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        try {
            BESContextManager::TheManager()->set_context(S3_TRACKING_CONTEXT, "request_id");
            // add_tracking_query_param() only works with S3 URLs. Bug? jhrg 8/9/18
            shared_ptr<http::url> data_url(new http::url("http://s3.amazonaws.com/somewhereovertherainbow/foo.nc"));
            d_chunk.set_data_url(data_url);

            d_chunk.add_tracking_query_param();

            CPPUNIT_ASSERT(!d_chunk.d_query_marker.empty());
            DBG(cerr << prolog << "d_chunk.d_query_marker: " << d_chunk.d_query_marker << endl);
            CPPUNIT_ASSERT(d_chunk.d_query_marker == "cloudydap=request_id");

        }
        catch(BESError &be){
            CPPUNIT_FAIL(prolog + be.get_message());
        }
        catch(std::exception e){
            CPPUNIT_FAIL(prolog + "Caught std::exception. Message: "+e.what());
        }
    }

    void add_tracking_query_param_test_4()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        try {
            BESContextManager::TheManager()->set_context(S3_TRACKING_CONTEXT, "request_id");

            // add_tracking_query_param() only works with S3 URLs. Bug? jhrg 8/9/18
            shared_ptr<http::url> some_url( new http::url("http://s3.amazonaws.com/somewhereovertherainbow/foo.nc"));
            d_chunk.set_data_url(some_url);

            d_chunk.add_tracking_query_param();

            auto data_url = d_chunk.get_data_url();

            DBG(cerr << prolog << "data_url: " << data_url->str() << endl);
            CPPUNIT_ASSERT(!data_url->str().empty());
            CPPUNIT_ASSERT(data_url->str() == "http://s3.amazonaws.com/somewhereovertherainbow/foo.nc?cloudydap=request_id");
        }
        catch(BESError &be){
            CPPUNIT_FAIL(prolog + be.get_message());
        }
        catch(std::exception e){
            CPPUNIT_FAIL(prolog + "Caught std::exception. Message: "+e.what());
        }
    }

    void add_tracking_query_param_test_4_1() {
        DBG(cerr << prolog << "BEGIN" << endl);
        try {
            // An S3 URL, but no context.
            BESContextManager::TheManager()->unset_context(S3_TRACKING_CONTEXT);   //>set_context("cloudydap", "request_id");
            shared_ptr<http::url> data_url(new http::url("http://s3.amazonaws.com/somewhereovertherainbow/foo.nc"));
            d_chunk.set_data_url(data_url);
            d_chunk.add_tracking_query_param();
        }
        catch(BESError &be){
            CPPUNIT_FAIL(prolog + be.get_message());
        }
        catch(std::exception e){
            CPPUNIT_FAIL(prolog + "Caught std::exception. Message: "+e.what());
        }

        CPPUNIT_ASSERT(d_chunk.d_query_marker.empty());
    }

    // Test the non-default ctor
    void add_tracking_query_param_test_5()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        try {
            BESContextManager::TheManager()->set_context(S3_TRACKING_CONTEXT, "request_id");
            shared_ptr<http::url> sotr(new http::url("http://s3.amazonaws.com/somewhereovertherainbow/foo.nc"));
            unique_ptr<Chunk> l_chunk(new Chunk(sotr, "", 100, 10, ""));

            CPPUNIT_ASSERT(!l_chunk->d_query_marker.empty());
            DBG(cerr << prolog << "l_chunk->d_query_marker: " << l_chunk->d_query_marker << endl);
            CPPUNIT_ASSERT(l_chunk->d_query_marker == "cloudydap=request_id");

            auto data_url = l_chunk->get_data_url();

            DBG(cerr << prolog << "data_url: " << data_url << endl);
            CPPUNIT_ASSERT(!data_url->str().empty());
            CPPUNIT_ASSERT(data_url->str() == "http://s3.amazonaws.com/somewhereovertherainbow/foo.nc?cloudydap=request_id");
        }
        catch(BESError &be){
            CPPUNIT_FAIL(prolog + be.get_message());
        }
        catch(std::exception e){
            CPPUNIT_FAIL(prolog + "Caught std::exception. Message: "+e.what());
        }
    }

    void add_tracking_query_param_test_5_1()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        try {
            // No context, S3 URL, non-default ctor
            BESContextManager::TheManager()->unset_context(S3_TRACKING_CONTEXT);
            shared_ptr<http::url> sotr(new http::url("http://s3.amazonaws.com/somewhereovertherainbow"));
            unique_ptr<Chunk> l_chunk(new Chunk(sotr, "", 100, 10, ""));

            CPPUNIT_ASSERT(l_chunk->d_query_marker.empty());

            auto data_url = l_chunk->get_data_url();

            DBG(cerr << prolog << "data_url: " << data_url << endl);
            CPPUNIT_ASSERT(!data_url->str().empty());
            CPPUNIT_ASSERT(data_url->str() == "http://s3.amazonaws.com/somewhereovertherainbow");
        }
        catch(BESError &be){
            CPPUNIT_FAIL(prolog + be.get_message());
        }
        catch(std::exception e){
            CPPUNIT_FAIL(prolog + "Caught std::exception. Message: "+e.what());
        }

    }

   CPPUNIT_TEST_SUITE( ChunkTest );

    CPPUNIT_TEST(set_position_in_array_test);
    CPPUNIT_TEST(set_position_in_array_test_2);

    CPPUNIT_TEST_EXCEPTION(set_position_in_array_test_3, BESError);
    CPPUNIT_TEST_EXCEPTION(set_position_in_array_test_4, BESError);
    CPPUNIT_TEST_EXCEPTION(set_position_in_array_test_5, BESError);

    CPPUNIT_TEST_FAIL(set_position_in_array_test_6);

    CPPUNIT_TEST(add_tracking_query_param_test);
    CPPUNIT_TEST(add_tracking_query_param_test_2);
    CPPUNIT_TEST(add_tracking_query_param_test_3);
    CPPUNIT_TEST(add_tracking_query_param_test_4);
    CPPUNIT_TEST(add_tracking_query_param_test_4_1);
    CPPUNIT_TEST(add_tracking_query_param_test_5);
    CPPUNIT_TEST(add_tracking_query_param_test_5_1);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ChunkTest);

} // namespace dmrpp

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "dD");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = true;  // debug is a static global
            break;
        case 'D':
            debug = true;  // debug is a static global
            bes_debug = true;  // debug is a static global
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
            test = dmrpp::ChunkTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
