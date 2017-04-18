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

#include <iostream>

#include "BESXMLCommand.h"
#include "BESResponseHandlerList.h"
#include "BESSyntaxUserError.h"
#include "BESDataNames.h"
#include "BESLog.h"

using std::flush;

map<string, p_xmlcmd_builder> BESXMLCommand::cmd_list;

/** @brief Creates a BESXMLCommand document given a base data handler
 * interface object
 *
 * Since there can be multiple commands within a single BES request
 * document, there can be multiple data handler interface objects
 * created. Use the one passed as the base interface handler object
 */
BESXMLCommand::BESXMLCommand(const BESDataHandlerInterface &base_dhi)
{
    _dhi.make_copy(base_dhi);
}

/** @brief The request has been parsed, use the command action name to
 * set the response handler
 */
void BESXMLCommand::set_response()
{
    _dhi.response_handler = BESResponseHandlerList::TheList()->find_handler(_dhi.action);
    if (!_dhi.response_handler) {
        throw BESSyntaxUserError(string("Command '") + _dhi.action + "' does not have a registered response handler",
            __FILE__, __LINE__);
    }

    // The _str_cmd is a text version of the xml command used for the log.
    // It is not used for anything else. I think the 'sql like' syntax is
    // actually no longer used by the BES and that the software in cmdln
    // translates that syntax into XML. But I'm not 100% sure... jhrg 12/29/15
    _dhi.data[DATA_REQUEST] = _str_cmd;

    BESLog::TheLog()->flush_me();
    string m = BESLog::mark;
    *(BESLog::TheLog()) << _dhi.data[REQUEST_FROM] << m
        << _str_cmd << m <<
        "request received" << m << endl;
    BESLog::TheLog()->flush_me();
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
    BESXMLCommand::cmd_list[cmd_str] = cmd;
}

/** @brief Deletes the command called cmd_str from the list of possible
 * commands
 *
 * @param cmd_str The name of the command to remove from the list
 */
bool BESXMLCommand::del_command(const string &cmd_str)
{
    bool ret = false;

    BESXMLCommand::cmd_iter iter = BESXMLCommand::cmd_list.find(cmd_str);
    if (iter != BESXMLCommand::cmd_list.end()) {
        BESXMLCommand::cmd_list.erase(iter);
    }
    return ret;
}

/** @brief Find the BESXMLCommand creation function with the given name
 *
 * @param cmd_str The name of the command creation function to find
 */
p_xmlcmd_builder BESXMLCommand::find_command(const string &cmd_str)
{
    return BESXMLCommand::cmd_list[cmd_str];
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

