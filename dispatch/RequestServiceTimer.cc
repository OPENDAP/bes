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

#include <cerrno>
#include <string>
#include <iostream>
#include <cstring>
#include <mutex>
#include <sstream>

#include "BESDebug.h"
#include "BESLog.h"
#include "RequestServiceTimer.h"

using std::string;
using std::endl;
using std::ostream;

using namespace std::chrono;

#define MODULE "bes"
#define prolog string("RequestServiceTimer::").append(__func__).append("() - ")

#define DEFAULT_BES_TIMEOUT_SECONDS 600

RequestServiceTimer *RequestServiceTimer::d_instance = nullptr;
static std::once_flag d_rst_init_once;


RequestServiceTimer::RequestServiceTimer():
        bes_timeout(std::chrono::seconds(DEFAULT_BES_TIMEOUT_SECONDS)),
        start_time(std::chrono::steady_clock::now()),
        timeout_enabled(false) {
}

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

std::chrono::steady_clock::time_point RequestServiceTimer::start(unsigned int timeout_seconds){
    return start(seconds{timeout_seconds});
}

steady_clock::time_point RequestServiceTimer::start(milliseconds timeout_seconds){
    std::lock_guard<std::recursive_mutex> lock_me(d_rst_lock_mutex);

    if(timeout_seconds > seconds{0}){
        timeout_enabled = true;
        bes_timeout = timeout_seconds;
    }
    start_time = steady_clock::now();
    return start_time;
}

std::chrono::steady_clock::duration RequestServiceTimer::elapsed() const {
    std::lock_guard<std::recursive_mutex> lock_me(d_rst_lock_mutex);
    return steady_clock::now() - start_time;
}

std::chrono::milliseconds RequestServiceTimer::elapsed_ms() const {
    std::lock_guard<std::recursive_mutex> lock_me(d_rst_lock_mutex);
    return duration_cast<milliseconds>(steady_clock::now() - start_time);
}

std::chrono::steady_clock::duration RequestServiceTimer::remaining() const {
    std::lock_guard<std::recursive_mutex> lock_me(d_rst_lock_mutex);
    steady_clock::duration remaining = std::chrono::steady_clock::duration(DEFAULT_BES_TIMEOUT_SECONDS);
    if (timeout_enabled) {
        remaining = bes_timeout - elapsed();
    }
    else {
        if (bes_timeout > steady_clock::duration(0)) {
            remaining = bes_timeout;
        }
    }
    return remaining;
}

std::chrono::milliseconds RequestServiceTimer::remaining_ms() const {
    std::lock_guard<std::recursive_mutex> lock_me(d_rst_lock_mutex);
    return duration_cast<milliseconds>(remaining());
}


bool RequestServiceTimer::is_expired() const {
    std::lock_guard<std::recursive_mutex> lock_me(d_rst_lock_mutex);
    return timeout_enabled && (remaining() <= steady_clock::duration(0));
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
    ss << "bes_timeout: " << bes_timeout.count() << "ms ";
    if(pretty){ ss << endl << "  "; }
    ss << "start_time: " << start_time.time_since_epoch().count() << "s ";
    if(pretty){ ss << endl << "  "; }
    ss << "timeout_enabled: " << (timeout_enabled?"true ":"false ");
    if(pretty){ ss << endl << "  "; }
    ss << "elapsed: " << elapsed().count() << "s ";
    if(pretty){ ss << endl << "  "; }
    ss << "remaining: " << remaining().count() << "s ";
    if(pretty){ ss << endl << "  "; }
    ss << "is_expired: " <<  (is_expired()?"true ":"false ");
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
    strm << dump(false) << endl;
}

