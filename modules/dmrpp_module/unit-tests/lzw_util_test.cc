// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of bes, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2026 OPeNDAP, Inc.
// Author: Samuel Lloyd
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

// Tests for lzw_util.{h,cc}: the TIFF-variant LZW decoder and the TIFF predictor
// reversal used for GeoTIFF-derived DMR++ chunks.
//
// Where the expected values come from:
//   - Streams built by hand from the LZW definition (literals, table references, the
//     "code == next free entry" case, consecutive CLEAR codes, error cases).
//   - A stream produced by libtiff (tiffcp -c lzw) -- an independent encoder.
//   - Floating point predictor vectors produced by imagecodecs and checked against a
//     second implementation written from TIFF Technical Note 3.
//   - Predictor 2 values worked out by hand.
//   - Round trips through the small reference encoder/forward predictors in this file,
//     which cover the cases that are too big to embed (code width changes, a full
//     string table, long runs). The encoder is itself checked against the libtiff stream
//     (test_reference_encoder_matches_libtiff) so it is not just testing the decoder
//     against itself.

#include <cstdint>
#include <cstring>
#include <algorithm>
#include <iostream>     // std::cout, std::endl
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include "lzw_util.h"

#include "modules/common/run_tests_cppunit.h"

using namespace std;

#define prolog std::string("lzw_util_test::").append(__func__).append("() - ")

namespace dmrpp {

    namespace {

        typedef vector<uint8_t> bytes;
        typedef vector<pair<unsigned, unsigned> > code_list;    // (code, width in bits)

        const unsigned CLEAR = 256;
        const unsigned EOI = 257;

// 900 bytes from lcg_data(900, 5) compressed by libtiff (tiffcp -c lzw). The stream is
// long enough that the code width changes from 9 to 10 bits part way through.
        const uint8_t libtiff_lzw_vector[] = {
                0x80, 0x00, 0x00, 0x80, 0x40, 0x10, 0x08, 0x06, 0x01, 0x00, 0x00, 0x80,
                0x10, 0xB8, 0x34, 0x12, 0x10, 0x03, 0x01, 0xC0, 0xC0, 0x70, 0x68, 0x9C,
                0x12, 0x22, 0x01, 0x8B, 0xC6, 0x20, 0x80, 0x28, 0x18, 0x02, 0x2E, 0x02,
                0x00, 0xC2, 0xA3, 0x60, 0x48, 0x98, 0x06, 0x30, 0x03, 0x00, 0x42, 0x21,
                0x00, 0x49, 0x3C, 0x2E, 0x21, 0x09, 0x94, 0x41, 0xA3, 0xD2, 0x28, 0x7C,
                0xA6, 0x49, 0x27, 0x99, 0x43, 0xA4, 0x32, 0xE8, 0x84, 0xA2, 0x3F, 0x32,
                0x97, 0x43, 0x26, 0x72, 0x08, 0x3C, 0x8A, 0x65, 0x05, 0x84, 0xC2, 0xE5,
                0x31, 0x19, 0xB5, 0x12, 0x2D, 0x09, 0x82, 0xC8, 0x29, 0xF1, 0xE8, 0x8C,
                0xA2, 0x4D, 0x1B, 0x8D, 0x48, 0x65, 0x10, 0x2A, 0x4C, 0x0E, 0x1B, 0x08,
                0x85, 0x42, 0xA2, 0x11, 0x88, 0x34, 0x9A, 0x4D, 0x51, 0x88, 0xD8, 0x68,
                0xB4, 0x19, 0x24, 0x40, 0x09, 0x50, 0xB1, 0xC9, 0x6A, 0xD2, 0x9A, 0x38,
                0x06, 0x3B, 0x45, 0x8A, 0x47, 0x26, 0x12, 0x4A, 0x8C, 0x36, 0xAD, 0x07,
                0x98, 0xDD, 0xAD, 0xF0, 0x59, 0x65, 0xDA, 0x05, 0x04, 0x8F, 0x4B, 0x65,
                0x30, 0xC9, 0x05, 0x22, 0xAF, 0x09, 0x93, 0x53, 0x2A, 0xF2, 0x88, 0x94,
                0x63, 0x13, 0x4B, 0x99, 0xE4, 0x2B, 0x93, 0x08, 0x9C, 0x1F, 0x0F, 0x7C,
                0x8F, 0xC3, 0xE1, 0x50, 0x2A, 0x24, 0xCF, 0x3D, 0x12, 0x92, 0x4B, 0x28,
                0xB1, 0x29, 0x74, 0x12, 0x47, 0x4A, 0x87, 0xE4, 0x6D, 0xB5, 0x28, 0x5C,
                0x0E, 0x07, 0x82, 0xBB, 0x4A, 0xB5, 0x91, 0xCC, 0x6E, 0x93, 0x3B, 0x88,
                0xD5, 0x4B, 0x2F, 0x30, 0xED, 0xE4, 0xE2, 0x29, 0xC2, 0xAE, 0x55, 0x61,
                0x91, 0x6E, 0x44, 0x2A, 0x57, 0x3D, 0xC8, 0xC7, 0x2A, 0xF2, 0xCC, 0x1C,
                0xB2, 0x61, 0x67, 0x92, 0xE4, 0x2D, 0xDB, 0xC9, 0xF5, 0xBB, 0x0F, 0x06,
                0xA4, 0x49, 0x2E, 0x96, 0xE8, 0xAF, 0x87, 0x4F, 0x1E, 0xA4, 0xD1, 0x33,
                0xB5, 0x4C, 0x5E, 0x7A, 0x3D, 0x6D, 0xE1, 0x70, 0xB4, 0x5D, 0x4B, 0x2D,
                0x8A, 0x0B, 0x64, 0xCA, 0xC7, 0x27, 0xB6, 0x28, 0x87, 0x82, 0xCC, 0xB7,
                0xAA, 0xCF, 0xC3, 0x2C, 0xB5, 0x28, 0x2A, 0xA2, 0xD2, 0x9F, 0x29, 0x08,
                0x12, 0xEC, 0x82, 0xAC, 0x6D, 0xCA, 0xDC, 0xCB, 0x36, 0xEB, 0x02, 0xA8,
                0x8A, 0x32, 0xEF, 0x52, 0x26, 0xF4, 0x2B, 0x6D, 0x1B, 0xEE, 0xB0, 0x35,
                0xC8, 0x13, 0x28, 0xC7, 0x2A, 0x0B, 0x53, 0x10, 0xB0, 0xA1, 0x08, 0x4B,
                0xE4, 0xB7, 0x44, 0x68, 0x62, 0x56, 0xDE, 0x3C, 0x11, 0x33, 0x16, 0xCB,
                0x26, 0x2E, 0x5A, 0xDE, 0xD7, 0x31, 0xAC, 0x1B, 0x28, 0xAD, 0x31, 0xCE,
                0x42, 0x53, 0x12, 0xA4, 0x11, 0x32, 0xBD, 0x1D, 0xBF, 0x4D, 0x9A, 0xA9,
                0x00, 0x3F, 0xAA, 0x92, 0xA4, 0xE8, 0xB7, 0x31, 0x9B, 0x9E, 0x9D, 0xAD,
                0xB2, 0x4B, 0x08, 0xB1, 0x3D, 0x08, 0x5A, 0x42, 0xC1, 0x23, 0xAE, 0x12,
                0x40, 0x8D, 0x3D, 0x08, 0x08
        };

// Deterministic test data. The same generator was used to make the libtiff vector above.
        bytes lcg_data(size_t n, unsigned alphabet, uint32_t seed = 12345)
        {
            bytes data(n);
            uint32_t x = seed;
            for (size_t i = 0; i < n; ++i) {
                x = x * 1664525u + 1013904223u;
                data[i] = static_cast<uint8_t>((x >> 24) % alphabet);
            }
            return data;
        }

// Writes codes of varying width, most significant bit first, as TIFF LZW does.
        class bit_writer {
            bytes d_out;
            uint32_t d_acc = 0;
            unsigned d_nbits = 0;

        public:
            void put(unsigned code, unsigned width)
            {
                d_acc = (d_acc << width) | code;
                d_nbits += width;
                while (d_nbits >= 8) {
                    d_out.push_back(static_cast<uint8_t>((d_acc >> (d_nbits - 8)) & 0xFF));
                    d_nbits -= 8;
                }
                d_acc &= (1u << d_nbits) - 1;
            }

            bytes finish()
            {
                if (d_nbits > 0) {
                    d_out.push_back(static_cast<uint8_t>((d_acc << (8 - d_nbits)) & 0xFF));
                    d_nbits = 0;
                    d_acc = 0;
                }
                return d_out;
            }
        };

        bytes pack_codes(const code_list &codes)
        {
            bit_writer w;
            for (const auto &c: codes)
                w.put(c.first, c.second);
            return w.finish();
        }

/**
 * A small TIFF-variant LZW encoder, used only to make test data. It follows the conventions of
 * libtiff's encoder (9..12 bit codes, a CLEAR code first, a CLEAR code when the table is about to
 * be full, and the width increase one code early). Unlike libtiff it never clears the table because
 * of a falling compression ratio, which is only an optimization.
 */
        bytes lzw_encode(const bytes &in)
        {
            bit_writer w;
            unsigned nbits = 9;
            unsigned maxcode = 511;
            unsigned free_ent = 258;
            unordered_map<uint32_t, uint16_t> table;

            w.put(CLEAR, nbits);
            if (in.empty()) {
                w.put(EOI, nbits);
                return w.finish();
            }

            unsigned ent = in[0];
            for (size_t i = 1; i < in.size(); ++i) {
                const uint8_t c = in[i];
                const uint32_t key = (ent << 8) | c;
                auto it = table.find(key);
                if (it != table.end()) {
                    ent = it->second;
                    continue;
                }

                w.put(ent, nbits);
                table[key] = static_cast<uint16_t>(free_ent++);
                ent = c;

                if (free_ent == 4094) {         // table is (nearly) full; start over
                    table.clear();
                    free_ent = 258;
                    w.put(CLEAR, nbits);
                    nbits = 9;
                    maxcode = 511;
                }
                else if (free_ent > maxcode) {
                    ++nbits;
                    maxcode = (1u << nbits) - 1;
                }
            }

            w.put(ent, nbits);
            ++free_ent;
            if (free_ent == 4094) {
                w.put(CLEAR, nbits);
                nbits = 9;
            }
            else if (free_ent > maxcode) {
                ++nbits;
            }
            w.put(EOI, nbits);

            return w.finish();
        }

        uint64_t load_uint(const uint8_t *p, unsigned n, bool big_endian)
        {
            uint64_t v = 0;
            for (unsigned i = 0; i < n; ++i)
                v |= static_cast<uint64_t>(p[i]) << (8 * (big_endian ? n - 1 - i : i));
            return v;
        }

        void store_uint(uint8_t *p, unsigned n, bool big_endian, uint64_t v)
        {
            for (unsigned i = 0; i < n; ++i)
                p[i] = static_cast<uint8_t>((v >> (8 * (big_endian ? n - 1 - i : i))) & 0xFF);
        }

// The forward (writing) side of TIFF predictor 2; the inverse of tiff_predictor_decode().
        void predictor2_encode(bytes &data, unsigned bps, unsigned spp, size_t row_bytes, bool big_endian)
        {
            for (size_t row = 0; row < data.size(); row += row_bytes) {
                const size_t nsamples = row_bytes / bps;
                // Work from the end of the row so each difference uses the original previous sample.
                for (size_t s = nsamples; s-- > spp;) {
                    const uint64_t cur = load_uint(&data[row + s * bps], bps, big_endian);
                    const uint64_t prev = load_uint(&data[row + (s - spp) * bps], bps, big_endian);
                    store_uint(&data[row + s * bps], bps, big_endian, cur - prev);
                }
            }
        }

// The forward side of TIFF predictor 3. The samples in 'data' are in the byte order given by big_endian.
        void predictor3_encode(bytes &data, unsigned bps, unsigned spp, size_t row_bytes, bool big_endian)
        {
            for (size_t row = 0; row < data.size(); row += row_bytes) {
                const size_t wc = row_bytes / bps;
                const bytes samples(data.begin() + row, data.begin() + row + row_bytes);
                bytes planes(row_bytes);
                for (size_t k = 0; k < wc; ++k)
                    for (unsigned p = 0; p < bps; ++p)  // plane 0 holds the most significant bytes
                        planes[p * wc + k] = samples[k * bps + (big_endian ? p : bps - 1 - p)];
                for (size_t i = row_bytes; i-- > spp;)
                    data[row + i] = static_cast<uint8_t>(planes[i] - planes[i - spp]);
                for (size_t i = 0; i < spp && i < row_bytes; ++i)
                    data[row + i] = planes[i];
            }
        }

// Host byte order independent conversion of floating point values to bytes.
        bytes float32_bytes(const vector<float> &values, bool big_endian)
        {
            bytes out(values.size() * 4);
            for (size_t i = 0; i < values.size(); ++i) {
                uint32_t u;
                memcpy(&u, &values[i], sizeof(u));
                store_uint(&out[i * 4], 4, big_endian, u);
            }
            return out;
        }

        bytes float64_bytes(const vector<double> &values, bool big_endian)
        {
            bytes out(values.size() * 8);
            for (size_t i = 0; i < values.size(); ++i) {
                uint64_t u;
                memcpy(&u, &values[i], sizeof(u));
                store_uint(&out[i * 8], 8, big_endian, u);
            }
            return out;
        }

    } // anonymous namespace

    class lzw_util_test: public CppUnit::TestFixture {

    private:

        // Decode 'stream' into a buffer of 'capacity' bytes that starts out filled with 0xFF so that
        // zero-filling by lzw_decode() is visible.
        static lzw_status decode(const bytes &stream, size_t capacity, bytes &out, size_t &out_len)
        {
            out.assign(capacity, 0xFF);
            out_len = 12345;
            return lzw_decode(stream.empty() ? nullptr : &stream[0], stream.size(),
                              out.empty() ? nullptr : &out[0], out.size(), &out_len);
        }

        // Encode 'plain' with the reference encoder and check that decoding gives it back.
        void check_lzw_round_trip(const bytes &plain, const string &name)
        {
            const bytes encoded = lzw_encode(plain);
            bytes out;
            size_t out_len = 0;
            lzw_status status = decode(encoded, plain.size(), out, out_len);

            DBG(cerr << prolog << name << ": plain " << plain.size() << " bytes, encoded " << encoded.size()
                     << " bytes, status " << lzw_status_str(status) << endl);

            CPPUNIT_ASSERT_MESSAGE(prolog + name + ": status", status == LZW_OK);
            CPPUNIT_ASSERT_MESSAGE(prolog + name + ": decoded length", out_len == plain.size());
            CPPUNIT_ASSERT_MESSAGE(prolog + name + ": decoded data", out == plain);
        }

        void check_predictor2_known(const bytes &stored, const bytes &expected, unsigned bps, unsigned spp,
                                    size_t row_bytes, bool big_endian)
        {
            bytes data = stored;
            tiff_predictor_status status = tiff_predictor_decode(&data[0], data.size(), 2, bps, spp, row_bytes, big_endian);
            CPPUNIT_ASSERT_MESSAGE(prolog + "status", status == TIFF_PREDICTOR_OK);
            CPPUNIT_ASSERT_MESSAGE(prolog + "data", data == expected);
        }

        void check_predictor3_known(const bytes &stored, const bytes &expected_values_le, const bytes &expected_values_be,
                                    unsigned bps, unsigned spp, size_t row_bytes)
        {
            // The stored bytes are the same whatever the file byte order; big_endian chooses the output order.
            bytes le = stored;
            CPPUNIT_ASSERT(tiff_predictor_decode(&le[0], le.size(), 3, bps, spp, row_bytes, false) == TIFF_PREDICTOR_OK);
            CPPUNIT_ASSERT_MESSAGE(prolog + "little endian output", le == expected_values_le);

            bytes be = stored;
            CPPUNIT_ASSERT(tiff_predictor_decode(&be[0], be.size(), 3, bps, spp, row_bytes, true) == TIFF_PREDICTOR_OK);
            CPPUNIT_ASSERT_MESSAGE(prolog + "big endian output", be == expected_values_be);
        }

    public:
        // Called once before everything gets tested
        lzw_util_test() = default;

        // Called at the end of the test
        ~lzw_util_test() override = default;

        // Called before each test
        void setUp() override
        {
            DBG(cerr << "\n");
        }

        // Called after each test
        void tearDown() override
        {
        }

        // ---------------------------------------------------------------------
        // lzw_decode(): streams built by hand
        // ---------------------------------------------------------------------

        void test_lzw_decode_literals()
        {
            const bytes stream = pack_codes({{CLEAR, 9}, {'A', 9}, {'B', 9}, {'C', 9}, {EOI, 9}});
            bytes out;
            size_t len = 0;

            CPPUNIT_ASSERT(decode(stream, 3, out, len) == LZW_OK);
            CPPUNIT_ASSERT(len == 3);
            CPPUNIT_ASSERT(out == bytes({'A', 'B', 'C'}));
        }

        void test_lzw_decode_string_reference()
        {
            // 'A', 'B' define entry 258 = "AB"; code 258 then writes it out.
            const bytes stream = pack_codes({{CLEAR, 9}, {'A', 9}, {'B', 9}, {258, 9}, {EOI, 9}});
            bytes out;
            size_t len = 0;

            CPPUNIT_ASSERT(decode(stream, 4, out, len) == LZW_OK);
            CPPUNIT_ASSERT(out == bytes({'A', 'B', 'A', 'B'}));
        }

        void test_lzw_decode_kwkwk()
        {
            // Code 258 is used by the encoder before the decoder could have defined it: the special
            // "code == next free entry" case. The string is the previous string plus its own first byte.
            const bytes stream = pack_codes({{CLEAR, 9}, {'A', 9}, {258, 9}, {EOI, 9}});
            bytes out;
            size_t len = 0;

            CPPUNIT_ASSERT(decode(stream, 3, out, len) == LZW_OK);
            CPPUNIT_ASSERT(out == bytes({'A', 'A', 'A'}));
        }

        void test_lzw_decode_consecutive_clear_codes()
        {
            const bytes stream = pack_codes({{CLEAR, 9}, {CLEAR, 9}, {CLEAR, 9}, {'A', 9}, {EOI, 9}});
            bytes out;
            size_t len = 0;

            CPPUNIT_ASSERT(decode(stream, 1, out, len) == LZW_OK);
            CPPUNIT_ASSERT(out == bytes({'A'}));
        }

        void test_lzw_decode_clear_resets_the_table()
        {
            // After the second CLEAR, entry 258 is redefined as "XY". If the table were not reset it would
            // still be "AB" and the result would end in "AB" instead of "XY".
            const bytes stream = pack_codes({{CLEAR, 9}, {'A', 9}, {'B', 9}, {CLEAR, 9}, {'X', 9}, {'Y', 9}, {258, 9}, {EOI, 9}});
            bytes out;
            size_t len = 0;

            CPPUNIT_ASSERT(decode(stream, 6, out, len) == LZW_OK);
            CPPUNIT_ASSERT(out == bytes({'A', 'B', 'X', 'Y', 'X', 'Y'}));
        }

        // ---------------------------------------------------------------------
        // lzw_decode(): a stream written by libtiff
        // ---------------------------------------------------------------------

        void test_lzw_decode_libtiff_vector()
        {
            const bytes plain = lcg_data(900, 5);
            const bytes stream(libtiff_lzw_vector, libtiff_lzw_vector + sizeof(libtiff_lzw_vector));
            bytes out;
            size_t len = 0;

            lzw_status status = decode(stream, plain.size(), out, len);
            DBG(cerr << prolog << "compressed " << stream.size() << " bytes, status " << lzw_status_str(status) << endl);

            CPPUNIT_ASSERT(status == LZW_OK);
            CPPUNIT_ASSERT(len == plain.size());
            CPPUNIT_ASSERT(out == plain);
        }

        void test_reference_encoder_matches_libtiff()
        {
            // If this fails the encoder used for the round trip tests no longer follows libtiff's conventions
            // (most likely the point at which the code width changes).
            const bytes stream(libtiff_lzw_vector, libtiff_lzw_vector + sizeof(libtiff_lzw_vector));
            CPPUNIT_ASSERT(lzw_encode(lcg_data(900, 5)) == stream);
        }

        // ---------------------------------------------------------------------
        // lzw_decode(): round trips
        // ---------------------------------------------------------------------

        void test_lzw_decode_round_trip_small()
        {
            check_lzw_round_trip(bytes({'A'}), "one byte");
            check_lzw_round_trip(bytes({'A', 'A'}), "two equal bytes");
            check_lzw_round_trip(bytes({0, 255, 0, 255, 1}), "binary bytes");

            const string s = "TOBEORNOTTOBEORTOBEORNOT";
            check_lzw_round_trip(bytes(s.begin(), s.end()), "TOBEORNOT");
        }

        void test_lzw_decode_round_trip_long_runs()
        {
            // Every string reference is the "code == next free entry" case.
            check_lzw_round_trip(bytes(100000, 0), "100000 zeros");
            check_lzw_round_trip(bytes(5000, 0x7F), "5000 x 0x7F");
        }

        void test_lzw_decode_round_trip_code_width_changes()
        {
            // Several thousand codes: the width goes from 9 to 10, 11 and 12 bits.
            check_lzw_round_trip(lcg_data(6000, 6), "6000 bytes, 6 symbols");
            check_lzw_round_trip(lcg_data(20000, 3), "20000 bytes, 3 symbols");
        }

        void test_lzw_decode_round_trip_full_table()
        {
            // Close to one code per byte, so the string table fills up (and a CLEAR code is
            // written) about ten times.
            check_lzw_round_trip(lcg_data(40000, 256), "40000 random bytes");
        }

        void test_lzw_decode_round_trip_image_like_data()
        {
            // Smooth rows with a little noise, as in a satellite image strip.
            bytes plain(64 * 512);
            const bytes noise = lcg_data(plain.size(), 4);
            for (size_t i = 0; i < plain.size(); ++i)
                plain[i] = static_cast<uint8_t>((i % 512) / 4 + noise[i]);
            check_lzw_round_trip(plain, "image-like strip");
        }

        // ---------------------------------------------------------------------
        // lzw_decode(): output buffer sizes
        // ---------------------------------------------------------------------

        void test_lzw_decode_short_output()
        {
            // The stream ends before the buffer is full: not a failure, the rest is zero filled.
            const bytes stream = pack_codes({{CLEAR, 9}, {'A', 9}, {'B', 9}, {'C', 9}, {EOI, 9}});
            bytes out;
            size_t len = 0;

            CPPUNIT_ASSERT(decode(stream, 10, out, len) == LZW_SHORT_OUTPUT);
            CPPUNIT_ASSERT(len == 3);
            CPPUNIT_ASSERT(out == bytes({'A', 'B', 'C', 0, 0, 0, 0, 0, 0, 0}));
        }

        void test_lzw_decode_empty_image()
        {
            const bytes stream = pack_codes({{CLEAR, 9}, {EOI, 9}});
            bytes out;
            size_t len = 0;

            CPPUNIT_ASSERT(decode(stream, 4, out, len) == LZW_SHORT_OUTPUT);
            CPPUNIT_ASSERT(len == 0);
            CPPUNIT_ASSERT(out == bytes(4, 0));
        }

        void test_lzw_decode_output_smaller_than_stream()
        {
            // The stream holds more data than the buffer: the result is the start of the data, and there
            // is no error (libtiff behaves the same way), including when a string is cut in the middle.
            const bytes plain = lcg_data(900, 5);
            const bytes stream(libtiff_lzw_vector, libtiff_lzw_vector + sizeof(libtiff_lzw_vector));

            for (size_t capacity: {1, 2, 3, 100, 457, 899}) {
                bytes out;
                size_t len = 0;
                CPPUNIT_ASSERT_MESSAGE(prolog + "status, capacity " + to_string(capacity),
                                       decode(stream, capacity, out, len) == LZW_OK);
                CPPUNIT_ASSERT_MESSAGE(prolog + "length, capacity " + to_string(capacity), len == capacity);
                CPPUNIT_ASSERT_MESSAGE(prolog + "data, capacity " + to_string(capacity),
                                       out == bytes(plain.begin(), plain.begin() + capacity));
            }
        }

        void test_lzw_decode_zero_capacity()
        {
            const bytes stream(libtiff_lzw_vector, libtiff_lzw_vector + sizeof(libtiff_lzw_vector));
            size_t len = 12345;

            CPPUNIT_ASSERT(lzw_decode(&stream[0], stream.size(), nullptr, 0, &len) == LZW_OK);
            CPPUNIT_ASSERT(len == 0);
        }

        void test_lzw_decode_dst_len_is_optional()
        {
            const bytes stream = pack_codes({{CLEAR, 9}, {'A', 9}, {EOI, 9}});
            uint8_t out[1] = {0};

            CPPUNIT_ASSERT(lzw_decode(&stream[0], stream.size(), out, 1, nullptr) == LZW_OK);
            CPPUNIT_ASSERT(out[0] == 'A');
        }

        // ---------------------------------------------------------------------
        // lzw_decode(): bad input must be reported, and must never write outside the buffer
        // ---------------------------------------------------------------------

        void test_lzw_decode_error_no_clear_code()
        {
            const bytes stream = pack_codes({{'A', 9}, {'B', 9}, {EOI, 9}});
            bytes out;
            size_t len = 0;

            CPPUNIT_ASSERT(decode(stream, 4, out, len) == LZW_ERR_NO_CLEAR);
            CPPUNIT_ASSERT(out == bytes(4, 0));
        }

        void test_lzw_decode_error_code_not_in_table()
        {
            // 400 is far beyond the entries defined so far.
            const bytes stream = pack_codes({{CLEAR, 9}, {'A', 9}, {400, 9}, {EOI, 9}});
            bytes out;
            size_t len = 0;

            CPPUNIT_ASSERT(decode(stream, 8, out, len) == LZW_ERR_BAD_CODE);
            CPPUNIT_ASSERT(len == 1);
            CPPUNIT_ASSERT(out[0] == 'A');
            CPPUNIT_ASSERT(bytes(out.begin() + 1, out.end()) == bytes(7, 0));
        }

        void test_lzw_decode_error_first_code_not_a_literal()
        {
            const bytes stream = pack_codes({{CLEAR, 9}, {300, 9}, {EOI, 9}});
            bytes out;
            size_t len = 0;

            CPPUNIT_ASSERT(decode(stream, 8, out, len) == LZW_ERR_BAD_CODE);
            CPPUNIT_ASSERT(len == 0);
        }

        void test_lzw_decode_error_truncated_input()
        {
            // Half of a valid stream: no EOI and not enough data to fill the buffer.
            const bytes plain = lcg_data(900, 5);
            const bytes whole(libtiff_lzw_vector, libtiff_lzw_vector + sizeof(libtiff_lzw_vector));
            const bytes stream(whole.begin(), whole.begin() + whole.size() / 2);
            bytes out;
            size_t len = 0;

            CPPUNIT_ASSERT(decode(stream, plain.size(), out, len) == LZW_ERR_TRUNCATED_INPUT);
            CPPUNIT_ASSERT(len > 0 && len < plain.size());
            // What was decoded is correct, and the rest is zero.
            CPPUNIT_ASSERT(bytes(out.begin(), out.begin() + len) == bytes(plain.begin(), plain.begin() + len));
            CPPUNIT_ASSERT(bytes(out.begin() + len, out.end()) == bytes(plain.size() - len, 0));
        }

        void test_lzw_decode_error_empty_input()
        {
            bytes out;
            size_t len = 0;

            CPPUNIT_ASSERT(decode(bytes(), 8, out, len) == LZW_ERR_TRUNCATED_INPUT);
            CPPUNIT_ASSERT(len == 0);
        }

        void test_lzw_decode_error_old_style_encoding()
        {
            // Very old libtiff versions wrote the codes least significant bit first. They are
            // recognized (a leading 0 byte followed by an odd byte) but not decoded.
            const bytes stream = {0x00, 0x01, 0x02, 0x03, 0x04};
            bytes out;
            size_t len = 0;

            CPPUNIT_ASSERT(decode(stream, 8, out, len) == LZW_ERR_OLD_STYLE);
            CPPUNIT_ASSERT(out == bytes(8, 0));
        }

        void test_lzw_decode_error_null_arguments()
        {
            const bytes stream = pack_codes({{CLEAR, 9}, {'A', 9}, {EOI, 9}});
            uint8_t out[4];
            size_t len = 0;

            CPPUNIT_ASSERT(lzw_decode(nullptr, stream.size(), out, sizeof(out), &len) == LZW_ERR_BAD_ARGS);
            CPPUNIT_ASSERT(lzw_decode(&stream[0], stream.size(), nullptr, sizeof(out), &len) == LZW_ERR_BAD_ARGS);
        }

        void test_lzw_decode_never_writes_past_the_buffer()
        {
            // Damage a valid stream in many ways and decode into a buffer with guard bytes on both sides.
            const bytes whole(libtiff_lzw_vector, libtiff_lzw_vector + sizeof(libtiff_lzw_vector));
            const size_t capacity = 900;
            const size_t guard = 16;
            const bytes noise = lcg_data(whole.size() * 4, 256, 99);

            for (size_t trial = 0; trial < 400; ++trial) {
                bytes stream = whole;
                stream[(trial * 7) % stream.size()] ^= static_cast<uint8_t>(1u << (trial % 8));
                stream[noise[trial] % stream.size()] = noise[trial + 1];
                if (trial % 5 == 0)
                    stream.resize(noise[trial + 2] % stream.size());

                bytes buf(capacity + 2 * guard, 0xAB);
                size_t len = 0;
                lzw_status status = lzw_decode(stream.empty() ? nullptr : &stream[0], stream.size(), &buf[guard], capacity, &len);

                CPPUNIT_ASSERT_MESSAGE(prolog + "length", len <= capacity);
                CPPUNIT_ASSERT_MESSAGE(prolog + "status " + lzw_status_str(status),
                                       bytes(buf.begin(), buf.begin() + guard) == bytes(guard, 0xAB));
                CPPUNIT_ASSERT_MESSAGE(prolog + "guard after the buffer",
                                       bytes(buf.end() - guard, buf.end()) == bytes(guard, 0xAB));
                CPPUNIT_ASSERT_MESSAGE(prolog + "zero fill after decoded data",
                                       bytes(buf.begin() + guard + len, buf.begin() + guard + capacity)
                                       == bytes(capacity - len, 0));
            }
        }

        void test_lzw_status_str()
        {
            const lzw_status all[] = {LZW_OK, LZW_SHORT_OUTPUT, LZW_ERR_BAD_ARGS, LZW_ERR_OLD_STYLE, LZW_ERR_NO_CLEAR,
                                      LZW_ERR_BAD_CODE, LZW_ERR_TRUNCATED_INPUT};
            set<string> seen;
            for (lzw_status s: all) {
                const char *msg = lzw_status_str(s);
                CPPUNIT_ASSERT(msg != nullptr);
                CPPUNIT_ASSERT(string(msg).size() > 0);
                seen.insert(msg);
            }
            CPPUNIT_ASSERT_MESSAGE(prolog + "each status has its own message", seen.size() == sizeof(all) / sizeof(all[0]));
        }

        // ---------------------------------------------------------------------
        // tiff_predictor_decode(): predictor 1 and 2, values worked out by hand
        // ---------------------------------------------------------------------

        void test_predictor_1_does_nothing()
        {
            bytes data = {1, 2, 3, 4, 5, 6};
            CPPUNIT_ASSERT(tiff_predictor_decode(&data[0], data.size(), 1, 1, 1, 6, false) == TIFF_PREDICTOR_OK);
            CPPUNIT_ASSERT(data == bytes({1, 2, 3, 4, 5, 6}));
        }

        void test_predictor_2_8bit()
        {
            // 10, then +1, +1, +2, +255 (that is, -1, so 14 -> 13), +1
            check_predictor2_known({10, 1, 1, 2, 255, 1}, {10, 11, 12, 14, 13, 14}, 1, 1, 6, false);
        }

        void test_predictor_2_8bit_wraps_modulo_256()
        {
            check_predictor2_known({250, 10, 250}, {250, 4, 254}, 1, 1, 3, false);
        }

        void test_predictor_2_8bit_three_bands()
        {
            // Pixels of three interleaved samples: each is added to the same band of the previous pixel.
            check_predictor2_known({10, 20, 30, 1, 2, 3, 1, 2, 3}, {10, 20, 30, 11, 22, 33, 12, 24, 36}, 1, 3, 9, false);
        }

        void test_predictor_2_16bit_little_endian()
        {
            // Differences 0x1000, +1, -1 (0xFFFF), +2  ->  0x1000, 0x1001, 0x1000, 0x1002
            check_predictor2_known({0x00, 0x10, 0x01, 0x00, 0xFF, 0xFF, 0x02, 0x00},
                                   {0x00, 0x10, 0x01, 0x10, 0x00, 0x10, 0x02, 0x10}, 2, 1, 8, false);
        }

        void test_predictor_2_16bit_big_endian()
        {
            // The same values as above, stored most significant byte first.
            check_predictor2_known({0x10, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0x00, 0x02},
                                   {0x10, 0x00, 0x10, 0x01, 0x10, 0x00, 0x10, 0x02}, 2, 1, 8, true);
        }

        void test_predictor_2_16bit_wraps_modulo_65536()
        {
            // 0xFFFF + 1 is 0, not 0x10000: the carry must not spill into the next sample.
            check_predictor2_known({0xFF, 0xFF, 0x01, 0x00, 0x05, 0x00}, {0xFF, 0xFF, 0x00, 0x00, 0x05, 0x00}, 2, 1, 6, false);
            check_predictor2_known({0xFF, 0xFF, 0x00, 0x01, 0x00, 0x05}, {0xFF, 0xFF, 0x00, 0x00, 0x00, 0x05}, 2, 1, 6, true);
        }

        void test_predictor_2_16bit_three_bands()
        {
            // 3 bands, little endian: (1,2,3) then (+1,+1,+1) -> (2,3,4)
            check_predictor2_known({1, 0, 2, 0, 3, 0, 1, 0, 1, 0, 1, 0}, {1, 0, 2, 0, 3, 0, 2, 0, 3, 0, 4, 0}, 2, 3, 12, false);
        }

        void test_predictor_2_32bit()
        {
            // Little endian: 1, then +0xFFFFFFFF (-1) -> 0, then +5 -> 5
            check_predictor2_known({0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0x00, 0x00, 0x00},
                                   {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00}, 4, 1, 12, false);
            // Big endian
            check_predictor2_known({0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x05},
                                   {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05}, 4, 1, 12, true);
        }

        void test_predictor_2_64bit()
        {
            // Big endian: 1, then +0xFFFFFFFFFFFFFFFF (-1) -> 0
            check_predictor2_known({0, 0, 0, 0, 0, 0, 0, 1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                   {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0}, 8, 1, 16, true);
        }

        void test_predictor_2_rows_are_independent()
        {
            // Each row starts over: the first sample of row 2 is not added to the end of row 1.
            check_predictor2_known({5, 1, 1, 7, 1, 1}, {5, 6, 7, 7, 8, 9}, 1, 1, 3, false);
        }

        void test_predictor_2_round_trip()
        {
            const size_t rows = 5;
            const size_t pixels = 17;

            for (unsigned bps: {1, 2, 4, 8}) {
                for (unsigned spp: {1, 3}) {
                    for (bool big_endian: {false, true}) {
                        const size_t row_bytes = pixels * spp * bps;
                        const bytes original = lcg_data(rows * row_bytes, 256, 7 + bps);

                        bytes data = original;
                        predictor2_encode(data, bps, spp, row_bytes, big_endian);
                        CPPUNIT_ASSERT_MESSAGE(prolog + "the forward predictor should change the data", data != original);

                        tiff_predictor_status status = tiff_predictor_decode(&data[0], data.size(), 2, bps, spp, row_bytes, big_endian);
                        const string what = " (bytes per sample " + to_string(bps) + ", samples per pixel " + to_string(spp)
                                            + (big_endian ? ", big endian)" : ", little endian)");
                        CPPUNIT_ASSERT_MESSAGE(prolog + "status" + what, status == TIFF_PREDICTOR_OK);
                        CPPUNIT_ASSERT_MESSAGE(prolog + "data" + what, data == original);
                    }
                }
            }
        }

        // ---------------------------------------------------------------------
        // tiff_predictor_decode(): predictor 3 (floating point)
        // ---------------------------------------------------------------------

        void test_predictor_3_float32_known_answer()
        {
            // 1.0, 2.0, 3.5, -4.25, 100.0, 0.0 as written by imagecodecs and by an implementation of
            // TIFF Technical Note 3 (byte planes, most significant byte first, then byte differences).
            const bytes stored = {0x3F, 0x01, 0x00, 0x80, 0x82, 0xBE, 0x80, 0x80, 0x60, 0x28, 0x40, 0x38,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            const vector<float> values = {1.0f, 2.0f, 3.5f, -4.25f, 100.0f, 0.0f};

            check_predictor3_known(stored, float32_bytes(values, false), float32_bytes(values, true), 4, 1, 24);
        }

        void test_predictor_3_float32_three_bands_known_answer()
        {
            // One row of two pixels with three interleaved samples each.
            const bytes stored = {0x3F, 0x41, 0xC0, 0x00, 0x00, 0x00, 0x81, 0xDF, 0x40, 0x20, 0x08, 0x20,
                                  0x20, 0xD8, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            const vector<float> values = {1.5f, 10.0f, -2.0f, 1.75f, 10.5f, -2.5f};

            check_predictor3_known(stored, float32_bytes(values, false), float32_bytes(values, true), 4, 3, 24);
        }

        void test_predictor_3_float64_known_answer()
        {
            const bytes stored = {0x3F, 0x80, 0x81, 0xA0, 0x10, 0xA0, 0x70, 0x00, 0x01, 0xFF, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            const vector<double> values = {0.5, -1.0, 1024.25};

            check_predictor3_known(stored, float64_bytes(values, false), float64_bytes(values, true), 8, 1, 24);
        }

        void test_predictor_3_round_trip()
        {
            const size_t rows = 4;
            const size_t pixels = 13;

            for (unsigned bps: {2, 4, 8}) {
                for (unsigned spp: {1, 3}) {
                    for (bool big_endian: {false, true}) {
                        const size_t row_bytes = pixels * spp * bps;
                        const bytes original = lcg_data(rows * row_bytes, 256, 31 + bps);

                        bytes data = original;
                        predictor3_encode(data, bps, spp, row_bytes, big_endian);
                        CPPUNIT_ASSERT_MESSAGE(prolog + "the forward predictor should change the data", data != original);

                        tiff_predictor_status status = tiff_predictor_decode(&data[0], data.size(), 3, bps, spp, row_bytes, big_endian);
                        const string what = " (bytes per sample " + to_string(bps) + ", samples per pixel " + to_string(spp)
                                            + (big_endian ? ", big endian)" : ", little endian)");
                        CPPUNIT_ASSERT_MESSAGE(prolog + "status" + what, status == TIFF_PREDICTOR_OK);
                        CPPUNIT_ASSERT_MESSAGE(prolog + "data" + what, data == original);
                    }
                }
            }
        }

        // ---------------------------------------------------------------------
        // tiff_predictor_decode(): bad arguments
        // ---------------------------------------------------------------------

        void test_predictor_bad_arguments()
        {
            bytes data(24, 1);
            uint8_t *p = &data[0];

            // Sizes that do not fit together
            CPPUNIT_ASSERT(tiff_predictor_decode(p, 24, 2, 2, 1, 0, false) == TIFF_PREDICTOR_BAD_ARGS);   // no row size
            CPPUNIT_ASSERT(tiff_predictor_decode(p, 24, 2, 2, 0, 8, false) == TIFF_PREDICTOR_BAD_ARGS);   // no samples per pixel
            CPPUNIT_ASSERT(tiff_predictor_decode(p, 24, 2, 2, 1, 7, false) == TIFF_PREDICTOR_BAD_ARGS);   // row is not whole samples
            CPPUNIT_ASSERT(tiff_predictor_decode(p, 24, 2, 2, 3, 8, false) == TIFF_PREDICTOR_BAD_ARGS);   // row is not whole pixels
            CPPUNIT_ASSERT(tiff_predictor_decode(p, 24, 2, 2, 1, 10, false) == TIFF_PREDICTOR_BAD_ARGS);  // data is not whole rows
            CPPUNIT_ASSERT(tiff_predictor_decode(nullptr, 24, 2, 2, 1, 8, false) == TIFF_PREDICTOR_BAD_ARGS);

            // Values the TIFF predictors do not allow
            CPPUNIT_ASSERT(tiff_predictor_decode(p, 24, 0, 1, 1, 8, false) == TIFF_PREDICTOR_UNSUPPORTED);
            CPPUNIT_ASSERT(tiff_predictor_decode(p, 24, 4, 1, 1, 8, false) == TIFF_PREDICTOR_UNSUPPORTED);
            CPPUNIT_ASSERT(tiff_predictor_decode(p, 24, 2, 3, 1, 12, false) == TIFF_PREDICTOR_UNSUPPORTED);  // 24 bit samples
            CPPUNIT_ASSERT(tiff_predictor_decode(p, 24, 3, 1, 1, 8, false) == TIFF_PREDICTOR_UNSUPPORTED);   // 8 bit floating point
            CPPUNIT_ASSERT(tiff_predictor_decode(p, 24, 3, 3, 1, 12, false) == TIFF_PREDICTOR_UNSUPPORTED);

            // Nothing was changed by any of the calls above
            CPPUNIT_ASSERT(data == bytes(24, 1));
        }

        void test_predictor_status_str()
        {
            const tiff_predictor_status all[] = {TIFF_PREDICTOR_OK, TIFF_PREDICTOR_BAD_ARGS, TIFF_PREDICTOR_UNSUPPORTED};
            set<string> seen;
            for (tiff_predictor_status s: all) {
                const char *msg = tiff_predictor_status_str(s);
                CPPUNIT_ASSERT(msg != nullptr);
                CPPUNIT_ASSERT(string(msg).size() > 0);
                seen.insert(msg);
            }
            CPPUNIT_ASSERT_MESSAGE(prolog + "each status has its own message", seen.size() == sizeof(all) / sizeof(all[0]));
        }

        CPPUNIT_TEST_SUITE( lzw_util_test );

        CPPUNIT_TEST(test_lzw_decode_literals);
        CPPUNIT_TEST(test_lzw_decode_string_reference);
        CPPUNIT_TEST(test_lzw_decode_kwkwk);
        CPPUNIT_TEST(test_lzw_decode_consecutive_clear_codes);
        CPPUNIT_TEST(test_lzw_decode_clear_resets_the_table);

        CPPUNIT_TEST(test_lzw_decode_libtiff_vector);
        CPPUNIT_TEST(test_reference_encoder_matches_libtiff);

        CPPUNIT_TEST(test_lzw_decode_round_trip_small);
        CPPUNIT_TEST(test_lzw_decode_round_trip_long_runs);
        CPPUNIT_TEST(test_lzw_decode_round_trip_code_width_changes);
        CPPUNIT_TEST(test_lzw_decode_round_trip_full_table);
        CPPUNIT_TEST(test_lzw_decode_round_trip_image_like_data);

        CPPUNIT_TEST(test_lzw_decode_short_output);
        CPPUNIT_TEST(test_lzw_decode_empty_image);
        CPPUNIT_TEST(test_lzw_decode_output_smaller_than_stream);
        CPPUNIT_TEST(test_lzw_decode_zero_capacity);
        CPPUNIT_TEST(test_lzw_decode_dst_len_is_optional);

        CPPUNIT_TEST(test_lzw_decode_error_no_clear_code);
        CPPUNIT_TEST(test_lzw_decode_error_code_not_in_table);
        CPPUNIT_TEST(test_lzw_decode_error_first_code_not_a_literal);
        CPPUNIT_TEST(test_lzw_decode_error_truncated_input);
        CPPUNIT_TEST(test_lzw_decode_error_empty_input);
        CPPUNIT_TEST(test_lzw_decode_error_old_style_encoding);
        CPPUNIT_TEST(test_lzw_decode_error_null_arguments);
        CPPUNIT_TEST(test_lzw_decode_never_writes_past_the_buffer);
        CPPUNIT_TEST(test_lzw_status_str);

        CPPUNIT_TEST(test_predictor_1_does_nothing);
        CPPUNIT_TEST(test_predictor_2_8bit);
        CPPUNIT_TEST(test_predictor_2_8bit_wraps_modulo_256);
        CPPUNIT_TEST(test_predictor_2_8bit_three_bands);
        CPPUNIT_TEST(test_predictor_2_16bit_little_endian);
        CPPUNIT_TEST(test_predictor_2_16bit_big_endian);
        CPPUNIT_TEST(test_predictor_2_16bit_wraps_modulo_65536);
        CPPUNIT_TEST(test_predictor_2_16bit_three_bands);
        CPPUNIT_TEST(test_predictor_2_32bit);
        CPPUNIT_TEST(test_predictor_2_64bit);
        CPPUNIT_TEST(test_predictor_2_rows_are_independent);
        CPPUNIT_TEST(test_predictor_2_round_trip);

        CPPUNIT_TEST(test_predictor_3_float32_known_answer);
        CPPUNIT_TEST(test_predictor_3_float32_three_bands_known_answer);
        CPPUNIT_TEST(test_predictor_3_float64_known_answer);
        CPPUNIT_TEST(test_predictor_3_round_trip);

        CPPUNIT_TEST(test_predictor_bad_arguments);
        CPPUNIT_TEST(test_predictor_status_str);

        CPPUNIT_TEST_SUITE_END();
    };

    CPPUNIT_TEST_SUITE_REGISTRATION(lzw_util_test);

} // namespace dmrpp

int main(int argc, char*argv[])
{
    auto bes_debug_args = "cerr,lzw";

    return bes_run_tests<dmrpp::lzw_util_test>(argc, argv, bes_debug_args) ? 0 : 1;
}