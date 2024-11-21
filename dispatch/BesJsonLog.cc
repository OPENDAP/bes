// BesJsonLog.cc

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
//      ndp       Nathan Potter <ndp@opendap.org>

#include "config.h"

#include <iostream>
#include <time.h>
#include <string>
#include <sstream>

#include "BesJsonLog.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "TheBESKeys.h"
#include "BESInternalFatalError.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#define ISO8601_TIME_IN_LOGS
#define MODULE "json_log"
#define prolog std::string("BesJsonLog::").append(__func__).append("() - ")

using namespace std;

BesJsonLog *BesJsonLog::d_instance = nullptr;

static auto TIME_KEY = "bes_start_time";
static auto PID_KEY = "pid";
static auto BESKeys_LOG_NAME_KEY = "BES.LogName";

static string torf(bool v){
  return (v?"true":"false");
}
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
 * @throws BESInternalError if BesJsonLogName is not set or if there are
 * problems opening or writing to the log file.
 * @see BESKeys
 */
BesJsonLog::BesJsonLog() :
    d_flushed(1), d_file_buffer(nullptr), d_suspended(false), d_verbose(false), d_use_local_time(false), d_use_unix_time(false)
{
    bool found = false;
	string init_state = prolog;;

    // By default, use UTC in the logs.
    string local_time;
    try {
        TheBESKeys::TheKeys()->get_value("BES.LogTimeLocal", local_time, found);
        d_use_local_time = found && (BESUtil::lowercase(local_time) == "yes");
        BESDEBUG(MODULE, prolog << "d_use_local_time: " << (d_use_local_time?"true":"false") << endl);
        init_state.append("d_use_local_time=").append(torf(d_use_local_time));
    }
    catch (...) {
        stringstream err;
        err << prolog << "FATAL ERROR: Caught unknown exception. Failed to read the value of BES.LogTimeLocal" << endl;
        BESDEBUG(MODULE,err.str());
        cerr << err.str() << endl;
        throw BESInternalFatalError(err.str(), __FILE__, __LINE__);
    }

    found = false;
    string s;
    TheBESKeys::TheKeys()->get_value("BES.LogVerbose", s, found);
    d_verbose = found && (BESUtil::lowercase(s) == "yes");
    BESDEBUG(MODULE, prolog << "d_verbose: " << (d_verbose?"true":"false") << endl);
    init_state.append(" d_verbose=").append(torf(d_verbose));

    found = false;
    s = "";
    TheBESKeys::TheKeys()->get_value("BES.LogUnixTime", s, found);
    d_use_unix_time = found && (BESUtil::lowercase(s)=="true");
    BESDEBUG(MODULE, prolog << "d_use_unix_time: " << (d_use_unix_time?"true":"false") << endl);
    init_state.append(" d_use_unix_time=").append(torf(d_use_unix_time));

    found = false;
    try {
        TheBESKeys::TheKeys()->get_value(BESKeys_LOG_NAME_KEY, d_file_name, found);
        BESDEBUG(MODULE, prolog << "d_file_name: "+ d_file_name+"\n");
        d_file_name = d_file_name.append(".json");
        BESDEBUG(MODULE, prolog << "Using d_file_name: "+ d_file_name+"\n");
     	init_state.append(" d_file_name=").append(d_file_name);
  }
    catch (BESInternalFatalError &bife) {
        stringstream msg;
        msg << prolog << "ERROR - Caught BESInternalFatalError! Will re-throw. Message: " << bife.get_message() << "  File: " << bife.get_file() << " Line: " << bife.get_line() << endl;
        BESDEBUG(MODULE,msg.str());
        cerr << msg.str();
        throw bife;
    }
    catch (...) {
        stringstream msg;
        msg << prolog << "FATAL ERROR: Caught unknown exception! Unable to determine log file name." << endl;
        BESDEBUG(MODULE,msg.str());
        cerr << msg.str();
        throw BESInternalFatalError(msg.str(), __FILE__, __LINE__);
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
    BESDEBUG(MODULE, prolog << "Successfully opened BES log file: " << d_file_name << "\n");
    BESDEBUG(MODULE, init_state << "\n");
    info(init_state);
    flush_me();


}

/** @brief Cleans up the logging mechanism
 *
 * Cleans up the logging mechanism by closing the log file.
 */
BesJsonLog::~BesJsonLog()
{
    d_file_buffer->close();
    delete d_file_buffer;
    d_file_buffer = 0;
}

void BesJsonLog::dump_time(){
#ifdef ISO8601_TIME_IN_LOGS
    time_t now;
    time(&now);

    char buf[sizeof "YYYY-MM-DDTHH:MM:SS zones"];
    if(d_use_unix_time){
        (*d_file_buffer) << "\"" << TIME_KEY <<"\": " << now << ", ";
    }
    else {
        struct tm date_time{};
        if (!d_use_local_time){
            gmtime_r(&now, &date_time);
        }
        else{
            localtime_r(&now, &date_time);
        }
        (void)strftime(buf, sizeof buf, "%FT%T %Z", &date_time);
        (*d_file_buffer) << "\"" << TIME_KEY <<"\": \"" << buf << "\", ";
    }
#else
    const time_t sctime = time(NULL);
    const struct tm *sttime = localtime(&sctime);
    char zone_name[10];
    strftime(zone_name, sizeof(zone_name), "%Z", sttime);
    char *b = asctime(sttime);

    (*d_file_buffer) << zone_name << " ";
    for (register int j = 0; b[j] != '\n'; j++)
        (*d_file_buffer) << b[j];

    string time_str = zone_name + " ";
    for (register int j = 0; b[j] != '\n'; j++){
        time_str += b[j];
    }
    (*d_file_buffer) << "\"" << TIME_KEY <<"\": \"" << time_str << "\", ";
#endif

    (*d_file_buffer) << "\"" << PID_KEY <<"\": " << getpid() << ", ";


    d_flushed = 0;

}


/** @brief Protected method that dumps the date/time and the pid to the log_entry
 *
 * Depending on the compile-time constant ISO8601_TIME_IN_LOGS,
 * the time is dumped to the log file in the format:
 * "MDT Thu Sep  9 11:05:16 2004", or in ISO8601 format:
 * "YYYY-MM-DDTHH:MM:SS zone"
 */
void BesJsonLog::add_time_and_pid(nlohmann::json &log_entry)
{
    BESDEBUG(MODULE, prolog << "BEGIN" << endl);


#ifdef ISO8601_TIME_IN_LOGS
    time_t now;
    time(&now);

    char buf[sizeof "YYYY-MM-DDTHH:MM:SS zones"];
    if(d_use_unix_time){
        log_entry[TIME_KEY] = now;
    }
    else {
        struct tm date_time{};
        if (!d_use_local_time){
            gmtime_r(&now, &date_time);
        }
        else{
            localtime_r(&now, &date_time);
        }
        (void)strftime(buf, sizeof buf, "%FT%T %Z", &date_time);
         log_entry[TIME_KEY] =  buf;
    }
#else

    const time_t sctime = time(NULL);
    const struct tm *sttime = localtime(&sctime);
    char zone_name[10];
    strftime(zone_name, sizeof(zone_name), "%Z", sttime);
    char *b = asctime(sttime);

    string time_str = zone_name + " ";
    for (register int j = 0; b[j] != '\n'; j++){
        time_str += b[j];
    }
    log_entry[time_key] = time_str;
#endif
    log_entry[PID_KEY] = getpid();
    d_flushed = 0;
    BESDEBUG(MODULE, prolog << "END" << "\n");
}

//#######################################################################
#if USE_OPERATORS
/** @brief Overloaded inserter that writes the specified string.
 *
 * @todo Decide if this is really necessary.
 *
 * @param s string to write to the log file
 */
BesJsonLog& BesJsonLog::operator<<(string &s)
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
BesJsonLog& BesJsonLog::operator<<(const string &s)
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
BesJsonLog& BesJsonLog::operator<<(char *val)
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
BesJsonLog& BesJsonLog::operator<<(const char *val)
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
BesJsonLog& BesJsonLog::operator<<(int val)
{
    if (!d_suspended) {
        if (d_flushed) dump_time();
        (*d_file_buffer) << val;
    }
    return *this;
}

BesJsonLog& BesJsonLog::operator<<(unsigned int val)
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
BesJsonLog& BesJsonLog::operator<<(char val)
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
BesJsonLog& BesJsonLog::operator<<(long val)
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
BesJsonLog& BesJsonLog::operator<<(unsigned long val)
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
BesJsonLog& BesJsonLog::operator<<(double val)
{
    if (!d_suspended) {
        if (d_flushed) dump_time();
        (*d_file_buffer) << val;
    }
    return *this;
}

#if 0
BesJsonLog& BesJsonLog::operator<<(nlohmann::json &log_entry)
{
    if (!d_suspended) {
        if (d_flushed) {
            add_time_and_pid(log_entry);
        }
        string json_value=log_entry.dump(4);
        (*d_file_buffer) << json_value << "\n";
        BESDEBUG(MODULE, prolog << "\n" << json_value << "\n");
    }
    return *this;
}
#endif

/** @brief Overloaded inserter that takes stream manipulation methods.
 *
 * Overloaded inserter that can take the address of endl, flush and ends
 * functions. This inserter is based on p_ostream_manipulator, therefore
 * the C++ standard functions for I/O endl, flush, and ends can be applied
 * to objects of the class BesJsonLog.
 */
BesJsonLog& BesJsonLog::operator<<(p_ostream_manipulator val)
{
    if (!d_suspended) {
        (*d_file_buffer) << val;
        if ((val == (p_ostream_manipulator) endl) || (val == (p_ostream_manipulator) flush)) d_flushed = 1;
    }
    return *this;
}

/** @brief Overloaded inserter that takes ios methods
 *
 * Overloaded inserter that can take the address oct, dec and hex functions.
 * This inserter is based on p_ios_manipulator, therefore the C++ standard
 * functions oct, dec and hex can be applied to objects of the class BesJsonLog.
 */
BesJsonLog& BesJsonLog::operator<<(p_ios_manipulator val)
{
    if (!d_suspended) (*d_file_buffer) << val;
    return *this;
}

#endif
//#######################################################################

void check_ostream(std::ostream &os){
    if(!os.good()){
        std::stringstream ss;
        ss << prolog << "OUCH! Logging OutputStream is in trouble!";
        ss << " good()=" << torf(os.good());
        ss << " eof()="  << torf(os.eof());
        ss << " fail()=" << torf(os.fail());
        ss << " bad()="  << torf(os.bad());
        throw BESInternalFatalError(ss.str(), __FILE__, __LINE__);
    }
}

void BesJsonLog::flush_me(){
    (*d_file_buffer) << flush;
    d_flushed = 1;
}

void BesJsonLog::message_worker(const std::string &type, const std::string &msg){
    nlohmann::json log_entry;
    if (!d_suspended) {
        add_time_and_pid(log_entry);
        log_entry["type"] = type;
        log_entry["message"] = msg;
        *d_file_buffer << log_entry.dump() << "\n" << flush;
    }
}

void BesJsonLog::request(nlohmann::json &log_entry){
    if (!d_suspended) {
        log_entry["type"] = "request";
        add_time_and_pid(log_entry);
        check_ostream(*d_file_buffer);
        *d_file_buffer << log_entry.dump() << "\n" << flush;
    }
}

void BesJsonLog::info(const std::string &msg){
    message_worker("info", msg);
}

void BesJsonLog::error(const std::string &msg){
    message_worker("error", msg);
}

void BesJsonLog::verbose(const std::string &msg){
  	if(is_verbose()){
    	message_worker("verbose", msg);
    }
}


/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance along with information about
 * the log file
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BesJsonLog::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BesJsonLog::dump - (" << (void *) this << ")" << endl;
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

BesJsonLog *
BesJsonLog::TheLog()
{
    if (d_instance == nullptr) {
        d_instance = new BesJsonLog;
    }
    return d_instance;
}

