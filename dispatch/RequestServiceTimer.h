// RequestServiceTimer.h

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
#define DEFAULT_BES_TIMEOUT_SECONDS 600

class RequestServiceTimer : public BESObj{
private:
    static RequestServiceTimer *d_instance;
    mutable std::recursive_mutex d_rst_lock_mutex;

    std::chrono::duration<int> bes_timeout;
    std::chrono::steady_clock::time_point start_time;
    bool timeout_enabled;

     explicit RequestServiceTimer():
            bes_timeout(std::chrono::seconds(DEFAULT_BES_TIMEOUT_SECONDS)),
            is_started(false),
            timeout_enabled(false),
            start_time(std::chrono::steady_clock::now()) {
    }

    static void delete_instance();
    static void initialize_instance();

public:
    static RequestServiceTimer *TheTimer();

    std::chrono::steady_clock::time_point start(int timeout_seconds);

    std::chrono::duration<int> elapsed() const;

    std::chrono::duration<int> remaining() const;

    bool is_expired() const;

    void disable_timeout();

    std::string dump() const ;

    void dump( std::ostream &strm ) const ;

};

#endif // I_RequestServiceTimer_h

