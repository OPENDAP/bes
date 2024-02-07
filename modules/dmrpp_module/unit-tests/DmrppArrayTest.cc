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
#include <future>
#include <list>
#include <atomic>

#include <libdap/util.h>
#include <libdap/debug.h>

#include "modules/common/run_tests_cppunit.h"

#include "BESContextManager.h"
#include "BESError.h"
#include "TheBESKeys.h"

#include "DmrppArray.h"
#include "DmrppByte.h"
#include "DmrppRequestHandler.h"

#include "test_config.h"

using namespace libdap;

#define prolog std::string("DmrppArrayTest::").append(__func__).append("() - ")

namespace dmrpp {

class DmrppArrayTest : public CppUnit::TestFixture {
    dmrpp::DmrppRequestHandler *foo = nullptr;

public:
    DmrppArrayTest() = default;

    ~DmrppArrayTest() override = default;

    // Called before each test
    void setUp() override {
        DBG(cerr << endl);
        // Contains BES Log parameters but not cache names
        TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");
        DBG(cerr << prolog << "TheBESKeys::ConfigFile: " << TheBESKeys::ConfigFile << endl);
        string val;
        bool found;
        TheBESKeys::TheKeys()->get_value("ff", val, found);

        unsigned int max_threads = 8;
        dmrpp::DmrppRequestHandler::d_use_transfer_threads = true;
        dmrpp::DmrppRequestHandler::d_max_transfer_threads = max_threads;

        // Various things will gripe about this not being used... This is how the
        // CurlHandlePool gets instantiated. jhrg 4/22/22
        foo = new dmrpp::DmrppRequestHandler("Chaos");
    }

    void tearDown() override {
        delete foo;
    }

protected:
    // ---- These tests were written by AI with some corrections by me (WIP) ----
    void test_empty_futures() {
        std::list<std::future<bool>> futures;
        std::atomic_uint thread_counter{0};
        CPPUNIT_ASSERT(!get_next_future(futures, thread_counter, 100, ""));
    }

    void test_future_ready() {
        std::list<std::future<bool>> futures;
        futures.emplace_back(std::async([]() { return true; }));
        std::atomic_uint thread_counter{1};
        CPPUNIT_ASSERT(get_next_future(futures, thread_counter, 100, ""));
        CPPUNIT_ASSERT(futures.empty());
        CPPUNIT_ASSERT(thread_counter == 0);
    }

    void test_future_timeout() {
        std::list<std::future<bool>> futures;
        futures.emplace_back(std::async([]() { std::this_thread::sleep_for(std::chrono::milliseconds(200)); return true; }));
        std::atomic_uint thread_counter{1};
        CPPUNIT_ASSERT(!get_next_future(futures, thread_counter, 100, ""));
        CPPUNIT_ASSERT(!futures.empty()); // Future should still be in the list after timeout
        CPPUNIT_ASSERT(thread_counter == 1);
    }

    void test_future_invalid() {
        std::list<std::future<bool>> futures;
        futures.emplace_back(std::async([]() { return true; }));
        if (!futures.front().valid())
            CPPUNIT_FAIL("The future should be valid here.");
        if (!futures.front().get())
            CPPUNIT_FAIL("The future should be return true.");
        if (futures.front().valid())
            CPPUNIT_FAIL("The future should be not valid here.");
        // Now test the way get_next_future() behaves with an invalid future.
        std::atomic_uint thread_counter{1};
        CPPUNIT_ASSERT(get_next_future(futures, thread_counter, 100, ""));
        CPPUNIT_ASSERT(futures.empty());
        CPPUNIT_ASSERT(thread_counter == 0);
    }

    void test_future_success() {
        std::list<std::future<bool>> futures;
        futures.emplace_back(std::async([]() { return true; }));
        std::atomic_uint thread_counter{1};
        CPPUNIT_ASSERT(get_next_future(futures, thread_counter, 100, ""));
        CPPUNIT_ASSERT(futures.empty());
        CPPUNIT_ASSERT(thread_counter == 0);
    }

    void test_future_failure() {
        std::list<std::future<bool>> futures;
        futures.emplace_back(std::async([]() -> bool { throw std::runtime_error("Test failure"); }));
        std::atomic_uint thread_counter{1};
        CPPUNIT_ASSERT_THROW(get_next_future(futures, thread_counter, 100, ""), std::runtime_error);
        CPPUNIT_ASSERT(futures.empty());
        CPPUNIT_ASSERT(thread_counter == 0);
    }

    void test_thread_counter() {
        std::list<std::future<bool>> futures;
        futures.emplace_back(std::async([]() { return true; }));
        futures.emplace_back(std::async([]() { return true; }));
        std::atomic_uint thread_counter{2};
        CPPUNIT_ASSERT(get_next_future(futures, thread_counter, 100, ""));
        CPPUNIT_ASSERT(1 == thread_counter);
        CPPUNIT_ASSERT(get_next_future(futures, thread_counter, 100, ""));
        CPPUNIT_ASSERT(0 == thread_counter);
    }

    void test_equal_ranks() {
        std::vector<unsigned long long> address = {1, 2};
        std::vector<unsigned long long> shape = {3, 4};
        CPPUNIT_ASSERT_EQUAL(6ULL, get_index(address, shape));
    }

    void test_unequal_ranks() {
        std::vector<unsigned long long> address = {1, 2, 3};
        std::vector<unsigned long long> shape = {3, 4};
        CPPUNIT_ASSERT_THROW(get_index(address, shape), BESInternalError);
    }

    void test_index_out_of_bounds() {
        std::vector<unsigned long long> address = {1, 4};
        std::vector<unsigned long long> shape = {3, 4};
        CPPUNIT_ASSERT_THROW(get_index(address, shape), BESInternalError);
    }

    void test_zero_dimensions() {
        std::vector<unsigned long long> address, shape;
        CPPUNIT_ASSERT_EQUAL(0ULL, get_index(address, shape));
    }

    void test_single_dimension() {
        std::vector<unsigned long long> address = {3};
        std::vector<unsigned long long> shape = {5};
        CPPUNIT_ASSERT_EQUAL(3ULL, get_index(address, shape));
    }

    void test_multiple_dimensions() {
        std::vector<unsigned long long> address = {2, 1, 0};
        std::vector<unsigned long long> shape = {3, 4, 5};
        CPPUNIT_ASSERT_EQUAL(45ULL, get_index(address, shape));
    }

    // ---- These tests were written by OPeNDAP ----

    void read_contiguous_sc_test() {
        DBG(cerr << prolog << "BEGIN" << endl);

        auto data_url = make_shared<http::url>(
                string("file://").append(TEST_DATA_DIR).append("/").append("big_ole_chunky_test.txt"));
        int target_file_size = 4194300;

        DmrppArray tiat(string("foo"), new libdap::Byte("foo"));
        tiat.append_dim(target_file_size, "test_dim");
        tiat.set_filter("");

        vector<unsigned long long> chunk_dim_sizes = {1};
        tiat.set_chunk_dimension_sizes(chunk_dim_sizes);
        vector<unsigned long long> position_in_array;
        position_in_array.push_back(0);
        tiat.add_chunk(data_url, "LE", target_file_size, 0, position_in_array);

        try {
            tiat.read_contiguous();
        }
        catch (const BESError &be) {
            CPPUNIT_FAIL("Caught BESError. Message: " + be.get_verbose_message());
        }
        catch (const libdap::Error &lde) {
            CPPUNIT_FAIL("Caught libdap::Error. Message: " + lde.get_error_message());
        }
        catch (const std::exception &e) {
            CPPUNIT_FAIL(string("Caught libdap::Error. Message: ").append(e.what()));
        }

        DBG(cerr << prolog << "END" << endl);
    }

    void read_contiguous_test() {
        DBG(cerr << prolog << "BEGIN" << endl);

        auto data_url = std::make_shared<http::url>(
                string("file://").append(TEST_DATA_DIR).append("/").append("big_ole_chunky_test.txt"));
        int target_file_size = 4194300;

        DmrppArray tiat(string("foo"), new libdap::Byte("foo"));
        tiat.append_dim(target_file_size, "test_dim");
        tiat.set_filter("");

        vector<unsigned long long> chunk_dim_sizes{1};
        tiat.set_chunk_dimension_sizes(chunk_dim_sizes);
        vector<unsigned long long> pia{0};

        tiat.add_chunk(data_url, "LE", target_file_size, 0, pia);

        try {
            tiat.read_contiguous();
        }
        catch (const BESError &be) {
            CPPUNIT_FAIL("Caught BESError. Message: " + be.get_verbose_message());
        }
        catch (const libdap::Error &lde) {
            CPPUNIT_FAIL("Caught libdap::Error. Message: " + lde.get_error_message());
        }
        catch (const std::exception &e) {
            CPPUNIT_FAIL(string("Caught libdap::Error. Message: ").append(e.what()));
        }

        DBG(cerr << prolog << "END" << endl);
    }

    CPPUNIT_TEST_SUITE( DmrppArrayTest );

        CPPUNIT_TEST(test_empty_futures);
        CPPUNIT_TEST(test_future_ready);
        CPPUNIT_TEST_FAIL(test_future_timeout);
        CPPUNIT_TEST(test_future_invalid);
        CPPUNIT_TEST(test_future_success);
        CPPUNIT_TEST(test_future_failure);
        CPPUNIT_TEST(test_thread_counter);

        CPPUNIT_TEST(test_equal_ranks);
        CPPUNIT_TEST(test_unequal_ranks);
        CPPUNIT_TEST(test_index_out_of_bounds);
        CPPUNIT_TEST(test_zero_dimensions);
        CPPUNIT_TEST(test_single_dimension);
        CPPUNIT_TEST(test_multiple_dimensions);

        CPPUNIT_TEST(read_contiguous_sc_test);
        CPPUNIT_TEST(read_contiguous_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DmrppArrayTest);

} // namespace dmrpp

int main(int argc, char*argv[])
{
    bool status = bes_run_tests<dmrpp::DmrppArrayTest>(argc, argv, "cerr,bes,http,curl,dmrpp");
    return status ? 0: 1;

#if 0
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
#endif
}
