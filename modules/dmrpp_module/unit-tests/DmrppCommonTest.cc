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

#include <memory>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <XMLWriter.h>
#include <GetOpt.h>
#include <util.h>
#include <debug.h>

#include <BESError.h>
#include <BESDebug.h>

#include "DmrppCommon.h"

#include "read_test_baseline.h"
#include "test_config.h"

using namespace libdap;
using namespace bes;

static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)

namespace dmrpp {

class DmrppCommonTest: public CppUnit::TestFixture {
private:
    DmrppCommon d_dc;

public:
    // Called once before everything gets tested
    DmrppCommonTest()
    {
    }

    // Called at the end of the test
    ~DmrppCommonTest()
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

    void test_ingest_chunk_dimension_sizes_1()
    {
        d_dc.parse_chunk_dimension_sizes("51 17");
        CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.size() == 2);
        CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.at(0) == 51);
        CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.at(1) == 17);
    }

    void test_ingest_chunk_dimension_sizes_2()
    {
        d_dc.parse_chunk_dimension_sizes("51");
        CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.size() == 1);
        CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.at(0) == 51);
    }

    void test_ingest_chunk_dimension_sizes_3()
    {
        d_dc.parse_chunk_dimension_sizes("51,17");
        CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.size() == 2);
        CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.at(0) == 51);
        CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.at(1) == 17);
    }

    void test_ingest_chunk_dimension_sizes_4()
    {
        d_dc.parse_chunk_dimension_sizes("[51,17]");
        CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.size() == 2);
        CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.at(0) == 51);
        CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.at(1) == 17);
    }

    void test_ingest_chunk_dimension_sizes_5()
    {
        d_dc.parse_chunk_dimension_sizes("");
        CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.size() == 0);
    }

    void test_ingest_compression_type_1()
    {
        d_dc.ingest_compression_type("deflate");
        CPPUNIT_ASSERT(d_dc.d_deflate == true);
    }

    void test_ingest_compression_type_2()
    {
        d_dc.ingest_compression_type("shuffle");
        CPPUNIT_ASSERT(d_dc.d_shuffle == true);
    }

    void test_ingest_compression_type_3()
    {
        d_dc.ingest_compression_type("");
        CPPUNIT_ASSERT(d_dc.d_deflate == false);
        CPPUNIT_ASSERT(d_dc.d_shuffle == false);
    }

    void test_ingest_compression_type_4()
    {
        d_dc.d_deflate = true;
        d_dc.d_shuffle = true;
        d_dc.ingest_compression_type("");
        CPPUNIT_ASSERT(d_dc.d_deflate == true);
        CPPUNIT_ASSERT(d_dc.d_shuffle == true);
    }

    void test_ingest_compression_type_5()
    {
        d_dc.ingest_compression_type("foobar");
        CPPUNIT_ASSERT(d_dc.d_deflate == false);
        CPPUNIT_ASSERT(d_dc.d_shuffle == false);
    }

    // add_chunk(const string &data_url, unsigned long long size, unsigned long long offset, string position_in_array)

    void test_add_chunk_1()
    {
        CPPUNIT_ASSERT(d_dc.d_chunks.size() == 0);
        int size = d_dc.add_chunk("url", 100, 200, "[10,20]");

        CPPUNIT_ASSERT(size == 1);
        CPPUNIT_ASSERT(d_dc.d_chunks.size() == 1);
        Chunk &c = d_dc.d_chunks[0];
        CPPUNIT_ASSERT(c.d_data_url == "url");
        CPPUNIT_ASSERT(c.d_size == 100);
        CPPUNIT_ASSERT(c.d_offset = 200);
        CPPUNIT_ASSERT(c.d_chunk_position_in_array.size() == 2);
        CPPUNIT_ASSERT(c.d_chunk_position_in_array.at(0) == 10);
        CPPUNIT_ASSERT(c.d_chunk_position_in_array.at(1) == 20);
    }

    void test_add_chunk_2()
    {
        vector<unsigned int> pia;
        pia.push_back(10);
        pia.push_back(20);

        int size = d_dc.add_chunk("url", 100, 200, pia);

        CPPUNIT_ASSERT(size == 1);
        CPPUNIT_ASSERT(d_dc.d_chunks.size() == 1);
        Chunk &c = d_dc.d_chunks[0];
        CPPUNIT_ASSERT(c.d_data_url == "url");
        CPPUNIT_ASSERT(c.d_size == 100);
        CPPUNIT_ASSERT(c.d_offset = 200);
        CPPUNIT_ASSERT(c.d_chunk_position_in_array.size() == 2);
        CPPUNIT_ASSERT(c.d_chunk_position_in_array.at(0) == 10);
        CPPUNIT_ASSERT(c.d_chunk_position_in_array.at(1) == 20);
    }

    void test_print_chunks_element_1()
    {
        d_dc.parse_chunk_dimension_sizes("51 17");
        d_dc.add_chunk("url", 100, 200, "[10,20]");

        XMLWriter writer;
        d_dc.print_chunks_element(writer, "dmrpp");

        string baseline = read_test_baseline(string(TEST_SRC_DIR).append("/baselines/print_chunks_element_1.xml"));
        DBG(cerr << writer.get_doc() << endl);
        CPPUNIT_ASSERT(baseline == string (writer.get_doc()));
    }

    void test_print_chunks_element_2()
    {
        d_dc.parse_chunk_dimension_sizes("51 17");
        d_dc.add_chunk("url", 100, 200, "[10,20]");
        int size = d_dc.add_chunk("url", 100, 300, "[20,30]");

        CPPUNIT_ASSERT(size == 2);

        XMLWriter writer;
        d_dc.print_chunks_element(writer, "dmrpp_2");

        string baseline = read_test_baseline(string(TEST_SRC_DIR).append("/baselines/print_chunks_element_2.xml"));
        DBG(cerr << writer.get_doc() << endl);
        CPPUNIT_ASSERT(baseline == string (writer.get_doc()));
    }

    void test_print_chunks_element_3()
    {
        d_dc.d_deflate = true;
        d_dc.parse_chunk_dimension_sizes("51 17");
        d_dc.add_chunk("url", 100, 200, "[10,20]");
        int size = d_dc.add_chunk("url", 100, 300, "[20,30]");

        CPPUNIT_ASSERT(size == 2);

        XMLWriter writer;
        d_dc.print_chunks_element(writer, "dmrpp");

        string baseline = read_test_baseline(string(TEST_SRC_DIR).append("/baselines/print_chunks_element_3.xml"));
        DBG(cerr << writer.get_doc() << endl);
        CPPUNIT_ASSERT(baseline == string (writer.get_doc()));
    }

    void test_print_chunks_element_4()
    {
        d_dc.d_deflate = true;
        d_dc.d_shuffle = true;
        d_dc.parse_chunk_dimension_sizes("51 17");
        d_dc.add_chunk("url", 100, 200, "[10,20]");
        int size = d_dc.add_chunk("url", 100, 300, "[20,30]");

        CPPUNIT_ASSERT(size == 2);

        XMLWriter writer;
        d_dc.print_chunks_element(writer, "dmrpp");

        string baseline = read_test_baseline(string(TEST_SRC_DIR).append("/baselines/print_chunks_element_4.xml"));
        DBG(cerr << writer.get_doc() << endl);
        CPPUNIT_ASSERT(baseline == string (writer.get_doc()));
    }

    void test_print_chunks_element_5()
    {
        d_dc.d_deflate = false;
        d_dc.d_shuffle = true;
        d_dc.parse_chunk_dimension_sizes("51 17");
        d_dc.add_chunk("url", 100, 200, "[10,20]");
        int size = d_dc.add_chunk("url", 100, 300, "[20,30]");

        CPPUNIT_ASSERT(size == 2);

        XMLWriter writer;
        d_dc.print_chunks_element(writer, "DMRpp");

        string baseline = read_test_baseline(string(TEST_SRC_DIR).append("/baselines/print_chunks_element_5.xml"));
        DBG(cerr << writer.get_doc() << endl);
        CPPUNIT_ASSERT(baseline == string (writer.get_doc()));
    }

    CPPUNIT_TEST_SUITE( DmrppCommonTest );

    CPPUNIT_TEST(test_ingest_chunk_dimension_sizes_1);
    CPPUNIT_TEST(test_ingest_chunk_dimension_sizes_2);
    CPPUNIT_TEST_EXCEPTION(test_ingest_chunk_dimension_sizes_3, BESError);
    CPPUNIT_TEST_EXCEPTION(test_ingest_chunk_dimension_sizes_4, BESError);
    CPPUNIT_TEST(test_ingest_chunk_dimension_sizes_5);

    CPPUNIT_TEST(test_ingest_compression_type_1);
    CPPUNIT_TEST(test_ingest_compression_type_2);
    CPPUNIT_TEST(test_ingest_compression_type_3);
    CPPUNIT_TEST(test_ingest_compression_type_4);
    CPPUNIT_TEST(test_ingest_compression_type_5);

    CPPUNIT_TEST(test_add_chunk_1);
    CPPUNIT_TEST(test_add_chunk_2);

    CPPUNIT_TEST(test_print_chunks_element_1);
    CPPUNIT_TEST(test_print_chunks_element_2);
    CPPUNIT_TEST(test_print_chunks_element_3);
    CPPUNIT_TEST(test_print_chunks_element_4);
    CPPUNIT_TEST(test_print_chunks_element_5);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DmrppCommonTest);

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
            test = dmrpp::DmrppCommonTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
