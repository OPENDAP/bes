// BESStopWatch.cc

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
//      ndp         Nathan Potter <ndp@opendap.org>
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "config.h"

#include <cerrno>
#include <string>
#include <iostream>
#include <cstring>
#include <sstream>

#include "BESStopWatch.h"
#include "BESDebug.h"
#include "BESLog.h"

using std::string;
using std::endl;
using std::ostream;

#define TIMING_LOG(x) MR_LOG(TIMING_LOG_KEY, x)

#define MODULE TIMING_LOG_KEY
#define prolog string("BESStopWatch::").append(__func__).append("() - ")

namespace bes_timing {
BESStopWatch *elapsedTimeToReadStart = nullptr;
BESStopWatch *elapsedTimeToTransmitStart = nullptr;
}

/**
 * Starts the timer. NB: This method will attempt to write logging
 * information to the BESDebug::GetStrm() stream.
 *
 * @param name The name of the timer.
 */
bool
BESStopWatch::start(const string &name) {
    return start(name, MISSING_LOG_PARAM);
}

/**
 * Starts the timer.
 * NB: This method will attempt to write logging
 * information to the BESDebug::GetStrm() stream.
 * @param name The name of the timer.
 * @param reqID The client's request ID associated with this activity. Not used.
 * Available from the DataHandlerInterface object.
 */
bool
BESStopWatch::start(const string &name, const string &reqID) {
    d_timer_name = name;
    d_req_id = reqID;
    // get timing for current usage

    if (!get_time_of_day(d_start_usage)) {
        d_started = false;
        return d_started;
    }
    d_started = true;

    std::stringstream msg;
    if (BESLog::TheLog()->is_verbose()) {
        msg << "start_us" << BESLog::mark << get_start_us() << BESLog::mark;
        msg << (d_req_id.empty() ? "-" : d_req_id) << BESLog::mark;
        msg << d_timer_name << endl;
        TIMING_LOG(msg.str());
    }
#if 0
    if (BESDebug::GetStrm()) {
        msg << get_debug_log_line_prefix();
        msg << "[" << d_log_name << "]";
        msg << "[STARTED][" << get_start_us() << " us]";
        msg << "[" << d_req_id << "]";
        msg << "[" << d_timer_name << "]" << endl;
        *(BESDebug::GetStrm()) << msg.str();
    }
#endif
    // either we started the stop watch, or failed to start it. Either way,
    // no timings are available, so set stopped to false.
#if 0
    d_stopped = false;
#endif
    return d_started;
}

bool BESStopWatch::get_time_of_day(struct timeval &time_val) const {
    bool retval = true;
    if (gettimeofday(&time_val, nullptr) != 0) {
        const char *c_err = strerror(errno);
        string errno_msg = (c_err != nullptr) ? c_err : "unknown error";
        std::stringstream msg;
        msg << prolog << "ERROR The gettimeofday() function failed. errno_msg: " << errno_msg << endl;
        if (BESDebug::GetStrm()) {
            *(BESDebug::GetStrm()) << get_debug_log_line_prefix() << msg.str();
        }
        ERROR_LOG(msg.str());
        retval = false;
    }
    return retval;
}

/**
 * This destructor is "special" in that it's execution signals the
 * timer to stop if it has been started. Stopping the timer will
 * initiate an attempt to write logging information to the
 * BESDebug::GetStrm() stream. If the start method has not been called
 * then the method exits silently.
 */
BESStopWatch::~BESStopWatch() {
    // if we have started, then stop and update the log.
    if (d_started) {
        // get timing for current usage

        if (!get_time_of_day(d_stop_usage)) {
            d_started = false;
#if 0
            d_stopped = false;
#endif
            return;
        }

#if 0
        d_stopped = true;
#endif
#if 0
        if (BESDebug::GetStrm()) {
            std::unique_lock<std::mutex> lck(bes_debug_log_mutex);
            std::stringstream msg;
            msg << get_debug_log_line_prefix();
            msg << "[" << d_log_name << "]";
            msg << "[ELAPSED][" << get_elapsed_us() << " us]";
            msg << "[STARTED][" << get_start_us() << " us]";
            msg << "[STOPPED][" << get_stop_us() << " us]";
            msg << "[" << (d_req_id.empty() ? "-" : d_req_id) << "]";
            msg << "[" << d_timer_name << "]";
            *(BESDebug::GetStrm()) << msg.str() << endl;
        }
#endif
        std::stringstream msg;
        msg << "elapsed_us" << BESLog::mark << get_elapsed_us() << BESLog::mark;
        msg << "start_us" << BESLog::mark << get_start_us() << BESLog::mark;
        msg << "stop_us" << BESLog::mark << get_stop_us() << BESLog::mark;
        msg << (d_req_id.empty() ? "-" : d_req_id) << BESLog::mark;
        msg << d_timer_name << endl;
        TIMING_LOG(msg.str());
    }
}

/**
 * timeval_subtract() seems so complex.
 *
 * @return
 */
unsigned long int BESStopWatch::get_elapsed_us() const {
    return get_stop_us() - get_start_us();
}

unsigned long int BESStopWatch::get_start_us() const {
    return d_start_usage.tv_sec * 1'000'000 + d_start_usage.tv_usec;
}

unsigned long int BESStopWatch::get_stop_us() const {
    return d_stop_usage.tv_sec * 1'000'000 + d_stop_usage.tv_usec;
}


#if 0
/**
 * struct timeval {
 *    time_t      tv_sec;   // Number of whole seconds of elapsed time
 *    long int    tv_usec;  // Number of microseconds of rest of elapsed time minus tv_sec. Always less than one million
 * };
 * @return
 */
bool
BESStopWatch::timeval_subtract()
{
    // struct
    // time_t         tv_sec      seconds
    // suseconds_t    tv_usec     microseconds

    /* Perform the carry for the later subtraction by updating y. */
    if( d_stop_usage.tv_usec < d_start_usage.tv_usec )
    {
        int nsec = (d_start_usage.tv_usec - d_stop_usage.tv_usec) / 1000000 + 1 ;
        d_start_usage.tv_usec -= 1000000 * nsec ;
        d_start_usage.tv_sec += nsec ;
    }
    if( d_stop_usage.tv_usec - d_start_usage.tv_usec > 1000000 )
    {
        int nsec = (d_start_usage.tv_usec - d_stop_usage.tv_usec) / 1000000 ;
        d_start_usage.tv_usec += 1000000 * nsec ;
        d_start_usage.tv_sec -= nsec ;
    }

    /* Compute the time remaining to wait.
    tv_usec  is certainly positive. */
    d_result.tv_sec = d_stop_usage.tv_sec - d_start_usage.tv_sec ;
    d_result.tv_usec = d_stop_usage.tv_usec - d_start_usage.tv_usec ;

    /* Return 1 if result is negative. */
    return !(d_stop_usage.tv_sec < d_start_usage.tv_sec) ;
}

/**
 * Starts the timer.
 * NB: This method will attempt to write logging
 * information to the BESDebug::GetStrm() stream.
 * @param name The name of the timer.
 * @param reqID The client's request ID associated with this activity.
 * Available from the DataHandlerInterfact object.
 */
bool
BESStopWatch::start(string name, string reqID)
{
    d_timer_name = name;
    d_req_id = reqID;
    // get timing for current usage
    if( getrusage( RUSAGE_SELF, &_start_usage ) != 0 )
    {
        int myerrno = errno ;
        char *c_err = strerror( myerrno ) ;
        string err = "getrusage failed in start: " ;
        err += (c_err != 0) ? c_err : "unknown error";
#if 0
        if( c_err )
        {
            err += c_err ;
        }
        else
        {
            err += "unknown error" ;
        }
#endif
        std::stringstream msg;
        msg << "[" << BESDebug::GetPidStr() << "]["<< d_log_name << "][" << d_req_id << "]";
        msg << "[ERROR][" << d_timer_name << "][" << err << "]" << endl;

        if(!BESLog::TheLog()->is_verbose() && BESDebug::GetStrm())
            *(BESDebug::GetStrm()) << msg.str();
        VERBOSE(msg.str());
        d_started = false ;
    }
    else
    {
        d_started = true ;
        struct timeval &start = _start_usage.ru_utime ;
        double starttime =  start.tv_sec*1000.0 + start.tv_usec/1000.0;

        std::stringstream msg;
        msg << "[" << BESDebug::GetPidStr() << "]["<< d_log_name << "][" << d_req_id << "]";
        msg << "[STARTED][" << starttime << " ms]["<< d_timer_name << "]" << endl;
        if(!BESLog::TheLog()->is_verbose() && BESDebug::GetStrm())
            *(BESDebug::GetStrm()) << msg.str();
        VERBOSE(msg.str());
    }

    // either we started the stop watch, or failed to start it. Either way,
    // no timings are available, so set stopped to false.
    d_stopped = false ;


    return d_started ;
}

/**
 * This destructor is "special" in that it's execution signals the
 * timer to stop if it has been started. Stopping the timer will
 * initiate an attempt to write logging information to the
 * BESDebug::GetStrm() stream. If the start method has not been called
 * then the method exits silently.
 */
BESStopWatch::~BESStopWatch()
{
    // if we have started, then stop and update the log.
    if (d_started) {
        // get timing for current usage
        if (getrusage(RUSAGE_SELF, &_stop_usage) != 0) {
            int myerrno = errno;
            char *c_err = strerror(myerrno);
            string err = "getrusage failed in stop: ";
            err += (c_err != 0) ? c_err : "unknown error";
#if 0
            if( c_err )
            {
                err += c_err ;
            }
            else
            {
                err += "unknown error" ;
            }
#endif
            std::stringstream msg;
            msg << "[" << BESDebug::GetPidStr() << "][" << d_log_name << "]";
            msg << "[" << d_req_id << "][ERROR][" << d_timer_name << "][" << err << "]" << endl;
            if (!BESLog::TheLog()->is_verbose() && BESDebug::GetStrm())
                *(BESDebug::GetStrm()) << msg.str();
            VERBOSE(msg.str());

            d_started = false;
            d_stopped = false;
        } else {
            // get the difference between the _start_usage and the
            // _stop_usage and save the difference in _result.
            bool success = timeval_subtract();
            if (!success)
            {
                std::stringstream msg;
                msg << "[" << BESDebug::GetPidStr() << "][" << d_log_name << "]";
                msg << "[" << d_req_id << "][ERROR][" << d_timer_name << "][Failed to get timing.]" << endl;

                if (!BESLog::TheLog()->is_verbose() && BESDebug::GetStrm())
                    *(BESDebug::GetStrm()) << msg.str();
                VERBOSE(msg.str());

                d_started = false;
                d_stopped = false;
            }
            else
                {
                d_stopped = true;

                struct timeval &stop = _stop_usage.ru_utime;
                double stoptime = stop.tv_sec * 1000.0 + stop.tv_usec / 1000.0;
                double elapsed = _result.tv_sec * 1000.0 + _result.tv_usec / 1000.0;

                std::stringstream msg;
                msg << "[" << BESDebug::GetPidStr() << "][" << d_log_name << "]";
                msg << "[" << d_req_id << "][STOPPED][" << stoptime << " ms]";
                msg << "[" << d_timer_name << "][ELAPSED][" << elapsed << " ms]" << endl;

                if (!BESLog::TheLog()->is_verbose() && BESDebug::GetStrm())
                    *(BESDebug::GetStrm()) << msg.str();
                VERBOSE(msg.str() );
            }
        }
    }
}

bool
BESStopWatch::timeval_subtract()
{
    struct timeval &start = _start_usage.ru_utime ;
    struct timeval &stop = _stop_usage.ru_utime ;

    /* Perform the carry for the later subtraction by updating y. */
    if( stop.tv_usec < start.tv_usec )
    {
        int nsec = (start.tv_usec - stop.tv_usec) / 1000000 + 1 ;
        start.tv_usec -= 1000000 * nsec ;
        start.tv_sec += nsec ;
    }
    if( stop.tv_usec - start.tv_usec > 1000000 )
    {
        int nsec = (start.tv_usec - stop.tv_usec) / 1000000 ;
        start.tv_usec += 1000000 * nsec ;
        start.tv_sec -= nsec ;
    }

    /* Compute the time remaining to wait.
    tv_usec  is certainly positive. */
    _result.tv_sec = stop.tv_sec - start.tv_sec ;
    _result.tv_usec = stop.tv_usec - start.tv_usec ;

    /* Return 1 if result is negative. */
    return !(stop.tv_sec < start.tv_sec) ;
}

#endif

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESStopWatch::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "BESStopWatch::dump - ("
         << (void *) this << ")" << endl;
}





