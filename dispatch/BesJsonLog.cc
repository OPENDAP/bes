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

#include <BESContextManager.h>
#include <BESDataHandlerInterface.h>
#include <BESDataNames.h>

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

/**
 * @TODO Is this better or possibly worse than a #define in a .cc file??
 *   How can we make this a single source of truth? Like move these to BesJsonLog
 *   So we have BesJsonLog::PID_KEY etc. What could go wrong?
 */
const static auto TIME_KEY = "time";
const static auto BES_START_TIME_KEY = "bes_start_time";
const static auto PID_KEY = "pid";
const static auto TYPE_KEY = "type";
const static auto MESSAGE_KEY = "message";
const static auto REQUEST_LOG_TYPE = "request";
const static auto ERROR_LOG_TYPE = "error";
const static auto INFO_LOG_TYPE = "info";
const static auto VERBOSE_LOG_TYPE = "verbose";
const static auto ACTION_KEY = "action";
const static auto RETURN_AS_KEY = "return_as";
const static auto LOCAL_PATH_KEY = "local_path";
const static auto CE_KEY = "constraint_expression";
const static auto D4_FUNCTION_KEY = "dap4_function";
const static auto COMMA_SPACE = ", ";


static auto BESKeys_LOG_NAME_KEY = "BES.LogName";
static auto BESKeys_LOG_TIME_LOCAL_KEY = "BES.LogTimeLocal";
static auto BESKeys_LOG_VERBOSE_KEY = "BES.LogVerbose";
static auto BESKeys_LOG_UNIXTIME_KEY = "BES.LogUnixTime";
/**
 * @brief boolean to string
 */
static string torf(bool v){
  return v?"true":"false";
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
        TheBESKeys::TheKeys()->get_value(BESKeys_LOG_TIME_LOCAL_KEY, local_time, found);
        d_use_local_time = found && (BESUtil::lowercase(local_time) == "yes");
        BESDEBUG(MODULE, prolog << "d_use_local_time: " << torf(d_use_local_time) << endl);
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
    TheBESKeys::TheKeys()->get_value(BESKeys_LOG_VERBOSE_KEY, s, found);
    d_verbose = found && (BESUtil::lowercase(s) == "yes");
    BESDEBUG(MODULE, prolog << "d_verbose: " << torf(d_verbose) << endl);
    init_state.append(" d_verbose=").append(torf(d_verbose));

    found = false;
    s = "";
    TheBESKeys::TheKeys()->get_value(BESKeys_LOG_UNIXTIME_KEY, s, found);
    d_use_unix_time = found && (BESUtil::lowercase(s)=="true");
    BESDEBUG(MODULE, prolog << "d_use_unix_time: " << torf(d_use_unix_time) << endl);
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
    info_log(init_state);
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
    d_file_buffer = nullptr;
}


//#######################################################################
#if USE_OPERATORS
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

static std::string &json_sanitize(std::string &str) {
    const auto the_bad_things ="\r\n\"\\\t";
    //const auto the_bad_things ="\n";
    size_t pos = 0;
    while ((pos = str.find_first_of(the_bad_things, pos)) != std::string::npos) {
        str[pos] = ' ';
    }
    return str;
}

/**
 * @brief Write a kvp as json where the value is a "json santizied" string.
 * @param os The output stream to which to write characters.
 * @param key The key name to be used in the kvp pair.
 * @param value The value, will be sanitized and modified!!
 * @param trailer A string that will be appended to the end of the value expression.
 */
void BesJsonLog::kvp_json_string_esc(std::ostream *os, const std::string &key, std::string& value, const std::string &trailer)
{
    *os << "\"" << key << "\": \"" << json_sanitize(value) << "\"" << trailer;
}
/**
 * @brief Write a kvp as json where the value is a  string.
 * @param os The output stream to which to write characters.
 * @param key The key name to be used in the kvp pair.
 * @param value The value.
 */
void BesJsonLog::kvp_json_string(std::ostream *os, const std::string &key, const std::string& value, const std::string &trailer)
{
    *os << "\"" << key << "\": \"" << value << "\"" << trailer;
}

/**
 * @brief Write a kvp as json where the value is numeric.
 * @param os The outout stream to which to write characters.
 * @param key The key name to be used in the kvp pair.
 * @param value The numeric value to be used as the value. Must be submitted as a string.
 */
void BesJsonLog::kvp_json_number(std::ostream *os, const std::string &key, const std::string& value, const std::string &trailer)
{
    *os << "\"" << key << "\": " << value << trailer;
}

void BesJsonLog::message_log_worker(const std::string &type, std::string &msg, const bool escape){
    if (!d_suspended) {
        time_t now;
        time(&now);
        *d_file_buffer << "{";
        kvp_json_number(d_file_buffer , TIME_KEY, to_string(now), COMMA_SPACE);
        kvp_json_number(d_file_buffer , PID_KEY, to_string(getpid()), COMMA_SPACE);
        kvp_json_string(d_file_buffer, TYPE_KEY, type, COMMA_SPACE);
        if(escape) {
            kvp_json_string_esc(d_file_buffer, MESSAGE_KEY, msg);

        } else {
            kvp_json_string(d_file_buffer, MESSAGE_KEY, msg);
        }
         *d_file_buffer << "}\n" << flush;
   }
}

void BesJsonLog::info_log(std::string &msg){
    message_log_worker(INFO_LOG_TYPE, msg, false);
    // message_worker("info", msg);
}

void BesJsonLog::error_log(std::string &msg){
    message_log_worker(ERROR_LOG_TYPE, msg, true);
    // message_worker("error", msg);
}

void BesJsonLog::verbose_log(std::string &msg){
  	if(is_verbose()){
        message_log_worker(VERBOSE_LOG_TYPE, msg, false);
    	// message_worker("verbose", msg);
    }
}

/**
 * @brief Writes the request log record, as a json, to the log_stream.
 *
 * @param d_dhi_ptr The BESDataHandlerInterface for the current request.
 * @param log_stream The stream to which the request log message will be written.
 */
void BesJsonLog::request_log(BESDataHandlerInterface *d_dhi_ptr, std::ostream *log_stream)
{
    *log_stream << "{ ";

    time_t now;
    time(&now);

    kvp_json_number(log_stream, TIME_KEY, to_string(now), COMMA_SPACE);
    kvp_json_number(log_stream, PID_KEY, to_string(getpid()), COMMA_SPACE);
    kvp_json_number(log_stream, TYPE_KEY, REQUEST_LOG_TYPE, COMMA_SPACE);

    // If the OLFS sent its log info, integrate that into the log output
    bool found = false;
    string olfs_log_line = BESContextManager::TheManager()->get_context("olfsLog", found);
    if(found){
        *log_stream << "\"olfs\": " << olfs_log_line << COMMA_SPACE;
    }

    kvp_json_string(log_stream, ACTION_KEY, d_dhi_ptr->action, COMMA_SPACE);

    string return_as("-");
    if (!d_dhi_ptr->data[RETURN_CMD].empty()) {
        return_as = d_dhi_ptr->data[RETURN_CMD];
    }
    kvp_json_string(log_stream, RETURN_AS_KEY, return_as, COMMA_SPACE);

    // Assume this is DAP and thus there is at most one container. Log a warning if that's
    // not true. jhrg 11/14/17
    BESContainer *c = *(d_dhi_ptr->containers.begin());
    if (c) {
        // Add the "path" of the requested data to the log line
        string path("-");
        if (!c->get_real_name().empty()) {
            path = c->get_real_name();
        }
        kvp_json_string(log_stream, LOCAL_PATH_KEY, path, COMMA_SPACE);

        // Add the constraint expression to the log line
        // Try for a DAP2 CE first
        string ce("-");
        string d4_func("-");
        if (!c->get_constraint().empty()) {
            ce = c->get_constraint();
        }
        else {
            // No DAP2 CE? Try DAP4...
            if (!c->get_dap4_constraint().empty()) {
                ce = c->get_dap4_constraint();
            }
            if (!c->get_dap4_function().empty()) {
                d4_func= c->get_dap4_function();
            }
        }
        // We need the escaping version because a ce may have legit double quotes.
        kvp_json_string_esc(log_stream, CE_KEY, ce, COMMA_SPACE);
        // We need the escaping version because a dap4 ce may have legit double quotes.
        kvp_json_string_esc(log_stream, D4_FUNCTION_KEY, d4_func);
    }
    *log_stream << "}\n";
    *log_stream << flush;
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

