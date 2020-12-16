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
#define prolog std::string("SuperChunkTest::").append(__func__).append("() - ")

namespace dmrpp {

class SuperChunkTest: public CppUnit::TestFixture {
private:

public:
    // Called once before everything gets tested
    SuperChunkTest()
    {
    }

    // Called at the end of the test
    ~SuperChunkTest()
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
        TheBESKeys::TheKeys()->set_key("Http.cache.effective.urls.skip.regex.pattern","^.*$");

        if (debug) BESDebug::SetUp("cerr,bes,http,curl,dmrpp");

        unsigned long long int max_threads = 8;
        dmrpp::DmrppRequestHandler::d_use_parallel_transfers = true;
        dmrpp::DmrppRequestHandler::d_max_parallel_transfers = max_threads;
        auto foo = new dmrpp::DmrppRequestHandler("Chaos");

    }

    // Called after each test
    void tearDown()
    {
    }

    void sc_test_01() {
        DBG(cerr << prolog << "BEGIN" << endl);

        string data_url = string("file://").append(TEST_DATA_DIR).append("/").append("chunked_gzipped_fourD.h5");
        DBG(cerr << prolog << "data_url: " << data_url << endl);

        string chunk_position_in_array = "[0]";
        try  {
            DBG( cerr << prolog << "Creating: shared_ptr<Chunk> c1" << endl);
            shared_ptr<Chunk> c1(new Chunk(data_url, "", 1000,0,chunk_position_in_array));
            {
                SuperChunk sc;
                DBG( cerr << prolog << "Adding c1 to SuperChunk" << endl);
                sc.add_chunk(c1);
                DBG( cerr << prolog << "Calling SuperChunk::read()" << endl);
                sc.read();
            }

        }
        catch( BESError be){
            stringstream msg;
            msg << prolog << "CAUGHT BESError: " << be.get_verbose_message() << endl;
            cerr << msg.str();
            CPPUNIT_FAIL(msg.str());
        }
        catch( std::exception se ){
            stringstream msg;
            msg << "CAUGHT std::exception: " << se.what() << endl;
            cerr << msg.str();
            CPPUNIT_FAIL(msg.str());
        }
        catch( ... ){
            cerr << "CAUGHT Unknown Exception." << endl;
        }
        DBG( cerr << prolog << "END" << endl);
    }

    CPPUNIT_TEST_SUITE( SuperChunkTest );

    CPPUNIT_TEST(sc_test_01);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(SuperChunkTest);

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
            test = dmrpp::SuperChunkTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
