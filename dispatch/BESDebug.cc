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

#include <time.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>

#include "BESDebug.h"
#include "BESInternalError.h"
#include "BESLog.h"

using namespace std;

ostream *BESDebug::_debug_strm = NULL;
bool BESDebug::_debug_strm_created = false;
map<string, bool> BESDebug::_debug_map;

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
void BESDebug::SetUp(const string &values)
{
    if (values.empty())
    {
        string err = "Empty debug options";
        throw BESInternalError(err, __FILE__, __LINE__);
    }
    string::size_type comma = 0;
    comma = values.find(',');
    if (comma == string::npos)
    {
        string err = "Missing comma in debug options: " + values;
        throw BESInternalError(err, __FILE__, __LINE__);
    }
    ostream *strm = 0;
    bool created = false;
    string s_strm = values.substr(0, comma);
    if (s_strm == "cerr")
    {
        strm = &cerr;
    }
    else if (s_strm == "LOG")
    {
        strm = BESLog::TheLog()->get_log_ostream();
    }
    else
    {
        strm = new ofstream(s_strm.c_str(), ios::out);
        if (strm && !(*strm))
        {
            delete strm;
            strm = 0;
            string err = "Unable to open the debug file: " + s_strm;
            throw BESInternalError(err, __FILE__, __LINE__);
        }
        created = true;
    }

    BESDebug::SetStrm(strm, created);

    string::size_type new_comma = 0;
    while ((new_comma = values.find(',', comma + 1)) != string::npos)
    {
        string flagName = values.substr(comma + 1, new_comma - comma - 1);
        if (flagName[0] == '-')
        {
            string newflag = flagName.substr(1, flagName.length() - 1);
            BESDebug::Set(newflag, false);
        }
        else
        {
            BESDebug::Set(flagName, true);
        }
        comma = new_comma;
    }
    string flagName = values.substr(comma + 1, values.length() - comma - 1);
    if (flagName[0] == '-')
    {
        string newflag = flagName.substr(1, flagName.length() - 1);
        BESDebug::Set(newflag, false);
    }
    else
    {
        BESDebug::Set(flagName, true);
    }
}

/** @brief return the pid as a string
 *
 * @return the pid as a string
 */
string BESDebug::GetPidStr()
{
    ostringstream strm;
    const time_t sctime = time(NULL);
    const struct tm *sttime = localtime(&sctime);
    char zone_name[10];
    strftime(zone_name, sizeof(zone_name), "%Z", sttime);
    char *b = asctime(sttime);
    strm << zone_name << " ";
    for (register int j = 0; b[j] != '\n'; j++)
        strm << b[j];
    pid_t thepid = getpid();
    strm << " id: " << thepid;
    return strm.str();
}

/** @brief Writes help information for so that developers know what can
 * be set for debugging.
 *
 * Displays information about possible debugging context, such as nc,
 * hdf4, bes
 *
 * @param strm output stream to write the help information to
 */
void BESDebug::Help(ostream &strm)
{
    strm << "Debug help:" << endl << "  Set on the command line with " << "-d \"file_name|cerr,[-]context1,...,[-]context\"" << endl
            << "  context with dash (-) in front will be turned off" << endl << "  context of all will turn on debugging for all contexts" << endl << endl
            << "Possible context(s):" << endl;

    if (_debug_map.size())
    {
        BESDebug::debug_citer i = _debug_map.begin();
        BESDebug::debug_citer e = _debug_map.end();
        for (; i != e; i++)
        {
            strm << "  " << (*i).first << ": ";
            if ((*i).second)
                strm << "on" << endl;
            else
                strm << "off" << endl;
        }
    }
    else
    {
        strm << "  none specified" << endl;
    }
}

bool BESDebug::IsContextName(const string &name)
{
    return _debug_map.count(name) > 0;
}

/** This method looks at the current setting of the BESDebug object and builds
 *  a string that, when passed to a beslistener as the argument of the -d
 *  option, will mirror those settings. This is useful in code like the
 *  besdaemon, where debug contexts are set/cleared but that information has
 *  to be sent to the beslisteners to be used. The new option string will be
 *  built and the beslisteners restarted using it.
 */
string BESDebug::GetOptionsString()
{
    ostringstream oss;

    if (_debug_map.size())
    {
        BESDebug::debug_citer i = _debug_map.begin();
        BESDebug::debug_citer e = _debug_map.end();
        for (; i != e; i++)
        {
            if (!(*i).second)
                oss << "-";
            oss << (*i).first << ",";
        }
        string retval = oss.str();
        return retval.erase(retval.length()-1);
    }
    else
    {
        return "";
    }
}

