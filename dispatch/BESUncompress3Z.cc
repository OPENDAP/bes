// BESUncompress3Z.c

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author:
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      dnadeau     Denis Nadeau <dnadeau@pop600.gsfc.nasa.gov>

#include "config.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cerrno>
#include <cstdio>
#include <cstring>

#include "BESDebug.h"
#include "BESInternalError.h"
#include "BESUncompress3Z.h"

using std::endl;
using std::string;

/** @brief uncompress a file with the .gz file extension
 *
 * @param src file that will be uncompressed
 * @param target file to uncompress the src file to
 */
void BESUncompress3Z::uncompress(const string &src, int fd) {
    int srcFile = 0;
    int my_errno = 0;

    /* -------------------------------------------------------------------- */
    /*      Open the file to be read                                        */
    /* -------------------------------------------------------------------- */

    BESDEBUG("bes", "BESUncompress3Z::uncompress - src=" << src.c_str() << endl);

    srcFile = open(src.c_str(), O_RDONLY);
    my_errno = errno;
    if (srcFile == -1) {
        string err = "Unable to open the compressed file " + src + ": ";
        char *serr = strerror(my_errno);
        if (serr) {
            err.append(serr);
        } else {
            err.append("unknown error occurred");
        }
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    /* ==================================================================== */
    /*      Start decompress LZW inspired from ncompress-4.2.4.orig         */
    /* ==================================================================== */

    BESDEBUG("bes", "BESUncompress3Z::uncompress - start decompress" << endl);

#define FIRSTBYTE (unsigned char)'\037'  /* First byte of compressed file*/
#define SECONDBYTE (unsigned char)'\235' /* Second byte of compressed file*/
#define FIRST 257
#define BIT_MASK 0x1f
#define BLOCK_MODE 0x80
#define MAXCODE(n) (1L << (n))
#define BITS 16
#define INIT_BITS 9
#define CLEAR 256 /* table clear output code*/
#define HBITS 17  /* 50% occupancy */
#define HSIZE (1 << HBITS)
#define HMASK (HSIZE - 1)
#define BITS 16
#define de_stack ((unsigned char *)&(htab[HSIZE - 1]))
#define BYTEORDER 0000
#define NOALLIGN 0

    unsigned char htab[HSIZE * 4];
    unsigned short codetab[HSIZE];

    int block_mode = BLOCK_MODE;
    int maxbits = BITS;
    unsigned char inbuf[BUFSIZ + 64];    /* Input buffer */
    unsigned char outbuf[BUFSIZ + 2048]; /* Output buffer */
    unsigned char *stackp;
    long int code;
    int finchar;
    long int oldcode;
    long int incode;
    int inbits;
    int posbits;
    int outpos;
    int insize;
    int bitmask;
    long int free_ent;
    long int maxcode;
    long int maxmaxcode;
    int n_bits;
    int rsize = 0;

    insize = 0;

    BESDEBUG("bes", "BESUncompress3Z::uncompress - read file" << endl);
    ;
    /* -------------------------------------------------------------------- */
    /*       Verify if the .Z file start with 0x1f and 0x9d                 */
    /* -------------------------------------------------------------------- */
    while (insize < 3 && (rsize = read(srcFile, inbuf + insize, BUFSIZ)) > 0) {
        insize += rsize;
    }
    BESDEBUG("bes", "BESUncompress3Z::uncompress - insize: " << insize << endl);
    ;

    /* -------------------------------------------------------------------- */
    /*       Do we have compressed file?                                    */
    /* -------------------------------------------------------------------- */
    if ((insize < 3) || (inbuf[0] != FIRSTBYTE) || (inbuf[1] != SECONDBYTE)) {
        BESDEBUG("bes", "BESUncompress3Z::uncompress - not a compress file" << endl);
        ;
        if (rsize < 0) {
            string err = "Could not read file ";
            err += src.c_str();
            close(srcFile);
            throw BESInternalError(err, __FILE__, __LINE__);
        }

        if (insize > 0) {
            string err = src.c_str();
            err += ": not in compressed format";
            close(srcFile);
            throw BESInternalError(err, __FILE__, __LINE__);
        }

        string err = "unknown error";
        close(srcFile);
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    /* -------------------------------------------------------------------- */
    /*       handle compression                                             */
    /* -------------------------------------------------------------------- */
    maxbits = inbuf[2] & BIT_MASK;
    block_mode = inbuf[2] & BLOCK_MODE;
    maxmaxcode = MAXCODE(maxbits);

    if (maxbits > BITS) {
        string err = src.c_str();
        err += ": compressed with ";
        err += maxbits;
        err += " bits, can only handle";
        err += BITS;
        close(srcFile);
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    maxcode = MAXCODE(n_bits = INIT_BITS) - 1;
    bitmask = (1 << n_bits) - 1;
    oldcode = -1;
    finchar = 0;
    outpos = 0;
    posbits = 3 << 3;

    free_ent = ((block_mode) ? FIRST : 256);

    BESDEBUG("bes", "BESUncompress3Z::uncompress - entering loop" << endl);

    memset(codetab, 0, 256);

    for (code = 255; code >= 0; --code) {
        ((unsigned char *)(htab))[code] = (unsigned char)code;
    }

    do {
    resetbuf:;
        {
            int i;
            // int e;
            int o;

            int e = insize - (o = (posbits >> 3));

            for (i = 0; i < e; ++i)
                inbuf[i] = inbuf[i + o];

            insize = e;
            posbits = 0;
        }

        if ((unsigned int)insize < sizeof(inbuf) - BUFSIZ) {
            if ((rsize = read(srcFile, inbuf + insize, BUFSIZ)) < 0) {
                string err = "Could not read file ";
                err += src.c_str();
                close(srcFile);
                throw BESInternalError(err, __FILE__, __LINE__);
            }

            insize += rsize;
        }

        inbits = ((rsize > 0) ? (insize - insize % n_bits) << 3 : (insize << 3) - (n_bits - 1));

        while (inbits > posbits) {
            if (free_ent > maxcode) {
                posbits = ((posbits - 1) + ((n_bits << 3) - (posbits - 1 + (n_bits << 3)) % (n_bits << 3)));

                ++n_bits;
                if (n_bits == maxbits)
                    maxcode = maxmaxcode;
                else
                    maxcode = MAXCODE(n_bits) - 1;

                bitmask = (1 << n_bits) - 1;
                goto resetbuf;
            }

            unsigned char *p = &inbuf[posbits >> 3];

            code = ((((long)(p[0])) | ((long)(p[1]) << 8) | ((long)(p[2]) << 16)) >> (posbits & 0x7)) & bitmask;

            posbits += n_bits;

            if (oldcode == -1) {
                if (code >= 256) {
                    string err = "oldcode:-1 code: ";
                    err += code;
                    err += " !!!! uncompress: corrupt input!!!";
                    close(srcFile);
                    throw BESInternalError(err, __FILE__, __LINE__);
                }
                outbuf[outpos++] = (unsigned char)(finchar = (int)(oldcode = code));
                continue;
            }

            /* Clear */
            if (code == CLEAR && block_mode) {
                memset(codetab, 0, 256);
                free_ent = FIRST - 1;
                posbits = ((posbits - 1) + ((n_bits << 3) - (posbits - 1 + (n_bits << 3)) % (n_bits << 3)));
                maxcode = MAXCODE(n_bits = INIT_BITS) - 1;
                bitmask = (1 << n_bits) - 1;
                goto resetbuf;
            }

            incode = code;
            stackp = de_stack;

            /* Special case for KwKwK string.*/
            if (code >= free_ent) {
                if (code > free_ent) {
                    close(srcFile);
                    throw BESInternalError("uncompress: corrupt input", __FILE__, __LINE__);
                }

                *--stackp = (unsigned char)finchar;
                code = oldcode;
            }

            /* Generate output characters in reverse order */
            while ((unsigned long)code >= (unsigned long)256) {
                *--stackp = htab[code];
                code = codetab[code];
            }

            *--stackp = (unsigned char)(finchar = htab[code]);

            /* And put them out in forward order */
            {
                int i;
                if (outpos + (i = (de_stack - stackp)) >= BUFSIZ) {
                    do {

                        if (i > BUFSIZ - outpos) {
                            i = BUFSIZ - outpos;
                        }

                        if (i > 0) {
                            memcpy(outbuf + outpos, stackp, i);
                            outpos += i;
                        }

                        if (outpos >= BUFSIZ) {
                            if (write(fd, outbuf, outpos) != outpos) {
                                string err = "uncompress: write eror";
                                close(srcFile);
                                throw BESInternalError(err, __FILE__, __LINE__);
                            }
                            outpos = 0;
                        }
                        stackp += i;
                    } while ((i = (de_stack - stackp)) > 0); /* de-stack */
                } else {
                    memcpy(outbuf + outpos, stackp, i);
                    outpos += i;
                }
            }
            /* Generate the new entry. */
            if ((code = free_ent) < maxmaxcode) {
                codetab[code] = (unsigned short)oldcode;
                htab[code] = (unsigned char)finchar;
                free_ent = code + 1;
            }

            oldcode = incode; /* Remember previous code.	*/
        }
    }

    while (rsize > 0); /* end of do */

    if (outpos > 0 && write(fd, outbuf, outpos) != outpos) {
        string err = "uncompress: write eror";
        close(srcFile);
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    close(srcFile);

    BESDEBUG("bes", "BESUncompress3Z::uncompress - end decompres" << endl);
}
