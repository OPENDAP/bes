// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2016 OPeNDAP, Inc.
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

#include <memory>
#include <unitstd.h>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <BESError.h>
#include <BESDebug.h>

#include "DmrppUtil.h"

//#include "GetOpt.h"
#include "test_config.h"
#include <libdap/util.h>

using namespace libdap;

static bool debug = false;
static bool bes_debug = false;

namespace dmrpp {

class DmrppUtilTest: public CppUnit::TestFixture {
private:

public:
    // Called once before everything gets tested
    DmrppUtilTest()
    {
    }

    // Called at the end of the test
    ~DmrppUtilTest()
    {
    }

    // Called before each test
    void setUp()
    {
        if (bes_debug) BESDebug::SetUp("cerr,dmrpp");
    }

    // Called after each test
    void tearDown()
    {
    }

    /**
     * @brief Read a chunk
     * @param file
     * @param offset
     * @param size
     * @return Return a char vector with the data
     */
    vector<char> read_chunk(const string &file, unsigned int offset, unsigned int size)
    {
        BESDEBUG("dmrpp", __FUNCTION__ << " opening: " << file << endl);
        ifstream ifs(file.c_str());
        if (!ifs) throw BESError("Could not open file: " + file, BES_INTERNAL_ERROR, __FILE__, __LINE__);

        ifs.seekg(offset);
        vector<char> buf(size);
        ifs.read(buf.data(), size);

        if (ifs.bad() || ifs.fail()) throw BESError("Could not open file: " + file, BES_INTERNAL_ERROR, __FILE__,
        __LINE__);

        return buf;
    }

    void test_uncompressed_chunk()
    {
        try {
            // chunk holds float data
            const unsigned int chunk_size = 40000; // bytes
            vector<char> buf = read_chunk(test_data_dir + "/chunked_oneD.h5", 3496, chunk_size);

            for (unsigned int i = 0; i < chunk_size / sizeof(dods_float32); ++i) {
                dods_float32 value = *(reinterpret_cast<dods_float32*>(buf.data() + i * sizeof(dods_float32)));
                BESDEBUG("dmrpp", "buf[" << i << "]: " << value << endl);
                CPPUNIT_ASSERT(double_eq(value, i));
            }
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (exception &e) {
            CPPUNIT_FAIL(e.what());
        }
    }

    void test_compressed_chunk()
    {
        try {
            // chunk holds float data
            const unsigned int uncomp_size = 40000; // bytes
            const unsigned int comp_size = 11393; // bytes
            vector<char> buf = read_chunk(test_data_dir + "/chunked_gzipped_oneD.h5", 3496, comp_size);

            for (unsigned int i = 0; i < comp_size; ++i) {
                BESDEBUG("dmrpp", "Compressed data - buf[" << i << "]: " << (unsigned int)buf[i] << endl);
            }

            vector<char> dest(uncomp_size);

            inflate(dest.data(), dest.size(), buf.data(), buf.size());

            for (unsigned int i = 0; i < dest.size() / sizeof(dods_float32); ++i) {
                dods_float32 value = *(reinterpret_cast<dods_float32*>(dest.data() + i * sizeof(dods_float32)));
                BESDEBUG("dmrpp", "dest[" << i << "]: " << value << endl);
                CPPUNIT_ASSERT(double_eq(value, i));
            }
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (exception &e) {
            CPPUNIT_FAIL(e.what());
        }
    }

    // make a fake 'shuffled' array of ints and see if the code can un-shuffle it.
    void test_unshuffle() {
        try {
            unsigned int elems = 4;
            unsigned int width = sizeof(dods_int32); // 4 bytes

            // load in the values: if the four numbers are 1, 2, 3, 4, the (little endian) hex will be:
            // 0x 01 00 00 00; 02 00 00 00; 03 00 00 00; 04 00 00 00 and shuffled this will be:
            //    01 02 03 04  00 00 00 00  00 00 00 00  00 00 00 00
            char src[] = {1, 2, 3, 4,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0};

            vector<char> dest(elems * width);

            unshuffle(dest.data(), src, elems * width, width);

            for (unsigned int i = 0; i < elems; ++i) {
                dods_int32 value = *(reinterpret_cast<dods_int32*>(dest.data() + i * sizeof(dods_int32)));
                BESDEBUG("dmrpp", "dest[" << i << "]: " << value << endl);
                CPPUNIT_ASSERT(value == (dods_int32)i + 1);
            }
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (exception &e) {
            CPPUNIT_FAIL(e.what());
        }
    }

    // The same values as above, but now we add some 'left over' bytes because the chunk
    // is not an integral multiple of the element size.
    void test_unshuffle2() {
        try {
            unsigned int elems = 4;
            unsigned int width = sizeof(dods_int32); // 4 bytes

            unsigned int src_size = elems * width + 3;

            // load in the values: if the four numbers are 1, 2, 3, 4, the (little endian) hex will be:
            // 0x 01 00 00 00; 02 00 00 00; 03 00 00 00; 04 00 00 00 and shuffled this will be:
            //    01 02 03 04  00 00 00 00  00 00 00 00  00 00 00 00
            char src[] = {1, 2, 3, 4,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  16, 17, 18};

            vector<char> dest(src_size);

            unshuffle(dest.data(), src, src_size, width);

            for (unsigned int i = 0; i < elems; ++i) {
                dods_int32 value = *(reinterpret_cast<dods_int32*>(dest.data() + i * sizeof(dods_int32)));
                BESDEBUG("dmrpp", "dest[" << i << "]: " << value << endl);
                CPPUNIT_ASSERT(value == (dods_int32)i + 1);
            }

            // Now look for the left behind...
            CPPUNIT_ASSERT(dest[16] == 16);
            CPPUNIT_ASSERT(dest[17] == 17);
            CPPUNIT_ASSERT(dest[18] == 18);
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (exception &e) {
            CPPUNIT_FAIL(e.what());
        }
    }

    // Now test byte values
    void test_unshuffle3() {
        try {
            unsigned int elems = 16;
            unsigned int width = sizeof(dods_byte); // 1

            unsigned int src_size = elems * width;

            char src[] = {0, 1, 2, 3,  4, 5, 6, 7,  8, 9, 10, 11,  12, 13, 14, 15};

            vector<char> dest(src_size);

            unshuffle(dest.data(), src, src_size, width);

            for (unsigned int i = 0; i < elems; ++i) {
                dods_byte value = *(reinterpret_cast<dods_byte*>(dest.data() + i * sizeof(dods_byte)));
                BESDEBUG("dmrpp", "dest[" << i << "]: " << value << endl);
                CPPUNIT_ASSERT(value == (dods_byte)i);
            }
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (exception &e) {
            CPPUNIT_FAIL(e.what());
        }
    }

    CPPUNIT_TEST_SUITE( DmrppUtilTest );

    CPPUNIT_TEST(test_uncompressed_chunk);
    CPPUNIT_TEST(test_compressed_chunk);

    // __BYTE_ORDER__ is documented for gcc and, works on the llvm/gcc that
    // ships with OSX. So these tests are run on OSX (10.12 tested; 1/20/17).
    // I added this because I'm pretty sure the code will work on a big endian
    // machine but I know the tests will fail because the values will be wrong.
    //
    // The tests could be rewritten to just test shuffling of bytes ...

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    CPPUNIT_TEST(test_unshuffle);
    CPPUNIT_TEST(test_unshuffle2);
#endif

    CPPUNIT_TEST(test_unshuffle3);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DmrppUtilTest);

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

    bool wasSuccessful = true;
    string test = "";
    int i = optind;
    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = dmrpp::DmrppUtilTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        ++i;
        }
    }
    return wasSuccessful ? 0 : 1;
}
