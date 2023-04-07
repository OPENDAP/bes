// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES

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

#include <fstream>
#include <string>
#include <vector>

#include <cstdlib>  // for system

#include "BESInternalError.h"

#include "read_test_baseline.h"

using namespace std;

namespace bes {
/**
 * @brief Used for tests; read data from a baseline file
 *
 * Read the text of a baseline file and return it, verbatim, in a C++
 * string object.
 *
 * @param fn The filename
 * @return A string that holds the verbatim contents of the string.
 */
string
read_test_baseline(const string &fn)
{
    ifstream is;
    is.open (fn.c_str(), ios::binary );

    if (!is)
        return "Could not read baseline file: "+fn;

    // get length of file:
    is.seekg (0, ios::end);
    long length = is.tellg();

    // back to start
    is.seekg (0, ios::beg);

    // allocate memory:
    vector<char> buffer(length+1);

    // read data as a block:
    is.read (buffer.data(), length);
    is.close();
    buffer[length] = '\0';

    return string(buffer.data());
}

void clean_cache_dir(const string &cache)
{
    string cache_dir = cache + "/*";

    string command = string("rm ") + cache_dir + " 2>/dev/null";

    int status = system(command.c_str());

    // it's normal for this to 'fail' because the clean operation has already
    // been run or because it's the first run of the tests. But, fork and waitpid
    // should not return an error and the shell should be found.
    if (status == -1 || status == 127)
        throw BESInternalError("Failed to clean cache dir: " + cache_dir, __FILE__, __LINE__);
}

} // namespace bes
