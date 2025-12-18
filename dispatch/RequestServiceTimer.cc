// RequestServiceTimer.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2022 OPeNDAP, Inc
// Authors:
//      ndp         Nathan Potter <ndp@opendap.org>
//      dan         Dan Holloway  <dholloway@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
#include "config.h"

#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

#include "BESDebug.h"
#include "BESLog.h"
#include "BESTimeoutError.h"
#include "RequestServiceTimer.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

using std::endl;
using std::ostream;
using std::string;

using namespace std::chrono;

#define MODULE "RST"
#define prolog string("RequestServiceTimer::").append(__func__).append("() - ")

/** @brief Return a pointer to a singleton timer instance.  If an instance does not exist
 * it will create and initialize one.  The initialization sets the start_time to now and
 * time_out disabled.
 */
RequestServiceTimer *RequestServiceTimer::TheTimer() {
    static RequestServiceTimer timer;
    return &timer;
}

/** @brief Set/Reset the timer start_time to now().
 *
 * @param timeout_ms the time_out, in milliseconds.  A timeout_ms of 0
 * sets timeout_disabled to true.
 *
 * The RequestServiceTimer is a per request timer, the start method should
 * be called prior to executing each request.
 */
void RequestServiceTimer::start(milliseconds timeout_ms) {
    std::lock_guard<std::recursive_mutex> lock_me(d_rst_lock_mutex);

    if (timeout_ms > milliseconds{0}) {
        timeout_enabled = true;
        d_bes_timeout = timeout_ms;
    } else {
        timeout_enabled = false;
        d_bes_timeout = milliseconds{0};
    }
    start_time = steady_clock::now();
}

/**
 * @brief Return the time duration in milliseconds since the timer was started.
 *
 */
milliseconds RequestServiceTimer::elapsed() const {
    std::lock_guard<std::recursive_mutex> lock_me(d_rst_lock_mutex);
    return duration_cast<milliseconds>(steady_clock::now() - start_time);
}

/**
 * @brief If the time_out is enabled returns the time remaining. If the time_out is disabled returns 0.
 *
 * The return value is only of interest if the timeout is enabled so checking is_timeout_enabled()
 * is an important precondition before assessing the value of remaining().
 * @return
 */
milliseconds RequestServiceTimer::remaining() const {
    std::lock_guard<std::recursive_mutex> lock_me(d_rst_lock_mutex);
    milliseconds remaining{0};
    BESDEBUG(MODULE, prolog << "init remaining: " << remaining.count() << endl);
    if (timeout_enabled) {
        BESDEBUG(MODULE, prolog << "timeout enabled" << endl);
        remaining = d_bes_timeout - elapsed();
    } else {
        BESDEBUG(MODULE, prolog << "timeout disabled" << endl);
    }
    return remaining;
}

/** @brief if the time_out is disabled return false.
 *
 * If the time_out is enabled return false if there is time remaining
 * else return true; the time_out has expired.
 */
bool RequestServiceTimer::is_expired() const {
    std::lock_guard<std::recursive_mutex> lock_me(d_rst_lock_mutex);
    auto dt = remaining();
    BESDEBUG(MODULE, prolog + "remaining: " + std::to_string(dt.count()) + " ms\n");
    return timeout_enabled && (dt <= milliseconds{0});
}

/** @brief Set the time_out is disabled.
 *
 */
void RequestServiceTimer::disable_timeout() {
    std::lock_guard<std::recursive_mutex> lock_me(d_rst_lock_mutex);
    timeout_enabled = false;
}

string RequestServiceTimer::dump(bool pretty) const {
    std::stringstream ss;
    if (!pretty) {
        ss << "[";
    }
    ss << "RequestServiceTimer(" << (void *)this << ") - ";
    if (pretty) {
        ss << endl << "  ";
    }
    ss << "bes_timeout: " << d_bes_timeout.count() << "ms ";
    if (pretty) {
        ss << endl << "  ";
    }
    ss << "start_time: " << duration_cast<seconds>(start_time.time_since_epoch()).count() << "s ";
    if (pretty) {
        ss << endl << "  ";
    }
    ss << "is_timeout_enabled(): " << (is_timeout_enabled() ? "true " : "false ");
    if (pretty) {
        ss << endl << "  ";
    }
    ss << "elapsed(): " << elapsed().count() << "ms ";
    if (pretty) {
        ss << endl << "  ";
    }
    ss << "remaining(): " << remaining().count() << "ms ";
    if (pretty) {
        ss << endl << "  ";
    }
    ss << "is_expired(): " << (is_expired() ? "true" : "false");
    if (pretty) {
        ss << endl;
    } else {
        ss << "]";
    }
    return ss.str();
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void RequestServiceTimer::dump(ostream &strm) const { strm << dump() << endl; }

/** @brief Checks the RequestServiceTimer to determine if the
 * time spent servicing the request at this point has exceeded the
 * bes_timeout configuration element.   If the request timeout has
 * expired throw BESInternalFatalError.
 *
 * @param message to be delivered in error response.
 * @param file The file (__FILE__) that called this method
 * @param line The line (__LINE__) in the file that made the call to this method.
 */
void RequestServiceTimer::throw_if_timeout_expired(const string &message, const string &file, const int line) {
    bool expired = is_expired();

    if (expired) {
        auto time_out_seconds = std::to_string(((double)d_bes_timeout.count()) / 1000.00);
        auto elapsed_time_seconds = std::to_string(((double)elapsed().count()) / 1000.00);
        ERROR_LOG(prolog + "ERROR: Time to transmit timeout expired. " + "Elapsed Time: " + elapsed_time_seconds + " " +
                  "max_time_to_transmit: " + time_out_seconds + "\n");

        std::stringstream errMsg;
        errMsg << "The request that you submitted timed out. The server was unable to begin transmitting" << endl;
        errMsg << "a response in the time allowed. Requests processed by this server must begin transmitting" << endl;
        errMsg << "the response in less than " << time_out_seconds << " seconds." << endl;
        errMsg << "ElapsedTime: " << elapsed_time_seconds << " seconds" << endl;
        errMsg << "Some things you can try: Reissue the request but change the amount of data requested." << endl;
        errMsg << "You may reduce the size of the request by choosing just the variables you need and/or" << endl;
        errMsg << "by using the DAP index based array sub-setting syntax to additionally limit the amount" << endl;
        errMsg << "of data requested. You can also try requesting a different encoding for the response." << endl;
        errMsg << "If you asked for the response to be encoded as a NetCDF-3 or NetCDF-4 file be aware" << endl;
        errMsg << "that these response encodings are not streamable. In order to build these responses" << endl;
        errMsg << "the server must write the entire response to a temporary file before it can begin to" << endl;
        errMsg << "send the response to the requesting client. Changing to a different encoding, such" << endl;
        errMsg << "as DAP4 data, may allow the server to successfully respond to your request." << endl;
        errMsg << "The service component that ran out of time said: " << endl;
        errMsg << message << endl;

        throw BESTimeoutError(errMsg.str(), file, line);
    }
}
