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

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <BESError.h>
#include <BESDebug.h>

#include "DmrppUtil.h"

#include "GetOpt.h"
#include "test_config.h"
#include "util.h"

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
        ifs.read(&buf[0], size);

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
                dods_float32 value = *(reinterpret_cast<dods_float32*>(&buf[0] + i * sizeof(dods_float32)));
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

            deflate(&dest[0], dest.size(), &buf[0], buf.size());

            for (unsigned int i = 0; i < dest.size() / sizeof(dods_float32); ++i) {
                dods_float32 value = *(reinterpret_cast<dods_float32*>(&dest[0] + i * sizeof(dods_float32)));
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

    CPPUNIT_TEST_SUITE( DmrppUtilTest );

    CPPUNIT_TEST(test_uncompressed_chunk);
    CPPUNIT_TEST(test_compressed_chunk);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DmrppUtilTest);

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
            test = string("dmrpp::DmrppUtilTest::") + argv[i++];

            cerr << endl << "Running test " << test << endl << endl;

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
