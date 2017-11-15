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

#define ISO8601_TIME_IN_LOGS

using namespace std;

BESLog *BESLog::d_instance = 0;
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
 * By default, log using UTC. BES.LogTimeLocal=yes will switch to using local
 * time. Times are recorded in IOS8601.
 *
 * @throws BESInternalError if BESLogName is not set or if there are
 * problems opening or writing to the log file.
 * @see BESKeys
 */
BESLog::BESLog() :
    d_flushed(1), d_file_buffer(0), d_suspended(0), d_verbose(false), d_use_local_time(false)
{
    d_suspended = 0;
    bool found = false;
    try {
        TheBESKeys::TheKeys()->get_value("BES.LogName", d_file_name, found);
    }
    catch (...) {
        string err ="BES Fatal: unable to determine log file name. The key BES.LogName has multiple values";
        cerr << err << endl;
        throw BESInternalFatalError(err, __FILE__, __LINE__);
    }

    // By default, use UTC in the logs.
    found = false;
    string local_time = "no";
    try {
        TheBESKeys::TheKeys()->get_value("BES.LogTimeLocal", local_time, found);
        if (local_time == "YES" || local_time == "Yes" || local_time == "yes") {
            d_use_local_time = true;
        }

    }
    catch (...) {
        string err ="BES Fatal: Unable to read the value of BES.LogTimeUTC";
        cerr << err << endl;
        throw BESInternalFatalError(err, __FILE__, __LINE__);
    }

    if (d_file_name == "") {
        string err = "BES Fatal: unable to determine log file name. Please set BES.LogName in your initialization file";
        cerr << err << endl;
        throw BESInternalFatalError(err, __FILE__, __LINE__);
    }

    d_file_buffer = new ofstream(d_file_name.c_str(), ios::out | ios::app);
    if (!(*d_file_buffer)) {
        string err = "BES Fatal; cannot open log file " + d_file_name + ".";
        cerr << err << endl;
        throw BESInternalFatalError(err, __FILE__, __LINE__);
    }

    found = false;
    string verbose;
    TheBESKeys::TheKeys()->get_value("BES.LogVerbose", verbose, found);
    if (verbose == "YES" || verbose == "Yes" || verbose == "yes") {
        d_verbose = true;
    }
}

/** @brief Cleans up the logging mechanism
 *
 * Cleans up the logging mechanism by closing the log file.
 */
BESLog::~BESLog()
{
    d_file_buffer->close();
    delete d_file_buffer;
    d_file_buffer = 0;
}

/** @brief Protected method that dumps the date/time to the log file
 *
 * Depending on the compile-time constant ISO8601_TIME_IN_LOGS,
 * the time is dumped to the log file in the format:
 * "MDT Thu Sep  9 11:05:16 2004", or in ISO8601 format:
 * "YYYY-MM-DDTHH:MM:SS zone"
 */
void BESLog::dump_time()
{
#ifdef ISO8601_TIME_IN_LOGS
    time_t now;
    time(&now);
    char buf[sizeof "YYYY-MM-DDTHH:MM:SSzone"];
    int status = 0;

    // From StackOverflow:
    // This will work too, if your compiler doesn't support %F or %T:
    // strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%S%Z", gmtime(&now));
    //
    // Apologies for the twisted logic - UTC is the default. Override to
    // local time using BES.LogTimeLocal=yes in bes.conf. jhrg 11/15/17
    if (!d_use_local_time)
        status = strftime(buf, sizeof buf, "%FT%T%Z", gmtime(&now));
    else
        status = strftime(buf, sizeof buf, "%FT%T%Z", localtime(&now));

    (*d_file_buffer) << buf;

#else
    const time_t sctime = time(NULL);
    const struct tm *sttime = localtime(&sctime);
    char zone_name[10];
    strftime(zone_name, sizeof(zone_name), "%Z", sttime);
    char *b = asctime(sttime);

    (*d_file_buffer) << zone_name << " ";
    for (register int j = 0; b[j] != '\n'; j++)
        (*d_file_buffer) << b[j];
#endif

    (*d_file_buffer) << mark << getpid() << mark;

    d_flushed = 0;
}

/** @brief Overloaded inserter that writes the specified string.
 *
 * @todo Decide if this is really necessary.
 *
 * @param s string to write to the log file
 */
BESLog& BESLog::operator<<(string &s)
{
    if (!d_suspended) {
        if (d_flushed) dump_time();
        (*d_file_buffer) << s;
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified const string.
 *
 * @param s const string to write to the log file
 */
BESLog& BESLog::operator<<(const string &s)
{
    if (!d_suspended) {
        if (d_flushed) dump_time();
        (*d_file_buffer) << s;
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified char *.
 *
 * @param val char * to write to the log file
 */
BESLog& BESLog::operator<<(char *val)
{
    if (!d_suspended) {
        if (d_flushed) dump_time();
        if (val)
            (*d_file_buffer) << val;
        else
            (*d_file_buffer) << "NULL";
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified const char *.
 *
 * @param val const char * to write to the log file
 */
BESLog& BESLog::operator<<(const char *val)
{
    if (!d_suspended) {
        if (d_flushed) {
            dump_time();
        }
        if (val)
            (*d_file_buffer) << val;
        else
            (*d_file_buffer) << "NULL";
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified int value.
 *
 * @param val int value to write to the log file
 */
BESLog& BESLog::operator<<(int val)
{
    if (!d_suspended) {
        if (d_flushed) dump_time();
        (*d_file_buffer) << val;
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified char value.
 *
 * @param val char value to write to the log file
 */
BESLog& BESLog::operator<<(char val)
{
    if (!d_suspended) {
        if (d_flushed) dump_time();
        (*d_file_buffer) << val;
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified long value.
 *
 * @param val long value to write to the log file
 */
BESLog& BESLog::operator<<(long val)
{
    if (!d_suspended) {
        if (d_flushed) dump_time();
        (*d_file_buffer) << val;
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified unsigned long value.
 *
 * @param val unsigned long value to write to the log file
 */
BESLog& BESLog::operator<<(unsigned long val)
{
    if (!d_suspended) {
        if (d_flushed) dump_time();
        (*d_file_buffer) << val;
    }
    return *this;
}

/** @brief Overloaded inserter that writes the specified double value.
 *
 * @param val double value to write to the log file
 */
BESLog& BESLog::operator<<(double val)
{
    if (!d_suspended) {
        if (d_flushed) dump_time();
        (*d_file_buffer) << val;
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
    if (!d_suspended) {
        (*d_file_buffer) << val;
        if ((val == (p_ostream_manipulator) endl) || (val == (p_ostream_manipulator) flush)) d_flushed = 1;
    }
    return *this;
}

void BESLog::flush_me(){
    (*d_file_buffer) << flush;
    d_flushed = 1;
}

/** @brief Overloaded inserter that takes ios methods
 *
 * Overloaded inserter that can take the address oct, dec and hex functions.
 * This inserter is based on p_ios_manipulator, therefore the C++ standard
 * functions oct, dec and hex can be applied to objects of the class BESLog.
 */
BESLog& BESLog::operator<<(p_ios_manipulator val)
{
    if (!d_suspended) (*d_file_buffer) << val;
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
    strm << BESIndent::LMarg << "log file: " << d_file_name << endl;
    if (d_file_buffer && *d_file_buffer) {
        strm << BESIndent::LMarg << "log is valid" << endl;
    }
    else {
        strm << BESIndent::LMarg << "log is NOT valid" << endl;
    }
    strm << BESIndent::LMarg << "is verbose: " << d_verbose << endl;
    strm << BESIndent::LMarg << "is flushed: " << d_flushed << endl;
    strm << BESIndent::LMarg << "is suspended: " << d_suspended << endl;
    BESIndent::UnIndent();
}

BESLog *
BESLog::TheLog()
{
    if (d_instance == 0) {
        d_instance = new BESLog;
    }
    return d_instance;
}

