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
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include "BESStopWatch.h"

#include <BESUtil.h>

#include "BESDebug.h"
#include "BESLog.h"

using std::endl;
using std::ostream;
using std::string;

#define MODULE TIMING_LOG_KEY
#define prolog string("BESStopWatch::").append(__func__).append("() - ")

namespace bes_timing {
BESStopWatch *elapsedTimeToReadStart = nullptr;
BESStopWatch *elapsedTimeToTransmitStart = nullptr;
} // namespace bes_timing

/**
 * Starts the timer. NB: This method will attempt to write logging
 * information to the BESDebug::GetStrm() stream.
 *
 * @param name The name of the timer.
 * @param dhi The name of the timer.
 */
bool BESStopWatch::start(const std::string &name, BESDataHandlerInterface *dhi) {
    string reqId = dhi->data[REQUEST_ID_KEY];
    if (!reqId.empty()) {
        string uuid = dhi->data[REQUEST_UUID_KEY];
        if (uuid.empty()) {
            uuid = "BESStopWatch-" + BESUtil::uuid();
            dhi->data[REQUEST_UUID_KEY] = uuid;
        }
        reqId = reqId + "-" + uuid;
    } else {
        // If we grab it from BESLog we know the uuid is already a part of the request id value.
        reqId = BESLog::TheLog()->get_request_id();
    }
    if (reqId.empty()) {
        reqId = prolog + "OUCH! The values of dhi->data[\"" REQUEST_ID_KEY
                         "\" and BESLog::TheLog()->get_request_id() were empty.";
    }
    return start(name, reqId);
}

/**
 * Starts the timer.
 * NB: This method will attempt to write logging
 * information to the BESDebug::GetStrm() stream.
 * @param name The name of the timer.
 * @param reqID The client's request ID associated with this activity.
 * If reqID is not provided then the value of BESLog::get_request_id() \
 * is utilized (see declaration in BESStopWatch.h)
 *
 */
bool BESStopWatch::start(const string &name, const string &reqID) {
    d_timer_name = name;
    d_req_id = (reqID.empty() ? "ReqIdEmpty" : reqID);

    // get timing for current usage
    if (!get_time_of_day(d_start_usage)) {
        d_started = false;
        return d_started;
    }
    d_started = true;

    std::stringstream msg;
    if (BESLog::TheLog()->is_verbose()) {
        msg << "start_us" << BESLog::mark << get_start_us() << BESLog::mark;
        msg << d_req_id << BESLog::mark;
        msg << d_timer_name << endl;
        TIMING_LOG(msg.str());
    }
    // either we started the stop watch, or failed to start it. Either way,
    // no timings are available, so set stopped to false.
    return d_started;
}

bool BESStopWatch::get_time_of_day(struct timeval &time_val) {
    bool retval = true;
    if (gettimeofday(&time_val, nullptr) != 0) {
        const char *c_err = strerror(errno);
        string errno_msg = c_err != nullptr ? c_err : "unknown error";
        string msg = prolog + "ERROR The gettimeofday() function failed. errno_msg: " + errno_msg + "\n";
        BESDEBUG(TIMING_LOG_KEY, msg);
        ERROR_LOG(msg);
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
            return;
        }

        BESDEBUG(TIMING_LOG_KEY, get_debug_log_line_prefix() + "["
                                     << d_log_name + "]" + "[ELAPSED][" + std::to_string(get_elapsed_us()) + " us]" +
                                            "[STARTED][" + std::to_string(get_start_us()) + " us]" + "[STOPPED][" +
                                            std::to_string(get_stop_us()) + " us]" + "[" +
                                            (d_req_id.empty() ? "-" : d_req_id) + "]" + "["
                                     << d_timer_name + "]\n");

        TIMING_LOG("elapsed-us" + BESLog::mark + std::to_string(get_elapsed_us()) + BESLog::mark + "start-us" +
                   BESLog::mark + std::to_string(get_start_us()) + BESLog::mark + "stop-us" + BESLog::mark +
                   std::to_string(get_stop_us()) + BESLog::mark + (d_req_id.empty() ? "-" : d_req_id) + BESLog::mark +
                   d_timer_name + "\n");
    }
}

/**
 * timeval_subtract() seems so complex.
 *
 * @return
 */
unsigned long int BESStopWatch::get_elapsed_us() const { return get_stop_us() - get_start_us(); }

unsigned long int BESStopWatch::get_start_us() const {
    return d_start_usage.tv_sec * 1000 * 1000 + d_start_usage.tv_usec;
}

unsigned long int BESStopWatch::get_stop_us() const { return d_stop_usage.tv_sec * 1000 * 1000 + d_stop_usage.tv_usec; }

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESStopWatch::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "BESStopWatch::dump - (" << (void *)this << ")" << endl;
}
