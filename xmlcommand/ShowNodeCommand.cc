// ShowNodeCommand.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2018 OPeNDAP, Inc
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

#include "BESContainerStorageList.h"

#include "BESNames.h"
#include "BESDataNames.h"
#include "BESXMLUtils.h"
#include "BESUtil.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"

#include "ShowNodeCommand.h"

using namespace bes;

ShowNodeCommand::ShowNodeCommand(const BESDataHandlerInterface &base_dhi) :
    BESXMLCommand(base_dhi)
{
}

/**
 * @brief Parse a show node command.
 *
 * ~~~{.xml}
 * <showNode node="name" [catalog="name"]/>
 * ~~~
 *
 * @param node xml2 element node pointer
 */
void ShowNodeCommand::parse_request(xmlNode *node)
{
    string name;
    string value;
    map<string, string> props;
    BESXMLUtils::GetNodeInfo(node, name, value, props);
    if (name != NODE_RESPONSE_STR) {
        string err = "The specified command " + name + " is not a showNode command";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    // the action is the same for show catalog and show info
    d_xmlcmd_dhi.action = NODE_RESPONSE;

    d_cmd_log_info = "show node";

    // node is an optional property, so could be empty string
    d_xmlcmd_dhi.data[CONTAINER] = props["node"];

    if (!d_xmlcmd_dhi.data[CONTAINER].empty()) {
        d_cmd_log_info += " for " + d_xmlcmd_dhi.data[CONTAINER];
    }

    // catalog is an optional property, so could be empty string
    d_xmlcmd_dhi.data[CATALOG] = props["catalog"];

    if (!d_xmlcmd_dhi.data[CATALOG].empty()) {
        d_cmd_log_info += " for " + d_xmlcmd_dhi.data[CATALOG];
    }

    d_cmd_log_info += ";";

    // Get the response handler for the action (dhi.action == show.node)
    BESXMLCommand::set_response();
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void ShowNodeCommand::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "ShowNodeCommand::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESXMLCommand::dump(strm);
    BESIndent::UnIndent();
}

BESXMLCommand *
ShowNodeCommand::CommandBuilder(const BESDataHandlerInterface &base_dhi)
{
    return new ShowNodeCommand(base_dhi);
}

