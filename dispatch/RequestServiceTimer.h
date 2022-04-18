// RequestServiceTimer.h

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
#ifndef I_RequestServiceTimer_h
#define I_RequestServiceTimer_h 1

#include <map>
#include <string>
#include <mutex>
#include <chrono>

#include "sys/time.h"
#include "sys/resource.h"
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "BESObj.h"

/** @brief The master request service timer for this server; a singleton
 *
 */

class RequestServiceTimer : public BESObj{
private:
    static RequestServiceTimer *d_instance;
    mutable std::recursive_mutex d_rst_lock_mutex;

    std::chrono::milliseconds bes_timeout;
    std::chrono::steady_clock::time_point start_time;
    bool timeout_enabled;

    RequestServiceTimer();

    static void delete_instance();
    static void initialize_instance();

public:
    static RequestServiceTimer *TheTimer();

    std::chrono::steady_clock::time_point start(std::chrono::milliseconds timeout_ms);

    std::chrono::milliseconds elapsed() const;

    std::chrono::milliseconds remaining() const;

    bool is_expired() const;

    void disable_timeout();

    std::string dump(bool pretty=false) const ;

    void dump( std::ostream &strm ) const ;

};

#endif // I_RequestServiceTimer_h

