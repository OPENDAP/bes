
// -*- mode: c++; c-basic-offset:4 -*-
//
// GatewayPathInfoCommand.cc
//
// This file is part of BES dap package
//
// Copyright (c) 2015v OPeNDAP, Inc.
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
// Please read the full copyright statement in the file COPYRIGHT_URI.
//

#include "config.h"

#include "GatewayPathInfoCommand.h"
#include "BESDataNames.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESXMLUtils.h"
#include "BESSyntaxUserError.h"

using std::endl;
using std::ostream;
using std::string;
using std::map;

GatewayPathInfoCommand::GatewayPathInfoCommand(const BESDataHandlerInterface &base_dhi) :
    BESXMLCommand(base_dhi)
{

}

/** @brief parse a show command. No properties or children elements
 *
 &lt;showCatalog node="containerName" /&gt;
 *
 * @param node xml2 element node pointer
 */
void GatewayPathInfoCommand::parse_request(xmlNode *node)
{
    string name;
    string value;
    map<string, string> props;
    BESXMLUtils::GetNodeInfo(node, name, value, props);
    if (name != SHOW_GATEWAY_PATH_INFO_RESPONSE_STR) {
        string err = "The specified command " + name + " is not a gateway show path info command";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    //  the action is to show the path info response
    d_xmlcmd_dhi.action = SHOW_GATEWAY_PATH_INFO_RESPONSE;
    d_xmlcmd_dhi.data[SHOW_GATEWAY_PATH_INFO_RESPONSE] = SHOW_GATEWAY_PATH_INFO_RESPONSE;
    d_cmd_log_info = "show gatewayPathInfo";

    // node is an optional property, so could be empty string
    d_xmlcmd_dhi.data[CONTAINER] = props["node"];
    if (!d_xmlcmd_dhi.data[CONTAINER].empty()) {
        d_cmd_log_info += " for " + d_xmlcmd_dhi.data[CONTAINER];
    }
    d_cmd_log_info += ";";

    BESDEBUG(SPI_DEBUG_KEY, "Built BES Command: '" << d_cmd_log_info << "'"<< endl );

    // now that we've set the action, go get the response handler for the
    // action by calling set_response() in our parent class
    BESXMLCommand::set_response();
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void GatewayPathInfoCommand::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "GatewayPathInfoCommand::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESXMLCommand::dump(strm);
    BESIndent::UnIndent();
}

BESXMLCommand *
GatewayPathInfoCommand::CommandBuilder(const BESDataHandlerInterface &base_dhi)
{
    return new GatewayPathInfoCommand(base_dhi);
}

