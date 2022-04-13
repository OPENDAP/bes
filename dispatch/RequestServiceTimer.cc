// RequestServiceTimer.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2022 OPeNDAP, Inc
// Author: Dan Holloway <dholloway@opendap.org>
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
#include <mutex>

#include "BESLog.h"
#include "RequestServiceTimer.h"

RequestServiceTimer *RequestServiceTimer::d_instance = nullptr;
static std::once_flag d_euc_init_once;

RequestServiceTimer::RequestServiceTimer() {}

RequestServiceTimer::~RequestServiceTimer() {}

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

unsigned long int RequestServiceTimer::get_start_us()
{
    return d_start_usage.tv_sec * 1000 * 1000 + d_start_usage.tv_usec;
}

unsigned long int RequestServiceTimer::get_stop_us()
{
    return d_stop_usage.tv_sec * 1000 * 1000 + d_stop_usage.tv_usec;
}



