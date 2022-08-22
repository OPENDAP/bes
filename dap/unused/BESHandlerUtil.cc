// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES, A C++ implementation of the OPeNDAP Data
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
#include "config.h"

#include <sys/stat.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <vector>
#include <string>

#include <BESInternalError.h>

#include "BESHandlerUtil.h"
#include "BESLog.h"

using namespace std;

namespace bes {

/**
 * @brief Free the temporary file
 *
 * Close the open descriptor and delete (unlink) the file name.
 */
TemporaryFile::~TemporaryFile()
{
    try {
        if (!close(d_fd))
            ERROR_LOG("Error closing temporary file: " << d_name << ": " << strerror(errno) << endl);
        if (!unlink(d_name.data()))
            ERROR_LOG("Error closing temporary file: " << d_name << ": " << strerror(errno) << endl);
    }
    catch (...) {
        // Do nothing. This just protects against BESLog (i.e., ERROR)
        // throwing an exception
    }
}

/**
 * @brief Get a new temporary file
 *
 * Get a new temporary file using the given template. The template must give
 * the fully qualified path for the temporary file and must end in one or more
 * Xs (but six are usually used) with no characters following.
 *
 * @note If you pass in a bad template, behavior of this class is undefined.
 *
 * @param path_template Template passed to mkstemp() to build the temporary
 * file pathname.
 */
TemporaryFile::TemporaryFile(const std::string &path_template)
{
    //string temp_file_name = FONcRequestHandler::temp_dir + "/ncXXXXXX";
    //vector<char> temp_file(path_template.size() + 1);
    d_name.reserve(path_template.size() + 1);

    string::size_type len = path_template.copy(d_name.data(), path_template.size());
    d_name[len] = '\0';

    // cover the case where older versions of mkstemp() create the file using
    // a mode of 666.
    mode_t original_mode = umask(077);
    d_fd = mkstemp(d_name.data());
    umask(original_mode);

    if (d_fd == -1) throw BESInternalError("Failed to open the temporary file.", __FILE__, __LINE__);
}

} // namespace bes


