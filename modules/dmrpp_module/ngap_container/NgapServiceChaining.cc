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

#include "config.h"

#include <atomic>
#include <string>

#include "BESError.h"

#include "NgapServiceChaining.h"

using namespace std;

namespace ngap {

// Written by the request handler before any data transfer threads start; read by
// those threads. jhrg 10/8/26
static std::atomic<bool> d_ngap_request{false};

void set_ngap_request(bool is_ngap) {
    d_ngap_request.store(is_ngap);
}

bool is_ngap_request() {
    return d_ngap_request.load();
}

void add_service_chaining_message(BESError &e, const string &prolog, const string &activity) {
    e.set_message(prolog + "Hyrax encountered a Service Chaining Error while " + activity + "\n" + e.get_message());
}

} // namespace ngap
