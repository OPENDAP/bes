// BESResponseNames.h

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

#ifndef D_BESResponseNames_H
#define D_BESResponseNames_H 1

/** @brief macros representing the default response objects handled
 *
 * These include
 * @verbatim
 * set
 * define
 * send
 * get
 *     stream
 * show
 *     help
 *     process
 *     version
 *     containers
 *     definitions
 *     config
 *     status
 * delete
 * @endverbatim
 */

#define SET_RESPONSE "set"
#define SETCONTAINER "set.container"
#define SETCONTAINER_STR "setContainer"
#define SET_CONTEXT "set.context"
#define SET_CONTEXT_STR "setContext"

#define DEFINE_RESPONSE "define"
#define DEFINE_RESPONSE_STR "define"

#define SEND_RESPONSE "send"

#define GET_RESPONSE "get"
#define STREAM_RESPONSE "get.stream"
#define STREAM_SERVICE "stream"
#define STREAM_RESPONSE_STR "getStream"

#define SHOW_RESPONSE "show"
#define HELP_RESPONSE "show.help"
#define HELP_RESPONSE_STR "showHelp"
#define PROCESS_RESPONSE "show.process"
#define PROCESS_RESPONSE_STR "showProcess"
#define VERS_RESPONSE "show.version"
#define VERS_RESPONSE_STR "showVersion"
#define SHOWCONTAINERS_RESPONSE "show.containers"
#define SHOWCONTAINERS_RESPONSE_STR "showContainers"
#define SHOWDEFS_RESPONSE "show.definitions"
#define SHOWDEFS_RESPONSE_STR "showDefinitions"
#define CONFIG_RESPONSE "show.config"
#define CONFIG_RESPONSE_STR "showConfig"
#define STATUS_RESPONSE "show.status"
#define STATUS_RESPONSE_STR "showStatus"
#define SERVICE_RESPONSE "show.servicedescriptions"
#define SERVICE_RESPONSE_STR "showServiceDescriptions"
#define SHOW_CONTEXT "show.context"
#define SHOW_CONTEXT_STR "showContext"
#define SHOW_ERROR "show.error"
#define SHOW_ERROR_STR "showError"

#define DELETE_RESPONSE "delete"
#define DELETE_CONTAINER "delete.container"
#define DELETE_CONTAINER_STR "deleteContainer"
#define DELETE_CONTAINERS "delete.containers"
#define DELETE_CONTAINERS_STR "deleteContainers"
#define DELETE_DEFINITION "delete.definition"
#define DELETE_DEFINITION_STR "deleteDefinition"
#define DELETE_DEFINITIONS "delete.definitions"
#define DELETE_DEFINITIONS_STR "deleteDefinitions"

#endif // E_BESResponseNames_H
