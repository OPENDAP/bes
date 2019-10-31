// BESXMLShowErrorCommand.cc

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

#include "BESXMLShowErrorCommand.h"
#include "BESXMLUtils.h"
#include "BESUtil.h"
#include "BESResponseNames.h"
#include "BESDataNames.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"

using std::endl;
using std::ostream;
using std::string;
using std::map;

BESXMLShowErrorCommand::BESXMLShowErrorCommand(const BESDataHandlerInterface &base_dhi) :
    BESXMLCommand(base_dhi)
{
}

/** @brief parse a set context command.
 *
 &lt;showError type="error_type_num" /&gt;
 *
 * Where error_type_num is one of the following
 * 1. Internal Error - the error is internal to the BES Server
 * 2. Internal Fatal Error - error is fatal, can not continue
 * 3. Syntax User Error - the requester has a syntax error in request or config
 * 4. Forbidden Error - the requester is forbidden to see the resource
 * 5. Not Found Error - the resource can not be found
 *
 * @param node xml2 element node pointer
 */
void BESXMLShowErrorCommand::parse_request(xmlNode *node)
{
    string etype;
    string value;
    string action;
    map<string, string> props;
    BESXMLUtils::GetNodeInfo(node, action, value, props);
    if (action != SHOW_ERROR_STR) {
        string err = "The specified command " + action + " is not a show error command";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    d_xmlcmd_dhi.action = SHOW_ERROR;

    etype = props["type"];
    if (etype.empty()) {
        string err = action + " command: error type property missing";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }
    // test the error type number in the response handler
    d_xmlcmd_dhi.data[SHOW_ERROR_TYPE] = etype;
    d_cmd_log_info = (string) "show error " + etype + ";";

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
void BESXMLShowErrorCommand::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESXMLShowErrorCommand::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESXMLCommand::dump(strm);
    BESIndent::UnIndent();
}

BESXMLCommand *
BESXMLShowErrorCommand::CommandBuilder(const BESDataHandlerInterface &base_dhi)
{
    return new BESXMLShowErrorCommand(base_dhi);
}

