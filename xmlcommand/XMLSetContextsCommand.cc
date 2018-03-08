// XMLSetContextsCommand.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2018 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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

#include "XMLSetContextsCommand.h"

#include "BESXMLUtils.h"
#include "BESUtil.h"
#include "BESDataNames.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"

#include "SetContextsNames.h"

#if !USE_CONTEXTS_RESPONSE_HANDLER
#include "BESContextManager.h"
#endif

using namespace bes;
using namespace std;

/** @brief parse a setContexts command.
 *
 * The form of this command is:
 * ~~~{.xml}
 * <setContexts>
 *     <context name="n">value</context>
 *     ...
 * </setContexts>
 * ~~~
 *
 * This command can be implemented in two ways: it can parse the XML and
 * add to the BES Context here or it can parse XML, load the command's
 * DHI with the values and have the associated ResponseHandler::execute()
 * method extract those values and add them the BES's Context Manager.
 *
 * I'll implement both versions and allow a compile time switch to choose.
 *
 * @param node xml2 element node pointer
 */
void XMLSetContextsCommand::parse_request(xmlNode *node)
{
    string value, name, action;
    map<string, string> props;
    BESXMLUtils::GetNodeInfo(node, action, value, props);
    if (action != SET_CONTEXTS_STR) {
        string err = "The specified command " + action + " is not a set context command";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    xmlNode *current_node = node->children;
    while (current_node) {
        if (current_node->type == XML_ELEMENT_NODE) {

            string name, value;
            map<string, string> attributes;
            BESXMLUtils::GetNodeInfo(current_node, name, value, attributes);

            if (name != "context")
                throw BESSyntaxUserError(string("Inside setContexts, expected a 'context' but got '"
                    + name +"'."), __FILE__, __LINE__);

            if (value.empty())
                throw BESSyntaxUserError(string("The 'context' element must contain a value"), __FILE__,
                    __LINE__);

            if (attributes.size() != 1 || attributes["name"].empty())
                throw BESSyntaxUserError(string("The 'context' element must contain a 'name' attribute."),
                    __FILE__, __LINE__);

            // Set the context _or_ push the information into the DHI data[] map
#if USE_CONTEXTS_RESPONSE_HANDLER
            // To record the information in the DHI data[] map, use one value to hold a
            // list of all of Context names, then use the names to hold the values. Read
            // this using names = data[CONTEXT_NAMES] and then:
            //
            // for each 'name' in names { value = data['context_'name] }
            //
            string context_key = string(CONTEXT_PREFIX).append(attributes["name"]);
            d_xmlcmd_dhi.data[CONTEXT_NAMES] =  d_xmlcmd_dhi.data[CONTEXT_NAMES].append(" ").append(context_key);
            d_xmlcmd_dhi.data[context_key] = value;

            BESDEBUG("besxml", "d_xmlcmd_dhi.data[" << context_key << "] = " << value << endl);
#else
            BESDEBUG("besxml", "In " << __func__ << " BESContextManager::TheManager()->set_context("
                << name << ", " << value << ")" << endl);

            BESContextManager::TheManager()->set_context(attributes["name"], value);
#endif
        }

        current_node = current_node->next;
    }

    d_cmd_log_info = string("set contexts for ").append(d_xmlcmd_dhi.data[CONTEXT_NAMES]);


#if USE_CONTEXTS_RESPONSE_HANDLER
    // The value set to DHI's action field is used to choose the matching ResponseHandler.
    d_xmlcmd_dhi.action = SET_CONTEXTS_ACTION;
#else
    // Set action_name here because the NULLResponseHandler (aka NULL_ACTION) won't know
    // which command used it (current ResponseHandlers set this because there is a 1-to-1
    // correlation between XMLCommands and ResponseHanlders). jhrg 2/8/18
    d_xmlcmd_dhi.action_name = SET_CONTEXTS_STR;
    d_xmlcmd_dhi.action = NULL_ACTION;
#endif

    // Set the response handler for the action in the command's DHI using the value of
    // the DHI.action field. And, as a bonus, copy the d_cmd_log_info into DHI.data[LOG_INFO]
    // and if VERBOSE logging is on, log that the command has been parsed.
   BESXMLCommand::set_response();
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void XMLSetContextsCommand::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "XMLSetContextsCommand::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESXMLCommand::dump(strm);
    BESIndent::UnIndent();
}

BESXMLCommand *
XMLSetContextsCommand::CommandBuilder(const BESDataHandlerInterface &base_dhi)
{
    return new XMLSetContextsCommand(base_dhi);
}

