// BESDebug.cc

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

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <ctime>
#include <unistd.h>

#include "BESDebug.h"
#include "BESInternalError.h"
#include "BESLog.h"

using namespace std;

ostream *BESDebug::_debug_strm = nullptr;
bool BESDebug::_debug_strm_created = false;
BESDebug::DebugMap BESDebug::_debug_map;

/** @brief Returns debug log line prefix containing date&time, pid, and thread id.
 *
 * @return The debug log line prefix containing date&time, pid, and thread id.
 */
string get_debug_log_line_prefix() {
    // Given C++11, this could be done with std::put_time() and std::localtime().
    std::ostringstream oss;

    // Time Field
    auto t = time(nullptr);
    struct tm stm{};
    localtime_r(&t, &stm);
    // Mimic zone + asctime: GMT Thu Nov 24 18:22:48 1986
    oss << std::put_time(&stm, "[%Z %c]");

    // PID field
    oss << "[pid:" << getpid() << "]";

    // Thread field
    oss << "[thread:" << pthread_self() << "]";

    return oss.str();
}

/** @brief Sets up debugging for the bes.
 *
 * This static method sets up debugging for the bes given a set of values
 * typically passed on the command line. Might look like the following:
 *
 * -d "bes.debug,nc,hdf4,bes"
 *
 * this method will break this down to set the output stream to an ofstream
 * for the file bes.debug and turn on debugging for nc, hdf4, and bes.
 *
 * @param values to be parsed and set for debugging, bes.debug,nc,hdf4,bes
 */
void BESDebug::SetUp(const string &values) {
    if (values.empty()) {
        string err = "Empty debug options";
        throw BESInternalError(err, __FILE__, __LINE__);
    }
    string::size_type comma = 0;
    comma = values.find(',');
    if (comma == string::npos) {
        string err = "Missing comma in debug options: " + values;
        throw BESInternalError(err, __FILE__, __LINE__);
    }
    ostream *strm = nullptr;
    bool created = false;
    string s_strm = values.substr(0, comma);
    if (s_strm == "cerr") {
        strm = &cerr;
    } else if (s_strm == "LOG") {
        strm = BESLog::TheLog()->get_log_ostream();
    } else {
        strm = new ofstream(s_strm.c_str(), ios::out);
        if (strm && strm->fail()) {
            delete strm;
            strm = nullptr;
            string err = "Unable to open the debug file: " + s_strm;
            throw BESInternalError(err, __FILE__, __LINE__);
        }
        created = true;
    }

    BESDebug::SetStrm(strm, created);

    string::size_type new_comma = 0;
    while ((new_comma = values.find(',', comma + 1)) != string::npos) {
        string flagName = values.substr(comma + 1, new_comma - comma - 1);
        if (flagName[0] == '-') {
            string newflag = flagName.substr(1, flagName.size() - 1);
            BESDebug::Set(newflag, false);
        } else {
            BESDebug::Set(flagName, true);
        }
        comma = new_comma;
    }
    string flagName = values.substr(comma + 1, values.size() - comma - 1);
    if (flagName[0] == '-') {
        string newflag = flagName.substr(1, flagName.size() - 1);
        BESDebug::Set(newflag, false);
    } else {
        BESDebug::Set(flagName, true);
    }
}

/** @brief set the debug context to the specified value
 *
 * Static function that sets the specified debug context (flagName)
 * to the specified debug value (true or false). If the context is
 * found then the value is set. Else the context is created and the
 * value set.
 *
 * @param flagName debug context flag to set to the given value
 * @param value set the debug context to this value
 */
void BESDebug::Set(const std::string &flagName, bool value) {
    if (value && flagName == "all") {
        std::for_each(_debug_map.begin(), _debug_map.end(), [](DebugMap::value_type &p) { p.second = true; });
    }
    _debug_map[flagName] = value;
}

/** @brief Writes help information for so that developers know what can
 * be set for debugging.
 *
 * Displays information about possible debugging context, such as nc,
 * hdf4, bes
 *
 * @param strm output stream to write the help information to
 */
void BESDebug::Help(ostream &strm) {
    strm << "Debug help:" << endl
         << "  Set on the command line with " << "-d \"file_name|cerr,[-]context1,...,[-]context\"" << endl
         << "  context with dash (-) in front will be turned off" << endl
         << "  context of all will turn on debugging for all contexts" << endl
         << endl
         << "Possible context(s):" << endl;

    if (!_debug_map.empty()) {
        std::for_each(_debug_map.begin(), _debug_map.end(), [&strm](const auto &pair) {
            strm << "  " << pair.first << ": ";
            if (pair.second)
                strm << "on" << endl;
            else
                strm << "off" << endl;
        });
    } else {
        strm << "  none specified" << endl;
    }
}

bool BESDebug::IsContextName(const string &name) { return _debug_map.count(name) > 0; }

/** This method looks at the current setting of the BESDebug object and builds
 *  a string that, when passed to a beslistener as the argument of the -d
 *  option, will mirror those settings. This is useful in code like the
 *  besdaemon, where debug contexts are set/cleared but that information has
 *  to be sent to the beslisteners to be used. The new option string will be
 *  built and the beslisteners restarted using it.
 */
string BESDebug::GetOptionsString() {
    ostringstream oss;

    if (!_debug_map.empty()) {
        std::for_each(_debug_map.begin(), _debug_map.end(), [&oss](const std::pair<std::string, bool> &p) {
            if (!p.second)
                oss << "-";
            oss << p.first << ",";
        });

        string retval = oss.str();
        return retval.erase(retval.size() - 1);
    } else {
        return {""};
    }
}
