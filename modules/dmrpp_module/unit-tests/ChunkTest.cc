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

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <zlib.h>

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

#include "url_impl.h"
#include "Chunk.h"

#include "test_config.h"

using namespace libdap;
using namespace std;

static bool debug = false;
static bool bes_debug = false;
static string bes_conf_file = "/bes.conf";

#undef DBG
#define DBG(x) do { if (debug) x; } while(false)
#define prolog std::string("ChunkTest::").append(__func__).append("() - ")

namespace http {
class mock_url: public url {
public:
    mock_url(): url() {
    }
    string str() const { return "http://test.url.tld/file.ext?aws-token=secret_stuff"; }
    string protocol() const { return "http"; }
    string host() const { return "test.url.tld"; }
    string path() const { return "/file.ext"; }
};
}

namespace dmrpp {

// Helpers for the filter_chunk() tests

/**
 * Make a TIFF LZW stream that holds the bytes of 'plain' as literal codes: a CLEAR code, one 9-bit
 * code per byte, and an EOI code. That is a valid (if uncompressed) stream as long as it holds fewer
 * than about 250 bytes; after that the width of the codes has to change. The streams that actually
 * compress, and are produced by libtiff, are tested in lzw_util_test.
 */
static vector<uint8_t> lzw_literal_stream(const vector<uint8_t> &plain)
{
    vector<uint8_t> out;
    uint32_t acc = 0;
    unsigned nbits = 0;

    auto put = [&](unsigned code) {
        acc = (acc << 9) | code;
        nbits += 9;
        while (nbits >= 8) {
            out.push_back(static_cast<uint8_t>((acc >> (nbits - 8)) & 0xFF));
            nbits -= 8;
        }
        acc &= (1u << nbits) - 1;
    };

    put(256);   // CLEAR
    for (uint8_t b: plain)
        put(b);
    put(257);   // EOI
    if (nbits > 0)
        out.push_back(static_cast<uint8_t>((acc << (8 - nbits)) & 0xFF));

    return out;
}

/// Make a Chunk with the given byte order whose read buffer holds a copy of 'data'.
static unique_ptr<Chunk> make_chunk(const string &byte_order, const vector<uint8_t> &data)
{
    unique_ptr<Chunk> chunk(new Chunk(byte_order, data.size(), 0, vector<unsigned long long>{0}));
    char *buf = new char[data.size()];
    memcpy(buf, data.data(), data.size());
    chunk->set_read_buffer(buf, data.size(), data.size(), true);    // the Chunk owns buf
    return chunk;
}

static bool buffer_equals(Chunk &chunk, const vector<uint8_t> &expected)
{
    return chunk.get_rbuf_size() == expected.size() && memcmp(chunk.get_rbuf(), expected.data(), expected.size()) == 0;
}

/// Floating point values as bytes, in the given byte order, whatever the byte order of this host is.
static vector<uint8_t> float32_bytes(const vector<float> &values, bool big_endian)
{
    vector<uint8_t> out(values.size() * 4);
    for (size_t i = 0; i < values.size(); ++i) {
        uint32_t u;
        memcpy(&u, &values[i], sizeof(u));
        for (unsigned j = 0; j < 4; ++j)
            out[i * 4 + j] = static_cast<uint8_t>((u >> (8 * (big_endian ? 3 - j : j))) & 0xFF);
    }
    return out;
}

class ChunkTest: public CppUnit::TestFixture {
private:
    Chunk d_chunk;

public:
    // Called once before everything gets tested
    ChunkTest()
    {
    }

    // Called at the end of the test
    ~ChunkTest()
    {
    }

    // Called before each test
    void setUp()
    {
        TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append(bes_conf_file);
        if (bes_debug) BESDebug::SetUp("cerr,http,curl,dmrpp");
        DBG(cerr << endl);
    }

    // Called after each test
    void tearDown()
    {
    }

    void test_process_s3_error_response_1() {
        string document = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                          "<Error>\n"
                          "  <Code>AccessDenied</Code>\n"
                          "  <Message>Go away!</Message>\n"
                          "  <Resource>/mybucket/myfoto.jpg</Resource> \n"
                          "  <RequestId>4442587FB7D0A2F9</RequestId>\n"
                          "</Error>";
        shared_ptr<http::mock_url> murl(new http::mock_url());

        // we could catch the exception with a macro, but I want to look at the text
        try {
            process_s3_error_response(murl, document);
            CPPUNIT_FAIL("Expected an BESError to be thrown");
        }
        catch(BESError &e) {
            DBG(cerr << "Caught a BESError: " << e.get_verbose_message() << endl);
            CPPUNIT_ASSERT_MESSAGE("The message should not contain a token",
                                  e.get_verbose_message().find("aws-token") == string::npos);
            CPPUNIT_ASSERT("Correctly caught a BESError");
        }
    }

    // Test the bad code
    void test_process_s3_error_response_2() {
        string document = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                          "<Error>\n"
                          "  <Code>Bogus</Code>\n"
                          "  <Message>We're just fussy!</Message>\n"
                          "  <Resource>/mybucket/myfoto.jpg</Resource> \n"
                          "  <RequestId>4442587FB7D0A2F9</RequestId>\n"
                          "</Error>";
        shared_ptr<http::mock_url> murl(new http::mock_url());

        // we could catch the exception with a macro, but I want to look at the text
        try {
            process_s3_error_response(murl, document);
            CPPUNIT_FAIL("Expected an BESError to be thrown");
        }
        catch(BESError &e) {
            DBG(cerr << "Caught a BESError: " << e.get_verbose_message() << endl);
            CPPUNIT_ASSERT_MESSAGE("The message should not contain a token",
                                  e.get_verbose_message().find("aws-token") == string::npos);
            CPPUNIT_ASSERT("Correctly caught a BESError");
        }
    }

    // What if we get something that's not XML?
    void test_process_s3_error_response_3() {
        string document = "it's not xml!\n";
        shared_ptr<http::mock_url> murl(new http::mock_url());

        // we could catch the exception with a macro, but I want to look at the text
        try {
            process_s3_error_response(murl, document);
            CPPUNIT_FAIL("Expected an BESError to be thrown");
        }
        catch(BESError &e) {
            DBG(cerr << "Caught a BESError: " << e.get_verbose_message() << endl);
            CPPUNIT_ASSERT_MESSAGE("The message should not contain a token",
                                  e.get_verbose_message().find("aws-token") == string::npos);
            CPPUNIT_ASSERT("Correctly caught a BESError");
        }
    }

    // What if we get an XML doc that is not an error
    void test_process_s3_error_response_4() {
        string document = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                          "<Information>\n"
                          "  <Word>Bogus</Word>\n"
                          "  <Sentence>We're just fussy!</Sentence>\n"
                          "</Information>";
        shared_ptr<http::mock_url> murl(new http::mock_url());

        // we could catch the exception with a macro, but I want to look at the text
        try {
            process_s3_error_response(murl, document);
            CPPUNIT_FAIL("Expected an BESError to be thrown");
        }
        catch(BESError &e) {
            DBG(cerr << "Caught a BESError: " << e.get_verbose_message() << endl);
            CPPUNIT_ASSERT_MESSAGE("The message should not contain a token",
                                  e.get_verbose_message().find("aws-token") == string::npos);
            CPPUNIT_ASSERT("Correctly caught a BESError");
        }
    }


    void set_position_in_array_test()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        d_chunk.set_position_in_array("[1,2,3,4]");
        vector<unsigned long long> pia = d_chunk.get_position_in_array();
        CPPUNIT_ASSERT(pia.size() == 4);
        CPPUNIT_ASSERT(pia.at(0) == 1);
        CPPUNIT_ASSERT(pia.at(1) == 2);
        CPPUNIT_ASSERT(pia.at(2) == 3);
        CPPUNIT_ASSERT(pia.at(3) == 4);
    }

    void set_position_in_array_test_2()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        d_chunk.set_position_in_array("[5]");
        vector<unsigned long long> pia = d_chunk.get_position_in_array();
        CPPUNIT_ASSERT(pia.size() == 1);
        CPPUNIT_ASSERT(pia.at(0) == 5);
    }

    void set_position_in_array_test_3()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        d_chunk.set_position_in_array("[]");
        CPPUNIT_FAIL("set_position_in_array() should throw on missing values");
    }

    void set_position_in_array_test_4()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        d_chunk.set_position_in_array("[1,2,3,4");
        CPPUNIT_FAIL("set_position_in_array() should throw on a missing bracket");
    }

    void set_position_in_array_test_5()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        d_chunk.set_position_in_array("[1,x]");
        CPPUNIT_FAIL("set_position_in_array() should throw on bad values");
    }

    // This test fails
    void set_position_in_array_test_6()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        d_chunk.set_position_in_array("[1,2,]");
        CPPUNIT_FAIL("set_position_in_array() should throw on bad values");
    }

    // Call filter_chunk() and fail the test, with the BES error message, if it throws.
    static void filter(Chunk &chunk, const string &filters, unsigned long long chunk_size, unsigned long long elem_width)
    {
        try {
            chunk.filter_chunk(filters, chunk_size, elem_width);
        }
        catch (BESError &be) {
            CPPUNIT_FAIL(prolog + be.get_message());
        }
    }

    // Call filter_chunk(), which must throw a BESError, and return its message.
    static string filter_error_message(Chunk &chunk, const string &filters, unsigned long long chunk_size,
                                       unsigned long long elem_width)
    {
        try {
            chunk.filter_chunk(filters, chunk_size, elem_width);
        }
        catch (BESError &be) {
            return be.get_message();
        }
        CPPUNIT_FAIL(prolog + "filter_chunk() should have thrown a BESError for filters '" + filters + "'");
        return "";
    }
    
    // ---- lzw

    void filter_chunk_lzw_test()
    {
        DBG(cerr << prolog << "BEGIN" << endl);

        const string text = "TIFF LZW data";
        const vector<uint8_t> plain(text.begin(), text.end());
        auto chunk = make_chunk("LE", lzw_literal_stream(plain));

        filter(*chunk, "lzw", plain.size(), 1);

        CPPUNIT_ASSERT(chunk->d_is_inflated);
        CPPUNIT_ASSERT(buffer_equals(*chunk, plain));
    }

    void filter_chunk_lzw_element_width_test()
    {
        DBG(cerr << prolog << "BEGIN" << endl);

        // The chunk size is given in elements; the decoded size is that times the element width.
        const vector<uint8_t> plain = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
        auto chunk = make_chunk("LE", lzw_literal_stream(plain));

        filter(*chunk, "lzw", 6, 2);

        CPPUNIT_ASSERT(buffer_equals(*chunk, plain));
    }

    void filter_chunk_lzw_short_chunk_test()
    {
        DBG(cerr << prolog << "BEGIN" << endl);

        // The stream holds 5 bytes but the chunk is 12 bytes, as for the last strip of a TIFF whose height
        // is not a multiple of RowsPerStrip. The chunk is padded with zeros to its full size.
        const vector<uint8_t> plain = {1, 2, 3, 4, 5};
        auto chunk = make_chunk("LE", lzw_literal_stream(plain));

        filter(*chunk, "lzw", 12, 1);

        CPPUNIT_ASSERT(chunk->d_is_inflated);
        CPPUNIT_ASSERT(buffer_equals(*chunk, {1, 2, 3, 4, 5, 0, 0, 0, 0, 0, 0, 0}));
    }

    void filter_chunk_lzw_longer_stream_test()
    {
        DBG(cerr << prolog << "BEGIN" << endl);

        // The stream holds more than the chunk can: the chunk gets the start of the data.
        const vector<uint8_t> plain = {1, 2, 3, 4, 5, 6, 7, 8};
        auto chunk = make_chunk("LE", lzw_literal_stream(plain));

        filter(*chunk, "lzw", 4, 1);

        CPPUNIT_ASSERT(buffer_equals(*chunk, {1, 2, 3, 4}));
    }

    void filter_chunk_lzw_is_done_once_test()
    {
        DBG(cerr << prolog << "BEGIN" << endl);

        // Like the other filters, filter_chunk() does nothing if the chunk was already filtered. If it
        // decoded the (already decoded) buffer again it would fail.
        const vector<uint8_t> plain = {10, 20, 30, 40};
        auto chunk = make_chunk("LE", lzw_literal_stream(plain));

        filter(*chunk, "lzw", 4, 1);
        filter(*chunk, "lzw", 4, 1);

        CPPUNIT_ASSERT(buffer_equals(*chunk, plain));
    }

    // ---- tiffpred (the TIFF predictor), with lzw

    void filter_chunk_tiffpred_lzw_little_endian_test()
    {
        DBG(cerr << prolog << "BEGIN" << endl);

        // Two rows of four 16-bit samples. Row 1 holds the differences 0x1000, +1, -1, +2 and row 2 the
        // differences 5, +1, +1, +1; the filters are listed in the order they were applied: predictor, then LZW.
        const vector<uint8_t> stored = {0x00, 0x10, 0x01, 0x00, 0xFF, 0xFF, 0x02, 0x00,
                                        0x05, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00};
        const vector<uint8_t> expected = {0x00, 0x10, 0x01, 0x10, 0x00, 0x10, 0x02, 0x10,
                                          0x05, 0x00, 0x06, 0x00, 0x07, 0x00, 0x08, 0x00};
        auto chunk = make_chunk("LE", lzw_literal_stream(stored));

        filter(*chunk, "tiffpred:2:1:4 lzw", 8, 2);

        CPPUNIT_ASSERT(buffer_equals(*chunk, expected));
    }

    void filter_chunk_tiffpred_lzw_big_endian_test()
    {
        DBG(cerr << prolog << "BEGIN" << endl);

        // The same values as the little endian test, stored with the most significant byte first.
        const vector<uint8_t> stored = {0x10, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0x00, 0x02,
                                        0x00, 0x05, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01};
        const vector<uint8_t> expected = {0x10, 0x00, 0x10, 0x01, 0x10, 0x00, 0x10, 0x02,
                                          0x00, 0x05, 0x00, 0x06, 0x00, 0x07, 0x00, 0x08};
        auto chunk = make_chunk("BE", lzw_literal_stream(stored));

        filter(*chunk, "tiffpred:2:1:4 lzw", 8, 2);

        CPPUNIT_ASSERT(buffer_equals(*chunk, expected));
    }

    void filter_chunk_tiffpred_lzw_three_bands_test()
    {
        DBG(cerr << prolog << "BEGIN" << endl);

        // Two rows of two RGB pixels (6 samples per row). Each sample is added to the same band of the
        // pixel before it.
        const vector<uint8_t> stored = {10, 20, 30, 1, 2, 3, 1, 1, 1, 1, 1, 1};
        const vector<uint8_t> expected = {10, 20, 30, 11, 22, 33, 1, 1, 1, 2, 2, 2};
        auto chunk = make_chunk("LE", lzw_literal_stream(stored));

        filter(*chunk, "tiffpred:2:3:6 lzw", 12, 1);

        CPPUNIT_ASSERT(buffer_equals(*chunk, expected));
    }

    void filter_chunk_tiffpred_lzw_float32_test()
    {
        DBG(cerr << prolog << "BEGIN" << endl);

        // 1.0, 2.0, 3.5, -4.25, 100.0, 0.0 with the floating point predictor (3). The stored bytes are the
        // same for either byte order; the chunk's byte order is the byte order of the result.
        const vector<uint8_t> stored = {0x3F, 0x01, 0x00, 0x80, 0x82, 0xBE, 0x80, 0x80, 0x60, 0x28, 0x40, 0x38,
                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        const vector<float> values = {1.0f, 2.0f, 3.5f, -4.25f, 100.0f, 0.0f};

        auto little = make_chunk("LE", lzw_literal_stream(stored));
        filter(*little, "tiffpred:3:1:6 lzw", 6, 4);
        CPPUNIT_ASSERT(buffer_equals(*little, float32_bytes(values, false)));

        auto big = make_chunk("BE", lzw_literal_stream(stored));
        filter(*big, "tiffpred:3:1:6 lzw", 6, 4);
        CPPUNIT_ASSERT(buffer_equals(*big, float32_bytes(values, true)));
    }

    void filter_chunk_tiffpred_short_chunk_test()
    {
        DBG(cerr << prolog << "BEGIN" << endl);

        // A short last strip (one row of the two the chunk holds) is padded with zero rows by lzw before
        // the predictor is reversed, so the predictor sees whole rows.
        const vector<uint8_t> stored = {5, 1, 1, 1};
        auto chunk = make_chunk("LE", lzw_literal_stream(stored));

        filter(*chunk, "tiffpred:2:1:4 lzw", 8, 1);

        CPPUNIT_ASSERT(buffer_equals(*chunk, {5, 6, 7, 8, 0, 0, 0, 0}));
    }

    // ---- tiffpred without lzw: the predictor is also used with other compression, e.g. deflate

    void filter_chunk_tiffpred_only_test()
    {
        DBG(cerr << prolog << "BEGIN" << endl);

        // With no compression filter listed the predictor is undone in place.
        auto chunk = make_chunk("LE", {5, 1, 1, 1, 7, 1, 1, 1});

        filter(*chunk, "tiffpred:2:1:4", 8, 1);

        CPPUNIT_ASSERT(buffer_equals(*chunk, {5, 6, 7, 8, 7, 8, 9, 10}));
    }

    void filter_chunk_tiffpred_deflate_test()
    {
        DBG(cerr << prolog << "BEGIN" << endl);

        // TIFF Compression=8 (Adobe deflate) with Predictor=2 is common, too. Check that the predictor
        // is undone after the existing deflate filter.
        const vector<uint8_t> stored = {5, 1, 1, 1, 7, 1, 1, 1};
        uLongf compressed_size = compressBound(stored.size());
        vector<uint8_t> compressed(compressed_size);
        CPPUNIT_ASSERT(compress(compressed.data(), &compressed_size, stored.data(), stored.size()) == Z_OK);
        compressed.resize(compressed_size);

        auto chunk = make_chunk("LE", compressed);

        filter(*chunk, "tiffpred:2:1:4 deflate", 8, 1);

        CPPUNIT_ASSERT(buffer_equals(*chunk, {5, 6, 7, 8, 7, 8, 9, 10}));
    }

    // ---- failures

    // Corrupt data: the stream does not start with a CLEAR code.
    void filter_chunk_lzw_error_test_1()
    {
        auto chunk = make_chunk("LE", {0x41, 0x42, 0x43, 0x44, 0x45, 0x46});
        chunk->filter_chunk("lzw", 8, 1);
    }

    // Truncated data: the stream stops before the chunk is full and before the end-of-information code.
    void filter_chunk_lzw_error_test_2()
    {
        vector<uint8_t> stream = lzw_literal_stream({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16});
        stream.resize(stream.size() / 2);
        auto chunk = make_chunk("LE", stream);
        chunk->filter_chunk("lzw", 16, 1);
    }

    // No data at all
    void filter_chunk_lzw_error_test_3()
    {
        Chunk chunk("LE", 0, 0, vector<unsigned long long>{0});
        chunk.filter_chunk("lzw", 8, 1);
    }

    // tiffpred without all of its fields
    void filter_chunk_tiffpred_error_test_1()
    {
        auto chunk = make_chunk("LE", lzw_literal_stream({1, 2, 3, 4}));
        chunk->filter_chunk("tiffpred:2:1 lzw", 4, 1);
    }

    // tiffpred with an empty field
    void filter_chunk_tiffpred_error_test_2()
    {
        auto chunk = make_chunk("LE", lzw_literal_stream({1, 2, 3, 4}));
        chunk->filter_chunk("tiffpred:2::4 lzw", 4, 1);
    }

    // tiffpred with no fields
    void filter_chunk_tiffpred_error_test_3()
    {
        auto chunk = make_chunk("LE", lzw_literal_stream({1, 2, 3, 4}));
        chunk->filter_chunk("tiffpred lzw", 4, 1);
    }

    // a predictor TIFF does not have
    void filter_chunk_tiffpred_error_test_4()
    {
        auto chunk = make_chunk("LE", lzw_literal_stream({1, 2, 3, 4}));
        chunk->filter_chunk("tiffpred:4:1:4 lzw", 4, 1);
    }

    // not a number
    void filter_chunk_tiffpred_error_test_5()
    {
        auto chunk = make_chunk("LE", lzw_literal_stream({1, 2, 3, 4}));
        chunk->filter_chunk("tiffpred:2:x:4 lzw", 4, 1);
    }

    // a negative number
    void filter_chunk_tiffpred_error_test_6()
    {
        auto chunk = make_chunk("LE", lzw_literal_stream({1, 2, 3, 4}));
        chunk->filter_chunk("tiffpred:2:-1:4 lzw", 4, 1);
    }

    // samples per pixel that do not divide the row
    void filter_chunk_tiffpred_error_test_7()
    {
        auto chunk = make_chunk("LE", lzw_literal_stream({1, 2, 3, 4}));
        chunk->filter_chunk("tiffpred:2:3:4 lzw", 4, 1);
    }

    // rows that do not divide the chunk
    void filter_chunk_tiffpred_error_test_8()
    {
        auto chunk = make_chunk("LE", lzw_literal_stream({1, 2, 3, 4, 5, 6, 7, 8}));
        chunk->filter_chunk("tiffpred:2:1:3 lzw", 8, 1);
    }

    // an element size the predictor cannot handle (24 bit samples)
    void filter_chunk_tiffpred_error_test_9()
    {
        auto chunk = make_chunk("LE", lzw_literal_stream({1, 2, 3, 4, 5, 6}));
        chunk->filter_chunk("tiffpred:2:1:2 lzw", 2, 3);
    }

    // the floating point predictor with 8-bit data
    void filter_chunk_tiffpred_error_test_10()
    {
        auto chunk = make_chunk("LE", lzw_literal_stream({1, 2, 3, 4}));
        chunk->filter_chunk("tiffpred:3:1:4 lzw", 4, 1);
    }

    void filter_chunk_error_messages_test()
    {
        DBG(cerr << prolog << "BEGIN" << endl);

        // The messages should say what is wrong; and a chunk that failed is not marked as filtered.
        auto corrupt = make_chunk("LE", {0x41, 0x42, 0x43, 0x44, 0x45, 0x46});
        string msg = filter_error_message(*corrupt, "lzw", 8, 1);
        DBG(cerr << prolog << msg << endl);
        CPPUNIT_ASSERT(msg.find("lzw filter") != string::npos);
        CPPUNIT_ASSERT(msg.find("CLEAR") != string::npos);
        CPPUNIT_ASSERT(!corrupt->d_is_inflated);

        // The filters are undone last to first, so lzw has already replaced the buffer by the time a bad
        // tiffpred filter is found: use a new chunk for each case.
        auto fresh = []() { return make_chunk("LE", lzw_literal_stream({1, 2, 3, 4})); };

        msg = filter_error_message(*fresh(), "tiffpred:2:1 lzw", 4, 1);
        DBG(cerr << prolog << msg << endl);
        CPPUNIT_ASSERT(msg.find("Malformed filter 'tiffpred:2:1'") != string::npos);

        msg = filter_error_message(*fresh(), "tiffpred:4:1:4 lzw", 4, 1);
        CPPUNIT_ASSERT(msg.find("Unsupported TIFF predictor") != string::npos);

        msg = filter_error_message(*fresh(), "tiffpred:2:x:4 lzw", 4, 1);
        CPPUNIT_ASSERT(msg.find("not a valid unsigned integer") != string::npos);

        // A sign is not part of a number here; it is not read as a (huge) unsigned value.
        msg = filter_error_message(*fresh(), "tiffpred:2:-1:4 lzw", 4, 1);
        CPPUNIT_ASSERT(msg.find("not a valid unsigned integer") != string::npos);

        msg = filter_error_message(*fresh(), "tiffpred:2:3:4 lzw", 4, 1);
        CPPUNIT_ASSERT(msg.find("must be > 0 and divide") != string::npos);

        msg = filter_error_message(*fresh(), "tiffpred:2:1:3 lzw", 4, 1);
        CPPUNIT_ASSERT(msg.find("inconsistent row/pixel/buffer sizes") != string::npos);
    }

#if ENABLE_TRACKING_QUERY_PARAMETER
    void add_tracking_query_param_test()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        CPPUNIT_ASSERT(d_chunk.d_query_marker.empty());
    }

#if 0

    void add_tracking_query_param_test_2()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        CPPUNIT_ASSERT(S3_TRACKING_CONTEXT == "cloudydap");
    }

#endif

    void add_tracking_query_param_test_3()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        try {
            BESContextManager::TheManager()->set_context(S3_TRACKING_CONTEXT, "request_id");
            // add_tracking_query_param() only works with S3 URLs. Bug? jhrg 8/9/18
            shared_ptr<http::url> data_url(new http::url("http://s3.amazonaws.com/somewhereovertherainbow/foo.nc"));
            d_chunk.set_data_url(data_url);

            d_chunk.add_tracking_query_param();

            CPPUNIT_ASSERT(!d_chunk.d_query_marker.empty());
            DBG(cerr << prolog << "d_chunk.d_query_marker: " << d_chunk.d_query_marker << endl);
            CPPUNIT_ASSERT(d_chunk.d_query_marker == "cloudydap=request_id");

        }
        catch(BESError &be){
            CPPUNIT_FAIL(prolog + be.get_message());
        }
        catch(std::exception e){
            CPPUNIT_FAIL(prolog + "Caught std::exception. Message: "+e.what());
        }
    }

    void add_tracking_query_param_test_4()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        try {
            BESContextManager::TheManager()->set_context(S3_TRACKING_CONTEXT, "request_id");

            // add_tracking_query_param() only works with S3 URLs. Bug? jhrg 8/9/18
            shared_ptr<http::url> some_url( new http::url("http://s3.amazonaws.com/somewhereovertherainbow/foo.nc"));
            d_chunk.set_data_url(some_url);

            d_chunk.add_tracking_query_param();

            auto data_url = d_chunk.get_data_url();

            DBG(cerr << prolog << "data_url: " << data_url->str() << endl);
            CPPUNIT_ASSERT(!data_url->str().empty());
            CPPUNIT_ASSERT(data_url->str() == "http://s3.amazonaws.com/somewhereovertherainbow/foo.nc?cloudydap=request_id");
        }
        catch(BESError &be){
            CPPUNIT_FAIL(prolog + be.get_message());
        }
        catch(std::exception e){
            CPPUNIT_FAIL(prolog + "Caught std::exception. Message: "+e.what());
        }
    }

    void add_tracking_query_param_test_4_1() {
        DBG(cerr << prolog << "BEGIN" << endl);
        try {
            // An S3 URL, but no context.
            BESContextManager::TheManager()->unset_context(S3_TRACKING_CONTEXT);   //>set_context("cloudydap", "request_id");
            shared_ptr<http::url> data_url(new http::url("http://s3.amazonaws.com/somewhereovertherainbow/foo.nc"));
            d_chunk.set_data_url(data_url);
            d_chunk.add_tracking_query_param();
        }
        catch(BESError &be){
            CPPUNIT_FAIL(prolog + be.get_message());
        }
        catch(std::exception e){
            CPPUNIT_FAIL(prolog + "Caught std::exception. Message: "+e.what());
        }

        CPPUNIT_ASSERT(d_chunk.d_query_marker.empty());
    }

#if ENABLE_TRACKING_QUERY_PARAMETER
    // Test the non-default ctor
    void add_tracking_query_param_test_5()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        try {
            BESContextManager::TheManager()->set_context(S3_TRACKING_CONTEXT, "request_id");
            shared_ptr<http::url> sotr(new http::url("http://s3.amazonaws.com/somewhereovertherainbow/foo.nc"));
            unique_ptr<Chunk> l_chunk(new Chunk(sotr, "", 100, 10, ""));

            CPPUNIT_ASSERT(!l_chunk->d_query_marker.empty());
            DBG(cerr << prolog << "l_chunk->d_query_marker: " << l_chunk->d_query_marker << endl);
            CPPUNIT_ASSERT(l_chunk->d_query_marker == "cloudydap=request_id");

            auto data_url = l_chunk->get_data_url();

            DBG(cerr << prolog << "data_url: " << data_url << endl);
            CPPUNIT_ASSERT(!data_url->str().empty());
            CPPUNIT_ASSERT(data_url->str() == "http://s3.amazonaws.com/somewhereovertherainbow/foo.nc?cloudydap=request_id");
        }
        catch(BESError &be){
            CPPUNIT_FAIL(prolog + be.get_message());
        }
        catch(std::exception e){
            CPPUNIT_FAIL(prolog + "Caught std::exception. Message: "+e.what());
        }
    }
#endif

    void add_tracking_query_param_test_5_1()
    {
        DBG(cerr << prolog << "BEGIN" << endl);
        try {
            // No context, S3 URL, non-default ctor
            BESContextManager::TheManager()->unset_context(S3_TRACKING_CONTEXT);
            shared_ptr<http::url> sotr(new http::url("http://s3.amazonaws.com/somewhereovertherainbow"));
            unique_ptr<Chunk> l_chunk(new Chunk(sotr, "", 100, 10, ""));

            CPPUNIT_ASSERT(l_chunk->d_query_marker.empty());

            auto data_url = l_chunk->get_data_url();

            DBG(cerr << prolog << "data_url: " << data_url << endl);
            CPPUNIT_ASSERT(!data_url->str().empty());
            CPPUNIT_ASSERT(data_url->str() == "http://s3.amazonaws.com/somewhereovertherainbow");
        }
        catch(BESError &be){
            CPPUNIT_FAIL(prolog + be.get_message());
        }
        catch(std::exception e){
            CPPUNIT_FAIL(prolog + "Caught std::exception. Message: "+e.what());
        }

    }
#endif

   CPPUNIT_TEST_SUITE( ChunkTest );

    CPPUNIT_TEST(set_position_in_array_test);
    CPPUNIT_TEST(set_position_in_array_test_2);

    CPPUNIT_TEST_EXCEPTION(set_position_in_array_test_3, BESError);
    CPPUNIT_TEST_EXCEPTION(set_position_in_array_test_4, BESError);
    CPPUNIT_TEST_EXCEPTION(set_position_in_array_test_5, BESError);
    CPPUNIT_TEST_EXCEPTION(set_position_in_array_test_6, BESError);

    CPPUNIT_TEST(test_process_s3_error_response_1);
    CPPUNIT_TEST(test_process_s3_error_response_2);
    CPPUNIT_TEST(test_process_s3_error_response_3);
    CPPUNIT_TEST(test_process_s3_error_response_4);

    CPPUNIT_TEST(filter_chunk_lzw_test);
    CPPUNIT_TEST(filter_chunk_lzw_element_width_test);
    CPPUNIT_TEST(filter_chunk_lzw_short_chunk_test);
    CPPUNIT_TEST(filter_chunk_lzw_longer_stream_test);
    CPPUNIT_TEST(filter_chunk_lzw_is_done_once_test);

    CPPUNIT_TEST(filter_chunk_tiffpred_lzw_little_endian_test);
    CPPUNIT_TEST(filter_chunk_tiffpred_lzw_big_endian_test);
    CPPUNIT_TEST(filter_chunk_tiffpred_lzw_three_bands_test);
    CPPUNIT_TEST(filter_chunk_tiffpred_lzw_float32_test);
    CPPUNIT_TEST(filter_chunk_tiffpred_short_chunk_test);
    CPPUNIT_TEST(filter_chunk_tiffpred_only_test);
    CPPUNIT_TEST(filter_chunk_tiffpred_deflate_test);

    CPPUNIT_TEST_EXCEPTION(filter_chunk_lzw_error_test_1, BESError);
    CPPUNIT_TEST_EXCEPTION(filter_chunk_lzw_error_test_2, BESError);
    CPPUNIT_TEST_EXCEPTION(filter_chunk_lzw_error_test_3, BESError);
    CPPUNIT_TEST_EXCEPTION(filter_chunk_tiffpred_error_test_1, BESError);
    CPPUNIT_TEST_EXCEPTION(filter_chunk_tiffpred_error_test_2, BESError);
    CPPUNIT_TEST_EXCEPTION(filter_chunk_tiffpred_error_test_3, BESError);
    CPPUNIT_TEST_EXCEPTION(filter_chunk_tiffpred_error_test_4, BESError);
    CPPUNIT_TEST_EXCEPTION(filter_chunk_tiffpred_error_test_5, BESError);
    CPPUNIT_TEST_EXCEPTION(filter_chunk_tiffpred_error_test_6, BESError);
    CPPUNIT_TEST_EXCEPTION(filter_chunk_tiffpred_error_test_7, BESError);
    CPPUNIT_TEST_EXCEPTION(filter_chunk_tiffpred_error_test_8, BESError);
    CPPUNIT_TEST_EXCEPTION(filter_chunk_tiffpred_error_test_9, BESError);
    CPPUNIT_TEST_EXCEPTION(filter_chunk_tiffpred_error_test_10, BESError);
    CPPUNIT_TEST(filter_chunk_error_messages_test);

#if ENABLE_TRACKING_QUERY_PARAMETER
        CPPUNIT_TEST(add_tracking_query_param_test);
#if 0
        CPPUNIT_TEST(add_tracking_query_param_test_2);
#endif
    CPPUNIT_TEST(add_tracking_query_param_test_3);
    CPPUNIT_TEST(add_tracking_query_param_test_4);
    CPPUNIT_TEST(add_tracking_query_param_test_4_1);

#if ENABLE_TRACKING_QUERY_PARAMETER
    CPPUNIT_TEST(add_tracking_query_param_test_5);
#endif

    CPPUNIT_TEST(add_tracking_query_param_test_5_1);
#endif

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ChunkTest);

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
            test = dmrpp::ChunkTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
