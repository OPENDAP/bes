// BESXMLCommand.cc

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

#include <iostream>

#include "BESXMLCommand.h"
#include "BESResponseHandlerList.h"
#include "BESSyntaxUserError.h"
#include "BESDataNames.h"
#include "BESLog.h"

using std::endl;
using std::ostream;
using std::map;
using std::string;

map<string, p_xmlcmd_builder> BESXMLCommand::factory;

/** @brief Creates a BESXMLCommand document given a base data handler
 * interface object
 *
 * Since there can be multiple commands within a single BES request
 * document, there can be multiple data handler interface objects
 * created. Use the one passed as the base interface handler object
 */
BESXMLCommand::BESXMLCommand(const BESDataHandlerInterface &base_dhi)
{
    d_xmlcmd_dhi.make_copy(base_dhi);
}

/** @brief The request has been parsed, use the command action name to
 * set the response handler
 */
void BESXMLCommand::set_response()
{
    d_xmlcmd_dhi.response_handler = BESResponseHandlerList::TheList()->find_handler(d_xmlcmd_dhi.action);
    if (!d_xmlcmd_dhi.response_handler) {
        throw BESSyntaxUserError(string("Command '") + d_xmlcmd_dhi.action
            + "' does not have a registered response handler", __FILE__, __LINE__);
    }

    d_xmlcmd_dhi.data[LOG_INFO] = d_cmd_log_info;

    VERBOSE(d_xmlcmd_dhi.data[REQUEST_FROM] + " [" + d_xmlcmd_dhi.data[LOG_INFO] + "] parsed" );
}

/** @brief Add a command to the possible commands allowed by this BES
 *
 * This adds a function to parse a specific BES command within the BES
 * request document using the given name. If a command element is found
 * with the name cmd_str, then the XMLCommand object is created using
 * the passed cmd object.
 *
 * @param cmd_str The name of the command
 * @param cmd The function to call to create the BESXMLCommand object
 */
void BESXMLCommand::add_command(const string &cmd_str, p_xmlcmd_builder cmd)
{
    BESXMLCommand::factory[cmd_str] = cmd;
}

/** @brief Deletes the command called cmd_str from the list of possible
 * commands
 *
 * @param cmd_str The name of the command to remove from the list
 */
void BESXMLCommand::del_command(const string &cmd_str)
{
    BESXMLCommand::cmd_iter iter = BESXMLCommand::factory.find(cmd_str);
    if (iter != BESXMLCommand::factory.end()) {
        BESXMLCommand::factory.erase(iter);
    }
}

/** @brief Find the BESXMLCommand creation function with the given name
 *
 * @param cmd_str The name of the command creation function to find
 */
p_xmlcmd_builder BESXMLCommand::find_command(const string &cmd_str)
{
    return BESXMLCommand::factory[cmd_str];
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESXMLCommand::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESXMLCommand::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESIndent::UnIndent();
}

