// BESDataNames.h

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

#ifndef D_BESDataNames_h
#define D_BESDataNames_h 1

/**
 * @brief Names used with the DHI data map.
 *
 * The DataHandlerInterface uses a map<string, string> hash map as a simple
 * 'database' of key/value pairs to pass information about a request from
 * place to place in the BESInterface, BESCommand, RequestHandler and
 * ResponseHandler classes. Knowing these names and how the information
 * is used helps reveal the BES flow of control.
 */

/// Information is built up describing the command and then dumped to the bes.log
#define LOG_INFO "log_info"

#define REQUEST_ID_KEY "reqID"
#define REQUEST_UUID_KEY "reqUUID"
#define BES_CLIENT_ID_KEY "besClientId"
#define BES_CLIENT_CMD_COUNT_KEY "clientCmdCount"
/// The IP and port numbers from which the BES read this information.
#define REQUEST_FROM "from"

#define AGG_CMD "aggregation_command"
#define AGG_HANDLER "aggregation_handler"

#define POST_CONSTRAINT "post_constraint"

#define DAP4_FUNCTION "dap4Function"
#define DAP4_CONSTRAINT "dap4Constraint"

#define ASYNC "async"
#define STORE_RESULT "store_result"

#define RETURN_CMD "return_command"

#define USER_ADDRESS "user_address"
#define USER_NAME "username"
#define USER_TOKEN "token"

/// The pid for this instance of the BES
#define SERVER_PID "pid"

#define CONTAINER_NAME "container_name"
#define STORE_NAME "store_name"
#define SYMBOLIC_NAME "symbolic_name"
#define REAL_NAME "real_name"

#define CONTAINER_TYPE "type"

#define DEF_NAME "def_name"
#define DEFINITIONS "definitions"

#define CONTAINER "container"
#define CATALOG "catalog"

#define DEFAULT_CATALOG "catalog"

#define BES_KEY "besKey"

/*
 * Context
 */
#define CONTEXT_NAME "context_name"
#define CONTEXT_VALUE "context_value"

/*
 * Show Error Type Number
 */
#define SHOW_ERROR_TYPE "error_type_num"

/*
 * Options
 */
#define SILENT "silent"
#define BUFFERED "buffered"

/// Context name used to select XML errors regardless of bes.conf setting.
#define XML_ERRORS "xml"

#define DAP4_CHECKSUMS_CONTEXT_KEY "dap4_checksums"

#endif // D_BESDataNames_h
