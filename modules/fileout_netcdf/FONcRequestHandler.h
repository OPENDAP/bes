// FONcRequestHandler.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef I_FONcRequestHandler_H
#define I_FONcRequestHandler_H 1

#include "BESRequestHandler.h"

/** @brief A Request Handler for the Fileout NetCDF request
 *
 * This class is used to represent the Fileout NetCDF module, including
 * functions to build the help and version responses. Data handlers are
 * used to build a Dap DataDDS object, so those functions are not needed
 * here.
 */
class FONcRequestHandler: public BESRequestHandler {
public:
    FONcRequestHandler(const std::string &name);
    virtual ~FONcRequestHandler(void);

    virtual void dump(std::ostream &strm) const;

    static std::string temp_dir;
    static bool byte_to_short;
    static bool use_compression;
    static bool use_shuffle;
    static unsigned long long chunk_size;
    static bool classic_model;
    static bool reduce_dim;
    static bool no_global_attrs;
    static unsigned long long request_max_size_kb;
    static bool nc3_classic_format;

    static bool build_help(BESDataHandlerInterface &dhi);
    static bool build_version(BESDataHandlerInterface &dhi);
    static unsigned long long get_request_max_size_kb(){
        return request_max_size_kb;
    }
};

#endif

