// BESDapService.h

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
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

#ifndef I_BESDapService_h
#define I_BESDapService_h 1

#include <string>

/** @brief static helper functions to register a handler to handle dap
 * services and add commands to the dap service
 *
 * To support the dap services a handler must provide the ability to handle
 * the filling in of the OPeNDAP objects DAS, DDS, and DataDDS. The dap
 * service provides additional commands that use these basic three commands.
 * These additional commands are DDX and, provided by the (old) dap-server
 * modules, the ascii, info_page, and html_form commands.
 *
 * @see BESServiceRegistry
 */
class BESDapService
{
public:
    /** @brief static function to register a handler to handle the dap
     * services
     *
     * @param handler the name of the handler, such as nc for netcdf_handler
     * @see BESServiceRegistry
     */
    static void	handle_dap_service( const std::string &handler ) ;

    /** @brief static function to add commands to the dap service
     *
     * This helper function allows the caller to add commands to the dap
     * service, such as dap-server adding the ascii, info_page and html_form
     * commands.
     *
     * @param cmd the name of the command to add
     * @param desc a description of the command being added
     * @see BESServiceRegistry
     */
    static void add_to_dap_service( const std::string &cmd, const std::string &desc ) ;
} ;

#endif // I_BESDapService_h

