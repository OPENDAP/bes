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

#include <chrono>
#include <map>
#include <mutex>
#include <string>

// #include "BESObj.h"

/** @brief The master request service timer for this server; a singleton
 *
 */

class RequestServiceTimer {
private:
    mutable std::recursive_mutex d_rst_lock_mutex;

    std::chrono::milliseconds d_bes_timeout{0};
    std::chrono::steady_clock::time_point start_time{std::chrono::steady_clock::now()};
    bool timeout_enabled{false};

    RequestServiceTimer() = default;

public:
    ~RequestServiceTimer() = default;

    RequestServiceTimer(const RequestServiceTimer &) = delete;
    RequestServiceTimer &operator=(const RequestServiceTimer &) = delete;

    static RequestServiceTimer *TheTimer();

    void start(std::chrono::milliseconds timeout_ms);

    std::chrono::steady_clock::time_point get_start_time() const { return start_time; }

    std::chrono::milliseconds elapsed() const;

    std::chrono::milliseconds remaining() const;

    bool is_timeout_enabled() const { return timeout_enabled; }

    bool is_expired() const;

    void disable_timeout();

    std::string dump(bool pretty = false) const;

    void dump(std::ostream &strm) const;

    void throw_if_timeout_expired(const std::string &message, const std::string &file, const int line);
};

#endif // I_RequestServiceTimer_h
