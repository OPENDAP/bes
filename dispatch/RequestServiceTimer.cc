// RequestServiceTimer.cc

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
//      pwest       Patrick West  <pwest@ucar.edu>
//      jgarcia     Jose Garcia  <jgarcia@ucar.edu>

#include "config.h"
#include <mutex>

#include <cerrno>
#include <string>
#include <iostream>
#include <cstring>
#include <sstream>

#include "BESDebug.h"
#include "BESLog.h"
#include "RequestServiceTimer.h"

using std::string;
using std::endl;
using std::ostream;

#define TIMING_LOG(x) MR_LOG(TIMING_LOG_KEY, x)

#define MODULE TIMING_LOG_KEY
#define prolog string("RequestServiceTimer::").append(__func__).append("() - ")

RequestServiceTimer *RequestServiceTimer::d_instance = nullptr;
static std::once_flag d_euc_init_once;

RequestServiceTimer *
RequestServiceTimer::TheTimer()
{
    std::call_once(d_euc_init_once,RequestServiceTimer::initialize_instance);
    return d_instance;
}

void RequestServiceTimer::initialize_instance() {
    d_instance = new RequestServiceTimer;
#ifdef HAVE_ATEXIT
    atexit(delete_instance);
#endif
}

void RequestServiceTimer::delete_instance() {
    delete d_instance;
    d_instance = 0;
}

unsigned long int RequestServiceTimer::remaining()
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);
    return d_duration;
}

unsigned long int RequestServiceTimer::elapsed()
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);
    return get_stop_us() - get_start_us();
}

/**
 * Starts the timer. NB: This method will attempt to write logging
 * information to the BESDebug::GetStrm() stream.
 *
 * @param name The name of the timer.
 */
bool
RequestServiceTimer::start(string name)
{
    return start(name, MISSING_LOG_PARAM) ;
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
RequestServiceTimer::start(string name, string reqID)
{
    d_timer_name = name;
    d_req_id = reqID;
    // get timing for current usage

    if(!get_time_of_day(d_start_usage)){
        d_started = false;
        return d_started;
    }

#if 0
    if( gettimeofday(&d_start_usage, NULL) != 0 )
    {
        int myerrno = errno ;
        char *c_err = strerror( myerrno ) ;
        string errno_msg = ((c_err != 0) ? c_err : "unknown error");

        if(BESDebug::GetStrm()){
            std::stringstream msg;
            msg << "[" << BESDebug::GetPidStr() << "][" << d_log_name << "][" << d_req_id << "][ERROR]";
            msg << "["<< d_timer_name << "]";
            msg << "[" << prolog << "gettimeofday() failed. Message: " <<  errno_msg << "]" << endl;
            *(BESDebug::GetStrm()) << msg.str();
        }
        std::stringstream msg;
        msg << prolog << "gettimeofday() failed. Message: " << errno_msg << endl;
        ERROR_LOG(msg.str());
        d_started = false ;
    }
    else
    {
    }
#endif
    d_started = true ;
    // Convert to milliseconds. Multiply seconds by 1000, divide micro seconds by 1000
    // double starttime =  d_start_usage.tv_sec*1000.0 + d_start_usage.tv_usec/1000.0;

    // Convert to microseconds
    //unsigned long int start_time_us = d_start_usage.tv_sec*1000*1000 + d_start_usage.tv_usec;

    std::stringstream msg;
    if(BESLog::TheLog()->is_verbose()){
        msg << "start_us" << BESLog::mark << get_start_us() << BESLog::mark;
        msg << (d_req_id.empty()?"-":d_req_id) << BESLog::mark;
        msg << d_timer_name << endl;
        TIMING_LOG(msg.str());
    }
    if ( BESDebug::GetStrm()) {
        msg << get_debug_log_line_prefix();
        msg << "[" << d_log_name << "]";
        msg << "[STARTED][" << get_start_us() << " us]";
        msg << "[" << d_req_id << "]";
        msg << "[" << d_timer_name << "]" << endl;
        *(BESDebug::GetStrm()) << msg.str();
    }

    // }
    // either we started the stop watch, or failed to start it. Either way,
    // no timings are available, so set stopped to false.
    d_stopped = false ;
    return d_started ;
}

bool RequestServiceTimer::get_time_of_day(struct timeval &time_val)
{
    bool retval = true;
    if( gettimeofday(&time_val, NULL) != 0 )
    {
        int myerrno = errno;
        char *c_err = strerror(myerrno);
        string errno_msg = (c_err != 0) ? c_err : "unknown error";
        std::stringstream msg;
        msg <<  prolog << "ERROR The gettimeofday() function failed. errno_msg: " << errno_msg << endl;
        if ( BESDebug::GetStrm()) {
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
RequestServiceTimer::~RequestServiceTimer()
{
    // if we have started, then stop and update the log.
    if (d_started) {
        // get timing for current usage

        if(!get_time_of_day(d_stop_usage)){
            d_started = false;
            d_stopped = false;
            return;
        }
#if 0
        if( gettimeofday(&d_stop_usage, NULL) != 0 )
        {
            int myerrno = errno;
            char *c_err = strerror(myerrno);
            string errno_msg = (c_err != 0) ? c_err : "unknown error";

#if 0
            std::stringstream msg;
            msg << "[" << BESDebug::GetPidStr() << "][" << d_log_name << "]";
            msg << "[" << d_req_id << "][ERROR][" << d_timer_name << "][" << errno_msg << "]" << endl;
#endif

            if (BESDebug::GetStrm()){
                std::stringstream msg;
                msg << "[" << BESDebug::GetPidStr() << "][" << d_log_name << "][" << d_req_id << "][ERROR]";
                msg << "["<< d_timer_name << "]";
                msg << "[" << prolog << "gettimeofday() failed. errno_msg: " <<  errno_msg << "]" << endl;
                *(BESDebug::GetStrm()) << msg.str();
            }
            std::stringstream msg;
            msg << prolog << "gettimeofday() failed. errno_msg: " << errno_msg << endl;
            ERROR_LOG(msg.str());

            d_started = false;
            d_stopped = false;
        }
        else {
#endif
        d_stopped = true;
        if (BESDebug::GetStrm()) {
            std::unique_lock<std::mutex> lck (bes_debug_log_mutex);
            std::stringstream msg;
            msg << get_debug_log_line_prefix();
            msg << "[" << d_log_name << "]";
            msg << "[ELAPSED][" << get_elapsed_us() << " us]";
            msg << "[STARTED][" << get_start_us() << " us]";
            msg << "[STOPPED][" << get_stop_us() << " us]";
            msg << "[" << (d_req_id.empty()?"-":d_req_id) << "]";
            msg << "[" << d_timer_name << "]";
            *(BESDebug::GetStrm()) << msg.str() << endl;
        }
        std::stringstream msg;
        msg << "elapsed_us" << BESLog::mark << get_elapsed_us() << BESLog::mark;
        msg << "start_us" << BESLog::mark << get_start_us() << BESLog::mark;
        msg << "stop_us" << BESLog::mark << get_stop_us() << BESLog::mark;
        msg << (d_req_id.empty()?"-":d_req_id) << BESLog::mark;
        msg << d_timer_name << endl;
        TIMING_LOG(msg.str());

        //}
    }
}

/**
* timeval_subtract() seems so complex.
*
* @return
*/
unsigned long int RequestServiceTimer::get_elapsed_us()
{
    return get_stop_us() - get_start_us();
}

unsigned long int RequestServiceTimer::get_start_us()
{
    return d_start_usage.tv_sec * 1000 * 1000 + d_start_usage.tv_usec;
}

unsigned long int RequestServiceTimer::get_stop_us()
{
    return d_stop_usage.tv_sec * 1000 * 1000 + d_stop_usage.tv_usec;
}

bool RequestServiceTimer::isTimeoutEnabled()
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);
    return d_timeout_enabled;
}

void RequestServiceTimer::disableTimeout()
{
    std::lock_guard<std::recursive_mutex> lock_me(d_cache_lock_mutex);
    d_timeout_enabled = false;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
RequestServiceTimer::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "RequestServiceTimer::dump - ("
         << (void *)this << ")" << endl ;
}