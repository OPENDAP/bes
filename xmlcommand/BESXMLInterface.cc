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

#include <iostream>
#include <sstream>

using std::endl;
using std::cout;
using std::stringstream;

#include "BESXMLInterface.h"
#include "BESXMLCommand.h"
#include "BESXMLUtils.h"
#include "BESDataNames.h"
#include "BESDebug.h"
#include "BESLog.h"
#include "BESSyntaxUserError.h"
#include "BESReturnManager.h"
#include "BESAggFactory.h"
#include "BESAggregationServer.h"
#include "BESTransmitterNames.h"


BESXMLInterface::BESXMLInterface(const string &xml_doc, ostream *strm) :
    BESInterface(strm)
{
    _dhi = &_base_dhi;
    _dhi->data[DATA_REQUEST] = "xml document";
    // NB: The xml_doc is only used one place in the BES and that's on
    // line 108  in this file. We could make this a field of the object and
    // cut down on the use of the map. The downside is that putting the
    // xml in the map makes it accessible when we look at the DHI. jhrg 2/23/16
    _dhi->data["XMLDoc"] = xml_doc;
}

BESXMLInterface::~BESXMLInterface()
{
    clean();
}

int BESXMLInterface::execute_request(const string &from)
{
#if 0
    return BESBasicInterface::execute_request(from);
#endif
    return BESInterface::execute_request(from);
}

/** @brief Initialize the BES
 */
void BESXMLInterface::initialize()
{
#if 0
    // BESBasicInterface::initialize();
#endif

    // dhi has not been filled in at this point, so let's set a default
    // transmitter given the protocol. The transmitter might change after
    // parsing a request and given a return manager to use. This is done in
    // build_data_plan.
    //
    // The reason I moved this from the build_data_plan method is because a
    // registered initialization routine might throw an exception and we
    // will need to transmit the exception info, which needs a transmitter.
    // If an exception happens before this then the exception info is just
    // printed to cout (see BESInterface::transmit_data()). -- pcw 09/05/06
    BESDEBUG("bes", "Finding " << BASIC_TRANSMITTER << " transmitter ... " << endl);

    _transmitter = BESReturnManager::TheManager()->find_transmitter( BASIC_TRANSMITTER);
    if (!_transmitter) {
        string s = (string) "Unable to find transmitter " + BASIC_TRANSMITTER;
        throw BESInternalError(s, __FILE__, __LINE__);
    }
    BESDEBUG("bes", "OK" << endl);

    BESInterface::initialize();

}

/** @brief Validate the incoming request information
 */
void BESXMLInterface::validate_data_request()
{
#if 0
    BESBasicInterface::validate_data_request();
#endif
    BESInterface::validate_data_request();
}

/** @brief Build the data request plan using the BESCmdParser.
 */
void BESXMLInterface::build_data_request_plan()
{
    BESDEBUG("bes", "Entering: " <<  __PRETTY_FUNCTION__ << endl);
    BESDEBUG("besxml", "building request plan for xml document: " << endl << _dhi->data["XMLDoc"] << endl);

    if (BESLog::TheLog()->is_verbose()) {
        *(BESLog::TheLog()) << _dhi->data[SERVER_PID] << " from " << _dhi->data[REQUEST_FROM] << "] building" << endl;
    }

    // I do not know why, but uncommenting this macro breaks some tests
    // on linux but not OSX (CentOS 6, Ubuntu 12 versus OSX 10.11) by
    // causing some XML elements in DMR responses to be twiddled in the 
    // responses build on linux but not on OSX.
    // LIBXML_TEST_VERSION

    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;
    xmlNode *current_node = NULL;

    try {
        // set the default error function to my own
        vector<string> parseerrors;
        xmlSetGenericErrorFunc((void *) &parseerrors, BESXMLUtils::XMLErrorFunc);

        // XML_PARSE_NONET
        doc = xmlReadMemory(_dhi->data["XMLDoc"].c_str(), _dhi->data["XMLDoc"].size(), "" /* base URL */,
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
        if (!root_element) {
            string err = "There is no root element in the xml document";
            throw BESSyntaxUserError(err, __FILE__, __LINE__);
        }

        string root_name;
        string root_val;
        map<string, string> props;
        BESXMLUtils::GetNodeInfo(root_element, root_name, root_val, props);
        if (root_name != "request") {
            string err = (string) "The root element should be a request element, " + "name is "
                + (char *) root_element->name;
            throw BESSyntaxUserError(err, __FILE__, __LINE__);
        }
        if (root_val != "") {
            string err = (string) "The request element must not contain a value, " + root_val;
            throw BESSyntaxUserError(err, __FILE__, __LINE__);
        }

        // there should be a request id property with one value.
        string &reqId = props[REQUEST_ID];
        if (reqId.empty()) {
            string err = (string) "request id value empty";
            throw BESSyntaxUserError(err, __FILE__, __LINE__);
        }
        _dhi->data[REQUEST_ID] = reqId;

        BESDEBUG("besxml", "request id = " << _dhi->data[REQUEST_ID] << endl);

        // iterate through the children of the request element. Each child is an
        // individual command.
        bool has_response = false;
        current_node = root_element->children;

        while (current_node) {
            if (current_node->type == XML_ELEMENT_NODE) {
                // given the name of this node we should be able to find a
                // BESXMLCommand object
                string node_name = (char *) current_node->name;

                p_xmlcmd_builder bldr = BESXMLCommand::find_command(node_name);
                if (bldr) {
                    BESXMLCommand *current_cmd = bldr(_base_dhi);
                    if (!current_cmd) {
                        string err = (string) "Failed to build command object for " + node_name;
                        throw BESInternalError(err, __FILE__, __LINE__);
                    }

                    // push this new command to the back of the list
                    _cmd_list.push_back(current_cmd);

                    // only one of the commands can build a response. If more
                    // than one builds a response, throw an error
                    bool cmd_has_response = current_cmd->has_response();
                    if (has_response && cmd_has_response) {
                        string err = "Multiple responses not allowed";
                        throw BESSyntaxUserError(err, __FILE__, __LINE__);
                    }
                    has_response = cmd_has_response;

                    // parse the request given the current node
                    BESDEBUG("besxml", "parse request using " << node_name << endl);
                    current_cmd->parse_request(current_node);

                    BESDataHandlerInterface &current_dhi = current_cmd->get_dhi();

                    BESDEBUG("besxml", node_name << " parsed request, dhi = " << current_dhi << endl);

                    string returnAs = current_dhi.data[RETURN_CMD];
                    if (returnAs != "") {
                        BESDEBUG("xml", "Finding transmitter: " << returnAs << " ...  " << endl);
                        BESTransmitter *transmitter = BESReturnManager::TheManager()->find_transmitter(returnAs);
                        if (!transmitter) {
                            string s = (string) "Unable to find transmitter " + returnAs;
                            throw BESSyntaxUserError(s, __FILE__, __LINE__);
                        }
                        BESDEBUG("xml", "OK" << endl);
                    }
                }
                else {
                    string err = (string) "Unable to find command for " + node_name;
                    throw BESSyntaxUserError(err, __FILE__, __LINE__);
                }
            }
            current_node = current_node->next;
        }
    }
    catch (... /* BESError &e; changed 7/1/15 jhrg */) {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        throw /*e*/;
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

    BESDEBUG("besxml", "Done building request plan" << endl);

#if 0
    BESBasicInterface::build_data_request_plan();
#endif

    // The default _transmitter (either basic or http depending on the
    // protocol passed) has been set in initialize. If the parsed command
    // sets a RETURN_CMD (a different transmitter) then look it up here. If
    // it's set but not found then this is an error. If it's not set then
    // just use the defaults.

    if (_dhi->data[RETURN_CMD] != "") {
        BESDEBUG("bes", "Finding transmitter: " << _dhi->data[RETURN_CMD] << " ...  " << endl);

        _transmitter = BESReturnManager::TheManager()->find_transmitter(_dhi->data[RETURN_CMD]);
        if (!_transmitter) {
            string s = (string) "Unable to find transmitter " + _dhi->data[RETURN_CMD];
            throw BESSyntaxUserError(s, __FILE__, __LINE__);
        }
    }
}

/** @brief Execute the data request plan
 */
void BESXMLInterface::execute_data_request_plan()
{
    vector<BESXMLCommand *>::iterator i = _cmd_list.begin();
    vector<BESXMLCommand *>::iterator e = _cmd_list.end();
    for (; i != e; i++) {
        (*i)->prep_request();
        _dhi = &(*i)->get_dhi();

#if 0
        BESBasicInterface::execute_data_request_plan();
#endif

        if (BESLog::TheLog()->is_verbose()) {
            *(BESLog::TheLog()) << _dhi->data[SERVER_PID] << " from " << _dhi->data[REQUEST_FROM] << " ["
                << _dhi->data[DATA_REQUEST] << "] executing" << endl;
        }
        BESInterface::execute_data_request_plan();
    }
}

/** @brief Invoke the aggregation server, if there is one
 */
void BESXMLInterface::invoke_aggregation()
{
#if 0
    BESBasicInterface::invoke_aggregation();
#endif

    if (_dhi->data[AGG_CMD] == "") {
        if (BESLog::TheLog()->is_verbose()) {
            *(BESLog::TheLog()) << _dhi->data[SERVER_PID] << " from " << _dhi->data[REQUEST_FROM] << " ["
                << _dhi->data[DATA_REQUEST] << "]" << " not aggregating, command empty" << endl;
        }
    }
    else {
        BESAggregationServer *agg = BESAggFactory::TheFactory()->find_handler(_dhi->data[AGG_HANDLER]);
        if (!agg) {
            if (BESLog::TheLog()->is_verbose()) {
                *(BESLog::TheLog()) << _dhi->data[SERVER_PID] << " from " << _dhi->data[REQUEST_FROM] << " ["
                    << _dhi->data[DATA_REQUEST] << "]" << " not aggregating, no handler" << endl;
            }
        }
        else {
            if (BESLog::TheLog()->is_verbose()) {
                *(BESLog::TheLog()) << _dhi->data[SERVER_PID] << " from " << _dhi->data[REQUEST_FROM] << " ["
                    << _dhi->data[DATA_REQUEST] << "] aggregating" << endl;
            }
        }
    }
    BESInterface::invoke_aggregation();
}

/** @brief Transmit the response object
 */
void BESXMLInterface::transmit_data()
{
    string returnAs = _dhi->data[RETURN_CMD];
    if (returnAs != "") {
        BESDEBUG("xml", "Setting transmitter: " << returnAs << " ...  " << endl);
        _transmitter = BESReturnManager::TheManager()->find_transmitter(returnAs);
        if (!_transmitter) {
            string s = (string) "Unable to find transmitter " + returnAs;
            throw BESSyntaxUserError(s, __FILE__, __LINE__);
        }
        BESDEBUG("xml", "OK" << endl);
    }

#if 0
    BESBasicInterface::transmit_data();
#endif

    if (BESLog::TheLog()->is_verbose()) {
        *(BESLog::TheLog()) << _dhi->data[SERVER_PID] << " from " << _dhi->data[REQUEST_FROM] << " ["
            << _dhi->data[DATA_REQUEST] << "] transmitting" << endl;
    }
    BESInterface::transmit_data();
}

/** @brief Log the status of the request to the BESLog file

 @see BESLog
 */
void BESXMLInterface::log_status()
{
    vector<BESXMLCommand *>::iterator i = _cmd_list.begin();
    vector<BESXMLCommand *>::iterator e = _cmd_list.end();
    for (; i != e; i++) {
        _dhi = &(*i)->get_dhi();

#if 0
        BESBasicInterface::log_status();
#endif

       // Following code is from BESBasicInterface::log_status.
        string result = "completed";
        if (_dhi->error_info) result = "failed";
        if (BESLog::TheLog()->is_verbose()) {
            *(BESLog::TheLog()) << _dhi->data[SERVER_PID] << " from " << _dhi->data[REQUEST_FROM] << " ["
                << _dhi->data[DATA_REQUEST] << "] " << result << endl;
        }
    }
}

/** @brief Report the request and status of the request

 If interested in reporting the request and status of the request then
 one must register a BESReporter with BESReporterList::TheList().

 If no BESReporter objects are registered then nothing happens.

 Since there are multiple commands in an XML request document we
 iterate through the commands and execute report_request for each of
 the commands, giving the reporter a chance to report on each of the
 commands.

 @see BESReporterList
 @see BESReporter
 */
void BESXMLInterface::report_request()
{
    vector<BESXMLCommand *>::iterator i = _cmd_list.begin();
    vector<BESXMLCommand *>::iterator e = _cmd_list.end();
    for (; i != e; i++) {
        _dhi = &(*i)->get_dhi();

#if 0
        BESBasicInterface::report_request();
#endif

        BESInterface::report_request();
    }
}

/** @brief Clean up after the request is completed
 */
void BESXMLInterface::clean()
{
    vector<BESXMLCommand *>::iterator i = _cmd_list.begin();
    vector<BESXMLCommand *>::iterator e = _cmd_list.end();
    for (; i != e; i++) {
        BESXMLCommand *cmd = *i;
        _dhi = &cmd->get_dhi();

#if 0
        BESBasicInterface::clean();
#endif

        BESInterface::clean();
        if (BESLog::TheLog()->is_verbose()) {
            *(BESLog::TheLog()) << _dhi->data[SERVER_PID] << " from " << _dhi->data[REQUEST_FROM] << " ["
                << _dhi->data[DATA_REQUEST] << "] cleaning" << endl;
        }
        delete cmd;
    }
    _cmd_list.clear();
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

#if 0
    BESBasicInterface::dump();
#endif

    BESInterface::dump(strm);
    vector<BESXMLCommand *>::const_iterator i = _cmd_list.begin();
    vector<BESXMLCommand *>::const_iterator e = _cmd_list.end();
    for (; i != e; i++) {
        BESXMLCommand *cmd = *i;
        cmd->dump(strm);
    }
    BESIndent::UnIndent();
}

