// -*- mode: c++; c-basic-offset:4 -*-
//
// FoJsonRequestHandler.h
//
// This file is part of BES JSON File Out Module
//
// Copyright (c) 2014 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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
// (c) COPYRIGHT URI/MIT 1995-1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#ifndef I_FoJsonRequestHandler_H
#define I_FoJsonRequestHandler_H 1

#include "BESRequestHandler.h"

/** @brief A Request Handler for the Fileout NetCDF request
 *
 * This class is used to represent the Fileout NetCDF module, including
 * functions to build the help and version responses. Data handlers are
 * used to build a Dap DataDDS object, so those functions are not needed
 * here.
 */
class FoJsonRequestHandler: public BESRequestHandler {
public:
    FoJsonRequestHandler(const std::string &name);
    virtual ~FoJsonRequestHandler(void);

    void dump(std::ostream &strm) const override;

    static bool build_help(BESDataHandlerInterface &dhi);
    static bool build_version(BESDataHandlerInterface &dhi);
};

#endif

