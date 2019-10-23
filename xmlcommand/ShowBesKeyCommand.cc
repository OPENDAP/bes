// -*- mode: c++; c-basic-offset:4 -*-
//
// ShowBesKeyCommand.cc
//
// This file is part of the BES default command set
//
// Copyright (c) 2018 OPeNDAP, Inc
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


#include "ShowBesKeyCommand.h"
#include "BESDataNames.h"
#include "BESDebug.h"
#include "BESError.h"
#include "BESUtil.h"
#include "BESXMLUtils.h"
#include "BESSyntaxUserError.h"

using std::endl;
using std::ostream;

ShowBesKeyCommand::ShowBesKeyCommand(const BESDataHandlerInterface &base_dhi) :
    BESXMLCommand(base_dhi)
{

}

/** @brief parse a show command. No properties or children elements
 *
 * ~~~{.xml}
 * <showBesKey key="key_name" />
 * ~~~
 *
 * @param node xml2 element node pointer
 */
void ShowBesKeyCommand::parse_request(xmlNode *node)
{
    string name;
    string value;
    map<string, string> props;
    BESXMLUtils::GetNodeInfo(node, name, value, props);
    if (name != SHOW_BES_KEY_RESPONSE_STR) {
        string err = "The specified command " + name + " is not a " + SHOW_BES_KEY_RESPONSE_STR + " command";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    // the action is to show the requested BES key value info response
    d_xmlcmd_dhi.action = SHOW_BES_KEY_RESPONSE;
    d_xmlcmd_dhi.data[SHOW_BES_KEY_RESPONSE] = SHOW_BES_KEY_RESPONSE;
    d_cmd_log_info = "show besKey";

    // key is a required property, so it MAY NOT be the empty string

    string requested_bes_key =  props["key"];

    if(requested_bes_key.empty())
        throw BESError("Ouch! A Key name was not submitted with the request for a Key value from BESKeys", BES_SYNTAX_USER_ERROR, __FILE__, __LINE__);

    d_xmlcmd_dhi.data[BES_KEY] = requested_bes_key;

    if (!d_xmlcmd_dhi.data[BES_KEY].empty()) {
        d_cmd_log_info += " for " + d_xmlcmd_dhi.data[BES_KEY];
    }
    d_cmd_log_info += ";";

    BESDEBUG(SBK_DEBUG_KEY, "Built BES Command: '" << d_cmd_log_info << "'"<< endl );

    // Given that we've set the action above, set the response handler for the
    // action by calling set_response() in our parent class
    BESXMLCommand::set_response();
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void ShowBesKeyCommand::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "ShowBesKeyCommand::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESXMLCommand::dump(strm);
    BESIndent::UnIndent();
}

BESXMLCommand *
ShowBesKeyCommand::CommandBuilder(const BESDataHandlerInterface &base_dhi)
{
    return new ShowBesKeyCommand(base_dhi);
}



