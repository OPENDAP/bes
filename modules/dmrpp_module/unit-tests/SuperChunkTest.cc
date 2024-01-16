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
#include <libdap/util.h>
#include <libdap/debug.h>

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

        if (bes_debug) BESDebug::SetUp("cerr,bes,http,curl,dmrpp");

        unsigned long long int max_threads = 8;
        dmrpp::DmrppRequestHandler::d_use_transfer_threads = true;
        dmrpp::DmrppRequestHandler::d_max_transfer_threads = max_threads;

        // This call instantiates the curlHandlePool. jhrg 5/24/22
        auto foo = new dmrpp::DmrppRequestHandler("Chaos");
    }

    // Called after each test
    void tearDown()
    {
    }

    void sc_one_chunk_test() {
        DBG(cerr << prolog << "BEGIN" << endl);

        // chunked_gzipped_fourD.h5 is 2,870,087 bytes (2.9 MB on disk)
        string url_s = string("file://").append(TEST_DATA_DIR).append("/").append("chunked_gzipped_fourD.h5");
        shared_ptr<http::url> data_url(new http::url(url_s));
        DBG(cerr << prolog << "data_url: " << data_url << endl);

        string chunk_position_in_array = "[0]";
        try  {
            DBG( cerr << prolog << "Creating: shared_ptr<Chunk> c1" << endl);
            shared_ptr<Chunk> c1(new Chunk(data_url, "", 1000,0,chunk_position_in_array));
            {
                SuperChunk sc(prolog);
                DBG( cerr << prolog << "Adding c1 to SuperChunk" << endl);
                sc.add_chunk(c1);
                DBG( cerr << prolog << "Calling SuperChunk::retrieve_data()" << endl);
                sc.retrieve_data();
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

    void sc_chunks_test_01() {
        DBG(cerr << prolog << "BEGIN" << endl);

        // chunked_gzipped_fourD.h5 is 2,870,087 bytes (2.9 MB on disk)
        string url_s = string("file://").append(TEST_DATA_DIR).append("/").append("chunked_gzipped_fourD.h5");
        shared_ptr<http::url> data_url(new http::url(url_s));
        DBG(cerr << prolog << "data_url: " << data_url << endl);

        string chunk_position_in_array = "[0]";
        try  {
            DBG( cerr << prolog << "Creating: shared_ptr<Chunk> c1, c2, c3, c4" << endl);
            shared_ptr<Chunk> c1(new Chunk(data_url, "", 100000,0,chunk_position_in_array));
            shared_ptr<Chunk> c2(new Chunk(data_url, "", 100000,100000,chunk_position_in_array));
            shared_ptr<Chunk> c3(new Chunk(data_url, "", 100000,200000,chunk_position_in_array));
            shared_ptr<Chunk> c4(new Chunk(data_url, "", 100000,300000,chunk_position_in_array));

            {
                SuperChunk sc(prolog);
                bool chunk_was_added;
                chunk_was_added = sc.add_chunk(c1);
                DBG( cerr << prolog << "Chunk c1 was "<< (chunk_was_added?"":"NOT ") << "added to SuperChunk" << endl);
                CPPUNIT_ASSERT(chunk_was_added);

                chunk_was_added = sc.add_chunk(c2);
                DBG( cerr << prolog << "Chunk c2 was "<< (chunk_was_added?"":"NOT ") << "added to SuperChunk" << endl);
                CPPUNIT_ASSERT(chunk_was_added);

                chunk_was_added = sc.add_chunk(c3);
                DBG( cerr << prolog << "Chunk c3 was "<< (chunk_was_added?"":"NOT ") << "added to SuperChunk" << endl);
                CPPUNIT_ASSERT(chunk_was_added);

                chunk_was_added = sc.add_chunk(c4);
                DBG( cerr << prolog << "Chunk c4 was "<< (chunk_was_added?"":"NOT ") << "added to SuperChunk" << endl);
                CPPUNIT_ASSERT(chunk_was_added);

                // Read the data
                sc.retrieve_data();

            }

        }
        catch( BESError &be){
            stringstream msg;
            msg << prolog << "CAUGHT BESError: " << be.get_verbose_message() << endl;
            cerr << msg.str();
            CPPUNIT_FAIL(msg.str());
        }
        catch( std::exception &se ){
            stringstream msg;
            msg << "CAUGHT std::exception message: " << se.what() << endl;
            cerr << msg.str();
            CPPUNIT_FAIL(msg.str());
        }
        DBG( cerr << prolog << "END" << endl);
    }

    void sc_chunks_test_02() {
        DBG(cerr << prolog << "BEGIN" << endl);

        // this_is_a_test.txt is 1106 bytes and contains human readable text chunk content.
        string url_s = string("file://").append(TEST_DATA_DIR).append("/").append("this_is_a_test.txt");
        shared_ptr<http::url> data_url(new http::url(url_s));
        DBG(cerr << prolog << "data_url: " << data_url << endl);

        string chunk_position_in_array = "[0]";
        try  {
            DBG( cerr << prolog << "Creating shared_ptr<Chunk> l1, c2, c3, c4" << endl);
            shared_ptr<Chunk> T0(new Chunk(data_url, "", 100, 0, chunk_position_in_array));
            shared_ptr<Chunk> h0(new Chunk(data_url, "", 100, 100, chunk_position_in_array));
            shared_ptr<Chunk> i0(new Chunk(data_url, "", 100, 200, chunk_position_in_array));
            shared_ptr<Chunk> s0(new Chunk(data_url, "", 100, 300, chunk_position_in_array));

            shared_ptr<Chunk> i1(new Chunk(data_url, "", 100, 402, chunk_position_in_array));
            shared_ptr<Chunk> s1(new Chunk(data_url, "", 100, 502, chunk_position_in_array));

            shared_ptr<Chunk> a0(new Chunk(data_url, "", 100, 604, chunk_position_in_array));
#if 0
            // Don't need these yet but, i typed them...
            shared_ptr<Chunk> t0(new Chunk(data_url, "", 100, 706, chunk_position_in_array));
            shared_ptr<Chunk> e0(new Chunk(data_url, "", 100, 806, chunk_position_in_array));
            shared_ptr<Chunk> s3(new Chunk(data_url, "", 100, 906, chunk_position_in_array));
            shared_ptr<Chunk> t2(new Chunk(data_url, "", 100, 1006, chunk_position_in_array));
#endif
            {

                SuperChunk word_a(prolog+"word_a");
                SuperChunk word_test(prolog+"word_test");
                bool chunk_was_added;

                SuperChunk word_this(prolog+"word_this");
                chunk_was_added = word_this.add_chunk(T0);
                DBG( cerr << prolog << "Chunk T0 was "<< (chunk_was_added?"":"NOT ") << "added to SuperChunk word_this" << endl);
                CPPUNIT_ASSERT(chunk_was_added);

                chunk_was_added = word_this.add_chunk(h0);
                DBG( cerr << prolog << "Chunk h0 was "<< (chunk_was_added?"":"NOT ") << "added to SuperChunk word_this" << endl);
                CPPUNIT_ASSERT(chunk_was_added);

                chunk_was_added = word_this.add_chunk(i0);
                DBG( cerr << prolog << "Chunk i0 was "<< (chunk_was_added?"":"NOT ") << "added to SuperChunk word_this" << endl);
                CPPUNIT_ASSERT(chunk_was_added);

                chunk_was_added = word_this.add_chunk(s0);
                DBG( cerr << prolog << "Chunk s0 was "<< (chunk_was_added?"":"NOT ") << "added to SuperChunk word_this" << endl);
                CPPUNIT_ASSERT(chunk_was_added);

                // The i1 chunk is not contiguous with s0 and should be rejected by the word_this SuperChunk.
                chunk_was_added = word_this.add_chunk(i1);
                DBG( cerr << prolog << "Chunk i1 was "<< (chunk_was_added?"":"NOT ") << "added to SuperChunk word_this" << endl);
                CPPUNIT_ASSERT(!chunk_was_added);

                word_this.retrieve_data();
                char target_this[] = "This";
                size_t letter_index=0;
                for(const auto& chunk: word_this.d_chunks) {
                    DBG(cerr << prolog << "Checking chunk for target char '"<< target_this[letter_index] << "'" << endl);
                    DBG(cerr << prolog << "chunk->get_is_read(): "<< (chunk->get_is_read()?"true":"false") << endl);
                    CPPUNIT_ASSERT(chunk->get_is_read());
                    DBG(cerr << prolog << "chunk->get_bytes_read(): "<< chunk->get_bytes_read() << endl);
                    CPPUNIT_ASSERT(chunk->get_bytes_read() == 100);
                    char *rbuf = chunk->get_rbuf();
                    for (size_t i = 0; i < 100; i++) {
                        // DBG( cerr << prolog << "rbuf["<<i<<"]: '"<< rbuf[i] << "'" << endl);
                        CPPUNIT_ASSERT(rbuf[i] == target_this[letter_index]);
                    }
                    letter_index++;
                }
                SuperChunk word_is(prolog+"word_is");
                chunk_was_added = word_is.add_chunk(i1);
                DBG( cerr << prolog << "Chunk i1 was "<< (chunk_was_added?"":"NOT ") << "added to SuperChunk word_is" << endl);
                CPPUNIT_ASSERT(chunk_was_added);

                chunk_was_added = word_is.add_chunk(s1);
                DBG( cerr << prolog << "Chunk s1 was "<< (chunk_was_added?"":"NOT ") << "added to SuperChunk word_is" << endl);
                CPPUNIT_ASSERT(chunk_was_added);

                // The a0 chunk is not contiguous with s1 and should be rejected by the word_is SuperChunk.
                chunk_was_added = word_is.add_chunk(a0);
                DBG( cerr << prolog << "Chunk a0 was "<< (chunk_was_added?"":"NOT ") << "added to SuperChunk word_is" << endl);
                CPPUNIT_ASSERT(!chunk_was_added);

                word_is.retrieve_data();
                char target_is[] = "is";
                letter_index=0;
                for(const auto& chunk: word_is.d_chunks) {
                    DBG(cerr << prolog << "Checking chunk for target char '"<< target_is[letter_index] << "'" << endl);
                    DBG(cerr << prolog << "chunk->get_is_read(): "<< (chunk->get_is_read()?"true":"false") << endl);
                    CPPUNIT_ASSERT(chunk->get_is_read());
                    DBG(cerr << prolog << "chunk->get_bytes_read(): "<< chunk->get_bytes_read() << endl);
                    CPPUNIT_ASSERT(chunk->get_bytes_read() == 100);
                    char *rbuf = chunk->get_rbuf();
                    for (size_t i = 0; i < 100; i++) {
                        // DBG( cerr << prolog << "rbuf["<<i<<"]: '"<< rbuf[i] << "'" << endl);
                        CPPUNIT_ASSERT(rbuf[i] == target_is[letter_index]);
                    }
                    letter_index++;
                }

                //char target_a[] = "a";
                //char target_test[] = "test";

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

        CPPUNIT_TEST(sc_one_chunk_test);
        CPPUNIT_TEST(sc_chunks_test_01);
        CPPUNIT_TEST(sc_chunks_test_02);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(SuperChunkTest);

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
            test = dmrpp::SuperChunkTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
