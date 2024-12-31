// BESLog.h

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

#ifndef BESLog_h_
#define BESLog_h_ 1

#include "config.h"

#include <fstream>
#include <string>

#include "BESObj.h"

// Note that the BESLog::operator<<() methods will prefix output with
// the time and PID by checking for the flush and endl stream operators.
//
// TRACE_LOGGING provides a way to see just where in the code the log info
// is written from. jhrg 11/14/17

#undef  TRACE_LOGGING

#ifdef TRACE_LOGGING
#define REQUEST_LOG(x) do { BESLog::TheLog()->trace_request(x, __FILE__ ,__LINE__); } while(0)
#define INFO_LOG(x)    do { BESLog::TheLog()->trace_info(x, __FILE__ ,__LINE__); } while(0)
#define ERROR_LOG(x)   do { BESLog::TheLog()->trace_error(x, __FILE__ ,__LINE__); } while(0)
#define VERBOSE(x)     do {if (BESLog::TheLog()->is_verbose()) BESLog::TheLog()->trace_verbose(x, __FILE__, __LINE__; } while(0)
#define TIMING_LOG(x)  do { BESLog::TheLog()->trace_timing(x, __FILE__ ,__LINE__); } while(0)
#else
#define REQUEST_LOG(x) do { BESLog::TheLog()->request(x); } while(0)
#define INFO_LOG(x)    do { BESLog::TheLog()->info(x); } while(0)
#define ERROR_LOG(x)   do { BESLog::TheLog()->error(x); } while(0)
#define VERBOSE(x)     do {if (BESLog::TheLog()->is_verbose()) BESLog::TheLog()->verbose(x); } while(0)
#define TIMING_LOG(x)  do { BESLog::TheLog()->timing(x); } while(0)
#endif


/** @brief Provides a mechanism for applications to log information to an
 * external file.
 *
 * BESLog provides a mechanism for applications to log information to an
 * external file, such as debugging information. This file is defined in the
 * BESKeys mechanism using the key BES.LogName.
 *
 * Also provides a mechanism to define whether debugging information should be
 * verbose or not using the BESKeys key/value pair BES.LogVerbose.
 *
 * Logging can also be suspended and resumed using so named methods.
 *
 * A log record is synonymous with a log line.
 * Log fields are seperated by BESLog::mark (currently "|&|")
 *
 * All log records have a "prolog" which consists of:
 * <pre>
 * current_time + BESLog::mark + to_string(getpid()) + BESLog::mark + "record_type" + BESLog::mark
 * </pre>
 *
 * The BESLog API implements methods to write log records of type request, info, error,
 * verbose, and timing. All of these logs take a single message string as their input.
 * The users of the request logger (BESXMLInterface) and the timing logger (BESStopWatch)
 * are responsible for injecting additional log fields for their respective use cases into
 * the message and ultimately the log record.
 *
 * It is preferred to use the logging macros REQUEST_LOG(x), INFO_LOG(x), ERROR_LOG(x),
 * VERBOSE_LOG(x), and TIMING_LOG(x) where x is the message string.
 *
 * <PRE>
 *     INFO_LOG("This is some information to be logged...");
 *     ERROR_LOG("OUCH! The bad things happened!");
 *     VERBOSE_LOG("I'm feeling chatty.");
 * </PRE>
 *
 * Note:
 *  - The content and order of the request log fields are determined in BESXMLInterface::log_the_command()
 *  - The content and order of the timing log fields are determined in BESStopWatch::~BESStopWatch();
 * <PRE>
 *     TIMING_LOG("timing field value" + BESLog::mark + "next timing field value" + BESLog::mark + "another value");
 *     REQUEST_LOG("request field value" + BESLog::mark + "next request field value" + BESLog::mark + "another value");
 * </PRE>
 *
 * BESLog provides a static method for access to a single BESLog object,
 * TheLog.
 *
 * @see TheBESKeys
 */
class BESLog: public BESObj {
private:
    static BESLog * d_instance;

    std::ofstream *d_file_buffer = nullptr;
    std::string d_file_name;
    std::string d_instance_id = "-";
    std::string d_pid = "-";
    std::string d_log_record_prolog_base;

    // Flag to indicate whether to log verbose messages
    bool d_verbose = false;

    // Use UTC by default
    bool d_use_local_time = false;

    // Use the UNIX time value as the log time.
    bool d_use_unix_time = false;

    const char* REQUEST_LOG_TYPE_KEY = "request";
    const char* INFO_LOG_TYPE_KEY = "info";
    const char* ERROR_LOG_TYPE_KEY = "error";
    const char* VERBOSE_LOG_TYPE_KEY = "verbose";
    const char* TIMING_LOG_TYPE_KEY = "timing";

protected:
    BESLog();

    // Starts a log record with time and PID.
    std::string log_record_begin() const;

    void log_record(const std::string &record_type, const std::string &msg) const;
    void trace_log_record(const std::string &record_type, const std::string &msg, const std::string &file, int line) const;

public:
    ~BESLog() override;

    const static std::string mark;

    /** @brief turn on verbose logging
     *
     * This method turns on verbose logging, providing applications the
     * ability to log more detailed debugging information. If verbose is
     * already turned on then nothing is changed.
     */
    void verbose_on(){ d_verbose = true; }

    /** @brief turns off verbose logging
     *
     * This method turns off verbose logging. If verbose logging was not
     * already turned on then nothing changes.
     */
    void verbose_off() { d_verbose = false; }

    /** @brief Returns true if verbose logging is requested.
     *
     * This method returns true if verbose logging has been requested either
     * by setting the BESKeys key/value pair BES.LogVerbose=value or by
     * turning on verbose logging using the method verbose_on.
     *
     * If BES.LogVerbose is set to Yes, YES, or yes then verbose logging is
     * turned on. If set to anything else then verbose logging is not turned
     * on.
     *
     * @return true if verbose logging has been requested.
     * @see verbose_on
     * @see verbose_off
     * @see BESKeys
     */
    bool is_verbose() const { return d_verbose; }

    pid_t update_pid();

    /**
    * @brief Writes request msg to the log stream.
    */
    void request(const std::string &msg) const {
        log_record(REQUEST_LOG_TYPE_KEY, msg);
    }

    /**
    * @brief Writes info msg to the log stream.
    */
    void info(const std::string &msg) const {
        log_record(INFO_LOG_TYPE_KEY, msg);
    }

    /**
    * @brief Writes error msg to the log stream.
    */
    void error(const std::string &msg) const {
        log_record(ERROR_LOG_TYPE_KEY, msg);
    }

    /**
    * @brief Writes verbose msg to the log stream, if verbose logging is enabled.
    */
    void verbose(const std::string &msg) const {
        if(d_verbose) {
            log_record(VERBOSE_LOG_TYPE_KEY, msg);
        }
    }

    /**
    * @brief Writes timing msg to the log stream.
    */
    void timing(const std::string &msg) const {
        log_record(TIMING_LOG_TYPE_KEY, msg);
    }

    /**
    * @brief Writes request msg to the log stream with FILE and LINE
    */
    void trace_request(const std::string &msg, const std::string &file, int line) const {
        trace_log_record(REQUEST_LOG_TYPE_KEY, msg, file, line);
    }

    /**
    * @brief Writes info msg to the log stream with FILE and LINE
    */
    void trace_info(const std::string &msg, const std::string &file, int line) const {
        trace_log_record(INFO_LOG_TYPE_KEY, msg, file, line);
    }

    /**
    * @brief Writes error msg to the log stream with FILE and LINE
    */
    void trace_error(const std::string &msg, const std::string &file, int line) const {
        trace_log_record(ERROR_LOG_TYPE_KEY, msg, file, line);
    }

    /**
    * @brief Writes verbose msg to the log stream with FILE and LINE, if verbose logging is enabled.
    */
    void trace_verbose(const std::string &msg, const std::string &file, int line) const {
        if(d_verbose) {
            trace_log_record(VERBOSE_LOG_TYPE_KEY, msg, file, line);
        }
    }

    /**
    * @brief Writes timing msg to the log stream with FILE and LINE
    */
    void trace_timing(const std::string &msg, const std::string &file, int line) const {
        trace_log_record(TIMING_LOG_TYPE_KEY, msg, file, line);
    }

    void dump(std::ostream &strm) const override;

    static BESLog *TheLog();

    // I added this so that it's easy to route the BESDebug messages to the
    // log file. This will enable the Admin Interface to display the contents
    // of those debug messages when it displays the log file. jhrg
    std::ostream *get_log_ostream() const {
        return d_file_buffer;
    }
};

#endif // BESLog_h_

