// FONgRequestHandler.h

// This file is part of BES GDAL File Out Module

// Copyright (c) 2012 OPeNDAP, Inc.
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

#ifndef I_FONgRequestHandler_H
#define I_FONgRequestHandler_H 1

#include "BESRequestHandler.h"

/** @brief A Request Handler for the Fileout GDAL request
 *
 * This class is used to represent the Fileout GDAL module, including
 * functions to build the help and version responses. Data handlers are
 * used to build a Dap DataDDS object, so those functions are not needed
 * here.
 */
class FONgRequestHandler: public BESRequestHandler {
    static bool d_use_byte_for_geotiff_bands;

public:
    FONgRequestHandler(const std::string &name);
    virtual ~FONgRequestHandler(void);

    virtual void dump(std::ostream &strm) const;

    static bool build_help(BESDataHandlerInterface &dhi);
    static bool build_version(BESDataHandlerInterface &dhi);

    static bool get_use_byte_for_geotiff_bands() { return d_use_byte_for_geotiff_bands; }
};

#endif

