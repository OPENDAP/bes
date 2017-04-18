// BESLog.cc

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

#include <iostream>
#include <time.h>
#include <string>

#include "BESLog.h"
#include "TheBESKeys.h"
#include "BESInternalFatalError.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

using namespace std;

BESLog *BESLog::_instance = 0;
const string BESLog::mark = string("|&|");

/** @brief constructor that sets up logging for the application.
 *
 * Sets up logging for the application by opening up the logging file and
 * determining verbose logging.
 *
 * The file name is determined using the BESKeys mechanism. The key used is
 * BES.LogName. The application must be able to write to this directory/file.
 *
 * Verbose logging is determined also using the BESKeys mechanism. The key
 * used is BES.LogVerbose.
 *
 * @throws BESInternalError if BESLogName is not set or if there are
 * problems opening or writing to the log file.
 * @see BESKeys
 */
BESLog::BESLog() :
    _flushed(1), _file_buffer(0), _suspended(0), _verbose(false)
{
    _suspended = 0;
    bool found = false;
    try {
        TheBESKeys::TheKeys()->get_value("BES.LogName", _file_name, found);
    }
    catch (...) {
        string err = (string) "BES Fatal: unable to determine log file name."
            + " The key BES.LogName has multiple values";
        cerr << err << endl;
        throw BESInternalFatalError(err, __FILE__, __LINE__);
    }
    if (_file_name == "") {
        string err = (string) "BES Fatal: unable to determine log file name."
            + " Please set BES.LogName in your initialization file";
        cerr << err << endl;
        throw BESInternalFatalError(err, __FILE__, __LINE__);
    }
    _file_buffer = new ofstream(_file_name.c_str(), ios::out | ios::app);
    if (!(*_file_buffer)) {
        string err = (string) "BES Fatal; cannot open log file " + _file_name + ".";
        cerr << err << endl;
        throw BESInternalFatalError(err, __FILE__, __LINE__);
    }
    found = false;
    string verbose;
    TheBESKeys::TheKeys()->get_value("BES.LogVerbose", verbose, found);
    if (verbose == "YES" || verbose == "Yes" || verbose == "yes") {
        _verbose = true;
    }
}

/** @brief Cleans up the logging mechanism
 *
 * Cleans up the logging mechanism by closing the log file.
 */
BESLog::~BESLog()
{
    _file_buffer->close();
    delete _file_buffer;
    _file_buffer = 0;
}

/** @brief Protected method that dumps the date/time to the log file
 *
 * The time is dumped to the log file in the format:
 *
 * [MDT Thu Sep  9 11:05:16 2004 id: &lt;pid&gt;]
 */
void BESLog::dump_time()
{
    const time_t sctime = time(NULL);
    const struct tm *sttime = localtime(&sctime);
    char zone_name[10];
    strftime(zone_name, sizeof(zone_name), "%Z", sttime);
    char *b = asctime(sttime);
    (*_file_buffer) << mark << zone_name << " ";
    for (register int j = 0; b[j] != '\n'; j++)
        (*_file_buffer) << b[j];
    pid_t thepid = getpid();
    (*_file_buffer) << mark << "pid: " << thepid << mark;
    _flushed = 0;
}

/** @brief Overloaded inserter that writes the specified string.
 *
 * @todo Decide if this is really necessary.
 *
 * @param s string to write to the log file
 */
BESLog& BESLog::operator<<(string &s)
{
    if (!_suspended) {
        if (_flushed) dump_time();
        (*_file_buffer) << s;
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified const string.
 *
 * @param s const string to write to the log file
 */
BESLog& BESLog::operator<<(const string &s)
{
    if (!_suspended) {
        if (_flushed) dump_time();
        (*_file_buffer) << s;
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified char *.
 *
 * @param val char * to write to the log file
 */
BESLog& BESLog::operator<<(char *val)
{
    if (!_suspended) {
        if (_flushed) dump_time();
        if (val)
            (*_file_buffer) << val;
        else
            (*_file_buffer) << "NULL";
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified const char *.
 *
 * @param val const char * to write to the log file
 */
BESLog& BESLog::operator<<(const char *val)
{
    if (!_suspended) {
        if (_flushed) {
            dump_time();
        }
        if (val)
            (*_file_buffer) << val;
        else
            (*_file_buffer) << "NULL";
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified int value.
 *
 * @param val int value to write to the log file
 */
BESLog& BESLog::operator<<(int val)
{
    if (!_suspended) {
        if (_flushed) dump_time();
        (*_file_buffer) << val;
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified char value.
 *
 * @param val char value to write to the log file
 */
BESLog& BESLog::operator<<(char val)
{
    if (!_suspended) {
        if (_flushed) dump_time();
        (*_file_buffer) << val;
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified long value.
 *
 * @param val long value to write to the log file
 */
BESLog& BESLog::operator<<(long val)
{
    if (!_suspended) {
        if (_flushed) dump_time();
        (*_file_buffer) << val;
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified unsigned long value.
 *
 * @param val unsigned long value to write to the log file
 */
BESLog& BESLog::operator<<(unsigned long val)
{
    if (!_suspended) {
        if (_flushed) dump_time();
        (*_file_buffer) << val;
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified double value.
 *
 * @param val double value to write to the log file
 */
BESLog& BESLog::operator<<(double val)
{
    if (!_suspended) {
        if (_flushed) dump_time();
        (*_file_buffer) << val;
    }
    return *this;
}

/** @brief Overloaded inserter that takes stream manipulation methods.
 *
 * Overloaded inserter that can take the address of endl, flush and ends
 * functions. This inserter is based on p_ostream_manipulator, therefore
 * the C++ standard functions for I/O endl, flush, and ends can be applied
 * to objects of the class BESLog.
 */
BESLog& BESLog::operator<<(p_ostream_manipulator val)
{
    if (!_suspended) {
        (*_file_buffer) << val;
        if ((val == (p_ostream_manipulator) endl) || (val == (p_ostream_manipulator) flush)) _flushed = 1;
    }
    return *this;
}

void BESLog::flush_me(){
    (*_file_buffer) << flush;
    _flushed = 1;
}

/** @brief Overloaded inserter that takes ios methods
 *
 * Overloaded inserter that can take the address oct, dec and hex functions.
 * This inserter is based on p_ios_manipulator, therefore the C++ standard
 * functions oct, dec and hex can be applied to objects of the class BESLog.
 */
BESLog& BESLog::operator<<(p_ios_manipulator val)
{
    if (!_suspended) (*_file_buffer) << val;
    return *this;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * the log file
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESLog::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESLog::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "log file: " << _file_name << endl;
    if (_file_buffer && *_file_buffer) {
        strm << BESIndent::LMarg << "log is valid" << endl;
    }
    else {
        strm << BESIndent::LMarg << "log is NOT valid" << endl;
    }
    strm << BESIndent::LMarg << "is verbose: " << _verbose << endl;
    strm << BESIndent::LMarg << "is flushed: " << _flushed << endl;
    strm << BESIndent::LMarg << "is suspended: " << _suspended << endl;
    BESIndent::UnIndent();
}

BESLog *
BESLog::TheLog()
{
    if (_instance == 0) {
        _instance = new BESLog;
    }
    return _instance;
}

