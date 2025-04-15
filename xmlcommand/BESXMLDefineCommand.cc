// BESXMLDefineCommand.cc

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

#include "BESXMLDefineCommand.h"
#include "BESContainerStorageList.h"
#include "BESContainerStorage.h"

#include "BESXMLUtils.h"
#include "BESUtil.h"
#include "BESResponseNames.h"
#include "BESDataNames.h"

#include "BESSyntaxUserError.h"
#include "BESInternalFatalError.h"
#include "BESDebug.h"

using std::endl;
using std::string;
using std::vector;
using std::ostream;
using std::map;

#ifndef DEFAULT
#define DEFAULT "default"
#endif

/** @brief parse a define command.
 *
 * ~~~{.xml}
 * <define name"d" space="default">
 *
 *     <!-- required -->
 *     <container name="c"/>
 *
 *     <!-- Optional -->
 *     <container name="c">
 *         <constraint> A valid DAP2 CE </constraint>
 *
 *         <dap4constraint> ... </dap4constraint>
 *         <dap4function> ... </dap4function>
 *
 *         <attributes> ... </attributes>
 *    </container>
 *
 * </define>
 * ~~~
 *
 * Requires the name property. The space property is optional. Requires at
 * least one container element. The container element requires the name
 * property. The constraint and attribute elements of container are
 * optional.
 *
 * @param node xml2 element node pointer
 */
void BESXMLDefineCommand::parse_request(xmlNode *node) {
    string value; // element value, should not be any
    string action; // element name, which is the request action
    map<string, string> props; // element attributes; 'name' and maybe 'space'

    BESXMLUtils::GetNodeInfo(node, action, value, props);
    if (action != DEFINE_RESPONSE_STR) {
        string err = "The specified command " + action + " is not a set context command";
        throw BESInternalFatalError(err, __FILE__, __LINE__);
    }

    d_xmlcmd_dhi.action = DEFINE_RESPONSE;

    string def_name = props["name"];
    if (def_name.empty())
        throw BESSyntaxUserError(string(action) + " command: definition name missing", __FILE__, __LINE__);

    d_xmlcmd_dhi.data[DEF_NAME] = def_name;
    d_cmd_log_info = (string) "define " + def_name;

    d_xmlcmd_dhi.data[STORE_NAME] = props["space"].empty() ? DEFAULT : props["space"];
    d_cmd_log_info += " in " + d_xmlcmd_dhi.data[STORE_NAME];

    int num_containers = 0;
    string child_name;
    string child_value;
    props.clear();
    xmlNode *child_node = BESXMLUtils::GetFirstChild(node, child_name, child_value, props);
    while (child_node) {
        if (child_name == "constraint") {
            // default constraint for all containers
            _default_constraint = child_value;
        }
        else if (child_name == "dap4constraint") {
            // default function for all containers
            _default_dap4_constraint = child_value;
        }
        else if (child_name == "dap4function") {
            // default function for all containers
            _default_dap4_function = child_value;
        }
        else if (child_name == "container") {
            handle_container_element(action, child_node, child_value, props);
            num_containers++;
        }
        else {
            throw BESSyntaxUserError(string(action) + " Unrecognized child element: " + child_name, __FILE__, __LINE__);
        }
#if 0
        else if (child_name == "aggregate") {
            handle_aggregate_element(action, child_node, child_value, props);
        }
#endif

        // get the next child element
        props.clear();
        child_name.clear();
        child_value.clear();
        child_node = BESXMLUtils::GetNextChild(child_node, child_name, child_value, props);
    }

    if (num_containers < 1)
        throw BESSyntaxUserError(string(action) + " The define element must contain at least one container element",
                                 __FILE__, __LINE__);

    d_cmd_log_info += " as ";
    bool first = true;
    vector<string>::iterator i = container_names.begin();
    vector<string>::iterator e = container_names.end();
    for (; i != e; i++) {
        if (!first) d_cmd_log_info += ",";
        d_cmd_log_info += (*i);
        first = false;
    }

    if (container_constraints.size() || container_dap4constraints.size() || container_dap4functions.size() ||
        container_attributes.size()) {
        d_cmd_log_info += " with ";
        first = true;
        i = container_names.begin();
        e = container_names.end();
        for (; i != e; i++) {
            if (container_constraints.count((*i))) {
                if (!first) d_cmd_log_info += ",";
                first = false;
                d_cmd_log_info += (*i) + ".constraint=\"" + container_constraints[(*i)] + "\"";
            }
            if (container_dap4constraints.count((*i))) {
                if (!first) d_cmd_log_info += ",";
                first = false;
                d_cmd_log_info += (*i) + ".dap4constraint=\"" + container_dap4constraints[(*i)] + "\"";
            }
            if (container_dap4functions.count((*i))) {
                if (!first) d_cmd_log_info += ",";
                first = false;
                d_cmd_log_info += (*i) + ".dap4function=\"" + container_dap4functions[(*i)] + "\"";
            }
            if (container_attributes.count((*i))) {
                if (!first) d_cmd_log_info += ",";
                first = false;
                d_cmd_log_info += (*i) + ".attributes=\"" + container_attributes[(*i)] + "\"";
            }
        }
    }

    d_cmd_log_info += ";";

    // now that we've set the action, go get the response handler for the action
    BESXMLCommand::set_response();
}

/** @brief handle a container element of the define element
 *
 * There are two possible cases: a <constraint> element with no child nodes
 * or one with child nodes.
 * ~~~{.xml}
 * <container name="c"/>
 *
 * <container name="c">
 *     <constraint> A valid DAP2 CE </constraint>
 *
 *     <dap4constraint> ... </dap4constraint>
 *     <dap4function> ... </dap4function>
 *
 *     <attributes> ... </attributes>
 * </container>
 * ~~~
 * The name is required. constraint and attribute sub elements are optional
 *
 * @param action we are working on
 * @param node xml node element for the container
 * @param value a value of the container element, should be empty
 * @param props properties of the container element
 */
void BESXMLDefineCommand::handle_container_element(const string &action, xmlNode *node, const string &/*value*/,
                                                   map<string, string> &props) {
    string name = props["name"];
    if (name.empty()) {
        string err = action + " command: container element missing name prop";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    container_names.push_back(name);

    container_store_names[name] = props["space"];

    bool have_constraint = false;
    bool have_dap4constraint = false;
    bool have_dap4function = false;
    bool have_attributes = false;
    string child_name;
    string child_value;
    string constraint;
    string attributes;
    map<string, string> child_props;
    xmlNode *child_node = BESXMLUtils::GetFirstChild(node, child_name, child_value, child_props);
    while (child_node) {
        if (child_name == "constraint") {
            if (child_props.size()) {
                string err = action + " command: constraint element " + "should not contain properties";
                throw BESSyntaxUserError(err, __FILE__, __LINE__);
            }
            // HYRAX-316, the empty constraint now is legal. It is used to transfer the whole file.
#if 0
            if (child_value.empty()) {
                string err = action + " command: constraint element " + "missing value";
                throw BESSyntaxUserError(err, __FILE__, __LINE__);
            }
#endif
            if (have_constraint) {
                string err = action + " command: container element " + "contains multiple constraint elements";
                throw BESSyntaxUserError(err, __FILE__, __LINE__);
            }
            have_constraint = true;
            container_constraints[name] = child_value;
        }
        else if (child_name == "dap4constraint") {
            if (child_props.size()) {
                string err = action + " command: constraint element " + "should not contain properties";
                throw BESSyntaxUserError(err, __FILE__, __LINE__);
            }
            // HYRAX-316, the empty constraint now is legal. It is used to transfer the whole file.
#if 0
            if (child_value.empty()) {
                string err = action + " command: constraint element " + "missing value";
                throw BESSyntaxUserError(err, __FILE__, __LINE__);
            }
#endif
            if (have_dap4constraint) {
                string err = action + " command: container element " + "contains multiple constraint elements";
                throw BESSyntaxUserError(err, __FILE__, __LINE__);
            }
            have_dap4constraint = true;
            container_dap4constraints[name] = child_value;
        }
        else if (child_name == "dap4function") {
            if (child_props.size()) {
                string err = action + " command: dap4_function element " + "should not contain properties";
                throw BESSyntaxUserError(err, __FILE__, __LINE__);
            }
            if (child_value.empty()) {
                string err = action + " command: dap4_function element " + "missing value";
                throw BESSyntaxUserError(err, __FILE__, __LINE__);
            }
            if (have_dap4function) {
                string err = action + " command: container element " + "contains multiple dap4_function elements";
                throw BESSyntaxUserError(err, __FILE__, __LINE__);
            }
            have_dap4function = true;
            container_dap4functions[name] = child_value;
        }
        else if (child_name == "attributes") {
            if (child_props.size()) {
                string err = action + " command: attributes element " + "should not contain properties";
                throw BESSyntaxUserError(err, __FILE__, __LINE__);
            }
            if (child_value.empty()) {
                string err = action + " command: attributes element " + "missing value";
                throw BESSyntaxUserError(err, __FILE__, __LINE__);
            }
            if (have_attributes) {
                string err = action + " command: container element " + "contains multiple attributes elements";
                throw BESSyntaxUserError(err, __FILE__, __LINE__);
            }
            have_attributes = true;
            container_attributes[name] = child_value;
        }

        // get the next child element
        props.clear();
        child_name.clear();
        child_value.clear();
        child_node = BESXMLUtils::GetNextChild(child_node, child_name, child_value, props);
    }
}

#if 0
/** @brief handle an aggregate element of the define element
 *
 &lt;aggregate handler="someHandler" cmd="someCommand" /&gt;
 *
 * The handler and cmd properties are required
 *
 * @todo I removed support for aggregations as the BES has defined them. jhrg 2/11/18
 *
 * @param action we are working on
 * @param node xml node element for the container
 * @param value a value of the container element, should be empty
 * @param props properties of the container element
 */
void BESXMLDefineCommand::handle_aggregate_element(const string &action, xmlNode */*node*/, const string &/*value*/,
    map<string, string> &props)
{
    string handler = props["handler"];
    string cmd = props["cmd"];
    if (handler.empty()) {
        string err = action + " command: must specify aggregation handler";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }
    if (cmd.empty()) {
        string err = action + " command: must specify aggregation cmd";
        throw BESSyntaxUserError(err, __FILE__, __LINE__);
    }

    d_xmlcmd_dhi.data[AGG_HANDLER] = handler;
    d_xmlcmd_dhi.data[AGG_CMD] = cmd;
    d_cmd_log_info += " aggregate using " + handler + " by " + cmd;
}
#endif


/** @brief prepare the define command by making sure the containers exist
 *
 * @todo This could just as easily be done at the end of the parse_request()
 * method. _Unless_ we want to support the behavior that <define> could come
 * before <setContainer> in the request document. That is, this method is
 * called by XMLInterface::execute_data_request_plan(), after all the elements
 * have been parsed. jhrg 2/11/18
 */
void BESXMLDefineCommand::prep_request() {
    vector<string>::iterator i = container_names.begin();
    vector<string>::iterator e = container_names.end();
    for (; i != e; i++) {
        // look for the specified container
        BESContainer *c = 0;

        // Is a particular store is being used - this is container store
        // not the definition store. If no store is named, search them all.
        string store = container_store_names[(*i)];
        if (!store.empty()) {
            BESContainerStorage *cs = BESContainerStorageList::TheList()->find_persistence(store);
            if (cs) c = cs->look_for((*i));
        }
        else {
            c = BESContainerStorageList::TheList()->look_for((*i));
        }

        if (c == 0)
            throw BESSyntaxUserError(string("Could not find the container ") + (*i), __FILE__, __LINE__);

        // What use case do we have in which the "default" value of the constraint is not an empty string?
        string constraint = container_constraints[(*i)];
        if (constraint.empty()) constraint = _default_constraint;
        c->set_constraint(constraint);

        // What use case do we have in which the "default" value of the dap4constraint is not an empty string?
        string dap4constraint = container_dap4constraints[(*i)];
        if (dap4constraint.empty()) dap4constraint = _default_dap4_constraint;
        c->set_dap4_constraint(dap4constraint);

        // What use case do we have in which the "default" value of the dap4function is not an empty string?
        string function = container_dap4functions[(*i)];
        if (function.empty()) function = _default_dap4_function;
        c->set_dap4_function(function);

        string attrs = container_attributes[(*i)];
        c->set_attributes(attrs);
        d_xmlcmd_dhi.containers.push_back(c);

        BESDEBUG("xml", "BESXMLDefineCommand::prep_request() - define using container: " << endl << *c << endl);
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */

void BESXMLDefineCommand::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "BESXMLDefineCommand::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESXMLCommand::dump(strm);
    BESIndent::UnIndent();
}

BESXMLCommand *
BESXMLDefineCommand::CommandBuilder(const BESDataHandlerInterface &base_dhi) {
    return new BESXMLDefineCommand(base_dhi);
}
