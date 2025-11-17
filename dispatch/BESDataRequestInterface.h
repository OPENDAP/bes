// BESDataRequestInterface.h

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

#ifndef BESDataRequestInterface_h_
#define BESDataRequestInterface_h_ 1

/** @brief Structure storing information from the Apache module
 */

using BESDataRequestInterface = struct _BESDataRequestInterface {
    /** @brief name of server running Apache server
     */
    const char *server_name;
    /** @brief not used
     */
    const char *server_address;
    /** @brief protocol of the request, such as "HTTP/0.9" or "HTTP/1.1"
     */
    const char *server_protocol;
    /** @brief TCP port number where the server running Apache is listening
     */
    const char *server_port;
    /** @brief uri of the request
     */
    const char *script_name;
    /** @brief remote ip address of client machine
     */
    const char *user_address;
    /** @brief information about the user agent originating the request, e.g. Mozilla/4.04 (X11; I; SunOS 5.4 sun4m)
     */
    const char *user_agent;
    /** @brief BES request string
     */
    const char *request;
    /** @brief server cookies set in users browser
     */
    const char *cookie;
    /** @brief session token passed in URL
     */
    const char *token;
};

#endif // BESDataRequestInterface_h_
