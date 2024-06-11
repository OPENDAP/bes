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
#include <sstream>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <libdap/XMLWriter.h>
#include <unistd.h>
#include <libdap/util.h>
#include <libdap/debug.h>

#include <BESError.h>
#include <BESDebug.h>
#include <url_impl.h>

#include "Base64.h"
#include "DmrppCommon.h"
#include "Chunk.h"

#include "read_test_baseline.h"
#include "test_config.h"

using namespace std;
using namespace libdap;
using namespace bes;

static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)
#define prolog std::string("DmrppCommonTest::").append(__func__).append("() - ")

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
        if(debug) cerr << endl;
        if (bes_debug) BESDebug::SetUp("cerr,dmrpp");
    }

    // Called after each test
    void tearDown()
    {
    }

    void test_ingest_chunk_dimension_sizes_1()
    {
        try {
            d_dc.parse_chunk_dimension_sizes("51 17");
            CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.size() == 2);
            CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.at(0) == 51);
            CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.at(1) == 17);
        }
        catch(BESError &be){
            ostringstream msg;
            msg << prolog << "Caught BESError! Message: " << be.get_verbose_message();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(std::exception &se){
            ostringstream msg;
            msg << prolog << "Caught std::exception! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(...){
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }
    }

    void test_ingest_chunk_dimension_sizes_2()
    {
        try {
            d_dc.parse_chunk_dimension_sizes("51");
            CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.size() == 1);
            CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.at(0) == 51);
        }
        catch(BESError &be){
            stringstream msg;
            msg << prolog << "Caught BESError! Message: " << be.get_verbose_message();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(std::exception &se){
            stringstream msg;
            msg << prolog << "Caught std::exception! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(...){
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }
    }

    void test_ingest_chunk_dimension_sizes_3()
    {
        try {
            d_dc.parse_chunk_dimension_sizes("51,17");
            CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.size() == 2);
            CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.at(0) == 51);
            CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.at(1) == 17);
        }
        catch(BESError &be){
            if(debug){
                stringstream msg;
                msg << prolog << "Caught BESError! Message: " << be.get_verbose_message();
                cerr << msg.str() << endl;
            }
            throw;
        }
        catch(std::exception &se){
            stringstream msg;
            msg << prolog << "Caught std::exception! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(...){
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }
    }

    void test_ingest_chunk_dimension_sizes_4()
    {
        try {
            d_dc.parse_chunk_dimension_sizes("[51,17]");
            CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.size() == 2);
            CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.at(0) == 51);
            CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.at(1) == 17);
        }
        catch(BESError &be){
            if(debug){
                stringstream msg;
                msg << prolog << "Caught BESError! Message: " << be.get_verbose_message();
                cerr << msg.str() << endl;
            }
            throw;
        }
        catch(std::exception &se){
            stringstream msg;
            msg << prolog << "Caught std::exception! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(...){
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }
    }

    void test_ingest_chunk_dimension_sizes_5()
    {
        try {
            d_dc.parse_chunk_dimension_sizes("");
            CPPUNIT_ASSERT(d_dc.d_chunk_dimension_sizes.size() == 0);
        }
        catch(BESError &be){
            stringstream msg;
            msg << prolog << "Caught BESError! Message: " << be.get_verbose_message();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(std::exception &se){
            stringstream msg;
            msg << prolog << "Caught std::exception! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(...){
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }
    }

    void test_ingest_compression_type_1()
    {
        try {
            d_dc.ingest_compression_type("deflate");
            CPPUNIT_ASSERT(d_dc.d_filters == "deflate");
        }
        catch(BESError &be){
            stringstream msg;
            msg << prolog << "Caught BESError! Message: " << be.get_verbose_message();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(std::exception &se){
            stringstream msg;
            msg << prolog << "Caught std::exception! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(...){
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }
    }

    void test_ingest_compression_type_2()
    {
        try {
            d_dc.ingest_compression_type("shuffle");
            CPPUNIT_ASSERT(d_dc.d_filters == "shuffle");
        }
        catch(BESError &be){
            stringstream msg;
            msg << prolog << "Caught BESError! Message: " << be.get_verbose_message();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(std::exception &se){
            stringstream msg;
            msg << prolog << "Caught std::exception! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(...){
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }
    }

    void test_ingest_compression_type_3()
    {
        try {
            d_dc.ingest_compression_type("");
            CPPUNIT_ASSERT(d_dc.d_filters == "");
        }
        catch(BESError &be){
            stringstream msg;
            msg << prolog << "Caught BESError! Message: " << be.get_verbose_message();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(std::exception &se){
            stringstream msg;
            msg << prolog << "Caught std::exception! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(...){
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }
    }

    void test_ingest_compression_type_4()
    {
        try {
            d_dc.ingest_compression_type("fletcher32");
            CPPUNIT_ASSERT(d_dc.d_filters == "fletcher32");
        }
        catch(BESError &be){
            stringstream msg;
            msg << prolog << "Caught BESError! Message: " << be.get_verbose_message();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(std::exception &se){
            stringstream msg;
            msg << prolog << "Caught std::exception! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(...){
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }
    }
    // add_chunk(const string &data_url, unsigned long long size, unsigned long long offset, string position_in_array)

    void test_add_chunk_1()
    {
        try {
            CPPUNIT_ASSERT(d_dc.d_chunks.size() == 0);
            string url_str = "http://url";
            shared_ptr<http::url> target_url(new http::url(url_str));
            int size = d_dc.add_chunk(target_url, "", 100, 200, "[10,20]");

            CPPUNIT_ASSERT(size == 1);
            CPPUNIT_ASSERT(d_dc.d_chunks.size() == 1);
            auto c = d_dc.d_chunks[0];
            CPPUNIT_ASSERT(c->d_data_url->str() == url_str);
            CPPUNIT_ASSERT(c->d_size == 100);
            CPPUNIT_ASSERT(c->d_offset = 200);
            CPPUNIT_ASSERT(c->d_chunk_position_in_array.size() == 2);
            CPPUNIT_ASSERT(c->d_chunk_position_in_array.at(0) == 10);
            CPPUNIT_ASSERT(c->d_chunk_position_in_array.at(1) == 20);
        }
        catch(BESError &be){
            stringstream msg;
            msg << prolog << "Caught BESError! Message: " << be.get_verbose_message();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(std::exception &se){
            stringstream msg;
            msg << prolog << "Caught std::exception! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(...){
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }
    }

    void test_add_chunk_2()
    {
        vector<unsigned long long> pia;
        pia.push_back(10);
        pia.push_back(20);
        try {
            string url_str = "http://url";
            shared_ptr<http::url> target_url(new http::url(url_str));
            int size = d_dc.add_chunk(target_url, "", 100, 200, pia);

            CPPUNIT_ASSERT(size == 1);
            CPPUNIT_ASSERT(d_dc.d_chunks.size() == 1);
            auto c = d_dc.d_chunks[0];
            CPPUNIT_ASSERT(c->d_data_url->str() == url_str);
            CPPUNIT_ASSERT(c->d_size == 100);
            CPPUNIT_ASSERT(c->d_offset = 200);
            CPPUNIT_ASSERT(c->d_chunk_position_in_array.size() == 2);
            CPPUNIT_ASSERT(c->d_chunk_position_in_array.at(0) == 10);
            CPPUNIT_ASSERT(c->d_chunk_position_in_array.at(1) == 20);

        }
        catch(BESError &be){
            stringstream msg;
            msg << prolog << "Caught BESError! Message: " << be.get_verbose_message();
            cerr << msg.str();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(std::exception &se){
            stringstream msg;
            msg << prolog << "Caught std::exception! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(...){
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }
    }

    void test_print_chunks_element_1()
    {
        try {
            string url_str = "http://url";
            shared_ptr<http::url> target_url(new http::url(url_str));
            d_dc.parse_chunk_dimension_sizes("51 17");
            d_dc.add_chunk(target_url, "", 100, 200, "[10,20]");

            XMLWriter writer;
            d_dc.print_chunks_element(writer, "dmrpp");

            string baseline = read_test_baseline(string(TEST_SRC_DIR).append("/baselines/print_chunks_element_1.xml"));
            DBG(cerr << writer.get_doc() << endl);
            CPPUNIT_ASSERT(baseline == string (writer.get_doc()));
        }
        catch(BESError &be){
            stringstream msg;
            msg << prolog << "Caught BESError! Message: " << be.get_verbose_message();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(std::exception &se){
            stringstream msg;
            msg << prolog << "Caught std::exception! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(...){
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }
    }

    void test_print_chunks_element_2()
    {
        try {
            string url_str = "http://url";
            shared_ptr<http::url> target_url(new http::url(url_str));
            d_dc.parse_chunk_dimension_sizes("51 17");
            d_dc.add_chunk(target_url, "", 100, 200, "[10,20]");
            int size = d_dc.add_chunk(target_url, "", 100, 300, "[20,30]");

            CPPUNIT_ASSERT(size == 2);

            XMLWriter writer;
            d_dc.print_chunks_element(writer, "dmrpp_2");

            string baseline = read_test_baseline(string(TEST_SRC_DIR).append("/baselines/print_chunks_element_2.xml"));
            DBG(cerr << writer.get_doc() << endl);
            CPPUNIT_ASSERT(baseline == string (writer.get_doc()));
        }
        catch(BESError &be){
            stringstream msg;
            msg << prolog << "Caught BESError! Message: " << be.get_verbose_message();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(std::exception &se){
            stringstream msg;
            msg << prolog << "Caught std::exception! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(...){
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }
    }

    void test_print_chunks_element_3()
    {
        try {
            string url_str = "http://url";
            shared_ptr<http::url> target_url(new http::url(url_str));
            d_dc.d_filters = "deflate";
            d_dc.parse_chunk_dimension_sizes("51 17");
            d_dc.add_chunk(target_url, "", 100, 200, "[10,20]");
            int size = d_dc.add_chunk(target_url, "", 100, 300, "[20,30]");

            CPPUNIT_ASSERT(size == 2);

            XMLWriter writer;
            d_dc.print_chunks_element(writer, "dmrpp");

            string baseline = read_test_baseline(string(TEST_SRC_DIR).append("/baselines/print_chunks_element_3.xml"));
            DBG(cerr << writer.get_doc() << endl);
            CPPUNIT_ASSERT(baseline == string (writer.get_doc()));
        }
        catch(BESError &be){
            stringstream msg;
            msg << prolog << "Caught BESError! Message: " << be.get_verbose_message();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(std::exception &se){
            stringstream msg;
            msg << prolog << "Caught std::exception! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(...){
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }
    }

    void test_print_chunks_element_4()
    {
        try {
            string url_str = "http://url";
            shared_ptr<http::url> target_url(new http::url(url_str));
            d_dc.d_filters = "deflate shuffle";
            d_dc.parse_chunk_dimension_sizes("51 17");
            d_dc.add_chunk(target_url, "", 100, 200, "[10,20]");
            int size = d_dc.add_chunk(target_url, "", 100, 300, "[20,30]");

            CPPUNIT_ASSERT(size == 2);

            XMLWriter writer;
            d_dc.print_chunks_element(writer, "dmrpp");

            string baseline = read_test_baseline(string(TEST_SRC_DIR).append("/baselines/print_chunks_element_4.xml"));
            DBG(cerr << writer.get_doc() << endl);
            CPPUNIT_ASSERT(baseline == string (writer.get_doc()));
        }
        catch(BESError &be){
            stringstream msg;
            msg << prolog << "Caught BESError! Message: " << be.get_verbose_message();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(std::exception &se){
            stringstream msg;
            msg << prolog << "Caught std::exception! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(...){
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }
    }

    void test_print_chunks_element_5()
    {
        try {
            string url_str = "http://url";
            shared_ptr<http::url> target_url(new http::url(url_str));
            d_dc.d_filters = "shuffle";
            d_dc.parse_chunk_dimension_sizes("51 17");
            d_dc.add_chunk(target_url, "", 100, 200, "[10,20]");
            int size = d_dc.add_chunk(target_url, "", 100, 300, "[20,30]");

            CPPUNIT_ASSERT(size == 2);

            XMLWriter writer;
            d_dc.print_chunks_element(writer, "DMRpp");

            string baseline = read_test_baseline(string(TEST_SRC_DIR).append("/baselines/print_chunks_element_5.xml"));
            DBG(cerr << writer.get_doc() << endl);
            CPPUNIT_ASSERT(baseline == string (writer.get_doc()));
        }
        catch(BESError &be){
            stringstream msg;
            msg << prolog << "Caught BESError! Message: " << be.get_verbose_message();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(std::exception &se){
            stringstream msg;
            msg << prolog << "Caught std::exception! Message: " << se.what();
            cerr << msg.str() << endl;
            CPPUNIT_FAIL(msg.str());
        }
        catch(...){
            CPPUNIT_FAIL(prolog + "Caught unknown exception.");
        }
    }

    void test_print_compact_element_base64_encoded_data() {
        // Arrange
        XMLWriter writer;

        std::string nameSpace = "dmrpp";
        std::string encodedData = "YWJjZA=="; // Base364 encoded data (example)

        // Act
        CPPUNIT_ASSERT_NO_THROW(d_dc.print_compact_element(writer, nameSpace, encodedData));

        // Assert (limited assertion due to mock)
        string element = writer.get_doc();
        DBG(cerr << element << endl);

        CPPUNIT_ASSERT_MESSAGE("The element should contain the encoded data, got: " + element,
                               element.find(encodedData) != string::npos);
    }

    void test_print_missing_data_element_base64_encoded_data() {
        // Arrange
        XMLWriter writer;

        std::string nameSpace = "dmrpp";
        std::string encodedData = "YWJjZA=="; // Base364 encoded data (example)

        // Act
        CPPUNIT_ASSERT_NO_THROW(d_dc.print_missing_data_element(writer, nameSpace, encodedData));

        // Assert (limited assertion due to mock)
        string element = writer.get_doc();
        DBG(cerr << element << endl);

        CPPUNIT_ASSERT_MESSAGE("The element should contain the encoded data, got: " + element,
                               element.find(encodedData) != string::npos);
    }

    // Test the missing data element builder that handles the base encoding of the raw data
    void test_print_missing_data_element_raw_data() {
        // Arrange
        XMLWriter writer;

        std::string nameSpace = "dmrpp";
        char raw_data[4] = {61, 62, 63, 64}; // Raw data (example)
        std::string encodedData = "PT4/QA=="; // Base364 encoded data (example)

        // Act
        CPPUNIT_ASSERT_NO_THROW(d_dc.print_missing_data_element(writer, nameSpace, raw_data, sizeof(raw_data)));

        // Assert (limited assertion due to mock)
        string element = writer.get_doc();
        DBG(cerr << element << endl);

        CPPUNIT_ASSERT_MESSAGE("The element should contain the encoded data, got: " + element,
                               element.find(encodedData) != string::npos);
    }

    // Does the xmlTextWriterWriteBase64() function encode the binary data just as Base64::encode() does?
    void test_base64_versus_libxml2() {
        // Arrange
        XMLWriter writer;

        std::string nameSpace = "dmrpp";
        u_int8_t raw_data[4] = {61, 62, 63, 64}; // Raw data (example)
        std::string encodedData = base64::Base64::encode(raw_data, sizeof(raw_data)); // Base364 encoded data (example)
        DBG(cerr << "Encoded data: " << encodedData << '\n');

        // Act
        CPPUNIT_ASSERT_NO_THROW(d_dc.print_missing_data_element(writer, nameSpace, (char *)raw_data, sizeof(raw_data)));

        // Assert (limited assertion due to mock)
        string element = writer.get_doc();
        DBG(cerr << element << endl);

        CPPUNIT_ASSERT_MESSAGE("The element should contain the encoded data, got: " + element,
                               element.find(encodedData) != string::npos);
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

        CPPUNIT_TEST(test_add_chunk_1);
        CPPUNIT_TEST(test_add_chunk_2);

        CPPUNIT_TEST(test_print_chunks_element_1);
        CPPUNIT_TEST(test_print_chunks_element_2);
        CPPUNIT_TEST(test_print_chunks_element_3);
        CPPUNIT_TEST(test_print_chunks_element_4);
        CPPUNIT_TEST(test_print_chunks_element_5);

        CPPUNIT_TEST(test_print_compact_element_base64_encoded_data);
        CPPUNIT_TEST(test_print_missing_data_element_base64_encoded_data);
        CPPUNIT_TEST(test_base64_versus_libxml2);
        CPPUNIT_TEST(test_print_missing_data_element_raw_data);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DmrppCommonTest);

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
            test = dmrpp::DmrppCommonTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
