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

#include <fstream>
#include <string>

// Note that the BESLog::operator<<() methods will prefix output with
// the time and PID by checking for the flush and endl stream operators.
#define LOG(x) do { *(BESLog::TheLog()) << x ; } while( 0 )
#define VERBOSE(x) do { if (BESLog::TheLog()->is_verbose()) *(BESLog::TheLog()) << x ; } while( 0 )

#include "BESObj.h"

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
 * @see BESKeys
 */
class BESLog: public BESObj {
private:
    static BESLog * _instance;
    int _flushed;
    std::ofstream * _file_buffer;
    std::string _file_name;
    // Flag to indicate the object is not routing data to its associated stream
    int _suspended;
    // Flag to indicate whether to log verbose messages
    bool _verbose;
protected:
    BESLog();

    // Dumps the current system time.
    void dump_time();
public:
    ~BESLog();

    const static string mark;

    /** @brief Suspend logging of any information until resumed.
     *
     * This method suspends any logging of information. If already suspended
     * then nothing changes, logging is still suspended.
     */
    void suspend()
    {
        _suspended = 1;
    }

    /** @brief Resumes logging after being suspended.
     *
     * This method resumes logging after suspended by the user. If logging was
     * not already suspended this method does nothing.
     */
    void resume()
    {
        _suspended = 0;
    }

    /** @brief turn on verbose logging
     *
     * This method turns on verbose logging, providing applications the
     * ability to log more detailed debugging information. If verbose is
     * already turned on then nothing is changed.
     */
    void verbose_on()
    {
        _verbose = true;
    }

    /** @brief turns off verbose logging
     *
     * This methods turns off verbose logging. If verbose logging was not
     * already turned on then nothing changes.
     */
    void verbose_off()
    {
        _verbose = false;
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
    bool is_verbose()
    {
        return _verbose;
    }

    /// Defines a data type p_ios_manipulator "pointer to function that takes ios& and returns ios&".
    typedef std::ios& (*p_ios_manipulator)(std::ios&);
    /// Defines a data type p_std::ostream_manipulator "pointer to function that takes std::ostream& and returns std::ostream&".
    typedef std::ostream& (*p_ostream_manipulator)(std::ostream&);

    BESLog& operator <<(std::string&);
    BESLog& operator <<(const std::string&);
    BESLog& operator <<(char*);
    BESLog& operator <<(const char*);
    BESLog& operator <<(int);
    BESLog& operator <<(char);
    BESLog& operator <<(long);
    BESLog& operator <<(unsigned long);
    BESLog& operator <<(double);

    BESLog& operator<<(p_ostream_manipulator);
    BESLog& operator<<(p_ios_manipulator);

    virtual void dump(std::ostream &strm) const;

    virtual void flush_me();

    static BESLog *TheLog();

    // I added this so that it's easy to route the BESDebug messages to the
    // log file. This will enable the Admin Interface to display the contents
    // of those debug messages when it displays the log file. jhrg
    std::ostream *get_log_ostream()
    {
        return _file_buffer;
    }
};

#endif // BESLog_h_

