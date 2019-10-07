// BESXMLDeleteContainersCommand.cc

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

#include "BESXMLDeleteContainersCommand.h"
#include "BESContainerStorageList.h"
#include "BESDataNames.h"
#include "BESResponseNames.h"
#include "BESXMLUtils.h"
#include "BESUtil.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"

using std::endl;

BESXMLDeleteContainersCommand::BESXMLDeleteContainersCommand(const BESDataHandlerInterface &base_dhi) :
    BESXMLCommand(base_dhi)
{
}

/** @brief parse a delete containers command.
 *
 &lt;deleteContainers space="storeName" /&gt;
 *
 * @param node xml2 element node pointer
 */
void BESXMLDeleteContainersCommand::parse_request(xmlNode *node)
{
    string name;
    string value;
    map<string, string> props;
    BESXMLUtils::GetNodeInfo(node, name, value, props);
    if (name != DELETE_CONTAINERS_STR) {
        string err = "The specified command " + name + " is not a delete container command";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    // optional property, defaults to default
    string storage = props["space"];
    d_xmlcmd_dhi.data[STORE_NAME] = storage;
    if (d_xmlcmd_dhi.data[STORE_NAME].empty()) {
        d_xmlcmd_dhi.data[STORE_NAME] = CATALOG /* DEFAULT jhrg 12/27/18 */;
        storage = CATALOG /* DEFAULT jhrg 12/27/18 */;
    }

    d_xmlcmd_dhi.action = DELETE_CONTAINERS;

    d_cmd_log_info = (string) "delete containers from " + storage + ";";

    // now that we've set the action, go get the response handler for the
    // action
    BESXMLCommand::set_response();
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESXMLDeleteContainersCommand::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESXMLDeleteContainersCommand::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESXMLCommand::dump(strm);
    BESIndent::UnIndent();
}

BESXMLCommand *
BESXMLDeleteContainersCommand::CommandBuilder(const BESDataHandlerInterface &base_dhi)
{
    return new BESXMLDeleteContainersCommand(base_dhi);
}

