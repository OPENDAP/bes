// BESXMLGetCommand.cc

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

#include <sstream>

#include "BESXMLGetCommand.h"
#include "BESDefinitionStorageList.h"
#include "BESDefinitionStorage.h"
#include "BESDefine.h"
#include "BESDataNames.h"
#include "BESResponseNames.h"
#include "BESDapNames.h"

#include "BESXMLUtils.h"
#include "BESUtil.h"
#include "BESSyntaxUserError.h"
#include "BESLog.h"
#include "BESDebug.h"

using namespace std;

BESXMLGetCommand::BESXMLGetCommand(const BESDataHandlerInterface &base_dhi) :
    BESXMLCommand(base_dhi), _sub_cmd(0)
{
}

/**
 * @brief parse a `get` command.
 *
 * The `get` commands have the form:
 *
 * `get type="dds" definition="d" returnAs="name"`
 *
 * and return the `returnAs` item derived from the `type` object
 * for the data referenced by the `definition`.
 *
 * @param node xml2 element node pointer
 */
void BESXMLGetCommand::parse_request(xmlNode *node)
{
    string name;
    string value;
    map<string, string> props;
    BESXMLUtils::GetNodeInfo(node, name, value, props);

    if (name != GET_RESPONSE) {
        string err = "The specified command " + name + " is not a get command";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    // grab the type first and check to see if there is a registered command
    // to handle get.<type> requests
    string type = props["type"];
    if (type.empty()) {
        string err = name + " command: Must specify data product type";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    // TODO I don't think this is ever used. jhrg 3/13/18
    string new_cmd = (string) GET_RESPONSE + "." + type;
    p_xmlcmd_builder bldr = BESXMLCommand::find_command(new_cmd);
    if (bldr) {
        // the base dhi was copied to this instance's _dhi variable.
        _sub_cmd = bldr(d_xmlcmd_dhi);
        if (!_sub_cmd) {
            string err = (string) "Failed to build command object for " + new_cmd;
            throw BESInternalError(err, __FILE__, __LINE__);
        }

        // parse the request given the current node
        _sub_cmd->parse_request(node);

        // return from this sub command
        return;
    }

    parse_basic_get(type, props);
    d_cmd_log_info += ";";

    // Now that we've set the action, get the response handler for the action
    BESXMLCommand::set_response();
}

/**
 * @brief Extract information from the 'props' map
 *
 * Extract the values for various properties from the whole command
 * file and load them into this command's DHI 'data' map or the specific
 * 'definition' or 'space' values. As a side effect, build the cmd_log_info
 * string (used to record information about this command in the BES log.
 *
 * @param type The thing to get (e.g., dds)
 * @param props Holds the definition, space, returnAs, etc., values to
 * be used with when running the command.
 */
void BESXMLGetCommand::parse_basic_get(const string &type, map<string, string> &props)
{
    d_cmd_log_info = "get ";    // Remove any old value of this string
    d_cmd_log_info.append(type);

    _definition = props["definition"];
    if (_definition.empty())
        throw BESSyntaxUserError("get command: Must specify definition", __FILE__, __LINE__);

    d_cmd_log_info.append(" for ").append(_definition);

    _space = props["space"];

    if (!_space.empty()) d_cmd_log_info.append(" in ").append(_space);

#if 0
    string returnAs = props["returnAs"];
    if (returnAs.empty()) {
        returnAs = DAP2_FORMAT;
    }
#endif

    // This is a holdover from before there was DAP4. An empty 'return as' value means
    // 'return a DAP response'. If the value is not empty, then it's the name of the
    // file format like netCDF4. jhrg 1/6/25
    if (props["returnAs"].empty()) {
        d_xmlcmd_dhi.data[RETURN_CMD] = DAP2_FORMAT;
    }
    else {
        d_xmlcmd_dhi.data[RETURN_CMD] = props["returnAs"];
    }

    if (!props["returnAs"].empty()) {
        d_cmd_log_info.append(" return as ").append(props["returnAs"]);
    }

#if 0
    d_cmd_log_info.append(" return as ").append(returnAs);
#endif

    d_xmlcmd_dhi.data[STORE_RESULT] = props[STORE_RESULT];
    d_xmlcmd_dhi.data[ASYNC] = props[ASYNC];

    d_xmlcmd_dhi.action = "get.";
    d_xmlcmd_dhi.action.append(BESUtil::lowercase(type));

    BESDEBUG("besxml", "Converted xml element name to command " << d_xmlcmd_dhi.action << endl);
}

/** @brief returns the BESDataHandlerInterface of either a sub command, if
 * one exists, or this command's
 *
 * @return BESDataHandlerInterface of sub command if it exists or this
 *         instances
 */
BESDataHandlerInterface &
BESXMLGetCommand::get_xmlcmd_dhi()
{
    if (_sub_cmd) return _sub_cmd->get_xmlcmd_dhi();

    return d_xmlcmd_dhi;
}

/** @brief Prepare any information needed to execute the request of
 * this get command
 *
 * This function is used to prepare the information needed to execute
 * the get request. It finds the definition specified in the element
 * and prepares all of the containers within that definition.
 */
void BESXMLGetCommand::prep_request()
{
    // if there is a sub command then execute the prep request on it
    // TODO I don't think this is ever used. jhrg 3/13/18
    if (_sub_cmd) {
        _sub_cmd->prep_request();
        return;
    }

    BESDefine *d = 0;

    if (!_space.empty()) {
        BESDefinitionStorage *ds = BESDefinitionStorageList::TheList()->find_persistence(_space);
        if (ds) {
            d = ds->look_for(_definition);
        }
    }
    else {
        d = BESDefinitionStorageList::TheList()->look_for(_definition);
    }

    if (!d) {
        string s = (string) "Unable to find definition " + _definition;
        throw BESSyntaxUserError(s, __FILE__, __LINE__);
    }

    BESDefine::containers_citer i = d->first_container();
    BESDefine::containers_citer ie = d->end_container();
    while (i != ie) {
        d_xmlcmd_dhi.containers.push_back(*i);
        i++;
    }

    // TODO Not ever used. jhrg 3/13/18
    d_xmlcmd_dhi.data[AGG_CMD] = d->get_agg_cmd();
    d_xmlcmd_dhi.data[AGG_HANDLER] = d->get_agg_handler();
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESXMLGetCommand::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESXMLGetCommand::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESXMLCommand::dump(strm);
    BESIndent::UnIndent();
}

BESXMLCommand *
BESXMLGetCommand::CommandBuilder(const BESDataHandlerInterface &base_dhi)
{
    return new BESXMLGetCommand(base_dhi);
}

