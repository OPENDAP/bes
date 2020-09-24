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

#include <unistd.h>
#include <time.h>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <Byte.h>

#include "BESInternalError.h"
#include "TheBESKeys.h"

#include "Chunk.h"
#include "DmrppArray.h"
#include "CurlHandlePool.h"

using namespace std;

static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)

namespace dmrpp {

class MockChunk : public Chunk {
    CurlHandlePool *d_chp;
    bool d_sim_err; // simulate and err

public:
    MockChunk(CurlHandlePool *chp, bool sim_err) : Chunk(), d_chp(chp), d_sim_err(sim_err) {}

    void inflate_chunk(bool, bool, unsigned int, unsigned int) {
        return;
    }

    virtual std::string get_data_url() const {
        return "https://httpbin.org/";
    }

    std::vector<unsigned int> mock_pia = {1};

    virtual const std::vector<unsigned int> &get_position_in_array() const {
        return mock_pia;
    }

    // void read_chunk() { return; }

    void read_chunk() {
        if (d_is_read) {
            return;
        }

        set_rbuf_to_size();

        dmrpp_easy_handle *handle = d_chp->get_easy_handle(this);
        if (!handle)
            throw BESInternalError("No more libcurl handles.", __FILE__, __LINE__);

        try {
            // handle->read_data();  // throws if error
            time_t t;
            srandom(time(&t));
            sleep(3);

            if (d_sim_err)
                throw BESInternalError("Simulated error", __FILE__, __LINE__);

            d_chp->release_handle(handle);
        }
        catch(...) {
            d_chp->release_handle(handle);
            throw;
        }
#if 0
        // If the expected byte count was not read, it's an error.
        if (get_size() != get_bytes_read()) {
            ostringstream oss;
            oss << "Wrong number of bytes read for chunk; read: " << get_bytes_read() << ", expected: " << get_size();
            throw BESInternalError(oss.str(), __FILE__, __LINE__);
        }
#endif
        d_is_read = true;
    }

};

class MockDmrppArray : public DmrppArray {
public:
    MockDmrppArray() : DmrppArray("mock_array", new libdap::Byte("mock_array")) {
        append_dim(10, "mock_dim");
    }

#if 0
    MockDmrppArray(const std::string &n, libdap::BaseType *v) : DmrppArray(n, v) {}
    MockDmrppArray(const std::string &n, const std::string &d, libdap::BaseType *v) : DmrppArray(n, d, v) {}
    MockDmrppArray(const DmrppArray &rhs) : DmrppArray(rhs) {}
#endif

    bool is_deflate_compression() const { return false; }

    bool is_shuffle_compression() const { return false; }

    virtual void insert_chunk(unsigned int dim, vector<unsigned int> *target_element_address,
                      vector<unsigned int> *chunk_element_address,
                      Chunk *chunk, const vector<unsigned int> &constrained_array_shape) {
        return;
    }

};

class CurlHandlePoolTest : public CppUnit::TestFixture {
private:
    CurlHandlePool *chp;

public:
    // Called once before everything gets tested
    CurlHandlePoolTest() {
    }

    // Called at the end of the test
    ~CurlHandlePoolTest() {
    }

    // Called before each test
    void setUp() {
        chp = new CurlHandlePool;
        TheBESKeys::ConfigFile = "curl_handle_pool_keys.conf";
        //TheBESKeys();
    }

    // Called after each test
    void tearDown() {
        delete chp;
        chp = 0;
    }

    void empty_test() {
        CPPUNIT_ASSERT(true);

        MockChunk *chunk = new MockChunk(chp, true);
        MockDmrppArray *array = new MockDmrppArray; //("mock_array", new libdap::Byte("mock_array"));
        vector<unsigned int> array_shape = {1};

        int num = chp->get_handles_available();
        CPPUNIT_ASSERT(num ==  chp->get_max_handles());

        try {
            process_one_chunk(chunk, array, array_shape);
        }
        catch (BESError &e) {
            cerr << "BES Exception: " << e.get_verbose_message() << endl;
            CPPUNIT_FAIL("BES Exception");
        }
        catch (std::exception &e) {
            cerr << "Exception: " << e.what() << endl;
            CPPUNIT_FAIL("Exception");
        }

        int num2 = chp->get_handles_available();
        CPPUNIT_ASSERT(num2 == num);
    }

CPPUNIT_TEST_SUITE(CurlHandlePoolTest);

        CPPUNIT_TEST(empty_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(CurlHandlePoolTest);

} // namespace dmrpp

int main(int argc, char *argv[]) {
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
            if (debug) cerr << "Running " << argv[i] << endl;
            string test = dmrpp::CurlHandlePoolTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
