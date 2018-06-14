// -*- mode: c++; c-basic-offset:4 -*-
//
// FoCovJsonRequestHandler.h
//
// This file is part of BES CovJSON File Out Module
//
// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Corey Hemphill <hemphilc@oregonstate.edu>
// Author: River Hendriksen <hendriri@oregonstate.edu>
// Author: Riley Rimer <rrimer@oregonstate.edu>
//
// Adapted from the File Out JSON module implemented by Nathan Potter
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

#ifndef I_FoCovJsonRequestHandler_H
#define I_FoCovJsonRequestHandler_H 1

#include "BESRequestHandler.h"

/** @brief A Request Handler for the Fileout NetCDF request
 *
 * This class is used to represent the Fileout NetCDF module, including
 * functions to build the help and version responses. Data handlers are
 * used to build a Dap DataDDS object, so those functions are not needed
 * here.
 */
class FoCovJsonRequestHandler: public BESRequestHandler {
public:
    FoCovJsonRequestHandler(const string &name);
    virtual ~FoCovJsonRequestHandler(void);

    virtual void dump(ostream &strm) const;

    static bool build_help(BESDataHandlerInterface &dhi);
    static bool build_version(BESDataHandlerInterface &dhi);
};

#endif

