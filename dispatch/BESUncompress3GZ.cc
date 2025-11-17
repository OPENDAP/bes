// BESUncompress3GZ.cc

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

#include <zlib.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sstream>

using std::ostringstream;
using std::string;

#include "BESInternalError.h"
#include "BESUncompress3GZ.h"
// #include "BESDebug.h"

#define CHUNK 4096

/** @brief uncompress a file with the .gz file extension
 *
 * @param src file that will be uncompressed
 * @param target the decompressed info from src is written to this
 * file. This must be an open file descriptor; it will not be
 * closed on exit from this static method.
 */
void BESUncompress3GZ::uncompress(const string &src, int dest_fd) {
    // buffer to hold the uncompressed data
    char in[CHUNK];

    // open the file to be read by gzopen. If the file is not compressed
    // using gzip then all this function will do is trasnfer the data to the
    // destination file.
    gzFile gsrc = gzopen(src.c_str(), "rb");
    if (gsrc == NULL) {
        string err = "Could not open the compressed file " + src;
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    // gzread will read the data in uncompressed. All we have to do is write
    // it to the destination file.
    bool done = false;
    while (!done) {
        int bytes_read = gzread(gsrc, in, CHUNK);
        if (bytes_read == 0) {
            done = true;
        } else {
            int bytes_written = write(dest_fd, in, bytes_read);
            if (bytes_written < bytes_read) {
                ostringstream strm;
                strm << "Error writing uncompressed data for file " << gsrc << ": wrote " << bytes_written
                     << " instead of " << bytes_read;
                gzclose(gsrc);

                throw BESInternalError(strm.str(), __FILE__, __LINE__);
            }
        }
    }

    gzclose(gsrc);
}
