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

#include "BESXMLSetContainerCommand.h"
#include "BESContainerStorageList.h"
#include "BESXMLUtils.h"
#include "BESUtil.h"
#include "BESResponseNames.h"
#include "BESDataNames.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"

BESXMLSetContainerCommand::BESXMLSetContainerCommand(const BESDataHandlerInterface &base_dhi) :
    BESXMLCommand(base_dhi)
{
}

/** @brief parse a set container command.
 *
 &lt;setContainer name="c" space="catalog"&gt;data/nc/fnoc1.nc&lt;/setContainer&gt;
 *
 * @param node xml2 element node pointer
 */
void BESXMLSetContainerCommand::parse_request(xmlNode *node)
{
    string action;	// name of the node, should be setContainer
    string name;	// symbolic name of the container as name=""
    string storage;	// storage of container, default is default, as space=
    string value;	// real name of the container, e.g. path

    map<string, string> props;
    BESXMLUtils::GetNodeInfo(node, action, value, props);
    if (action != SETCONTAINER_STR) {
        string err = "The specified command " + action + " is not a set container command";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    string cname;
    string cvalue;
    map<string, string> cprops;
    xmlNode *real = BESXMLUtils::GetFirstChild(node, cname, cvalue, cprops);

    if (value.empty() && !real) {
        string err = action + " command: container real name missing";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    // what is the symbolic name of this container
    name = props["name"];
    if (name.empty()) {
        string err = action + " command: name property missing";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }
    d_xmlcmd_dhi.data[SYMBOLIC_NAME] = name;

    // where should this container be stored
    d_xmlcmd_dhi.data[STORE_NAME] = PERSISTENCE_VOLATILE;
    storage = props["space"];
    if (!storage.empty()) {
        d_xmlcmd_dhi.data[STORE_NAME] = storage;
    }
    else {
        storage = PERSISTENCE_VOLATILE;
    }

    // this can be the empty string, so just set it this way
    string container_type = props["type"];
    d_xmlcmd_dhi.data[CONTAINER_TYPE] = container_type;

    // now that everything has passed tests, set the value in the dhi
    d_xmlcmd_dhi.data[REAL_NAME] = value;

    // if there is a child node, then the real value of the container is
    // this content, or is set in this content.
    if (real) {
        xmlBufferPtr buf = xmlBufferCreate();
        xmlNodeDump(buf, real->doc, real, 2, 1);
        if (buf->content) {
            value = (char *) buf->content;
            d_xmlcmd_dhi.data[REAL_NAME] = (char *) (buf->content);
        }
    }

    d_xmlcmd_dhi.action = SETCONTAINER;

    d_cmd_log_info = (string) "set container in " + storage + " values " + name + "," + value;
    if (!container_type.empty()) {
        d_cmd_log_info += "," + container_type;
    }
    d_cmd_log_info += ";";

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

