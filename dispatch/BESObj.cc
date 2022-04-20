// BESObj.cc

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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.// BESObj.cc

/** @brief top level BES object to house generic methods
 */

#include <utility>

#include "BESDebug.h"
#include "BESInternalFatalError.h"
#include "RequestServiceTimer.h"

using std::string;

/** @brief Checks the RequestServiceTimer to determine if the
 * time spent servicing the request at this point has exceeded the
 * bes_timeout configuration element.   If the request timeout has
 * expired throw BESInternalFatalError.
 *
 * @param message to be delivered in error response.
 * @param file The file (__FILE__) that called this method
 * @param line The line (__LINE__) in the file that made the call to this method.
*/
void BESObj::throw_if_timeout_expired(string message, string file, int line)
{
    if (RequestServiceTimer::TheTimer()->is_expired()) {
        throw BESInternalFatalError(std::move(message), std::move(file), line);
    }
}


