//
// Created by slloyd
//

// -*- mode: c++; c-basic-offset:4 -*-

// lzw_util.h
//
// Standalone decoders for the two TIFF-family decompression stages that
// GeoTIFF-derived DMR++ chunks may need:
//
//   1. TIFF "Compression = 5" LZW decoding (lzw_decode)
//   2. TIFF "Predictor = 2 / 3" reversal   (tiff_predictor_decode)
//
// Both functions work on caller-owned memory buffers, have no dependency on
// libtiff or on any BES/libdap type, and use only standard C++11 -- so they
// build the same way on Linux and macOS and can be unit tested in isolation.
//
// ---------------------------------------------------------------------------
// Third-party provenance / required notices
// ---------------------------------------------------------------------------
// The algorithms in lzw_util.cc are derived from libtiff's tif_lzw.c and
// tif_predict.c.  The following notices must be retained in all copies:
//
//   Copyright (c) 1988-1997 Sam Leffler
//   Copyright (c) 1991-1997 Silicon Graphics, Inc.
//   Copyright (c) 2022 Even Rouault
//
//   Permission to use, copy, modify, distribute, and sell this software and
//   its documentation for any purpose is hereby granted without fee, provided
//   that (i) the above copyright notices and this permission notice appear in
//   all copies of the software and related documentation, and (ii) the names
//   of Sam Leffler and Silicon Graphics may not be used in any advertising or
//   publicity relating to the software without the specific, prior written
//   permission of Sam Leffler and Silicon Graphics.
//
//   THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
//   EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
//   WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
//
//   IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
//   ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
//   OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
//   WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
//   LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
//   OF THIS SOFTWARE.
//
// libtiff's LZW code is in turn derived from the Berkeley "compress" program;
// that notice is reproduced in full at the bottom of lzw_util.cc.
//
// TODO: add the BES project's standard license header above this block.

#ifndef _dmrpp_lzw_util_h
#define _dmrpp_lzw_util_h 1

#include <cstddef>
#include <cstdint>

namespace dmrpp {

// ---------------------------------------------------------------------------
// LZW
// ---------------------------------------------------------------------------

/**
 * Result of lzw_decode().
 *
 * In every case (including errors) lzw_decode() writes the number of valid
 * decoded bytes to *dst_len and zero-fills the rest of dst, so the caller
 * never sees uninitialized memory.
 */
    enum lzw_status {
        /// The destination buffer was filled completely (dst_len == dst_capacity).
        LZW_OK = 0,

        /// The stream ended with an EOI code before the destination buffer was
        /// full. dst[0 .. dst_len) is valid and the remainder is zero-filled.
        /// This is not necessarily an error (e.g. a short final strip), so the
        /// caller decides whether to accept it.
        LZW_SHORT_OUTPUT,

        /// A null pointer was passed where data was required.
        LZW_ERR_BAD_ARGS,

        /// The stream looks like the obsolete bit-reversed (LSB-first) LZW that
        /// very old libtiff versions wrote. Not supported here.
        LZW_ERR_OLD_STYLE,

        /// The stream did not begin with a CLEAR code.
        LZW_ERR_NO_CLEAR,

        /// A code was used that is not (yet) in the string table.
        LZW_ERR_BAD_CODE,

        /// The input ran out before an EOI code and before the destination
        /// buffer was full.
        LZW_ERR_TRUNCATED_INPUT
    };

/**
 * Decode one TIFF-variant LZW stream (one strip or one tile).
 *
 * This implements the variant used by TIFF Compression=5 (and therefore by
 * GeoTIFF): MSB-first bit packing, 9..12 bit codes, CLEAR=256, EOI=257, and
 * the "early change" code-width increase that Aldus/Adobe TIFF writers use
 * (the width grows one code earlier than in classic Unix compress or GIF).
 * It is NOT compatible with Unix .Z files or GIF.
 *
 * Decoding stops when dst_capacity bytes have been produced or an EOI code is
 * read. If the stream would decode to more than dst_capacity bytes the output
 * is truncated at dst_capacity, which matches libtiff's behavior.
 *
 * @param src          compressed bytes
 * @param src_len      number of compressed bytes
 * @param dst          output buffer (caller-allocated)
 * @param dst_capacity size of dst in bytes (the expected decoded size)
 * @param dst_len      [out] number of valid bytes written to dst; may be null
 * @return a lzw_status; see above
 */
    lzw_status lzw_decode(const uint8_t *src, size_t src_len,
                          uint8_t *dst, size_t dst_capacity,
                          size_t *dst_len);

/// A short human-readable description of a status code (never null).
    const char *lzw_status_str(lzw_status status);

// ---------------------------------------------------------------------------
// TIFF predictor
// ---------------------------------------------------------------------------

    enum tiff_predictor_status {
        TIFF_PREDICTOR_OK = 0,

        /// Inconsistent sizes: zero stride/row size, or data_len / row_bytes is
        /// not a whole multiple of the pixel size / row size.
        TIFF_PREDICTOR_BAD_ARGS,

        /// Unknown predictor value or unsupported sample size.
        TIFF_PREDICTOR_UNSUPPORTED
    };

/**
 * Reverse a TIFF predictor, in place, on already-decompressed data.
 *
 *  - predictor 1: no prediction; the call does nothing.
 *  - predictor 2: horizontal differencing. bytes_per_sample must be
 *                 1, 2, 4 or 8. Samples are accumulated in the byte order
 *                 given by `big_endian`, which must be the byte order of the
 *                 data as it sits in the buffer (i.e. the TIFF file's byte
 *                 order: "II" = little endian, "MM" = big endian). The buffer
 *                 stays in that byte order; any later byte swapping is left
 *                 to the caller (e.g. DMR++'s existing byteOrder handling).
 *  - predictor 3: floating-point predictor. bytes_per_sample must be 2, 4 or
 *                 8. The TIFF format stores floating-point rows as byte
 *                 planes (most significant byte first) regardless of file
 *                 byte order, so `big_endian` here selects the byte order of
 *                 the OUTPUT samples.
 *
 * The predictor is applied independently to each row, exactly as libtiff
 * does for strips and tiles.
 *
 * @param data              buffer to modify in place
 * @param data_len          size of the buffer; must be a multiple of row_bytes
 * @param predictor         TIFF Predictor tag value (1, 2 or 3)
 * @param bytes_per_sample  BitsPerSample / 8
 * @param samples_per_pixel SamplesPerPixel for PlanarConfiguration=1 (chunky);
 *                          use 1 for PlanarConfiguration=2 (planar)
 * @param row_bytes         bytes in one row of the strip/tile, i.e.
 *                          row_width * samples_per_pixel * bytes_per_sample
 *                          (for tiles, row_width is the tile width)
 * @param big_endian        see above
 */
    tiff_predictor_status tiff_predictor_decode(uint8_t *data, size_t data_len,
                                                int predictor,
                                                unsigned bytes_per_sample,
                                                unsigned samples_per_pixel,
                                                size_t row_bytes,
                                                bool big_endian);

/// A short human-readable description of a status code (never null).
    const char *tiff_predictor_status_str(tiff_predictor_status status);

} // namespace dmrpp

#endif // _dmrpp_lzw_util_h
