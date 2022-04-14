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


steady_clock::time_point RequestServiceTimer::start(int timeout_seconds){
    std::lock_guard<std::recursive_mutex> lock_me(d_rst_lock_mutex);

    if(timeout_seconds > 0){
        timeout_enabled = true;
        bes_timeout = std::chrono::seconds(timeout_seconds);
    }
    start_time = steady_clock::now();
    return start_time;
}

duration<int> RequestServiceTimer::elapsed() const {
    std::lock_guard<std::recursive_mutex> lock_me(d_rst_lock_mutex);

        return duration_cast<duration<int>>(steady_clock::now() - start_time);

    return duration<int>(0);
}

std::chrono::duration<int>
RequestServiceTimer::remaining() const {
    std::lock_guard<std::recursive_mutex> lock_me(d_rst_lock_mutex);
    std::chrono::duration<int> remaining = duration<int>(DEFAULT_BES_TIMEOUT_SECONDS);
    if (timeout_enabled) {
        remaining = duration_cast<duration<int>>(bes_timeout - elapsed());
    }
    else {
        if (bes_timeout > duration<int>(0)) {
            remaining = bes_timeout;
        }
    }
    return remaining;
}

bool RequestServiceTimer::is_expired() const {
    std::lock_guard<std::recursive_mutex> lock_me(d_rst_lock_mutex);

    return timeout_enabled && (remaining() <= duration<int>(0));
}

void RequestServiceTimer::disable_timeout(){
    std::lock_guard<std::recursive_mutex> lock_me(d_rst_lock_mutex);
    timeout_enabled = false;
}



string RequestServiceTimer::dump() const {
    std::stringstream ss;
    ss << "[RequestServiceTimer: " << bes_timeout.count() << "s ";
    ss << "bes_timeout: " << bes_timeout.count() << "s ";
    ss << "start_time: " << start_time.time_since_epoch().count() << "s ";
    ss << "timeout_enabled: " << (timeout_enabled?"true ":"false ");
    ss << "elapsed: " << elapsed().count() << "s ";
    ss << "remaining: " << remaining().count() << "s ";
    ss << "is_expired: " <<  (is_expired()?"true ":"false ");
    ss << "]";
    return ss.str();

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
    strm << dump() << endl;
}

