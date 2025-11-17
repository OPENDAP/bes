// BESUncompress3BZ2.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "config.h"

#ifdef HAVE_BZLIB_H
#include <bzlib.h>
#endif

#include <cerrno>
#include <cstring>
#include <sstream>
#include <unistd.h>

using std::ostringstream;
using std::string;

#include "BESDebug.h"
#include "BESInternalError.h"
#include "BESUncompress3BZ2.h"

#define CHUNK 4096

#if 0
static void bz_internal_error(int errcode)
{
    ostringstream strm;
    strm << "internal error in bz2 library occurred: " << errcode;
    throw BESInternalError(strm.str(), __FILE__, __LINE__);
}
#endif

/** @brief uncompress a file with the .bz2 file extension
 *
 * @param src_name file that will be uncompressed
 * @param target file to uncompress the src file to
 * @return full path to the uncompressed file
 */
void BESUncompress3BZ2::uncompress(const string &src_name, int fd) {
#ifndef HAVE_BZLIB_H
    string err = "Unable to uncompress bz2 files, feature not built. Check config.h in bes directory for HAVE_BZLIB_H "
                 "flag set to 1";
    throw BESInternalError(err, __FILE__, __LINE__);
#else
    FILE *src = fopen(src_name.c_str(), "rb");
    if (!src) {
        char *serr = strerror(errno);
        string err = "Unable to open the compressed file " + src_name + ": ";
        if (serr) {
            err.append(serr);
        } else {
            err.append("unknown error occurred");
        }
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    int bzerror = 0;   // any error flags will be stored here
    int verbosity = 0; // 0 is silent up to 4 which is very verbose
    int small = 0;     // if non zero then memory management is different
    // unused void *unused = NULL; // any unused bytes would be stored in here
    // unused int nunused = 0; // the size of the unused buffer
    char in[CHUNK]; // input buffer used to read uncompressed data in bzRead

    BZFILE *bsrc = NULL;

    bsrc = BZ2_bzReadOpen(&bzerror, src, verbosity, small, NULL, 0);
    if (bsrc == NULL) {
        const char *berr = BZ2_bzerror(bsrc, &bzerror);
        string err = "bzReadOpen failed on " + src_name + ": ";
        if (berr) {
            err.append(berr);
        } else {
            err.append("Unknown error");
        }
        fclose(src);

        throw BESInternalError(err, __FILE__, __LINE__);
    }

    bool done = false;
    while (!done) {
        int bytes_read = BZ2_bzRead(&bzerror, bsrc, in, CHUNK);
        if (bzerror != BZ_OK && bzerror != BZ_STREAM_END) {
            const char *berr = BZ2_bzerror(bsrc, &bzerror);
            string err = "bzRead failed on " + src_name + ": ";
            if (berr) {
                err.append(berr);
            } else {
                err.append("Unknown error");
            }

            BZ2_bzReadClose(&bzerror, bsrc);
            fclose(src);

            throw BESInternalError(err, __FILE__, __LINE__);
        }
        // if( bytes_read == 0 || bzerror == BZ_STREAM_END )
        if (bzerror == BZ_STREAM_END) {
            done = true;
        }
        int bytes_written = write(fd, in, bytes_read);
        if (bytes_written < bytes_read) {
            ostringstream strm;
            strm << "Error writing uncompressed data: "
                 << "wrote " << bytes_written << " instead of " << bytes_read;

            BZ2_bzReadClose(&bzerror, bsrc);
            fclose(src);

            throw BESInternalError(strm.str(), __FILE__, __LINE__);
        }
    }

    BZ2_bzReadClose(&bzerror, bsrc);
    fclose(src);
#endif
}
