// DaemonCommandHandler.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2011 OPeNDAP
// Author: James Gallagher <jgallagher@opendap.org> Based on code by
// Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT OPeNDAP
// Please read the full copyright statement in the file COPYING.

#include <unistd.h>    // for getpid fork sleep
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/wait.h>  // for waitpid

#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <sstream>
#include <iostream>
#include <fstream>
#include <map>

using namespace std;

#include "DaemonCommandHandler.h"
#include "Connection.h"
#include "Socket.h"
#include "PPTStreamBuf.h"
#include "PPTProtocol.h"
#include "BESXMLUtils.h"
#include "BESInternalFatalError.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"
#include "TheBESKeys.h"

#include "XMLWriter.h"

// Defined in daemon.cc
extern void block_signals() ;
extern void unblock_signals() ;
extern int  start_master_beslistener() ;
extern bool stop_all_beslisteners( int ) ;
extern int master_beslistener_status;

DaemonCommandHandler::DaemonCommandHandler(const string &config)
    : d_bes_conf(config)
{
    // There is always a bes.conf file, even it does not use that exact name.
    string d_bes_name = d_bes_conf.substr(d_bes_conf.find_last_of('/') + 1);
    d_pathnames.insert(make_pair(d_bes_name, d_bes_conf));

    // *** hack
    string pathname_hack = d_bes_conf.substr(0, d_bes_conf.find_last_of('/') + 1);
    pathname_hack += "modules/";
    d_pathnames.insert(make_pair("csv.conf", pathname_hack + "csv.conf"));
    d_pathnames.insert(make_pair("dap-server.conf", pathname_hack + "dap-server.conf"));
    d_pathnames.insert(make_pair("dap.conf", pathname_hack + "dap.conf"));
    d_pathnames.insert(make_pair("ff.conf", pathname_hack + "ff.conf"));
    d_pathnames.insert(make_pair("fits.conf", pathname_hack + "fits.conf"));
    d_pathnames.insert(make_pair("fonc.conf", pathname_hack + "fonc.conf"));
    d_pathnames.insert(make_pair("gateway.conf", pathname_hack + "gateway.conf"));
    d_pathnames.insert(make_pair("h4.conf", pathname_hack + "h4.conf"));
    d_pathnames.insert(make_pair("h5.conf", pathname_hack + "h5.conf"));
    d_pathnames.insert(make_pair("nc.conf", pathname_hack + "nc.conf"));

    map<string,string>::iterator i = d_pathnames.begin();
    while(i != d_pathnames.end()) {
	BESDEBUG("besdaemon", "besdaemon: d_pathnames: [" << (*i).first << "]: " << d_pathnames[(*i).first] << endl);
	++i;
    }
#if 0
    // There will likely be subordinate config files for each module
    string module_regex;
    bool found = false;

    BESDEBUG("besdaemon", "besdaemon: keys file name: " << TheBESKeys::TheKeys()->keys_file_name() << endl);
    string verbose;
    TheBESKeys::TheKeys()->get_value( "BES.LogName", verbose, found ) ;
    BESDEBUG("besdaemon", "besdaemon: found BES.LogName: " << verbose << endl);

    TheBESKeys::TheKeys()->get_value("BES.Include", module_regex, found);
    BESDEBUG("besdaemon", "besdaemon: found BES.Include: " << module_regex << endl);

    if (found) {
	BESDEBUG("besdaemon", "besdaemon: 2 found BES.Include: " << module_regex << endl);
    }
    else {
	BESDEBUG("besdaemon", "besdaemon: did not find BES.Include " << endl);
    }

    BESDEBUG("besdaemon", "besdaemon: d_pathnames: [" << d_bes_name << "]: " << d_pathnames[d_bes_name] << endl);
#endif
}

/**
 * Lookup the command and return a constant.
 * @param command
 * @return
 */
DaemonCommandHandler::hai_command
DaemonCommandHandler::lookup_command(const string &command)
{
    if (command == "StopNow")
	return HAI_STOP_NOW;
    else if (command == "Start")
	return HAI_START;
    else if (command == "Exit")
	return HAI_EXIT;
    else if (command == "GetConfig")
	return HAI_GET_CONFIG;
    else if (command == "SetConfig")
	return HAI_SET_CONFIG;
    else
	return HAI_UNKNOWN;
}

vector<string> file2strings(const string &file_name)
{
    vector<string> file;
    string line;
    file.clear();
    ifstream infile(file_name.c_str(), std::ios_base::in);
    while (getline(infile, line, '\n')) {
	file.push_back(line);
    }

    return file;
}

/**
 *
 * @return A string containing the XML response document is returned in the
 * value-result param 'response'. The return code indicates
 * @param command The XML command
 */
void
DaemonCommandHandler::execute_command(const string &command, XMLWriter &writer)
{
    LIBXML_TEST_VERSION;

    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;
    xmlNode *current_node = NULL;

    try {
	// set the default error function to my own
	vector<string> parseerrors;
	xmlSetGenericErrorFunc((void *) &parseerrors, BESXMLUtils::XMLErrorFunc);

	// This cast is legal?
	doc = xmlParseDoc((xmlChar*)command.c_str());
	if (doc == NULL) {
	    string err = "";
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
	    throw BESSyntaxUserError("There is no root element in the xml document", __FILE__, __LINE__);
	}

	string root_name;
	string root_val;
	map<string, string> props;
	BESXMLUtils::GetNodeInfo(root_element, root_name, root_val, props);
	if (root_name != "BesAdminCmd") {
	    string err = (string) "The root element should be a BesAdminCmd element, name is "
		    + (char *) root_element->name;
	    throw BESSyntaxUserError(err, __FILE__, __LINE__);
	}
	if (root_val != "") {
	    string err = (string) "The BesAdminCmd element must not contain a value, " + root_val;
	    throw BESSyntaxUserError(err, __FILE__, __LINE__);
	}

	// iterate through the children of the request element. Each child is an
	// individual command.
	current_node = root_element->children;

	while (current_node) {
	    if (current_node->type == XML_ELEMENT_NODE) {
		string node_name = (char *) current_node->name;
		BESDEBUG("besdaemon", "besdaemon: looking for command " << node_name << endl);

		// While processing a command, block signals, which can also
		// be used to control the master beslistener. unblock at the
		// end of the while loop.
		block_signals();

		switch (lookup_command(node_name)) {
		case HAI_STOP_NOW:
		    BESDEBUG("besdaemon", "besdaemon: Received StopNow" << endl);

		    if (stop_all_beslisteners(SIGTERM) == false) {
			if (master_beslistener_status == BESLISTENER_RUNNING) {
			    throw BESInternalFatalError("Could not stop the master beslistener", __FILE__, __LINE__);
			}
			else {
			    throw BESSyntaxUserError("Received Stop command but the master beslistener was likely already stopped", __FILE__, __LINE__);
			}
		    }
		    else {
			if (xmlTextWriterStartElement(writer.get_writer(), (const xmlChar*) "hai:OK") < 0)
			    throw BESInternalFatalError("Could not write <hai:OK> element ", __FILE__, __LINE__);
			if (xmlTextWriterEndElement(writer.get_writer()) < 0)
			    throw BESInternalFatalError("Could not end <hai:OK> element ", __FILE__, __LINE__);
		    }
		    break;

		case HAI_START: {
		    BESDEBUG("besdaemon", "besdaemon: Received Start" << endl);
		    // start_master_beslistener assigns the mbes pid to a
		    // static global defined in daemon.cc that stop_all_bes...
		    // uses.
		    if (start_master_beslistener() == 0) {
			BESDEBUG("besdaemon", "besdaemon: Error starting; master_beslistener_status = " << master_beslistener_status << endl);
			if (master_beslistener_status == BESLISTENER_RUNNING) {
			    throw BESSyntaxUserError("Received Start command but the master beslistener is already running", __FILE__, __LINE__);
			}
			else {
			    throw BESInternalFatalError("Could not start the master beslistener", __FILE__, __LINE__);
			}
		    }
		    else {
			if (xmlTextWriterStartElement(writer.get_writer(), (const xmlChar*) "hai:OK") < 0)
			    throw BESInternalFatalError("Could not write <hai:OK> element ", __FILE__, __LINE__);
			if (xmlTextWriterEndElement(writer.get_writer()) < 0)
			    throw BESInternalFatalError("Could not end <hai:OK> element ", __FILE__, __LINE__);
		    }
		    break;
		}

		case HAI_EXIT:
		    BESDEBUG("besdaemon", "besdaemon: Received Exit" << endl);
		    stop_all_beslisteners(SIGTERM);
		    unblock_signals(); // called here because we're about to exit
		    exit(0);
		    break;

		case HAI_GET_CONFIG: {
		    BESDEBUG("besdaemon", "besdaemon: Received GetConfig" << endl);

		    if (d_pathnames.empty()) {
			throw BESInternalFatalError("There are no known configuration files for this BES!", __FILE__, __LINE__);
		    }

		    // For each of the configuration files, send an XML
		    // <BesConfig module="" /> element.
		    map<string,string>::iterator i = d_pathnames.begin();
		    while(i != d_pathnames.end()) {
			BESDEBUG("besdaemon", "besdaemon: d_pathnames: [" << (*i).first << "]: " << d_pathnames[(*i).first] << endl);

			BESDEBUG("besdaemon", "besdaemon: Received Get Config; config file is: " << get_config_file() << endl);
			if (xmlTextWriterStartElement(writer.get_writer(), (const xmlChar*) "hai:BesConfig") < 0)
			    throw BESInternalFatalError("Could not write <hai:Config> element ", __FILE__, __LINE__);

			if (xmlTextWriterWriteAttribute(writer.get_writer(), (const xmlChar*) "module", (const xmlChar*) (*i).first.c_str()) < 0)
			    throw BESInternalFatalError("Could not write fileName attribute ", __FILE__, __LINE__);

			vector<string> lines = file2strings(d_pathnames[(*i).first]);
			vector<string>::iterator j = lines.begin();
			while (j != lines.end()) {
			    if (xmlTextWriterWriteString(writer.get_writer(), (const xmlChar*) "\n") < 0)
				throw BESInternalFatalError("Could not write newline", __FILE__, __LINE__);

			    if (xmlTextWriterWriteString(writer.get_writer(), (const xmlChar*) (*j++).c_str()) < 0)
				throw BESInternalFatalError("Could not write line", __FILE__, __LINE__);
			}

			if (xmlTextWriterEndElement(writer.get_writer()) < 0)
			    throw BESInternalFatalError("Could not end <hai:BesConfig> element ", __FILE__, __LINE__);
			++i;
		    }

		    break;
		}

		case HAI_SET_CONFIG:
#if 0
		    xmlChar *xml_char_module = xmlGetProp(current_node, (const xmlChar*) "module");
		    if (xml_char_module) {
			string module = (const char *)xml_char_module;
			xmlFree(xml_char_module);

			BESDEBUG("besdaemon", "besdaemon: Received Get Config; module file is: " << module << endl);
			if (xmlTextWriterStartElement(writer.get_writer(), (const xmlChar*) "hai:BesConfig") < 0)
			throw BESInternalFatalError("Could not write <hai:Config> element ", __FILE__, __LINE__);

			if (xmlTextWriterWriteAttribute(writer.get_writer(), (const xmlChar*) "module",
					(const xmlChar*) module.c_str()) < 0)
			throw BESInternalFatalError("Could not write fileName attribute ", __FILE__, __LINE__);
#endif
		    BESDEBUG("besdaemon", "besdaemon: Received Set Config" << endl);
		    if (xmlTextWriterStartElement(writer.get_writer(), (const xmlChar*) "hai:Unimplemented") < 0)
			throw BESInternalFatalError("Could not write <hai:Unimplemented> element ", __FILE__, __LINE__);
		    if (xmlTextWriterEndElement(writer.get_writer()) < 0)
			throw BESInternalFatalError("Could not end <hai:Unimplemented> element ", __FILE__, __LINE__);
		    break;


		default:
		    throw BESSyntaxUserError("Command " + node_name + " unknown.", __FILE__, __LINE__);
		}
	    }

	    unblock_signals();
	    current_node = current_node->next;
	}
    }
    catch (BESError &e) {
	unblock_signals();
	xmlFreeDoc(doc);
	throw e;
    }

    xmlFreeDoc(doc);
}

static void send_bes_error(XMLWriter &writer, BESError &e)
{
    if (xmlTextWriterStartElement(writer.get_writer(), (const xmlChar*) "hai:BESError") < 0)
	throw BESInternalFatalError("Could not write <hai:OK> element ", __FILE__, __LINE__);

    ostringstream oss;
    oss << e.get_error_type() << std::ends;
    if (xmlTextWriterWriteElement(writer.get_writer(), (const xmlChar*) "hai:Type", (const xmlChar*) oss.str().c_str())
	    < 0)
	throw BESInternalFatalError("Could not write <hai:Type> element ", __FILE__, __LINE__);

    if (xmlTextWriterWriteElement(writer.get_writer(), (const xmlChar*) "hai:Type",
	    (const xmlChar*) e.get_message().c_str()) < 0)
	throw BESInternalFatalError("Could not write <hai:Type> element ", __FILE__, __LINE__);

    if (xmlTextWriterEndElement(writer.get_writer()) < 0)
	throw BESInternalFatalError("Could not end <hai:OK> element ", __FILE__, __LINE__);
}

void DaemonCommandHandler::handle(Connection *c)
{
#if 0
    // Use this for some simple white-listing of allowed clients?
    ostringstream strm;
    string ip = c->getSocket()->getIp();
    strm << "ip " << ip << ", port " << c->getSocket()->getPort();
    string from = strm.str();
#endif
    map<string, string> extensions;

    for (;;) {
	ostringstream ss;

	bool done = false;
	while (!done)
	    done = c->receive(extensions, &ss);

	if (extensions["status"] == c->exit()) {
	    // When the client communicating with the besdaemon exits,
	    // return control to the PPTServer::initConnection() method which
	    // will listen for another connect request.
	    return;
	}

	int descript = c->getSocket()->getSocketDescriptor();
	unsigned int bufsize = c->getSendChunkSize();
	PPTStreamBuf fds(descript, bufsize);

	std::streambuf *holder;
	holder = cout.rdbuf();
	cout.rdbuf(&fds);

	XMLWriter writer;

	try {
	    BESDEBUG("besdaemon", "besdaemon: cmd: " << ss.str() << endl);
	    // runs the command(s); throws on an error.
	    execute_command(ss.str(), writer);

	    cout << writer.get_doc() << endl;
	    fds.finish();
	    cout.rdbuf(holder);
	}
	catch (BESError &e) {
	    // an error has occurred.
	    // flush what we have in the stream to the client
	    cout << flush;

	    // Send the extension status=error to the client so that it
	    // can reset.
	    map<string, string> extensions;
	    extensions["status"] = "error";

	    switch (e.get_error_type()) {
	    case BES_INTERNAL_ERROR:
	    case BES_INTERNAL_FATAL_ERROR:
		BESDEBUG("besdaemon", "besdaemon: Internal/Fatal Error: " << ss.str() << endl);
		extensions["exit"] = "true";
		c->sendExtensions(extensions);
		send_bes_error(writer, e);
		// Send the BESError
		cout << writer.get_doc() << endl;
		fds.finish(); // we are finished, send the last chunk
		cout.rdbuf(holder); // reset the streams buffer
		return; // EXIT; disconnects from client
		break;

	    case BES_SYNTAX_USER_ERROR:
		// cerr << "syntax error" << endl;
		BESDEBUG("besdaemon", "besdaemon: Syntax Error: " << ss.str() << endl);
		c->sendExtensions(extensions);
		// Send the BESError
		send_bes_error(writer, e);
		break;

	    default:
		BESDEBUG("besdaemon", "besdaemon: Error (unknown command): " << ss.str() << endl);
		extensions["exit"] = "true";
		c->sendExtensions(extensions);
		// Send the BESError
		send_bes_error(writer, e);
		break;

	    }

	    cout << writer.get_doc() << endl;
	    fds.finish();	// we are finished, send the last chunk
	    cout.rdbuf(holder);	// reset the streams buffer
	}
    }
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
DaemonCommandHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "DaemonCommandHandler::dump - ("
			     << (void *)this << ")" << endl ;
}

