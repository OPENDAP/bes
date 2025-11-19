// -*- mode: c++; c-basic-offset:4 -*-
//
// DapFunctionsRequestHandler.h
//
// This file is part of BES DAP Functions handler
//
// Copyright (c) 2016 OPeNDAP, Inc.
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

#ifndef I_DapFunctionsRequestHandler_H
#define I_DapFunctionsRequestHandler_H 1

#include <string>
#include <ostream>

#include "BESRequestHandler.h"

class BESDataHandlerInterface;

//const std::string STARE_STORAGE_PATH = "FUNCTIONS.stareStoragePath";
//const std::string STARE_SIDECAR_SUFFIX = "FUNCTIONS.stareSidecarSuffix";
/** @brief A Request Handler for the DAP Functions module
 *
 */
class DapFunctionsRequestHandler: public BESRequestHandler {
public:
    DapFunctionsRequestHandler(const std::string &name);
    virtual ~DapFunctionsRequestHandler() {}

    void dump(std::ostream &strm) const override;

    static bool build_help(BESDataHandlerInterface &dhi);
    static bool build_version(BESDataHandlerInterface &dhi);
};

#endif

