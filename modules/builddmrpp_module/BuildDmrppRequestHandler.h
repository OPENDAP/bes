// BuildDmrppRequestHandler.cc

// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of builddmrpp_module, A C++ module that can be loaded in to
// the OPeNDAP Back-End Server (BES) and is able to handle remote requests.

// Copyright (c) 2023 OPeNDAP, Inc.
// Author: Daniel Holloway <dholloway@opendap.org>
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
// Authors:
//      ndp       Nathan Potter <ndp@opendap.org>
//      dan       Daniel Holloway <dholloway@opendap.org>

#ifndef I_BuildDmrppRequestHandler_H
#define I_BuildDmrppRequestHandler_H

#include "BESRequestHandler.h"

namespace builddmrpp {

    class BuildDmrppRequestHandler: public BESRequestHandler {
    public:
        explicit BuildDmrppRequestHandler(const std::string &name);
        ~BuildDmrppRequestHandler() override = default;

        void dump(std::ostream &strm) const override;

        static bool mkdmrpp_build_vers(BESDataHandlerInterface &dhi);
        static bool mkdmrpp_build_help(BESDataHandlerInterface &dhi);
    };

} // namespace builddmrpp

#endif // BuildDmrppRequestHandler.h
