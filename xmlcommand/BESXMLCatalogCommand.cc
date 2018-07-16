// BESXMLCatalogCommand.cc

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

#include "BESXMLCatalogCommand.h"
#include "BESContainerStorageList.h"
#include "BESNames.h"
#include "BESDataNames.h"
#include "BESXMLUtils.h"
#include "BESUtil.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"

BESXMLCatalogCommand::BESXMLCatalogCommand(const BESDataHandlerInterface &base_dhi) :
    BESXMLCommand(base_dhi)
{
}

/** @brief parse a show command. No properties or children elements
 *
 &lt;showCatalog node="containerName" /&gt;
 *
 * @param node xml2 element node pointer
 */
void BESXMLCatalogCommand::parse_request(xmlNode *node)
{
    string name;
    string value;
    map<string, string> props;
    BESXMLUtils::GetNodeInfo(node, name, value, props);
    if (name != CATALOG_RESPONSE_STR /*&& name != SHOW_INFO_RESPONSE_STR*/) {
        string err = "The specified command " + name + " is not a show catalog or show info command";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    // the action is the same for show catalog and show info
    d_xmlcmd_dhi.action = CATALOG_RESPONSE;

    d_xmlcmd_dhi.data[CATALOG_OR_INFO] = CATALOG_RESPONSE;
    d_cmd_log_info = "show catalog";
#if 0
    // the CATALOG_OR_INFO data value will say if it's a show catalog or
    // show info
    if (name == CATALOG_RESPONSE_STR) {
        d_xmlcmd_dhi.data[CATALOG_OR_INFO] = CATALOG_RESPONSE;
        d_cmd_log_info = "show catalog";
    }
    else {
        d_xmlcmd_dhi.data[CATALOG_OR_INFO] = SHOW_INFO_RESPONSE;
        d_cmd_log_info = "show info";
    }
#endif

    // node is an optional property, so could be empty string
    d_xmlcmd_dhi.data[CONTAINER] = props["node"];

    if (!d_xmlcmd_dhi.data[CONTAINER].empty()) {
        d_cmd_log_info += " for " + d_xmlcmd_dhi.data[CONTAINER];
    }

    d_cmd_log_info += ";";

    // Get the response handler for the action
    BESXMLCommand::set_response();
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESXMLCatalogCommand::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESXMLCatalogCommand::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESXMLCommand::dump(strm);
    BESIndent::UnIndent();
}

BESXMLCommand *
BESXMLCatalogCommand::CommandBuilder(const BESDataHandlerInterface &base_dhi)
{
    return new BESXMLCatalogCommand(base_dhi);
}

