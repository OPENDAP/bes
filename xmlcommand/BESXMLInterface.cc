// BESXMLInterface.cc

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
#include <sstream>

using namespace std;

#include "BESXMLInterface.h"
#include "BESXMLCommand.h"
#include "BESXMLUtils.h"
#include "BESDataNames.h"

#include "BESResponseHandler.h"
#include "BESReturnManager.h"
#include "BESInfo.h"
#include "BESStopWatch.h"

#include "BESDebug.h"
#include "BESLog.h"
#include "BESSyntaxUserError.h"

#define LOG_ONLY_GET_COMMANDS

BESXMLInterface::BESXMLInterface(const string &xml_doc, ostream *strm) :
    BESInterface(strm), d_xml_document(xml_doc)
{
    // This is needed because we want the parent to have access to the information
    // added to the DHI
    d_dhi_ptr = &d_xml_interface_dhi;
}

BESXMLInterface::~BESXMLInterface()
{
    clean();
}

/** @brief Build the data request plan using the BESCmdParser.
 */
void BESXMLInterface::build_data_request_plan()
{
    BESDEBUG("bes", "Entering: " << __PRETTY_FUNCTION__ << endl);
    BESDEBUG("bes", "building request plan for xml document: " << endl << d_xml_document << endl);

    // I do not know why, but uncommenting this macro breaks some tests
    // on Linux but not OSX (CentOS 6, Ubuntu 12 versus OSX 10.11) by
    // causing some XML elements in DMR responses to be twiddled in the
    // responses build on Linux but not on OSX.
    //
    // LIBXML_TEST_VERSION

    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;
    xmlNode *current_node = NULL;

    try {
        // set the default error function to my own
        vector<string> parseerrors;
        xmlSetGenericErrorFunc((void *) &parseerrors, BESXMLUtils::XMLErrorFunc);

        // XML_PARSE_NONET
        doc = xmlReadMemory(d_xml_document.c_str(), d_xml_document.size(), "" /* base URL */,
        NULL /* encoding */, XML_PARSE_NONET /* xmlParserOption */);

        if (doc == NULL) {
            string err = "Problem parsing the request xml document:\n";
            bool isfirst = true;
            vector<string>::const_iterator i = parseerrors.begin();
            vector<string>::const_iterator e = parseerrors.end();
            for (; i != e; i++) {
                if (!isfirst && (*i).compare(0, 6, "Entity") == 0) {
                    err += "\n";
                }
                err += (*i);
                isfirst = false;
            }
            throw BESSyntaxUserError(err, __FILE__, __LINE__);
        }

        // get the root element and make sure it exists and is called request
        root_element = xmlDocGetRootElement(doc);
        if (!root_element) throw BESSyntaxUserError("There is no root element in the xml document", __FILE__, __LINE__);

        string root_name;
        string root_val;
        map<string, string> props;
        BESXMLUtils::GetNodeInfo(root_element, root_name, root_val, props);
        if (root_name != "request")
            throw BESSyntaxUserError(
                string("The root element should be a request element, name is ").append((char *) root_element->name),
                __FILE__, __LINE__);

        if (!root_val.empty())
            throw BESSyntaxUserError(string("The request element must not contain a value, ").append(root_val),
            __FILE__, __LINE__);

        // there should be a request id property with one value.
        string &reqId = props[REQUEST_ID];
        if (reqId.empty()) throw BESSyntaxUserError("The request id value empty", __FILE__, __LINE__);

        d_dhi_ptr->data[REQUEST_ID] = reqId;

        BESDEBUG("besxml", "request id = " << d_dhi_ptr->data[REQUEST_ID] << endl);

        // iterate through the children of the request element. Each child is an
        // individual command.
        bool has_response = false;  // set to true when a command with a response is found.
        current_node = root_element->children;

        while (current_node) {
            if (current_node->type == XML_ELEMENT_NODE) {
                // given the name of this node we should be able to find a
                // BESXMLCommand object
                string node_name = (char *) current_node->name;

                // The Command Builder scheme is a kind of factory, but which uses lists and
                // a static method defined by each child of BESXMLCommand (called CommandBuilder).
                // These static methods make new instances of the specific commands and, in so
                // doing, _copy_ the DataHandlerInterface instance using that class' clone() method.
                // jhrg 11/7/17
                p_xmlcmd_builder bldr = BESXMLCommand::find_command(node_name);
                if (!bldr)
                    throw BESSyntaxUserError(string("Unable to find command for ").append(node_name), __FILE__,
                        __LINE__);

                BESXMLCommand *current_cmd = bldr(d_xml_interface_dhi);
                if (!current_cmd)
                    throw BESInternalError(string("Failed to build command object for ").append(node_name), __FILE__,
                        __LINE__);

                // push this new command to the back of the list
                d_xml_cmd_list.push_back(current_cmd);

                // only one of the commands can build a response
                bool cmd_has_response = current_cmd->has_response();
                if (has_response && cmd_has_response)
                    throw BESSyntaxUserError("Commands with multiple responses not supported.", __FILE__, __LINE__);

                has_response = cmd_has_response;

                // parse the request given the current node
                current_cmd->parse_request(current_node);

                BESDataHandlerInterface &current_dhi = current_cmd->get_xmlcmd_dhi();

                // Check if the correct transmitter is present. We look for it again in do_transmit()
                // where it is actually used. This test just keeps us from building a response that
                // cannot be transmitted. jhrg 11/8/17
                //
                // TODO We could add the 'transmitter' to the DHI.
                string return_as = current_dhi.data[RETURN_CMD];
                if (!return_as.empty() & !BESReturnManager::TheManager()->find_transmitter(return_as))
                    throw BESSyntaxUserError(string("Unable to find transmitter ").append(return_as), __FILE__,
                        __LINE__);
            }

            current_node = current_node->next;
        }
    }
    catch (...) {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        throw;
    }

    xmlFreeDoc(doc);

    // Removed since the docs indicate it's not needed and it might be
    // contributing to memory issues flagged by valgrind. 2/25/09 jhrg
    //
    // Added this back in. It seems to the the cause of BES-40 - where
    // When certain tests are run, the order of <Dimension..> elements
    // in a DMR for a server function result is different when the BESDEBUG
    // output is on versus when it is not. This was true only when the
    // BESDEBUG context was 'besxml' or timing,' which lead me here.
    // Making this call removes the errant behavior. I've run tests using
    // valgrind and I see no memory problems from this call. jhrg 9/25/15
    xmlCleanupParser();

    BESDEBUG("bes", "Done building request plan" << endl);
}

/** @brief Execute the data request plan
 */
void BESXMLInterface::execute_data_request_plan()
{
    vector<BESXMLCommand *>::iterator i = d_xml_cmd_list.begin();
    vector<BESXMLCommand *>::iterator e = d_xml_cmd_list.end();
    for (; i != e; i++) {
        (*i)->prep_request();

        d_dhi_ptr = &(*i)->get_xmlcmd_dhi();

        // In 'verbose' logging mode, log all the commands.
        VERBOSE(d_dhi_ptr->data[REQUEST_FROM] << " [" << d_dhi_ptr->data[LOG_INFO] << "] executing" << endl);

        // This is the main log entry when the server is not in 'verbose' mode.
        // There are two ays we can do this, one writes a log line for only the
        // get commands, the other write the set container, define and get commands.
        // TODO Make this configurable? jhrg 11/14/17
#ifdef LOG_ONLY_GET_COMMANDS
        // Special logging action for the 'get' command. In non-verbose logging mode,
        // only log the get command.
        if (d_dhi_ptr->action.find("get.") != string::npos) {

            string new_log_info = d_dhi_ptr->action;
            if (!d_dhi_ptr->data[RETURN_CMD].empty())
                new_log_info.append(",").append(d_dhi_ptr->data[RETURN_CMD]);

            // Assume this is DAP and thus there is at most one container. Log a warning if that's
            // not true. jhrg 11/14/17
            BESContainer *c = *(d_dhi_ptr->containers.begin());
            if (c) {
                if (!c->access().empty()) new_log_info.append(",").append(c->access());

                if (!c->get_constraint().empty()) {
                    new_log_info.append(",").append(c->get_constraint());
                }
                else {
                    if (!c->get_dap4_constraint().empty()) new_log_info.append(",").append(c->get_dap4_constraint());
                    if (!c->get_dap4_function().empty()) new_log_info.append(",").append(c->get_dap4_function());
                }
            }

            LOG(new_log_info << endl);

            if (d_dhi_ptr->containers.size() > 1)
                LOG("Warning: The previous command had multiple containers defined, but only the was logged.");
        }
#else
        if (!BESLog::TheLog()->is_verbose()) {
            if (d_dhi_ptr->action.find("set.context") == string::npos
                && d_dhi_ptr->action.find("show.catalog") == string::npos) {
                LOG(d_dhi_ptr->data[LOG_INFO] << endl);
            }
        }
#endif

        if (!d_dhi_ptr->response_handler)
            throw BESInternalError(string("The response handler '") + d_dhi_ptr->action + "' does not exist", __FILE__,
            __LINE__);

        d_dhi_ptr->response_handler->execute(*d_dhi_ptr);

        transmit_data();    // TODO move method body in here? jhrg 11/8/17
    }
}

/**
 * @brief Transmit the response object
 * @todo Remove?
 *
 * Only transmit if there is an error or if there is a ResponseHandler. For any
 * XML document with one or more commands, there should only be one ResponseHandler.
 */
void BESXMLInterface::transmit_data()
{
    if (d_dhi_ptr->error_info) {
        ostringstream strm;
        d_dhi_ptr->error_info->print(strm);
        LOG("Transmitting error: " << strm.str() << endl);

        d_dhi_ptr->error_info->transmit(d_transmitter, *d_dhi_ptr);
    }
    else if (d_dhi_ptr->response_handler) {
        VERBOSE(
            /*d_dhi_ptr->data[SERVER_PID] << " from " <<*/d_dhi_ptr->data[REQUEST_FROM] << " [" << d_dhi_ptr->data[LOG_INFO] << "] transmitting" << endl);

        BESStopWatch sw;
        if (BESISDEBUG(TIMING_LOG)) sw.start(d_dhi_ptr->data[LOG_INFO] + " transmitting", d_dhi_ptr->data[REQUEST_ID]);

        string return_as = d_dhi_ptr->data[RETURN_CMD];
        if (!return_as.empty()) {
            d_transmitter = BESReturnManager::TheManager()->find_transmitter(return_as);
            if (!d_transmitter) {
                throw BESSyntaxUserError(string("Unable to find transmitter ") + return_as, __FILE__, __LINE__);
            }
        }

        d_dhi_ptr->response_handler->transmit(d_transmitter, *d_dhi_ptr);
    }
}

/**
 * @brief Log the status of the request to the BESLog file
 *
 * This will only log information in the verbose mode.
 *
 * @see BESLog
 */
void BESXMLInterface::log_status()
{
    if (BESLog::TheLog()->is_verbose()) {
        vector<BESXMLCommand *>::iterator i = d_xml_cmd_list.begin();
        vector<BESXMLCommand *>::iterator e = d_xml_cmd_list.end();
        for (; i != e; i++) {
            d_dhi_ptr = &(*i)->get_xmlcmd_dhi();

            // IF the DHI's error_info object pointer is null, the request was successful.
            string result = (!d_dhi_ptr->error_info) ? "completed" : "failed";

            // This is only printed for verbose logging.
            LOG(
                /*d_dhi_ptr->data[SERVER_PID] << " from " <<*/d_dhi_ptr->data[REQUEST_FROM] << " [" << d_dhi_ptr->data[LOG_INFO] << "] " << result << endl);
        }
    }
}

/** @brief Clean up after the request is completed
 */
void BESXMLInterface::clean()
{
    vector<BESXMLCommand *>::iterator i = d_xml_cmd_list.begin();
    vector<BESXMLCommand *>::iterator e = d_xml_cmd_list.end();
    for (; i != e; i++) {
        BESXMLCommand *cmd = *i;
        d_dhi_ptr = &cmd->get_xmlcmd_dhi();

        if (d_dhi_ptr) {
            VERBOSE(
                /*d_dhi_ptr->data[SERVER_PID] << " from " <<*/d_dhi_ptr->data[REQUEST_FROM] << " [" << d_dhi_ptr->data[LOG_INFO] << "] cleaning" << endl);

            d_dhi_ptr->clean(); // Delete the ResponseHandler if present
        }

        delete cmd;
    }

    d_xml_cmd_list.clear();
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESXMLInterface::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESXMLInterface::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESInterface::dump(strm);
    vector<BESXMLCommand *>::const_iterator i = d_xml_cmd_list.begin();
    vector<BESXMLCommand *>::const_iterator e = d_xml_cmd_list.end();
    for (; i != e; i++) {
        BESXMLCommand *cmd = *i;
        cmd->dump(strm);
    }
    BESIndent::UnIndent();
}

