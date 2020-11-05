// -*- mode: c++; c-basic-offset:4 -*-
//
// W10nShowPathInfoCommand.cc
//
// This file is part of BES w10n handler
//
// Copyright (c) 2020 OPeNDAP, Inc.
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

#include "W10nShowPathInfoCommand.h"
#include "W10NNames.h"
#include "BESDataNames.h"
#include "BESDebug.h"
#include "BESUtil.h"
#include "BESXMLUtils.h"
#include "BESSyntaxUserError.h"

using std::endl;
using std::ostream;
using std::string;
using std::map;

#define W10N_SHOW_PATH_INFO_DHI_TAG "show.w10nPathInfo"

W10nShowPathInfoCommand::W10nShowPathInfoCommand(const BESDataHandlerInterface &base_dhi) :
    BESXMLCommand(base_dhi)
{
}

/** @brief parse a show command. No properties or children elements
 *
 &lt;showCatalog node="containerName" /&gt;
 *
 * @param node xml2 element node pointer
 */
void W10nShowPathInfoCommand::parse_request(xmlNode *node)
{
    string name;
    string value;
    map<string, string> props;
    BESXMLUtils::GetNodeInfo(node, name, value, props);
    if (name != W10N_SHOW_PATH_INFO_REQUEST) {
        string err = "The specified command " + name + " is not a show w10n command";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    // the the action is to show the w10n info response
    d_xmlcmd_dhi.action = W10N_SHOW_PATH_INFO_DHI_TAG;
    d_xmlcmd_dhi.data[W10N_SHOW_PATH_INFO_DHI_TAG] = W10N_SHOW_PATH_INFO_DHI_TAG;
    d_cmd_log_info = "show w10nPathInfo";

    // node is an optional property, so could be empty string
    d_xmlcmd_dhi.data[CONTAINER] = props["node"];
    if (!d_xmlcmd_dhi.data[CONTAINER].empty()) {
        d_cmd_log_info += " for " + d_xmlcmd_dhi.data[CONTAINER];
    }
    d_cmd_log_info += ";";

    BESDEBUG(W10N_DEBUG_KEY, "Built BES Command: '" << d_cmd_log_info << "'"<< endl );

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
void W10nShowPathInfoCommand::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "W10nShowPathInfoCommand::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESXMLCommand::dump(strm);
    BESIndent::UnIndent();
}

BESXMLCommand *
W10nShowPathInfoCommand::CommandBuilder(const BESDataHandlerInterface &base_dhi)
{
    return new W10nShowPathInfoCommand(base_dhi);
}

