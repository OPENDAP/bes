// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of nc_handler, a data handler for the OPeNDAP data
// server. 

// This file is part of the GDAL OPeNDAP Adapter

// Copyright (c) 2004 OPeNDAP, Inc.
// Author: Frank Warmerdam <warmerdam@pobox.com>
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

// CDFRequestHandler.h

#ifndef I_GDALRequestHandler_H
#define I_GDALRequestHandler_H 1

#include <BESRequestHandler.h>

class GDALRequestHandler: public BESRequestHandler {
private:
public:
    GDALRequestHandler(const std::string &name);
    virtual ~GDALRequestHandler(void);

    static bool gdal_build_das(BESDataHandlerInterface &dhi);
    static bool gdal_build_dds(BESDataHandlerInterface &dhi);
    static bool gdal_build_data(BESDataHandlerInterface &dhi);
    static bool gdal_build_dmr_using_dds(BESDataHandlerInterface &dhi);
    static bool gdal_build_dmr(BESDataHandlerInterface &dhi);
    static bool gdal_build_help(BESDataHandlerInterface &dhi);
    static bool gdal_build_version(BESDataHandlerInterface &dhi);
    
    // This handler supports the "not including attributes" in
    // the data access feature. Attributes are generated only
    // if necessary. KY 10/30/19
    void add_attributes(BESDataHandlerInterface &dhi);
};

#endif

