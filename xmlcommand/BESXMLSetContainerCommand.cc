// BESXMLSetContainerCommand.cc

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

#include "config.h"

#include "BESXMLSetContainerCommand.h"
#include "BESContainerStorageList.h"
#include "BESCatalog.h"

#include "BESXMLUtils.h"
#include "BESUtil.h"

#include "BESResponseNames.h"
#include "BESDataNames.h"

#include "BESSyntaxUserError.h"
#include "BESInternalError.h"
#include "BESLog.h"
#include "BESDebug.h"

using std::endl;
using std::ostream;
using std::string;
using std::map;

BESXMLSetContainerCommand::BESXMLSetContainerCommand(const BESDataHandlerInterface &base_dhi) :
    BESXMLCommand(base_dhi)
{
}

/** @brief parse a set container command.
 *
 * ~~~{.xml}
 * <setContainer name="c" space="catalog"> data/nc/fnoc1.nc </setContainer>
 * ~~~
 *
 * @param node xml2 element node pointer
 */
void BESXMLSetContainerCommand::parse_request(xmlNode *node)
{
    string action;	// name of the node, should be setContainer
    // string name;	// symbolic name of the container as name=""
    // string storage;	// storage of container, default is default, as space=
    string value;	// real name of the container, e.g. path

    map<string, string> props;
    BESXMLUtils::GetNodeInfo(node, action, value, props);
    if (action != SETCONTAINER_STR) {
        throw BESInternalError(string("The specified command ").append(action).append(" is not a set container command."), __FILE__, __LINE__);
    }

    if (value.empty())
        throw BESSyntaxUserError(action + " command: container real name missing", __FILE__, __LINE__);

    // what is the symbolic name of this container
    if (props["name"].empty())
        throw BESSyntaxUserError(action + " command: name property missing", __FILE__, __LINE__);

    d_xmlcmd_dhi.data[SYMBOLIC_NAME] = props["name"];

    // Is the path (i.e., the 'value') of this command in a virtual directory?
    // If so, use the corresponding catalog name as the value of the 'space'
    // attribute, overriding what the client may have sent.
    //
    // @TODO Really? seems odd. I'd expect the opposite behavior - use what was given
    // and set the space using the catalog name if 'space' was not provided. jhrg 1/7/19
    BESCatalog *cat = BESUtil::separateCatalogFromPath(value);
    if (cat) {
        if (!props["space"].empty())
            VERBOSE("SetContainer called with 'space=\"" + props["space"] + "\" but the pathname uses \"" + cat->get_catalog_name() + "\"");
        props["space"] = cat->get_catalog_name();
    }

    if (!props["space"].empty()) {
        d_xmlcmd_dhi.data[STORE_NAME] = props["space"];
    }
    else {
        d_xmlcmd_dhi.data[STORE_NAME] = CATALOG /* DEFAULT jhrg 12/27/18 */; // CATALOG == "catalog"; DEFAULT == "default"
    }

    // 'type' can be empty (not used), so just set it this way
    d_xmlcmd_dhi.data[CONTAINER_TYPE] = props["type"];

    // now that everything has passed tests, set the value in the dhi
    d_xmlcmd_dhi.data[REAL_NAME] = value;

    d_xmlcmd_dhi.action = SETCONTAINER;

    d_cmd_log_info = (string) "set container in " + props["space"] + " values " + props["name"] + "," + value;
    if (!props["type"].empty()) {
        d_cmd_log_info += "," + props["type"];
    }
    d_cmd_log_info += ";";

    // now that we've set the action, go get the response handler for the action.
    // The class the evaluates this command is dispatch/BESSetContainerresponseHandler.
    BESXMLCommand::set_response();
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESXMLSetContainerCommand::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESXMLSetContainerCommand::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESXMLCommand::dump(strm);
    BESIndent::UnIndent();
}

BESXMLCommand *
BESXMLSetContainerCommand::CommandBuilder(const BESDataHandlerInterface &base_dhi)
{
    return new BESXMLSetContainerCommand(base_dhi);
}

