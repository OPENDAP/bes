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

#include <string>
#include <iostream>
#include <mutex>
#include <sstream>

#include "BESDebug.h"
#include "BESInternalFatalError.h"
#include "RequestServiceTimer.h"

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

using std::string;
using std::endl;
using std::ostream;

using namespace std::chrono;

#define MODULE "bes"
#define prolog string("RequestServiceTimer::").append(__func__).append("() - ")

RequestServiceTimer *RequestServiceTimer::d_instance = nullptr;
static std::once_flag d_rst_init_once;


RequestServiceTimer *
RequestServiceTimer::TheTimer()
{
    std::call_once(d_rst_init_once,RequestServiceTimer::initialize_instance);
    return d_instance;
}

void RequestServiceTimer::initialize_instance() {
    d_instance = new RequestServiceTimer();
#ifdef HAVE_ATEXIT
    atexit(delete_instance);
#endif
}

void RequestServiceTimer::delete_instance() {
    delete d_instance;
    d_instance = nullptr;
}

void RequestServiceTimer::start(milliseconds timeout_ms){
    std::lock_guard<std::recursive_mutex> lock_me(d_rst_lock_mutex);

    if(timeout_ms > milliseconds{0}){
        timeout_enabled = true;
        d_bes_timeout = timeout_ms;
    }
    else {
        timeout_enabled = false;
        d_bes_timeout = milliseconds{0};
    }
    start_time = steady_clock::now();
}


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
    BESDEBUG(MODULE, prolog << "init remaining: " << remaining.count() <<  endl);
    if (timeout_enabled) {
        BESDEBUG(MODULE, prolog << "timeout enabled" << endl);
        remaining = d_bes_timeout - elapsed();
    }
    else {
        BESDEBUG(MODULE, prolog << "timeout disabled" << endl);
    }
    return remaining;
}

bool RequestServiceTimer::is_expired() const {
    std::lock_guard<std::recursive_mutex> lock_me(d_rst_lock_mutex);
    return timeout_enabled && (remaining() <= milliseconds {0});
}

void RequestServiceTimer::disable_timeout(){
    std::lock_guard<std::recursive_mutex> lock_me(d_rst_lock_mutex);
    timeout_enabled = false;
}

string RequestServiceTimer::dump(bool pretty) const {
    std::stringstream ss;
    if(!pretty){ ss<<"["; }
    ss << "RequestServiceTimer(" << (void *)this << ") - ";
    if(pretty){ ss << endl << "  "; }
    ss << "bes_timeout: " << d_bes_timeout.count() << "ms ";
    if(pretty){ ss << endl << "  "; }
    ss << "start_time: " << duration_cast<seconds>(start_time.time_since_epoch()).count() << "s ";
    if(pretty){ ss << endl << "  "; }
    ss << "is_timeout_enabled(): " << (is_timeout_enabled() ?"true ":"false ");
    if(pretty){ ss << endl << "  "; }
    ss << "elapsed(): " << elapsed().count() << "ms ";
    if(pretty){ ss << endl << "  "; }
    ss << "remaining(): " << remaining().count() << "ms ";
    if(pretty){ ss << endl << "  "; }
    ss << "is_expired(): " <<  (is_expired()?"true":"false");
    if(pretty){ ss << endl; }else{ ss << "]"; }
    return ss.str();
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void RequestServiceTimer::dump( ostream &strm ) const
{
    strm << dump() << endl;
}


/** @brief Checks the RequestServiceTimer to determine if the
 * time spent servicing the request at this point has exceeded the
 * bes_timeout configuration element.   If the request timeout has
 * expired throw BESInternalFatalError.
 *
 * @param message to be delivered in error response.
 * @param file The file (__FILE__) that called this method
 * @param line The line (__LINE__) in the file that made the call to this method.
*/
void RequestServiceTimer::throw_if_timeout_expired(string message, string file, int line)
{
    if (is_expired()) {
        throw BESInternalFatalError(std::move(message), std::move(file), line);
    }
}
