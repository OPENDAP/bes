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

#include <unistd.h>
#include <time.h>

#include <cstring>
#include <cassert>
#include <cerrno>
#include <list>
#include <memory>

#include <queue>
#include <iterator>
#include <thread>
#include <future>         // std::async, std::future
#include <chrono>         // std::chrono::milliseconds

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <Byte.h>

#include "BESInternalError.h"
#include "TheBESKeys.h"
#include "BESDebug.h"
#include "BESLog.h"

#include "Chunk.h"
#include "DmrppNames.h"
#include "DmrppArray.h"
#include "CurlHandlePool.h"

#include "test_config.h"

#define THREAD_SLEEP_TIME 1 // sleep time in seconds

using namespace std;

// FIXME Reset these to false before merging this code to master. jhrg 9/25/20
static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)
#define prolog std::string("CurlHandlePoolTest::").append(__func__).append("() - ")

namespace dmrpp {

class MockChunk : public Chunk {
    CurlHandlePool *d_chp;
    bool d_sim_err; // simulate and err

public:
    MockChunk(CurlHandlePool *chp, bool sim_err) : Chunk(), d_chp(chp), d_sim_err(sim_err)
    {}

    void inflate_chunk(bool, bool, unsigned int, unsigned int)
    {
        return;
    }

    virtual shared_ptr<http::url> get_data_url() const override {
        return shared_ptr<http::url>(new  http::url("https://httpbin.org/"));
    }

    virtual unsigned long long get_bytes_read() const  override
    {
        return 2;
    }

    virtual unsigned long long get_size() const  override
    {
        return 2;
    }

    std::vector<unsigned long long> mock_pia = {1};

    virtual const std::vector<unsigned long long> &get_position_in_array() const  override
    {
        return mock_pia;
    }

    // This is very close to the real read_chunk with only the call to d_handle->read_data()
    // replaced by code that throws (or not) depending on the value of 'sim_err' in the
    // constructor.
    void read_chunk() override
    {
        if (d_is_read) {
            return;
        }

        set_rbuf_to_size();

        dmrpp_easy_handle *handle = d_chp->get_easy_handle(this);
        if (!handle)
            throw BESInternalError(prolog + "No more libcurl handles.", __FILE__, __LINE__);

        try {
            // handle->read_data();  // throws if error
            time_t t;
            srandom(time(&t));
            sleep(THREAD_SLEEP_TIME);

            if (d_sim_err)
                throw BESInternalError(prolog + "Simulated error", __FILE__, __LINE__);

            d_chp->release_handle(handle);
        }
        catch (...) {
            d_chp->release_handle(handle);
            throw;
        }

        // If the expected byte count was not read, it's an error.
        if (get_size() != get_bytes_read()) {
            ostringstream oss;
            oss << prolog <<  "Wrong number of bytes read for chunk; read: " << get_bytes_read() << ", expected: " << get_size();
            throw BESInternalError(oss.str(), __FILE__, __LINE__);
        }

        d_is_read = true;
    }

};

class MockDmrppArray : public DmrppArray {
public:
    MockDmrppArray() : DmrppArray("mock_array", new libdap::Byte("mock_array"))
    {
        append_dim(10, "mock_dim");
    }

    bool is_deflate_compression() const override
    { return false; }

    bool is_shuffle_compression() const override
    { return false; }

    virtual void insert_chunk(unsigned int, vector<unsigned long long> *, vector<unsigned long long> *,
                              shared_ptr<Chunk>, const vector<unsigned long long> &) override
    {
        return;
    }

};

class CurlHandlePoolTest : public CppUnit::TestFixture {
private:
    CurlHandlePool *chp;

public:
    // Called once before everything gets tested
    CurlHandlePoolTest(): chp(0){ }

    // Called at the end of the test
    ~CurlHandlePoolTest(){}

    // Called before each test
    void setUp() override
    {
        DBG(cerr << endl);
        chp = new CurlHandlePool(4);
        TheBESKeys::ConfigFile = string(TEST_SRC_DIR) + "/curl_handle_pool_keys.conf";
        // The following will show threads joined after an exception was thrown by a thread
        if (bes_debug) BESDebug::SetUp("cerr,dmrpp:3");
    }

    // Called after each test
    void tearDown() override
    {
        delete chp;
        chp = 0;
    }

    void process_one_chunk_test()
    {
        DBG(cerr << prolog << "BEGIN" << endl);

        CPPUNIT_ASSERT(true);

        shared_ptr<Chunk> chunk(new MockChunk(chp, true));
        auto array = new MockDmrppArray;
        vector<unsigned long long> array_shape = {1};

        unsigned int num = chp->get_handles_available();
        CPPUNIT_ASSERT(num == chp->get_max_handles());

        try {
            process_one_chunk(chunk, array, array_shape);
        }
        catch (BESError &e) {
            DBG(cerr << prolog << "BES Exception: " << e.get_verbose_message() << endl);
            CPPUNIT_ASSERT("BES Exception caught");
        }
        catch (std::exception &e) {
            DBG(cerr << prolog << "Exception: " << e.what() << endl);
            CPPUNIT_FAIL("Exception");
        }

        unsigned int num2 = chp->get_handles_available();
        CPPUNIT_ASSERT(num2 == num);
        DBG(cerr << prolog << "END" << endl);
    }

    // This is a general proxy for the DmrppArray code that controls the parallel transfers.
    void dmrpp_array_thread_control(queue<shared_ptr<Chunk>> &chunks_to_read, MockDmrppArray *array,
                                    const vector<unsigned long long> &array_shape) {
        DBG(cerr << prolog << "BEGIN" << endl);
        process_chunks_concurrent("AnImaginarySuperChunk", chunks_to_read, array, array_shape);
        DBG(cerr << prolog << "END" << endl);
    }

#if 0
    // This is a general proxy for the DmrppArray code that controls the parallel transfers.
    void dmrpp_array_pthread_control(queue<shared_ptr<Chunk>> &chunks_to_read, MockDmrppArray *array,
                                    const vector<unsigned int> &array_shape) {
        DBG(cerr << prolog << "BEGIN" << endl);
        // This pipe is used by the child threads to indicate completion
        int fds[2];
        if (pipe(fds) < 0)
            throw BESInternalError(string("Could not open a pipe for thread communication: ").append(strerror(errno)),
                                   __FILE__, __LINE__);

        // Start the max number of processing pipelines
        //pthread_t threads[chp->get_max_handles()];
        vector<pthread_t> threads(chp->get_max_handles());
        memset(&threads[0], 0, sizeof(pthread_t) * chp->get_max_handles());

        try {
            unsigned int num_threads = 0;
            for (unsigned int i = 0; i < (unsigned int) chp->get_max_handles() && !chunks_to_read.empty(); ++i) {
                auto chunk = chunks_to_read.front();
                chunks_to_read.pop();

                // thread number is 'i'
                auto args = new one_chunk_args(fds, i, chunk, array, array_shape);
                int status = pthread_create(&threads[i], NULL, dmrpp::one_chunk_thread, (void *) args);
                if (0 == status) {
                    ++num_threads;
                    DBG(cerr << prolog << "started thread: " << i << endl);
                }
                else {
                    ostringstream oss("Could not start thread for chunk ", ios::ate);
                    oss << i << ": " << strerror(status);
                    DBG(cerr << prolog << oss.str());
                    throw BESInternalError(oss.str(), __FILE__, __LINE__);
                }
            }

            DBG(cerr << prolog << "Gathering threads..." << endl);
            // Now join the child threads, creating replacement threads if needed
            while (num_threads > 0) {
                DBG(cerr << prolog << num_threads << " threads to process..." << endl);
                unsigned char tid;   // bytes can be written atomically
                // Block here until a child thread writes to the pipe, then read the byte
                int bytes = read(fds[0], &tid, sizeof(tid));
                if (bytes != sizeof(tid))
                    throw BESInternalError(string("Could not read the thread id: ").append(strerror(errno)), __FILE__,
                                           __LINE__);

                if (tid >= chp->get_max_handles()) {
                    ostringstream oss("Invalid thread id read after thread exit: ", ios::ate);
                    oss << tid;
                    throw BESInternalError(oss.str(), __FILE__, __LINE__);
                }

                string *error;
                int status = pthread_join(threads[tid], (void **) &error);
                --num_threads;
                DBG(cerr << prolog << "joined thread: " << (unsigned int) tid << ", there are: " << num_threads << endl);

                if (status != 0) {
                    ostringstream oss("Could not join thread for chunk ", ios::ate);
                    oss << tid << ": " << strerror(status);
                    throw BESInternalError(oss.str(), __FILE__, __LINE__);
                }
                else if (error != 0) {
                    DBG(cerr << prolog << "Thread exception: " << (unsigned int) tid << endl);
                    BESInternalError e(*error, __FILE__, __LINE__);
                    delete error;
                    throw e;
                }
                else if (!chunks_to_read.empty()) {
                    auto chunk = chunks_to_read.front();
                    chunks_to_read.pop();

                    // thread number is 'tid,' the number of the thread that just completed
                    auto args = new one_chunk_args(fds, tid, chunk, array, array_shape);
                    status = pthread_create(&threads[tid], NULL, dmrpp::one_chunk_thread, (void *) args);
                    if (status != 0) {
                        ostringstream oss("Could not start thread for chunk ", ios::ate);
                        oss << tid << ": " << strerror(status);
                        throw BESInternalError(oss.str(), __FILE__, __LINE__);
                    }
                    ++num_threads;
                    DBG(cerr << prolog << "started thread: " << (unsigned int) tid << ", there are: " << num_threads << endl);
                }
            }

            // Once done with the threads, close the communication pipe.
            close(fds[0]);
            close(fds[1]);
        }
        catch (...) {
            // cancel all the threads, otherwise we'll have threads out there using up resources
            // defined in DmrppCommon.cc
            join_threads(&threads[0], chp->get_max_handles());
            // close the pipe used to communicate with the child threads
            close(fds[0]);
            close(fds[1]);
            // re-throw the exception
            throw;
        }
        DBG(cerr << prolog << "END" << endl);
    }
#endif

    // This replicates the code in DmrppArray::read_chunks() to orgainize and process_one_chunk()
    // using several threads.
    void process_one_chunk_threaded_test_0()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
#if 0
        queue<Chunk *> chunks_to_read;
        chunks_to_read.push(new MockChunk(chp, false));
        chunks_to_read.push(new MockChunk(chp, false));
        chunks_to_read.push(new MockChunk(chp, false));
        chunks_to_read.push(new MockChunk(chp, false));
#endif
        queue<shared_ptr<Chunk>> chunks_to_read;
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));

        //MockDmrppArray *array = new MockDmrppArray;
        auto array = new MockDmrppArray;
        vector<unsigned long long> array_shape = {1};

        vector<size_t> chunk_dim_sizes = {1};
        array->set_chunk_dimension_sizes(chunk_dim_sizes);

        try {
            dmrpp_array_thread_control(chunks_to_read, array, array_shape);
            CPPUNIT_ASSERT(chp->get_handles_available() == chp->get_max_handles());
        }
        catch(BESError &e) {
            CPPUNIT_FAIL(string("Caught BESError: ").append(e.get_verbose_message()));
        }
        catch(std::exception &e) {
            CPPUNIT_FAIL(string("Caught std::exception: ").append(e.what()));
        }
        DBG(cerr << prolog << "END" << endl);
    }

    // One of the threads throw an exception
    void process_one_chunk_threaded_test_1()
    {
#if 0
        queue<Chunk *> chunks_to_read;
        chunks_to_read.push(new MockChunk(chp, false));
        chunks_to_read.push(new MockChunk(chp, false));
        chunks_to_read.push(new MockChunk(chp, true));
        chunks_to_read.push(new MockChunk(chp, false));
#endif
        DBG(cerr << prolog << "BEGIN" << endl);
        queue<shared_ptr<Chunk>> chunks_to_read;
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, true)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));

        auto array = new MockDmrppArray;
        vector<unsigned long long> array_shape = {1};

        try {
            dmrpp_array_thread_control(chunks_to_read, array, array_shape);
            CPPUNIT_FAIL("dmrpp_array_thread_control() should have thrown an exception");
        }
        catch(BESInternalError &e) {
            DBG(cerr << prolog << "BESInternalError: " << e.get_verbose_message() << endl);
        }

        CPPUNIT_ASSERT(chp->get_handles_available() == chp->get_max_handles());
        DBG(cerr << prolog << "END" << endl);
    }

    // One thread not in the initial batch of threads throws.
    void process_one_chunk_threaded_test_2()
    {
#if 0
        queue<Chunk *> chunks_to_read;
        chunks_to_read.push(new MockChunk(chp, false));
        chunks_to_read.push(new MockChunk(chp, false));
        chunks_to_read.push(new MockChunk(chp, false));
        chunks_to_read.push(new MockChunk(chp, false));
        chunks_to_read.push(new MockChunk(chp, false));
        chunks_to_read.push(new MockChunk(chp, true));
        chunks_to_read.push(new MockChunk(chp, false));
#endif
        DBG(cerr << prolog << "BEGIN" << endl);
        queue<shared_ptr<Chunk>> chunks_to_read;
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, true)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));

        auto array = new MockDmrppArray;
        vector<unsigned long long> array_shape = {1};

        try {
            dmrpp_array_thread_control(chunks_to_read, array, array_shape);
            CPPUNIT_FAIL("dmrpp_array_thread_control() should have thrown an exception");
        }
        catch(BESInternalError &e) {
            DBG(cerr << prolog << "BESInternalError: " << e.get_verbose_message() << endl);
        }

        CPPUNIT_ASSERT(chp->get_handles_available() == chp->get_max_handles());
        DBG(cerr << prolog << "END" << endl);
    }

    // Two threads in the initial set throw
    void process_one_chunk_threaded_test_3()
    {
#if 0
        queue<Chunk *> chunks_to_read;
        chunks_to_read.push(new MockChunk(chp, false));
        chunks_to_read.push(new MockChunk(chp, true));
        chunks_to_read.push(new MockChunk(chp, true));
        chunks_to_read.push(new MockChunk(chp, false));
        chunks_to_read.push(new MockChunk(chp, false));
        chunks_to_read.push(new MockChunk(chp, false));
        chunks_to_read.push(new MockChunk(chp, false));
#endif

        DBG(cerr << prolog << "BEGIN" << endl);
        queue<shared_ptr<Chunk>> chunks_to_read;
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, true)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, true)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));

        auto array = new MockDmrppArray;
        vector<unsigned long long> array_shape = {1};

        try {
            dmrpp_array_thread_control(chunks_to_read, array, array_shape);
            CPPUNIT_FAIL("dmrpp_array_thread_control() should have thrown an exception");
        }
        catch(BESInternalError &e) {
            DBG(cerr << prolog << "BESInternalError: " << e.get_verbose_message() << endl);
        }

        CPPUNIT_ASSERT(chp->get_handles_available() == chp->get_max_handles());
        DBG(cerr << prolog << "END" << endl);
    }

    // two in the second set throw
    void process_one_chunk_threaded_test_4()
    {
#if 0
        queue<Chunk *> chunks_to_read;
        chunks_to_read.push(new MockChunk(chp, false));
        chunks_to_read.push(new MockChunk(chp, false));
        chunks_to_read.push(new MockChunk(chp, false));
        chunks_to_read.push(new MockChunk(chp, false));
        chunks_to_read.push(new MockChunk(chp, true));
        chunks_to_read.push(new MockChunk(chp, true));
        chunks_to_read.push(new MockChunk(chp, false));
#endif
        DBG(cerr << prolog << "BEGIN" << endl);
        queue<shared_ptr<Chunk>> chunks_to_read;
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, true)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, true)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));

        auto array = new MockDmrppArray;
        vector<unsigned long long> array_shape = {1};

        try {
            dmrpp_array_thread_control(chunks_to_read, array, array_shape);
            CPPUNIT_FAIL("dmrpp_array_thread_control() should have thrown an exception");
        }
        catch(BESInternalError &e) {
            DBG(cerr << prolog << "BESInternalError: " << e.get_verbose_message() << endl);
        }

        CPPUNIT_ASSERT(chp->get_handles_available() == chp->get_max_handles());
        DBG(cerr << prolog << "END" << endl);
    }

    // One in the first set and one in the second set throw
    void process_one_chunk_threaded_test_5()
    {
#if 0
        queue<Chunk *> chunks_to_read;
        chunks_to_read.push(new MockChunk(chp, false));
        chunks_to_read.push(new MockChunk(chp, false));
        chunks_to_read.push(new MockChunk(chp, false));
        chunks_to_read.push(new MockChunk(chp, true));
        chunks_to_read.push(new MockChunk(chp, true));
        chunks_to_read.push(new MockChunk(chp, false));
        chunks_to_read.push(new MockChunk(chp, false));
#endif
        DBG(cerr << prolog << "BEGIN" << endl);
        queue<shared_ptr<Chunk>> chunks_to_read;
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, true)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, true)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));

        auto array = new MockDmrppArray;
        vector<unsigned long long> array_shape = {1};

        try {
            dmrpp_array_thread_control(chunks_to_read, array, array_shape);
            CPPUNIT_FAIL("dmrpp_array_thread_control() should have thrown an exception");
        }
        catch(BESInternalError &e) {
            DBG(cerr << prolog << "BESInternalError: " << e.get_verbose_message() << endl);
        }

        CPPUNIT_ASSERT(chp->get_handles_available() == chp->get_max_handles());
        DBG(cerr << prolog << "END" << endl);
    }

    // Make sure the curl handle pool doesn't leak with multiple failures
    void process_one_chunk_threaded_test_6()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        queue<shared_ptr<Chunk>> chunks_to_read;
        for (int i = 0; i < 5; ++i) {
#if 0
            chunks_to_read.push(new MockChunk(chp, false));
            chunks_to_read.push(new MockChunk(chp, false));
            chunks_to_read.push(new MockChunk(chp, false));
            chunks_to_read.push(new MockChunk(chp, true));
            chunks_to_read.push(new MockChunk(chp, true));
            chunks_to_read.push(new MockChunk(chp, false));
            chunks_to_read.push(new MockChunk(chp, false));
#endif
            chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
            chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
            chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
            chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, true)));
            chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, true)));
            chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));
            chunks_to_read.push(shared_ptr<Chunk>(new MockChunk(chp, false)));

            auto array = new MockDmrppArray;
            vector<unsigned long long> array_shape = {1};

            try {
                dmrpp_array_thread_control(chunks_to_read, array, array_shape);
                CPPUNIT_FAIL("dmrpp_array_thread_control() should have thrown an exception");
            }
            catch (BESInternalError &e) {
                DBG(cerr << prolog << "BESInternalError: " << e.get_verbose_message() << endl);
            }
        }

        CPPUNIT_ASSERT(chp->get_handles_available() == chp->get_max_handles());
        DBG(cerr << prolog << "END" << endl);
    }

    CPPUNIT_TEST_SUITE(CurlHandlePoolTest);

    CPPUNIT_TEST(process_one_chunk_test);
    CPPUNIT_TEST(process_one_chunk_threaded_test_0);
    CPPUNIT_TEST(process_one_chunk_threaded_test_1);
    CPPUNIT_TEST(process_one_chunk_threaded_test_2);
    CPPUNIT_TEST(process_one_chunk_threaded_test_3);
    CPPUNIT_TEST(process_one_chunk_threaded_test_4);
    CPPUNIT_TEST(process_one_chunk_threaded_test_5);
    CPPUNIT_TEST(process_one_chunk_threaded_test_6);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(CurlHandlePoolTest);

} // namespace dmrpp

int main(int argc, char *argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());


    int option_char;
    while ((option_char = getopt(argc, argv, "db")) != -1)
        switch (option_char) {
            case 'd':
                debug = true;  // debug is a static global
                break;
            case 'b':
                debug = true;  // debug is a static global
                bes_debug = true;  // debug is a static global
                break;
            default:
                break;
        }

    argc -= optind;
    argv += optind;

    bool wasSuccessful = true;
    if (0 == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        int i = 0;
        while (i < argc) {
            if (debug) cerr << prolog << "Running " << argv[i] << endl;
            string test = dmrpp::CurlHandlePoolTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
