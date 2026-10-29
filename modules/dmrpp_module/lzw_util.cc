//
// Created by slloyd
//
// -*- mode: c++; c-basic-offset:4 -*-

/*
 * Original Berkeley "compress" notice, retained as required by the terms of
 * the code from which tif_lzw.c (and therefore this file) is derived:
 *
 * Copyright (c) 1985, 1986 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * James A. Woods, derived from original work by Spencer Thomas
 * and Joseph Orost.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

// lzw_util.cc
//
// TIFF-variant LZW decoder and TIFF predictor reversal.
//
// Derived from libtiff's tif_lzw.c and tif_predict.c (see lzw_util.h for the
// full list of required copyright notices; the original Berkeley "compress"
// notice is reproduced at the bottom of this file).
//
// Differences from the libtiff originals:
//   * One-shot API on plain memory buffers; no TIFF*, no codec vtable.
//   * No "restart an interrupted output operation" logic: DMR++ chunks are
//     decoded whole, into a buffer sized for the expected result.
//   * The string table is index-based (prefix codes) rather than pointer
//     based, and the bit reader is a simple byte-wise accumulator, so there
//     are no host-endianness or word-size specific code paths.
//   * The obsolete bit-reversed "LZW_COMPAT" stream variant is detected and
//     reported (LZW_ERR_OLD_STYLE) but not decoded.
//   * Predictor routines use explicit byte-order loads/stores instead of
//     host-order accesses, so they behave identically on every platform.

#include "lzw_util.h"

#include <cstring>
#include <vector>

namespace dmrpp {

// ---------------------------------------------------------------------------
// LZW
// ---------------------------------------------------------------------------

        namespace {

            const unsigned BITS_MIN = 9;   // codes start at 9 bits
            const unsigned BITS_MAX = 12;  // and grow to at most 12 bits
            const unsigned CODE_CLEAR = 256;
            const unsigned CODE_EOI = 257;
            const unsigned CODE_FIRST = 258;
            const unsigned TABLE_SIZE = 1u << BITS_MAX;  // 4096 entries

// maximum code value representable in n bits
            inline unsigned max_code(unsigned n) { return (1u << n) - 1u; }

// One string-table entry. A string is described by its last byte (value) and
// the code of the string that precedes it (prefix); walking the prefix chain
// yields the string in reverse.
            struct code_entry {
                uint16_t prefix;   // code of the string without its last byte
                uint16_t length;   // length of the whole string
                uint8_t first;     // first byte of the whole string
                uint8_t value;     // last byte of the whole string
            };

        } // anonymous namespace

        lzw_status lzw_decode(const uint8_t *src, size_t src_len,
        uint8_t *dst, size_t dst_capacity,
        size_t *dst_len)
        {
            size_t out = 0;
            lzw_status status = LZW_OK;

            if (dst_len)
                *dst_len = 0;

            if ((src == nullptr && src_len > 0) || (dst == nullptr && dst_capacity > 0))
                return LZW_ERR_BAD_ARGS;

            if (dst_capacity == 0)
                return LZW_OK;

            // libtiff's check for the obsolete bit-reversed encoding: a conformant
            // stream starts with CLEAR (256), whose first byte is 0x80.
            if (src_len >= 2 && src[0] == 0 && (src[1] & 0x1)) {
                std::memset(dst, 0, dst_capacity);
                return LZW_ERR_OLD_STYLE;
            }

            code_entry table[TABLE_SIZE];
            for (unsigned i = 0; i < 256; ++i) {
                table[i].prefix = 0;
                table[i].length = 1;
                table[i].first = static_cast<uint8_t>(i);
                table[i].value = static_cast<uint8_t>(i);
            }
            // CLEAR and EOI never appear in a string chain, but keep them defined.
            std::memset(&table[CODE_CLEAR], 0, (CODE_FIRST - CODE_CLEAR) * sizeof(code_entry));

            // Bit reader state: codes are packed most-significant-bit first.
            const uint8_t *ip = src;
            const uint8_t *const ip_end = src + src_len;
            uint32_t bitbuf = 0;
            unsigned bitcnt = 0;

            // Decoder state
            unsigned nbits = BITS_MIN;
            unsigned mask = max_code(BITS_MIN);
            unsigned next_free = CODE_FIRST;  // next table slot to be assigned
            unsigned old_code = 0;
            bool have_clear = false;

            // Fetch the next nbits-wide code; false if the input is exhausted.
            // (bitcnt never exceeds nbits + 7 <= 19, so a 32-bit buffer is enough;
            // bits above bitcnt are stale and are masked off.)
            auto next_code = [&](unsigned &code) -> bool {
                while (bitcnt < nbits) {
                    if (ip == ip_end)
                        return false;
                    bitbuf = (bitbuf << 8) | *ip++;
                    bitcnt += 8;
                }
                code = (bitbuf >> (bitcnt - nbits)) & mask;
                bitcnt -= nbits;
                return true;
            };

            // Register old_code + 'value' as a new table entry, then widen the codes
            // if needed. Following the TIFF/Aldus convention, the width grows when
            // the next free slot reaches the last code of the current width
            // ("early change"). Once the table is full no more entries are added.
            auto add_entry = [&](uint8_t value) {
                if (next_free >= TABLE_SIZE)
                    return;
                code_entry &e = table[next_free];
                e.prefix = static_cast<uint16_t>(old_code);
                e.length = static_cast<uint16_t>(table[old_code].length + 1);
                e.first = table[old_code].first;
                e.value = value;
                ++next_free;
                if (next_free >= mask && nbits < BITS_MAX) {
                    ++nbits;
                    mask = max_code(nbits);
                }
            };

            unsigned code = 0;

            while (out < dst_capacity) {
                if (!next_code(code)) {
                    status = LZW_ERR_TRUNCATED_INPUT;
                    break;
                }

                if (code == CODE_EOI) {
                    status = LZW_SHORT_OUTPUT;   // EOI before the buffer was full
                    break;
                }

                if (code == CODE_CLEAR) {
                    // Reset the table and code width.
                    nbits = BITS_MIN;
                    mask = max_code(BITS_MIN);
                    next_free = CODE_FIRST;
                    have_clear = true;

                    // Skip any run of consecutive CLEAR codes.
                    do {
                        if (!next_code(code)) {
                            status = LZW_ERR_TRUNCATED_INPUT;
                            break;
                        }
                    } while (code == CODE_CLEAR);
                    if (status != LZW_OK)
                        break;

                    if (code == CODE_EOI) {
                        status = LZW_SHORT_OUTPUT;
                        break;
                    }
                    if (code > CODE_EOI) {
                        // The first code after a CLEAR must be a literal.
                        status = LZW_ERR_BAD_CODE;
                        break;
                    }
                    dst[out++] = static_cast<uint8_t>(code);
                    old_code = code;
                    continue;
                }

                if (!have_clear) {
                    status = LZW_ERR_NO_CLEAR;
                    break;
                }

                if (code < 256) {
                    // A literal byte.
                    add_entry(static_cast<uint8_t>(code));
                    dst[out++] = static_cast<uint8_t>(code);
                    old_code = code;
                    continue;
                }

                // code >= CODE_FIRST: a string-table reference.
                if (code > next_free) {
                    // Not in the table yet, and not the special "code about to be
                    // defined" case (KwKwK).
                    status = LZW_ERR_BAD_CODE;
                    break;
                }
                if (code == next_free && next_free >= TABLE_SIZE) {
                    status = LZW_ERR_BAD_CODE;
                    break;
                }

                // Value of the new entry's last byte: the first byte of the current
                // string, or (KwKwK case) the first byte of the previous string.
                const uint8_t new_value =
                        (code == next_free) ? table[old_code].first : table[code].first;
                add_entry(new_value);

                // Emit the string for `code`. If it does not fit in what remains of
                // the output buffer, emit only its leading bytes (as libtiff does).
                const size_t remaining = dst_capacity - out;
                const size_t full_len = table[code].length;
                const size_t write_len = (full_len < remaining) ? full_len : remaining;

                unsigned idx = code;
                while (table[idx].length > write_len)
                    idx = table[idx].prefix;

                uint8_t *p = dst + out + write_len;
                for (;;) {
                    *--p = table[idx].value;
                    if (table[idx].length == 1)
                        break;
                    idx = table[idx].prefix;
                }
                out += write_len;
                old_code = code;
            }

            if (out < dst_capacity)
                std::memset(dst + out, 0, dst_capacity - out);

            if (dst_len)
                *dst_len = out;

            if (status == LZW_OK && out < dst_capacity)
                status = LZW_SHORT_OUTPUT;

            return status;
        }

        const char *lzw_status_str(lzw_status status)
        {
            switch (status) {
                case LZW_OK:
                    return "LZW: OK";
                case LZW_SHORT_OUTPUT:
                    return "LZW: stream ended (EOI) before the output buffer was full";
                case LZW_ERR_BAD_ARGS:
                    return "LZW: null buffer passed to decoder";
                case LZW_ERR_OLD_STYLE:
                    return "LZW: obsolete bit-reversed LZW encoding is not supported";
                case LZW_ERR_NO_CLEAR:
                    return "LZW: stream does not begin with a CLEAR code";
                case LZW_ERR_BAD_CODE:
                    return "LZW: corrupt stream (code not yet in table)";
                case LZW_ERR_TRUNCATED_INPUT:
                    return "LZW: input ended before an EOI code and before the output buffer was full";
            }
            return "LZW: unknown status";
        }

// ---------------------------------------------------------------------------
// TIFF predictor
// ---------------------------------------------------------------------------

        namespace {

// Load an unsigned integer of `n` bytes (1, 2, 4 or 8) in the given byte order.
            inline uint64_t load_uint(const uint8_t *p, unsigned n, bool big_endian)
            {
                uint64_t v = 0;
                if (big_endian) {
                    for (unsigned i = 0; i < n; ++i)
                        v = (v << 8) | p[i];
                }
                else {
                    for (unsigned i = n; i > 0; --i)
                        v = (v << 8) | p[i - 1];
                }
                return v;
            }

// Store the low `n` bytes of v in the given byte order.
            inline void store_uint(uint8_t *p, unsigned n, bool big_endian, uint64_t v)
            {
                if (big_endian) {
                    for (unsigned i = n; i > 0; --i) {
                        p[i - 1] = static_cast<uint8_t>(v & 0xff);
                        v >>= 8;
                    }
                }
                else {
                    for (unsigned i = 0; i < n; ++i) {
                        p[i] = static_cast<uint8_t>(v & 0xff);
                        v >>= 8;
                    }
                }
            }

// Predictor 2 for one row: each sample is stored as the difference from the
// same component of the previous pixel; undo that with a running sum
// (modulo 2^(8*bps)).
            void horizontal_accumulate_row(uint8_t *row, size_t row_bytes, unsigned bps,
                                           unsigned stride, bool big_endian)
            {
                if (bps == 1) {
                    for (size_t i = stride; i < row_bytes; ++i)
                        row[i] = static_cast<uint8_t>(row[i] + row[i - stride]);
                    return;
                }

                const size_t step = static_cast<size_t>(bps);
                const size_t lag = step * stride;
                for (size_t i = lag; i < row_bytes; i += step) {
                    const uint64_t v = load_uint(row + i, bps, big_endian)
                                       + load_uint(row + i - lag, bps, big_endian);
                    store_uint(row + i, bps, big_endian, v);   // store keeps only bps bytes
                }
            }

// Predictor 3 for one row: the row holds the sample bytes split into planes
// (most significant byte plane first), then differenced byte-wise with a lag
// of `stride`. Undo the differencing, then re-interleave the bytes.
            void float_accumulate_row(uint8_t *row, size_t row_bytes, unsigned bps,
                                      unsigned stride, bool big_endian,
            std::vector<uint8_t> &tmp)
            {
                for (size_t i = stride; i < row_bytes; ++i)
                    row[i] = static_cast<uint8_t>(row[i] + row[i - stride]);

                std::memcpy(tmp.data(), row, row_bytes);

                const size_t wc = row_bytes / bps;   // samples per row
                for (size_t k = 0; k < wc; ++k) {
                    for (unsigned b = 0; b < bps; ++b) {
                        // plane 0 holds the most significant bytes
                        const unsigned plane = big_endian ? b : (bps - 1 - b);
                        row[bps * k + b] = tmp[plane * wc + k];
                    }
                }
            }

        } // anonymous namespace

        tiff_predictor_status tiff_predictor_decode(uint8_t *data, size_t data_len,
        int predictor,
        unsigned bytes_per_sample,
        unsigned samples_per_pixel,
        size_t row_bytes,
        bool big_endian)
        {
            if (predictor == 1)
                return TIFF_PREDICTOR_OK;

            if (predictor != 2 && predictor != 3)
                return TIFF_PREDICTOR_UNSUPPORTED;

            if (predictor == 2) {
                if (bytes_per_sample != 1 && bytes_per_sample != 2
                    && bytes_per_sample != 4 && bytes_per_sample != 8)
                    return TIFF_PREDICTOR_UNSUPPORTED;
            }
            else {
                if (bytes_per_sample != 2 && bytes_per_sample != 4 && bytes_per_sample != 8)
                    return TIFF_PREDICTOR_UNSUPPORTED;
            }

            if (samples_per_pixel == 0 || row_bytes == 0 || (data == nullptr && data_len > 0))
                return TIFF_PREDICTOR_BAD_ARGS;

            const size_t pixel_bytes = static_cast<size_t>(bytes_per_sample) * samples_per_pixel;
            if (row_bytes % pixel_bytes != 0 || data_len % row_bytes != 0)
                return TIFF_PREDICTOR_BAD_ARGS;

            if (predictor == 2) {
                for (size_t off = 0; off < data_len; off += row_bytes)
                    horizontal_accumulate_row(data + off, row_bytes, bytes_per_sample,
                                              samples_per_pixel, big_endian);
            }
            else {
                std::vector<uint8_t> tmp(row_bytes);
                for (size_t off = 0; off < data_len; off += row_bytes)
                    float_accumulate_row(data + off, row_bytes, bytes_per_sample,
                                         samples_per_pixel, big_endian, tmp);
            }

            return TIFF_PREDICTOR_OK;
        }

        const char *tiff_predictor_status_str(tiff_predictor_status status)
        {
            switch (status) {
                case TIFF_PREDICTOR_OK:
                    return "TIFF predictor: OK";
                case TIFF_PREDICTOR_BAD_ARGS:
                    return "TIFF predictor: inconsistent row/pixel/buffer sizes";
                case TIFF_PREDICTOR_UNSUPPORTED:
                    return "TIFF predictor: unsupported predictor value or sample size";
            }
            return "TIFF predictor: unknown status";
        }

} // namespace dmrpp
