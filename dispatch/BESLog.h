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
// #define MR_LOG(tag, msg) do { *(BESLog::TheLog()) << "trace-" << tag << BESLog::mark << __FILE__  << BESLog::mark << __LINE__ << BESLog::mark << msg ; BESLog::TheLog()->flush_me() ; } while( 0 )

#define REQUEST_LOG(x) do { BESLog::TheLog()->trace_request(x, __FILE__ ,__LINE__); } while(0)
#define INFO_LOG(x)    do { BESLog::TheLog()->trace_info(x, __FILE__ ,__LINE__); } while(0)
#define ERROR_LOG(x)   do { BESLog::TheLog()->trace_error(x, __FILE__ ,__LINE__); } while(0)
#define VERBOSE(x)     do {if (BESLog::TheLog()->is_verbose()) BESLog::TheLog()->trace_verbose(x, __FILE__, __LINE__; } while(0)
#define TIMING_LOG(x)  do { BESLog::TheLog()->trace_timing(x, __FILE__ ,__LINE__); } while(0)
#else
// #define MR_LOG(tag, msg) do { *(BESLog::TheLog()) << tag << BESLog::mark << msg ; BESLog::TheLog()->flush_me() ; } while( 0 )
#define REQUEST_LOG(x) do { BESLog::TheLog()->request(x); } while(0)
#define INFO_LOG(x)    do { BESLog::TheLog()->info(x); } while(0)
#define ERROR_LOG(x)   do { BESLog::TheLog()->error(x); } while(0)
#define VERBOSE(x)     do {if (BESLog::TheLog()->is_verbose()) BESLog::TheLog()->verbose(x); } while(0)
#define TIMING_LOG(x)  do { BESLog::TheLog()->timing(x); } while(0)
#endif

#define REQUEST_LOG_KEY "request"
#define INFO_LOG_KEY "info"
#define ERROR_LOG_KEY "error"
#define VERBOSE_LOG_KEY "verbose"


#define USE_IO_OPS false

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
 * BESLog is used similar to cerr and cout using the overloaded operator <<.
 *
 * <PRE>
 * if( BESLog::TheLog()->is_verbose() )
 * {
 *     *(BESLog::TheLog()) << "This is some information to be logged"
 * 		  << endl ;
 * }
 * </PRE>
 *
 * Types of data that can be logged include:
 * <UL>
 * <LI>std::string
 * <LI>char *
 * <LI>const char *
 * <LI>int
 * <LI>char
 * <LI>long
 * <LI>unsigned long
 * <LI>double
 * <LI>stream manipulators endl, ends and flush
 * <LI>ios manipulators hex, oct, dec
 * </UL>
 *
 * BESLog provides a static method for access to a single BESLog object,
 * TheLog.
 *
 * @see TheBESKeys
 */
class BESLog: public BESObj {
private:
    static BESLog * d_instance;

    int d_flushed;
    std::ofstream * d_file_buffer;
    std::string d_file_name;

    // Flag to indicate the object is not routing data to its associated stream
    int d_suspended;

    // Flag to indicate whether to log verbose messages
    bool d_verbose;

    bool d_use_local_time; // Use UTC by default

    bool d_use_unix_time; // Use the UNIX time value as the log time.

protected:
    BESLog();

    // Dumps the current system time.
    std::string log_record_begin();
public:
    ~BESLog() override;

    const static std::string mark;

    /** @brief Suspend logging of any information until resumed.
     *
     * This method suspends any logging of information. If already suspended
     * then nothing changes, logging is still suspended.
     */
    void suspend()
    {
        d_suspended = 1;
    }

    /** @brief Resumes logging after being suspended.
     *
     * This method resumes logging after suspended by the user. If logging was
     * not already suspended this method does nothing.
     */
    void resume()
    {
        d_suspended = 0;
    }

    /** @brief turn on verbose logging
     *
     * This method turns on verbose logging, providing applications the
     * ability to log more detailed debugging information. If verbose is
     * already turned on then nothing is changed.
     */
    void verbose_on()
    {
        d_verbose = true;
    }

    /** @brief turns off verbose logging
     *
     * This methods turns off verbose logging. If verbose logging was not
     * already turned on then nothing changes.
     */
    void verbose_off()
    {
        d_verbose = false;
    }

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
    bool is_verbose() const
    {
        return d_verbose;
    }

#if USE_IO_OPS
    /// Defines a data type p_ios_manipulator "pointer to function that takes ios& and returns ios&".
    typedef std::ios& (*p_ios_manipulator)(std::ios&);
    /// Defines a data type p_std::ostream_manipulator "pointer to function that takes std::ostream& and returns std::ostream&".
    typedef std::ostream& (*p_ostream_manipulator)(std::ostream&);

    BESLog& operator <<(std::string&);
    BESLog& operator <<(const std::string&);
    BESLog& operator <<(char*);
    BESLog& operator <<(const char*);
    BESLog& operator <<(int);
    BESLog& operator <<(unsigned int);
    BESLog& operator <<(char);
    BESLog& operator <<(long);
    BESLog& operator <<(unsigned long);
    BESLog& operator <<(double);

    BESLog& operator<<(p_ostream_manipulator);
    BESLog& operator<<(p_ios_manipulator);

#endif

    void log(const std::string &tag, const std::string &msg);
    void request(const std::string &msg){ log(REQUEST_LOG_KEY,msg); }
    void info(const std::string &msg){ log(INFO_LOG_KEY,msg); }
    void error(const std::string &msg){ log(ERROR_LOG_KEY,msg); }
    void verbose(const std::string &msg){ log(VERBOSE_LOG_KEY,msg); }
    void timing(const std::string &msg){ log("timing",msg); }

    void trace_log(const std::string &tag, const std::string &msg, const std::string &file, int line);

    void trace_request(const std::string &msg, const std::string &file, int line){ trace_log(REQUEST_LOG_KEY,msg, file, line ); }
    void trace_info(const std::string &msg, const std::string &file, int line){ trace_log("info",msg, file, line ); }
    void trace_error(const std::string &msg, const std::string &file, int line){ trace_log("error",msg, file, line ); }
    void trace_verbose(const std::string &msg, const std::string &file, int line){ trace_log("verbose",msg, file, line ); }
    void trace_timing(const std::string &msg, const std::string &file, int line){ trace_log("timing",msg, file, line ); }

    void dump(std::ostream &strm) const override;

    virtual void flush_me();

    static BESLog *TheLog();

    // I added this so that it's easy to route the BESDebug messages to the
    // log file. This will enable the Admin Interface to display the contents
    // of those debug messages when it displays the log file. jhrg
    std::ostream *get_log_ostream()
    {
        return d_file_buffer;
    }
};

#endif // BESLog_h_

