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

#include <libdap/debug.h>

#include "BESContextManager.h"
#include "BESError.h"
#include "BESDebug.h"
#include "TheBESKeys.h"

#include "DmrppArray.h"
#include "DmrppByte.h"
#include "DmrppRequestHandler.h"
#include "CurlHandlePool.h"
#include "Chunk.h"
#include "SuperChunk.h"

#include "modules/common/run_tests_cppunit.h"
#include "test_config.h"

using namespace libdap;

#define prolog std::string("SuperChunkTest::").append(__func__).append("() - ")

namespace http {
class mock_url: public url {
public:
    mock_url() = default;

    string str() const override { return "http://test.url.tld/file.ext?aws-token=secret_stuff"; }
    string protocol() const override { return "http"; }
    string host() const override { return "test.url.tld"; }
    string path() const override { return "/file.ext"; }
};
}

namespace dmrpp {

#define THREAD_SLEEP_TIME 1 // sleep time in seconds

class MockChunk : public Chunk {
    // CurlHandlePool *d_chp;
    bool d_sim_err = false; // simulate and err

public:
    MockChunk() = default;
    explicit MockChunk(bool sim_err) : Chunk(), d_sim_err(sim_err)
    {}

    void inflate_chunk(bool, bool, unsigned int, unsigned int)
    {
        return;
    }

    shared_ptr<http::url> get_data_url() const override {
        return make_shared<http::url>("https://httpbin.org/");
    }

    unsigned long long get_bytes_read() const override
    {
        return 2;
    }

    unsigned long long get_size() const override
    {
        return 2;
    }

    std::vector<unsigned long long> mock_pia = {1};

    void set_pia(const std::vector<unsigned long long> &pia)
    {
        mock_pia = pia;
    }

    const std::vector<unsigned long long> &get_position_in_array() const  override
    {
        return mock_pia;
    }

    // This is very close to the real read_chunk with only the call to d_handle->read_data()
    // replaced by code that throws (or not) depending on the value of 'sim_err' in the
    // constructor.
    void read_chunk() override {
        if (d_is_read) {
            return;
        }

        // Does not call read_data() but simulates a delay.
        sleep(THREAD_SLEEP_TIME);

        if (d_sim_err)
            throw BESInternalError(prolog + "Simulated error", __FILE__, __LINE__);

        // If the expected byte count was not read, it's an error.
        if (get_size() != get_bytes_read()) {
            string msg = prolog + "Wrong number of bytes read for chunk; read: " + std::to_string(get_bytes_read())
                         + ", expected: " + std::to_string(get_size());
            throw BESInternalError(msg, __FILE__, __LINE__);
        }

        d_is_read = true;
    }
};

bool mock_process_chunk_data(shared_ptr<Chunk> chunk, bool read) {
    if (read) {
        chunk->read_chunk();
    }
    return true;
}

/// This class provides a child of DmrppArray with an inert 'insert_chunk' method. jhrg 2/9/24
class MockDmrppArray : public DmrppArray {
public:
    MockDmrppArray() : DmrppArray("mock_array", new libdap::Byte("mock_array"))
    {
        append_dim(10, "mock_dim");
    }

    bool is_filters_empty() const override
    { return true; }

    virtual void insert_chunk(unsigned int, vector<unsigned long long> *, vector<unsigned long long> *,
                              shared_ptr<Chunk>, const vector<unsigned long long> &) override
    {
        return;
    }
};

class SuperChunkTest : public CppUnit::TestFixture {

    CPPUNIT_TEST_SUITE( SuperChunkTest );

        CPPUNIT_TEST(test_initialize_chunk_processing_futures_empty_queue);
        CPPUNIT_TEST(test_initialize_chunk_processing_futures_one_chunk);
        CPPUNIT_TEST(test_initialize_chunk_processing_futures_thread_limit);

        CPPUNIT_TEST(test_next_ready_future_empty_list);
        CPPUNIT_TEST(test_next_ready_future_one_future);
        CPPUNIT_TEST(test_next_ready_future_two_futures);
        CPPUNIT_TEST(test_next_ready_future_throws_on_not_valid);
        CPPUNIT_TEST(test_next_ready_future_task_error);

        CPPUNIT_TEST(test_add_next_chunk_processing_future_one_chunk);
        CPPUNIT_TEST(test_add_next_chunk_processing_future_empty_chunks);
        CPPUNIT_TEST(test_add_next_chunk_processing_future_no_room);

        CPPUNIT_TEST(test_process_chunks_concurrent_no_chunks);
        CPPUNIT_TEST(test_process_chunks_concurrent_one_chunks);
        CPPUNIT_TEST(test_process_chunks_concurrent_three_chunks);

        CPPUNIT_TEST(sc_one_chunk_test);
        CPPUNIT_TEST(sc_chunks_test_01);
        CPPUNIT_TEST(sc_chunks_test_02);

    CPPUNIT_TEST_SUITE_END();

public:
    SuperChunkTest()
    {
        DmrppRequestHandler::curl_handle_pool = make_unique<CurlHandlePool>();
        DmrppRequestHandler::curl_handle_pool->initialize();
    }

    // Called at the end of the test
    ~SuperChunkTest() override = default;

    // Called before each test
    void setUp() override
    {
        DBG(cerr << endl);
        TheBESKeys::TheKeys()->set_key("BES.LogName", "/bes.log");
        TheBESKeys::TheKeys()->set_key("BES.LogVerbose", "yes");

        TheBESKeys::TheKeys()->set_key("BES.Catalog.Default", "default");
        TheBESKeys::TheKeys()->set_key("BES.Catalog.default.RootDirectory", TEST_DMRPP_CATALOG);
        TheBESKeys::TheKeys()->set_key("BES.Catalog.default.TypeMatch", "null:*;");

        // Was originally: "^(file|https?):\\/\\/.*$" jhrg 2/9/24
        TheBESKeys::TheKeys()->set_key("AllowedHosts", "^file.*$");

        DmrppRequestHandler::d_max_compute_threads = 2;
    }

    void test_next_ready_future_empty_list() {
        list <future<bool>> futures;
        CPPUNIT_ASSERT_MESSAGE("The futures list should be empty.", futures.empty());
        CPPUNIT_ASSERT_MESSAGE("The return value should be false.", !next_ready_future(futures));
    }

    void test_next_ready_future_one_future() {
        list <future<bool>> futures;
        auto mock_chunk = make_shared<MockChunk>();

        auto future = std::async(std::launch::async, mock_process_chunk_data, mock_chunk, false);
        futures.push_back(std::move(future));

        CPPUNIT_ASSERT_MESSAGE("The futures list should have one element.", futures.size() == 1);
        CPPUNIT_ASSERT_MESSAGE("The return value should be true.", next_ready_future(futures));
        CPPUNIT_ASSERT_MESSAGE("The futures list should be empty.", futures.empty());
    }

    void test_next_ready_future_two_futures() {
        list <future<bool>> futures;
        auto mock_chunk_1 = make_shared<MockChunk>();
        auto mock_chunk_2 = make_shared<MockChunk>();
        vector<unsigned long long> array_shape = {100};

        futures.push_back(std::async(std::launch::async, mock_process_chunk_data, mock_chunk_1, false));
        futures.push_back(std::async(std::launch::async, mock_process_chunk_data, mock_chunk_2, false));

        CPPUNIT_ASSERT_MESSAGE("The futures list should have one element.", futures.size() == 2);
        CPPUNIT_ASSERT_MESSAGE("The return value should be true.", next_ready_future(futures));
        CPPUNIT_ASSERT_MESSAGE("The futures list should have one element.", futures.size() == 1);
    }

    void test_next_ready_future_throws_on_not_valid() {
        list <future<bool>> futures;
        auto mock_chunk = make_shared<MockChunk>();
        vector<unsigned long long> array_shape = {100};

        futures.push_back(std::async(std::launch::async, mock_process_chunk_data, mock_chunk, false));

        // invalidate the future by calling get() on it.
        futures.front().get();

        CPPUNIT_ASSERT_MESSAGE("The futures list should have one element.", futures.size() == 1);
        // TODO Should we use a special exception type for invalid futures?
        CPPUNIT_ASSERT_THROW_MESSAGE("The return value should throw an exception.", next_ready_future(futures), BESInternalError);
        CPPUNIT_ASSERT_MESSAGE("The futures list should be empty.", futures.empty());
    }

    // In this test, the first future will throw an exception.
    void test_next_ready_future_task_error() {
        list <future<bool>> futures;
        auto mock_chunk_1 = make_shared<MockChunk>(true);   // simulate an error that causes an exception
        auto mock_chunk_2 = make_shared<MockChunk>();
        vector<unsigned long long> array_shape = {100};

        futures.push_back(std::async(std::launch::async, mock_process_chunk_data, mock_chunk_1, true));
        futures.push_back(std::async(std::launch::async, mock_process_chunk_data, mock_chunk_2, true));

        // to ensure they run to completion, wait for these futures to complete.
        for (const auto &future: futures) {
            future.wait();
        }

        CPPUNIT_ASSERT_MESSAGE("The futures list should have two elements.", futures.size() == 2);
        CPPUNIT_ASSERT_THROW_MESSAGE("The return value should throw an exception.", next_ready_future(futures), BESInternalError);
        CPPUNIT_ASSERT_MESSAGE("The futures list should have one element.", futures.size() == 1);
    }

    void test_add_next_chunk_processing_future_one_chunk() {
        list <future<bool>> futures;
        queue<shared_ptr<Chunk>> chunks;

        chunks.push(make_shared<MockChunk>());

        CPPUNIT_ASSERT_MESSAGE("The chunks queue should have one entry.", chunks.size() == 1);
        CPPUNIT_ASSERT_MESSAGE("The futures list should be empty.", futures.empty());

        vector<unsigned long long> constrained_array_shape = {100};
        bool state = add_next_chunk_processing_future(futures, chunks, nullptr, constrained_array_shape);
        CPPUNIT_ASSERT_MESSAGE("add_next_chunk_processing_future() return value should be true.", state);
        CPPUNIT_ASSERT_MESSAGE("The futures list should have one element.", futures.size() == 1);
        CPPUNIT_ASSERT_MESSAGE("The chunks queue should be empty.", chunks.empty() );
    }

    void test_add_next_chunk_processing_future_empty_chunks()
    {
        list <future<bool>> futures;
        queue <shared_ptr<Chunk>> chunks;

        CPPUNIT_ASSERT_MESSAGE("The chunks queue should be empty.", chunks.empty());
        CPPUNIT_ASSERT_MESSAGE("The futures list should be empty.", futures.empty());

        vector<unsigned long long> constrained_array_shape = {100};
        bool state = add_next_chunk_processing_future(futures, chunks, nullptr, constrained_array_shape);
        CPPUNIT_ASSERT_MESSAGE("add_next_chunk_processing_future() return value should be false.", state == false);
    }
    void test_add_next_chunk_processing_future_no_room()
    {
        list <future<bool>> futures;
        queue <shared_ptr<Chunk>> chunks;
        futures.push_back(std::async(std::launch::async, mock_process_chunk_data, make_shared<MockChunk>(), false));
        futures.push_back(std::async(std::launch::async, mock_process_chunk_data, make_shared<MockChunk>(), false));

        chunks.push(make_shared<MockChunk>());

        CPPUNIT_ASSERT_MESSAGE("The chunks queue should be one.", chunks.size() == 1);
        CPPUNIT_ASSERT_MESSAGE("The futures list should be empty.", futures.size() == 2);
        CPPUNIT_ASSERT_MESSAGE("max_compute_threads should be two.", DmrppRequestHandler::d_max_compute_threads == 2);;

        vector<unsigned long long> constrained_array_shape = {100};
        bool state = add_next_chunk_processing_future(futures, chunks, nullptr, constrained_array_shape);
        CPPUNIT_ASSERT_MESSAGE("add_next_chunk_processing_future() return value should be false.", state == false);
        CPPUNIT_ASSERT_MESSAGE("The chunks queue should be one.", chunks.size() == 1);
        CPPUNIT_ASSERT_MESSAGE("The futures list should be empty.", futures.size() == 2);
    }

    void test_initialize_chunk_processing_futures_empty_queue() {
        list <future<bool>> futures;
        queue<shared_ptr<Chunk>> chunks;
        vector<unsigned long long> constrained_array_shape = {100};
        CPPUNIT_ASSERT_MESSAGE("The chunks queue should be empty.", chunks.empty());
        initialize_chunk_processing_futures(futures, chunks, nullptr /*DmrppArray* */, constrained_array_shape);
        CPPUNIT_ASSERT_MESSAGE("The futures list should be empty given teh chunks queue is empty.", futures.empty());
    }

    void test_initialize_chunk_processing_futures_one_chunk() {
        list <future<bool>> futures;
        queue<shared_ptr<Chunk>> chunks;
        auto data_url = make_shared<http::mock_url>();
        std::vector<unsigned long long> chunk_position_in_array = {0};
        chunks.push(make_shared<Chunk>(data_url, "", 1000, 0, chunk_position_in_array));

        CPPUNIT_ASSERT_MESSAGE("The chunks queue should have one entry.", chunks.size() == 1);
        vector<unsigned long long> constrained_array_shape = {100};
        initialize_chunk_processing_futures(futures, chunks, nullptr /*DmrppArray* */, constrained_array_shape);
        CPPUNIT_ASSERT_MESSAGE("The futures list should have one element.", futures.size() == 1);
        CPPUNIT_ASSERT_MESSAGE("The chunks queue should be empty.", chunks.empty());
    }

    void test_initialize_chunk_processing_futures_thread_limit() {
        list <future<bool>> futures;
        queue<shared_ptr<Chunk>> chunks;
        auto data_url = make_shared<http::mock_url>();
        chunks.push(make_shared<Chunk>(data_url, "", 1000, 0, "[0]"));
        chunks.push(make_shared<Chunk>(data_url, "", 1000, 1000, "[1]"));
        chunks.push(make_shared<Chunk>(data_url, "", 1000, 2000, "[2]"));

        CPPUNIT_ASSERT_MESSAGE("The chunks queue should have three entries.", chunks.size() == 3);
        vector<unsigned long long> constrained_array_shape = {100};
        initialize_chunk_processing_futures(futures, chunks, nullptr /*DmrppArray* */, constrained_array_shape);
        CPPUNIT_ASSERT_MESSAGE("The futures list should have one element.", futures.size() == 2);
        CPPUNIT_ASSERT_MESSAGE("The chunks queue should have one entry.", chunks.size() == 1);
    }

    void test_process_chunks_concurrent_no_chunks() {
        auto data_url = make_shared<http::mock_url>();
        std::vector<unsigned long long> chunk_position_in_array = {0};
        queue<shared_ptr<Chunk>> chunks;

        CPPUNIT_ASSERT_MESSAGE("The chunks queue should have three entries.", chunks.empty());
        vector<unsigned long long> constrained_array_shape = {100};
        process_chunks_concurrent("test_process_chunks_concurrent", chunks, nullptr /*DmrppArray* */, constrained_array_shape);
        CPPUNIT_ASSERT_MESSAGE("The chunks queue should be empty.", chunks.empty());
    }

    void test_process_chunks_concurrent_one_chunks() {
        auto data_url = make_shared<http::mock_url>();
        std::vector<unsigned long long> chunk_position_in_array = {0};
        queue<shared_ptr<Chunk>> chunks;

        CPPUNIT_ASSERT_MESSAGE("The chunks queue should have three entries.", chunks.empty());
        vector<unsigned long long> constrained_array_shape = {100};
        process_chunks_concurrent("test_process_chunks_concurrent", chunks, nullptr /*DmrppArray* */, constrained_array_shape);
        CPPUNIT_ASSERT_MESSAGE("The chunks queue should be empty.", chunks.empty());
    }

    void test_process_chunks_concurrent_three_chunks() {
        auto data_url = make_shared<http::mock_url>();
        std::vector<unsigned long long> chunk_position_in_array = {0};
        queue<shared_ptr<Chunk>> chunks;
        chunks.push(make_shared<Chunk>(data_url, "", 1000, 0, chunk_position_in_array));
        chunk_position_in_array = {1};
        chunks.push(make_shared<Chunk>(data_url, "", 1000, 1000, chunk_position_in_array));
        chunk_position_in_array = {2};
        chunks.push(make_shared<Chunk>(data_url, "", 1000, 2000, chunk_position_in_array));

        CPPUNIT_ASSERT_MESSAGE("The chunks queue should have three entries.", chunks.size() == 3);
        vector<unsigned long long> constrained_array_shape = {100};
        process_chunks_concurrent("test_process_chunks_concurrent", chunks, nullptr /*DmrppArray* */, constrained_array_shape);
        CPPUNIT_ASSERT_MESSAGE("The chunks queue should be empty.", chunks.empty());
    }

    /// Original tests follow. jhrg 2/11/24

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
};

CPPUNIT_TEST_SUITE_REGISTRATION(SuperChunkTest);

} // namespace dmrpp

int main(int argc, char*argv[])
{
    return bes_run_tests<dmrpp::SuperChunkTest>(argc, argv, "bes,http,curl,dmrpp,dmrpp:3") ? 0 : 1;
}
