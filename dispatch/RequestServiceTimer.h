// RequestServiceTimer.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2022 OPeNDAP, Inc
// Author: James Gallagher <jgallagher@opendap.org>
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

#include "BESObj.h"

/** @brief The master request service timer for this server; a singleton
 *
 */
class RequestServiceTimer : public BESObj{
private:
    static RequestServiceTimer *d_instance;
    mutable std::recursive_mutex d_cache_lock_mutex;

    unsigned long int d_duration;
    struct timeval d_start_usage;
    struct timeval d_stop_usage;
    struct timeval d_result;

    static void initialize_instance();
    static void delete_instance();

    unsigned long int get_start_us();
    unsigned long int get_stop_us();

public:
    RequestServiceTimer();
    virtual ~RequestServiceTimer();

    static RequestServiceTimer *TheTimer();

    unsigned long int remaining();
    unsigned long int elapsed();

    virtual void dump( std::ostream &strm ) const ;


};

#endif // I_RequestServiceTimer_h

