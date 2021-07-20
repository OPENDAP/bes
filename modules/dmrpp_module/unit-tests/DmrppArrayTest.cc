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

#include <unistd.h>
#include <util.h>
#include <debug.h>

#include "BESContextManager.h"
#include "BESError.h"
#include "BESDebug.h"
#include "TheBESKeys.h"

#include "DmrppArray.h"
#include "DmrppByte.h"
#include "DmrppRequestHandler.h"
#include "Chunk.h"
#include "SuperChunk.h"

#include "test_config.h"

using namespace libdap;

static bool debug = false;
static bool bes_debug = false;
static string bes_conf_file = "/bes.conf";

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)
#define prolog std::string("DmrppArrayTest::").append(__func__).append("() - ")

namespace dmrpp {

class DmrppArrayTest: public CppUnit::TestFixture {
private:

public:
    // Called once before everything gets tested
    DmrppArrayTest()
    {
    }

    // Called at the end of the test
    ~DmrppArrayTest()
    {
    }

    // Called before each test
    void setUp()
    {
        DBG(cerr << endl);
        // Contains BES Log parameters but not cache names
        TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");
        DBG(cerr << prolog << "TheBESKeys::ConfigFile: " << TheBESKeys::ConfigFile << endl);
        string val;
        bool found;
        TheBESKeys::TheKeys()->get_value("ff",val,found);

        if (bes_debug) BESDebug::SetUp("cerr,bes,http,curl,dmrpp");

        unsigned long long int max_threads = 8;
        dmrpp::DmrppRequestHandler::d_use_transfer_threads = true;
        dmrpp::DmrppRequestHandler::d_max_transfer_threads = max_threads;
        auto foo = new dmrpp::DmrppRequestHandler("Chaos");

    }

    // Called after each test
    void tearDown()
    {
    }

    void read_contiguous_sc_test() {
        DBG(cerr << prolog << "BEGIN" << endl);
        // string target_file_name= string(TEST_DATA_DIR).append("contiguous/d_int.h5.dap");
        shared_ptr<http::url> data_url(new http::url(string("file://").append(TEST_DATA_DIR).append("/").append("big_ole_chunky_test.txt")));
        unsigned long long target_file_size = 4194300;

        DmrppArray tiat(string("foo"), new libdap::Byte("foo"));
        tiat.append_dim(target_file_size,"test_dim");
        tiat.set_shuffle(false);
        tiat.set_deflate(false);

        vector<size_t> chunk_dim_sizes = {1};
        tiat.set_chunk_dimension_sizes(chunk_dim_sizes);
        vector<unsigned long long> position_in_array;
        position_in_array.push_back(0);
        tiat.add_chunk(data_url,"LE",target_file_size,0,position_in_array);

        try {
            tiat.read_contiguous();
        }
        catch(BESError &be){
            CPPUNIT_FAIL("Caught BESError. Message: " + be.get_verbose_message() );
        }
        catch(libdap::Error &lde){
            CPPUNIT_FAIL("Caught libdap::Error. Message: " + lde.get_error_message() );
        }
        catch(std::exception &e){
            CPPUNIT_FAIL(string("Caught libdap::Error. Message: ").append(e.what()));
        }
        catch(...){
            CPPUNIT_FAIL("Caught unkown exception.");
        }

        char expected[] = {'T', 'h', 'i', 's', 'I', 's', 'A', 'T', 'e','s','t'};
        unsigned int expected_size = 11;
        unsigned long long num_bytes = tiat.width();
        char *result = tiat.get_buf();

        /*
        unsigned long long  expected_index=0;
        for(unsigned long long index=0; index <num_bytes; index++){
            CPPUNIT_ASSERT(result[index] == expected[expected_index++ % 11]);
        }
*/


        DBG(cerr << prolog << "END" << endl);
    }
    void read_contiguous_test() {
        DBG(cerr << prolog << "BEGIN" << endl);
        // string target_file_name= string(TEST_DATA_DIR).append("contiguous/d_int.h5.dap");
        shared_ptr<http::url> data_url( new http::url(string("file://").append(TEST_DATA_DIR).append("/").append("big_ole_chunky_test.txt")));
        unsigned long long target_file_size = 4194300;

        DmrppArray tiat(string("foo"), new libdap::Byte("foo"));
        tiat.append_dim(target_file_size,"test_dim");
        tiat.set_shuffle(false);
        tiat.set_deflate(false);

        vector<size_t> chunk_dim_sizes = {1};
        tiat.set_chunk_dimension_sizes(chunk_dim_sizes);
        vector<unsigned long long> position_in_array;
        position_in_array.push_back(0);
        tiat.add_chunk(data_url,"LE",target_file_size,0,position_in_array);

        try {
            tiat.read_contiguous();
        }
        catch(BESError &be){
            CPPUNIT_FAIL("Caught BESError. Message: " + be.get_verbose_message() );
        }
        catch(libdap::Error &lde){
            CPPUNIT_FAIL("Caught libdap::Error. Message: " + lde.get_error_message() );
        }
        catch(std::exception &e){
            CPPUNIT_FAIL(string("Caught libdap::Error. Message: ").append(e.what()));
        }
        catch(...){
            CPPUNIT_FAIL("Caught unkown exception.");
        }

        char expected[] = {'T', 'h', 'i', 's', 'I', 's', 'A', 'T', 'e','s','t'};
        unsigned int expected_size = 11;
        unsigned long long num_bytes = tiat.width();
        char *result = tiat.get_buf();

        /*
        unsigned long long  expected_index=0;
        for(unsigned long long index=0; index <num_bytes; index++){
            CPPUNIT_ASSERT(result[index] == expected[expected_index++ % 11]);
        }
*/


        DBG(cerr << prolog << "END" << endl);
    }

    CPPUNIT_TEST_SUITE( DmrppArrayTest );
        CPPUNIT_TEST(read_contiguous_sc_test);
        CPPUNIT_TEST(read_contiguous_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DmrppArrayTest);

} // namespace dmrpp

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    int option_char;
    while ((option_char = getopt(argc, argv, "dD")) != -1)
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

    argc -= optind;
    argv += optind;

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
            test = dmrpp::DmrppArrayTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
