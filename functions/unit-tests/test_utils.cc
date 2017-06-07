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

#include "config.h"

//#include <sys/param.h>	// See clear_cache_dir
//#include <unistd.h>
//#include <stdlib.h>	    // for system

#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#include <Array.h>      // libdap
#include <dods-datatypes.h>
#include <InternalErr.h>
#include <debug.h>

#include "test_utils.h"

using namespace std;
using namespace libdap;

string
readTestBaseline(const string &fn)
{
    int length;

    ifstream is;
    is.open (fn.c_str(), ios::binary );

    // get length of file:
    is.seekg (0, ios::end);
    length = is.tellg();

    // back to start
    is.seekg (0, ios::beg);

    // allocate memory:
    vector<char> buffer(length+1);

    // read data as a block:
    is.read (&buffer[0], length);
    is.close();
    buffer[length] = '\0';

    return string(&buffer[0]);
}

/**
 * @brief Read data from a text file
 *
 * Read data from a text file where those values are listed on one or more
 * lines. Each value is separated by a space, comma, or something that C++'s
 * istringstream won't confuse with a character that's part of the value.
 *
 * Assume the text file starts with a line that should be ignored.
 *
 * @param file Name of the input file
 * @param size Number of values to read
 * @param dest Chunk of memory with sizeof(T) * size bytes
 */
template<typename T>
void read_data_from_file(const string &file, unsigned int size, T *dest)
{
    fstream input(file.c_str(), fstream::in);
    if (input.eof() || input.fail()) throw Error(string(__FUNCTION__) + ": Could not open data (" + file + ").");

    // Read a line of text to get to the start of the data.
    string line;
    getline(input, line);
    if (input.eof() || input.fail()) throw Error(string(__FUNCTION__) + ": Could not read data.");

    // Get data line by line and load it into 'dest.' Assume that the last line
    // might not have as many values as the others.
    getline(input, line);
    if (input.eof() || input.fail()) throw Error(string(__FUNCTION__) + ": Could not read data.");

    while (!(input.eof() || input.fail())) {
        DBG(cerr << "line: " << line << endl);
        istringstream iss(line);
        while (!iss.eof()) {
            iss >> (*dest++);

            if (!size--) throw Error(string(__FUNCTION__) + ": Too many values in the data file.");
        }

        getline(input, line);
        if (input.bad())   // in the loop we only care about I/O failures, not logical errors like reading EOF.
            throw Error(string(__FUNCTION__) + ": Could not read data.");
    }
}

template<typename T>
void load_var(Array *var, const string &data_file, vector<T> &buf)
{
    if (!var) throw Error(string(__FUNCTION__) + ": The Array variable was null.");
    //string data_file = src_dir + "/" + file;
    read_data_from_file(data_file, buf.size(), &buf[0]);
    //if (!var) throw Error(string(__FUNCTION__) + ": Could not find '" + var->name() + "'.");
    if (!var->set_value(buf, buf.size())) throw Error(string(__FUNCTION__) + ": Could not set '" + var->name() + "'.");
    var->set_read_p(true);
}

// Explicit declaration to force code generation
template void load_var(Array *var, const string &data_file, vector<dods_float32> &buf);
template void load_var(Array *var, const string &data_file, vector<dods_float64> &buf);
