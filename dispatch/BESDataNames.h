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

#define DATA_REQUEST "request"
#define REQUEST_ID "reqID"
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

#define SERVER_PID "pid"

#define CONTAINER_NAME "container_name"
#define STORE_NAME "store_name"
#define SYMBOLIC_NAME "symbolic_name"
#define REAL_NAME "real_name"
#define REAL_NAME_LIST "real_name_list"
#define CONTAINER_TYPE "type"

#define DEF_NAME "def_name"
#define DEFINITIONS "definitions"

#define CONTAINER "container"
#define CATALOG "catalog"

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

#endif // D_BESDataNames_h
