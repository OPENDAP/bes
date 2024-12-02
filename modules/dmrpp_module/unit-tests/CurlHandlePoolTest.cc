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

#include <ctime>
//#include <cstring>
#include <cassert>
//#include <cerrno>
//#include <list>
#include <memory>

#include <queue>
#include <iterator>
#include <thread>
#include <future>         // std::async, std::future
#include <chrono>         // std::chrono::milliseconds

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <libdap/Byte.h>

#include "BESInternalError.h"
#include "TheBESKeys.h"
#include "BESDebug.h"
#include "BESLog.h"

#include "Chunk.h"
#include "DmrppNames.h"
#include "DmrppArray.h"
#include "CurlHandlePool.h"

#include "test_config.h"
#include "run_tests_cppunit.h"

#define THREAD_SLEEP_TIME 1 // sleep time in seconds

using namespace std;

#define prolog std::string("CurlHandlePoolTest::").append(__func__).append("() - ")

namespace dmrpp {

class MockChunk : public Chunk {
    CurlHandlePool *d_chp;
    bool d_sim_err; // simulate and err

public:
    MockChunk(CurlHandlePool *chp, bool sim_err) : Chunk(), d_chp(chp), d_sim_err(sim_err)
    {}

    void inflate_chunk(bool, bool, unsigned int, unsigned int) const
    {
        // Do nothing
    }

    shared_ptr<http::url> get_data_url() const override {
        return std::make_shared<http::url>("https://httpbin.org/");
    }

    unsigned long long get_bytes_read() const  override
    {
        return 2;
    }

    unsigned long long get_size() const  override
    {
        return 2;
    }

    std::vector<unsigned long long> mock_pia = {1};

    const std::vector<unsigned long long> &get_position_in_array() const  override
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
            time_t t;
            srandom((int)time(&t));
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

    bool is_filters_empty() const override
    { return true; }

    void insert_chunk(unsigned int, vector<unsigned long long> *, vector<unsigned long long> *,
                              shared_ptr<Chunk>, const vector<unsigned long long> &) override
    {
        // Do nothing
    }

};

class CurlHandlePoolTest : public CppUnit::TestFixture {
private:
    CurlHandlePool *chp = nullptr;

public:
    // Called once before everything gets tested
    CurlHandlePoolTest() = default;
    ~CurlHandlePoolTest() override = default;
    CurlHandlePoolTest(const CurlHandlePoolTest &other) = delete;
    CurlHandlePoolTest &operator=(const CurlHandlePoolTest &other) = delete;

    // Called before each test
    void setUp() override
    {
        DBG(cerr << endl);
        TheBESKeys::ConfigFile = string(TEST_SRC_DIR) + "/curl_handle_pool_keys.conf";
        // The following will show threads joined after an exception was thrown by a thread
        chp = new CurlHandlePool();
        chp->initialize();
    }

    // Called after each test
    void tearDown() override
    {
        delete chp;
        chp = nullptr;
    }

    void process_one_chunk_test()
    {
        DBG(cerr << prolog << "BEGIN" << endl);

        CPPUNIT_ASSERT(true);

        shared_ptr<Chunk> chunk(std::make_shared<MockChunk>(chp, true));
        auto array = std::make_unique<MockDmrppArray>();
        vector<unsigned long long> array_shape = {1};

        try {
            process_one_chunk(chunk, array.get(), array_shape);
        }
        catch (BESError &e) {
            DBG(cerr << prolog << "BES Exception: " << e.get_verbose_message() << endl);
            CPPUNIT_ASSERT("BES Exception caught");
        }
        catch (std::exception &e) {
            DBG(cerr << prolog << "Exception: " << e.what() << endl);
            CPPUNIT_FAIL("Exception");
        }

        DBG(cerr << prolog << "END" << endl);
    }

    // This is a general proxy for the DmrppArray code that controls the parallel transfers.
    void dmrpp_array_thread_control(queue<shared_ptr<Chunk>> &chunks_to_read, MockDmrppArray *array,
                                    const vector<unsigned long long> &array_shape) {
        DBG(cerr << prolog << "BEGIN" << endl);
        process_chunks_concurrent("AnImaginarySuperChunk", chunks_to_read, array, array_shape);
        DBG(cerr << prolog << "END" << endl);
    }

    // This replicates the code in DmrppArray::read_chunks() to orgainize and process_one_chunk()
    // using several threads.
    void process_one_chunk_threaded_test_0()
    {
        DBG(cerr << prolog << "BEGIN" << endl);

        queue<shared_ptr<Chunk>> chunks_to_read;
        chunks_to_read.push(shared_ptr<Chunk>(std::make_shared<MockChunk>(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(std::make_shared<MockChunk>(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(std::make_shared<MockChunk>(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(std::make_shared<MockChunk>(chp, false)));

        auto array = std::make_unique<MockDmrppArray>();
        vector<unsigned long long> array_shape = {1};

        vector<unsigned long long> chunk_dim_sizes = {1};
        array->set_chunk_dimension_sizes(chunk_dim_sizes);

        try {
            dmrpp_array_thread_control(chunks_to_read, array.get(), array_shape);
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
        DBG(cerr << prolog << "BEGIN" << endl);
        queue<shared_ptr<Chunk>> chunks_to_read;
        chunks_to_read.push(shared_ptr<Chunk>(std::make_shared<MockChunk>(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(std::make_shared<MockChunk>(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(std::make_shared<MockChunk>(chp, true)));
        chunks_to_read.push(shared_ptr<Chunk>(std::make_shared<MockChunk>(chp, false)));

        auto array = std::make_unique<MockDmrppArray>();
        vector<unsigned long long> array_shape = {1};

        try {
            dmrpp_array_thread_control(chunks_to_read, array.get(), array_shape);
            CPPUNIT_FAIL("dmrpp_array_thread_control() should have thrown an exception");
        }
        catch(BESInternalError &e) {
            DBG(cerr << prolog << "BESInternalError: " << e.get_verbose_message() << endl);
        }
        DBG(cerr << prolog << "END" << endl);
    }

    // One thread not in the initial batch of threads throws.
    void process_one_chunk_threaded_test_2()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        queue<shared_ptr<Chunk>> chunks_to_read;
        chunks_to_read.push(shared_ptr<Chunk>(std::make_shared<MockChunk>(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(std::make_shared<MockChunk>(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(std::make_shared<MockChunk>(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(std::make_shared<MockChunk>(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(std::make_shared<MockChunk>(chp, true)));
        chunks_to_read.push(shared_ptr<Chunk>(std::make_shared<MockChunk>(chp, false)));

        auto array = std::make_unique<MockDmrppArray>();
        vector<unsigned long long> array_shape = {1};

        try {
            dmrpp_array_thread_control(chunks_to_read, array.get(), array_shape);
            CPPUNIT_FAIL("dmrpp_array_thread_control() should have thrown an exception");
        }
        catch(BESInternalError &e) {
            DBG(cerr << prolog << "BESInternalError: " << e.get_verbose_message() << endl);
        }
        DBG(cerr << prolog << "END" << endl);
    }

    // Two threads in the initial set throw
    void process_one_chunk_threaded_test_3()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        queue<shared_ptr<Chunk>> chunks_to_read;
        chunks_to_read.push(shared_ptr<Chunk>(std::make_shared<MockChunk>(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(std::make_shared<MockChunk>(chp, true)));
        chunks_to_read.push(shared_ptr<Chunk>(std::make_shared<MockChunk>(chp, true)));
        chunks_to_read.push(shared_ptr<Chunk>(std::make_shared<MockChunk>(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(std::make_shared<MockChunk>(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(std::make_shared<MockChunk>(chp, false)));
        chunks_to_read.push(shared_ptr<Chunk>(std::make_shared<MockChunk>(chp, false)));

        auto array = std::make_unique<MockDmrppArray>();
        vector<unsigned long long> array_shape = {1};

        try {
            dmrpp_array_thread_control(chunks_to_read, array.get(), array_shape);
            CPPUNIT_FAIL("dmrpp_array_thread_control() should have thrown an exception");
        }
        catch(BESInternalError &e) {
            DBG(cerr << prolog << "BESInternalError: " << e.get_verbose_message() << endl);
        }

        DBG(cerr << prolog << "END" << endl);
    }

    // two in the second set throw
    void process_one_chunk_threaded_test_4()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        queue<shared_ptr<Chunk>> chunks_to_read;
        chunks_to_read.push(std::make_shared<MockChunk>(chp, false));
        chunks_to_read.push(std::make_shared<MockChunk>(chp, false));
        chunks_to_read.push(std::make_shared<MockChunk>(chp, false));
        chunks_to_read.push(std::make_shared<MockChunk>(chp, false));
        chunks_to_read.push(std::make_shared<MockChunk>(chp, true));
        chunks_to_read.push(std::make_shared<MockChunk>(chp, true));
        chunks_to_read.push(std::make_shared<MockChunk>(chp, false));

        auto array = std::make_unique<MockDmrppArray>();
        vector<unsigned long long> array_shape = {1};

        try {
            dmrpp_array_thread_control(chunks_to_read, array.get(), array_shape);
            CPPUNIT_FAIL("dmrpp_array_thread_control() should have thrown an exception");
        }
        catch(BESInternalError &e) {
            DBG(cerr << prolog << "BESInternalError: " << e.get_verbose_message() << endl);
        }

        DBG(cerr << prolog << "END" << endl);
    }

    // One in the first set and one in the second set throw
    void process_one_chunk_threaded_test_5()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        queue<shared_ptr<Chunk>> chunks_to_read;
        chunks_to_read.push(std::make_shared<MockChunk>(chp, false));
        chunks_to_read.push(std::make_shared<MockChunk>(chp, false));
        chunks_to_read.push(std::make_shared<MockChunk>(chp, false));
        chunks_to_read.push(std::make_shared<MockChunk>(chp, true));
        chunks_to_read.push(std::make_shared<MockChunk>(chp, true));
        chunks_to_read.push(std::make_shared<MockChunk>(chp, false));
        chunks_to_read.push(std::make_shared<MockChunk>(chp, false));

        auto array = std::make_unique<MockDmrppArray>();
        vector<unsigned long long> array_shape = {1};

        try {
            dmrpp_array_thread_control(chunks_to_read, array.get(), array_shape);
            CPPUNIT_FAIL("dmrpp_array_thread_control() should have thrown an exception");
        }
        catch(BESInternalError &e) {
            DBG(cerr << prolog << "BESInternalError: " << e.get_verbose_message() << endl);
        }

        DBG(cerr << prolog << "END" << endl);
    }

    // Make sure the curl handle pool doesn't leak with multiple failures
    void process_one_chunk_threaded_test_6()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        queue<shared_ptr<Chunk>> chunks_to_read;
        for (int i = 0; i < 5; ++i) {
            chunks_to_read.push(std::make_shared<MockChunk>(chp, false));
            chunks_to_read.push(std::make_shared<MockChunk>(chp, false));
            chunks_to_read.push(std::make_shared<MockChunk>(chp, false));
            chunks_to_read.push(std::make_shared<MockChunk>(chp, true));
            chunks_to_read.push(std::make_shared<MockChunk>(chp, true));
            chunks_to_read.push(std::make_shared<MockChunk>(chp, false));
            chunks_to_read.push(std::make_shared<MockChunk>(chp, false));

            auto array = std::make_unique<MockDmrppArray>();
            vector<unsigned long long> array_shape = {1};

            try {
                dmrpp_array_thread_control(chunks_to_read, array.get(), array_shape);
                CPPUNIT_FAIL("dmrpp_array_thread_control() should have thrown an exception");
            }
            catch (BESInternalError &e) {
                DBG(cerr << prolog << "BESInternalError: " << e.get_verbose_message() << endl);
            }
        }

        DBG(cerr << prolog << "END" << endl);
    }

#if 0
Write tests for get_easy_handle() and test the signed URL below. jhrg 11/19/24
dmrpp_easy_handle *
CurlHandlePool::get_easy_handle(Chunk *chunk)

Test using:
https://podaac-ops-cumulus-protected.s3.us-west-2.amazonaws.com/CYGNSS_L1_V3.1/cyg03.ddmi.
s20180801-000000-e20180801-235959.l1.power-brcs.a31.d32.nc?A-userid=jhrg&X-Amz-Algorithm=AWS4-HMAC-SHA256
        &X-Amz-Credential=ASIAxxxxxxxxxxxxxxxxus-west-2%2Fs3%2Faws4_request
&X-Amz-Date=20241119T222214Z
&X-Amz-Expires=3600
&X-Amz-Security-Token=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
&X-Amz-SignedHeaders=host
&X-Amz-Signature=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

#endif

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

int main(int argc, char*argv[]) {
    return bes_run_tests<dmrpp::CurlHandlePoolTest>(argc, argv, "cerr,dmrpp:3") ? 0 : 1;
}
