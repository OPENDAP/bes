// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES dmrpp_module/ngap_container.

// Copyright (c) 2026 OPeNDAP, Inc.
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

#ifndef BES_NGAPSERVICECHAINING_H_
#define BES_NGAPSERVICECHAINING_H_

#include <string>

class BESError;

namespace ngap {

/**
 * @brief Support for the 'Service Chaining Error' message.
 *
 * The 'Service Chaining Error' text is added to an error only when the request
 * is an NGAP request (it uses an NgapOwnedContainer) and an external service
 * (CMR, TEA, S3, ...) returned a 50x HTTP response.
 *
 * The NGAP-request flag is per-request. It is set by the DMR++ request handler
 * at the start of each request and read later, possibly from data transfer threads.
 */

/// Record whether the current request uses an NgapOwnedContainer.
void set_ngap_request(bool is_ngap);

/// @return True if the current request uses an NgapOwnedContainer.
bool is_ngap_request();

/// @return True if status is a 50x HTTP response code.
inline bool is_http_5xx(long status) { return status >= 500 && status < 600; }

/**
 * @brief Prefix the error's message with the Service Chaining text.
 * @param e The error to modify (e.g., http::HttpError or BESInternalError)
 * @param prolog Text that identifies the caller
 * @param activity What was being done, e.g. "attempting to retrieve a CMR record."
 */
void add_service_chaining_message(BESError &e, const std::string &prolog, const std::string &activity);

} // namespace ngap

#endif // BES_NGAPSERVICECHAINING_H_
