// Copyright (c) 2013 OPeNDAP, Inc. Author: James Gallagher
// <jgallagher@opendap.org>, Patrick West <pwest@opendap.org>
// Nathan Potter <npotter@opendap.org>
//
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2.1 of
// the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 U\ SA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI.
// 02874-0112.
#include "test_utils.h"

#include "config.h"

#include <stdlib.h>    // for system
#include <sys/param.h> // See clear_cache_dir
#include <unistd.h>

#include <fstream>
#include <string>
#include <vector>

#include "BESInternalError.h"

using namespace std;

string read_test_baseline(const string &fn) {
    int length;

    ifstream is;
    is.open(fn.c_str(), ios::binary);

    if (!is)
        throw BESInternalError(string("read_test_baseline: Could not open ").append(fn), __FILE__, __LINE__);

    // get length of file:
    is.seekg(0, ios::end);
    length = is.tellg();

    // back to start
    is.seekg(0, ios::beg);

    // allocate memory:
    vector<char> buffer(length + 1);

    // read data as a block:
    is.read(buffer.data(), length);
    is.close();
    buffer[length] = '\0';

    return string(buffer.data());
}

void clean_cache_dir(const string &cache) {
    string cache_dir = cache + "/*";

    string command = string("rm ") + cache_dir + " 2>/dev/null";

    int status = system(command.c_str());

    // it's normal for this to 'fail' because the clean operation has already
    // been run or because it's the first run of the tests. But, fork and waitpid
    // should not return an error and the shell should be found.
    if (status == -1 || status == 127)
        throw BESInternalError(string("Failed to clean cache dir: ").append(cache_dir), __FILE__, __LINE__);
}
