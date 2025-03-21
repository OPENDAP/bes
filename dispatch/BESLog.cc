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
#include <ctime>
#include <string>
#include <sstream>

#include "BESLog.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "TheBESKeys.h"
#include "BESInternalFatalError.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#define MODULE "bes"
#define prolog std::string("BESLog::").append(__func__).append("() - ")

using namespace std;

BESLog *BESLog::d_instance = nullptr;
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
BESLog::BESLog() {

    bool found = false;
    try {
        d_instance_id = TheBESKeys::TheKeys()->read_string_key("AWS.instance-id", "-");
    }
    catch (BESInternalFatalError &bife) {
        stringstream msg;
        msg << prolog << "ERROR - Caught BESInternalFatalError! Will re-throw. Message: " << bife.get_message() << "  File: " << bife.get_file() << " Line: " << bife.get_line() << endl;
        BESDEBUG(MODULE,msg.str());
        cerr << msg.str();
        throw;
    }
    catch (...) {
        stringstream msg;
        msg << prolog << "FATAL ERROR: Caught unknown exception! Unable to determine log file name." << endl;
        BESDEBUG(MODULE,msg.str());
        cerr << msg.str();
        throw BESInternalFatalError(msg.str(), __FILE__, __LINE__);
    }

    // Find the log filename.
    try {
        TheBESKeys::TheKeys()->get_value("BES.LogName", d_file_name, found);
    }
    catch (BESInternalFatalError &bife) {
        stringstream msg;
        msg << prolog << "ERROR - Caught BESInternalFatalError! Will re-throw. Message: " << bife.get_message() << "  File: " << bife.get_file() << " Line: " << bife.get_line() << endl;
        BESDEBUG(MODULE,msg.str());
        cerr << msg.str();
        throw;
    }
    catch (...) {
        stringstream msg;
        msg << prolog << "FATAL ERROR: Caught unknown exception! Unable to determine log file name." << endl;
        BESDEBUG(MODULE,msg.str());
        cerr << msg.str();
        throw BESInternalFatalError(msg.str(), __FILE__, __LINE__);
    }


    // By default, use UTC in the logs.
    found = false;
    try {
        string local_time;
        TheBESKeys::TheKeys()->get_value("BES.LogTimeLocal", local_time, found);
        d_use_local_time = found && (BESUtil::lowercase(local_time) == "yes");
        BESDEBUG(MODULE, prolog << "d_use_local_time: " << (d_use_local_time?"true":"false") << endl);
    }
    catch (...) {
        stringstream err;
        err << prolog << "FATAL ERROR: Caught unknown exception. Failed to read the value of BES.LogTimeLocal" << endl;
        BESDEBUG(MODULE,err.str());
        cerr << err.str() << endl;
        throw BESInternalFatalError(err.str(), __FILE__, __LINE__);
    }

    if (d_file_name.empty()) {
        stringstream err;
        err << prolog << "FATAL ERROR: unable to determine log file name. ";
        err << "Please set BES.LogName in your initialization file" << endl;
        BESDEBUG(MODULE,err.str());
        cerr << err.str() << endl;
        throw BESInternalFatalError(err.str(), __FILE__, __LINE__);
    }

    d_file_buffer = new ofstream(d_file_name.c_str(), ios::out | ios::app);
    if (!(*d_file_buffer)) {
        stringstream err;
        err << prolog << "BES Fatal; cannot open log file " + d_file_name + "." << endl;
        BESDEBUG(MODULE,err.str());
        cerr << err.str() << endl;
        throw BESInternalFatalError(err.str(), __FILE__, __LINE__);
    }

    found = false;
    string s;
    TheBESKeys::TheKeys()->get_value("BES.LogVerbose", s, found);
    d_verbose = found && (BESUtil::lowercase(s) == "yes");
    BESDEBUG(MODULE, prolog << "d_verbose: " << (d_verbose?"true":"false") << endl);

    found = false;
    s = "";
    TheBESKeys::TheKeys()->get_value("BES.LogUnixTime", s, found);
    d_use_unix_time = found && (BESUtil::lowercase(s)=="true");
    BESDEBUG(MODULE, prolog << "d_use_unix_time: " << (d_use_unix_time?"true":"false") << endl);

    // Set the pid and build the log rord prolog base...
    update_pid();
}

/** @brief Cleans up the logging mechanism
 *
 * Cleans up the logging mechanism by closing the log file.
 */
BESLog::~BESLog()
{
    d_file_buffer->close();
    delete d_file_buffer;
    d_file_buffer = nullptr;
}

/**
* @brief Update the d_pid and the d_log_record_prolog_base values.
*/
pid_t BESLog::update_pid()
{
    auto pid = getpid();
    d_pid = to_string(pid);
    d_log_record_prolog_base = mark + d_instance_id + mark + d_pid + mark;
    return pid;
}


/** @brief Protected method that returns a string with the first fields of a log record.
 *
 * By default, the time will be expressed in ISO8601 format: "YYYY-MM-DDTHH:MM:SS zone"
 *
 * However, if BES.LogUnixTime=true appears in the BES configuration then time
 * will be expressed as a numeric unix time value.
 *
 * @return The string: time + mark + pid + mark
 */
std::string BESLog::log_record_begin() const {
    string log_record_prolog;

    time_t now;
    time(&now);
    if(d_use_unix_time){
        log_record_prolog = std::to_string(now);
    }
    else {
        char buf[sizeof "YYYY-MM-DDTHH:MM:SS zones"];
        tm date_time{};
        if (!d_use_local_time){
            gmtime_r(&now, &date_time);
        }
        else{
            localtime_r(&now, &date_time);
        }
        (void)strftime(buf, sizeof buf, "%FT%T %Z", &date_time);
        log_record_prolog = buf;
    }

    log_record_prolog += d_log_record_prolog_base + get_request_id() + mark;
    return log_record_prolog;
}


/**
 * @brief Writes msg to a log record with type lrt.
 * @param lrt The log record type
 * @param msg The message to be logged.
 */
void BESLog::log_record(const std::string &lrt, const std::string &msg) const {

    *d_file_buffer << log_record_begin() << lrt << mark << msg ;
    if(!msg.empty() && msg.back() != '\n')
        *d_file_buffer << "\n";

    *d_file_buffer << flush;
}

/**
 * @brief Writes msg, file, and line to a trace log record with type lrt.
 * @param lrt The log record type
 * @param msg The message to be logged.
 */
void BESLog::trace_log_record(const std::string &lrt, const std::string &msg, const std::string &file, const int line) const {

    *d_file_buffer << log_record_begin() << "trace-" << lrt << mark;
    *d_file_buffer << file << mark << line << mark << msg ;
    if(!msg.empty() && msg.back() != '\n')
        *d_file_buffer << "\n";

    *d_file_buffer << flush;
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
    strm << BESIndent::LMarg << "BESLog::dump - (" << (void *) this << ")\n";
    BESIndent::Indent();
    strm << BESIndent::LMarg << "    log file: " << d_file_name;
    if (d_file_buffer && *d_file_buffer) {
        strm << BESIndent::LMarg << " (log is valid)\n";
    }
    else {
        strm << BESIndent::LMarg << " (log is NOT valid)\n";
    }
    strm << BESIndent::LMarg << "    d_verbose: " << (d_verbose?"enabled":"disable") << "\n";
    strm << BESIndent::LMarg << "d_instance_id: " << d_instance_id << "\n";
    strm << BESIndent::LMarg << "        d_pid: " << d_pid << "\n";
    BESIndent::UnIndent();
}

/**
* @brief Sets the current request id (cached in BESLog) to id
*/
void BESLog::set_request_id(const std::string &id){
    request_id = id;
    BESDEBUG(MODULE, "request_id: " << request_id << endl);
}



BESLog *
BESLog::TheLog()
{
    if (d_instance == nullptr) {
        d_instance = new BESLog;
    }
    return d_instance;
}

