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

using std::ostringstream ;
using std::cout ;
using std::endl ;
using std::cerr ;
using std::flush ;

#include "DaemonCommandHandler.h"
#include "Connection.h"
#include "Socket.h"
#include "PPTStreamBuf.h"
#include "PPTProtocol.h"
#include "BESXMLUtils.h"
#include "BESInternalFatalError.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"
//#include "TheBESKeys.h"

#include "XMLWriter.h"

// Defined in daemon.cc
extern int  start_master_beslistener() ;
extern void stop_all_beslisteners( int ) ;

DaemonCommandHandler::DaemonCommandHandler()
{
}

typedef enum {
    HAI_UNKNOWN,
    HAI_STOP_NOW,
    HAI_START,
    HAI_EXIT
} hai_command;

/**
 * Lookup the command and return a constant.
 * @param command
 * @return
 */
static hai_command lookup_command(const string &command)
{
    if (command == "StopNow")
	return HAI_STOP_NOW;
    else if (command == "Start")
	return HAI_START;
    else if (command == "Exit")
	return HAI_EXIT;
    else
	return HAI_UNKNOWN;
}

/**
 *
 * @return A string containing the XML response document is returned in the
 * value-result param 'response'. The return code indicates
 * @param command The XML command
 */
string
DaemonCommandHandler::execute_command(const string &command)
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

		hai_command command = lookup_command(node_name);
		switch (command) {
		case HAI_STOP_NOW:
		    BESDEBUG("besdaemon", "besdaemon: Received StopNow" << endl);
		    stop_all_beslisteners(SIGTERM);	// see daemon.cc
		    break;
		case HAI_START:
		{
		    BESDEBUG("besdaemon", "besdaemon: Received Start" << endl);
		    // start_master_beslistener assigns the mbes pid to a
		    // static global defined in daemon.cc that stop_all_bes...
		    // uses.
		    int mbes_pid = start_master_beslistener();
		    if (mbes_pid == 0)
			throw BESInternalFatalError("Could not start the master beslistener", __FILE__, __LINE__);
		    break;
		}
		case HAI_EXIT:
		    BESDEBUG("besdaemon", "besdaemon: Received Exit" << endl);
		    stop_all_beslisteners(SIGTERM);
		    exit(0);
		    break;
		default:
		    throw BESSyntaxUserError("Command " + node_name + " unknown.", __FILE__, __LINE__);
		}
	    }

	    current_node = current_node->next;
	}
    }
    catch (BESError &e) {
	xmlFreeDoc(doc);
	throw e;
    }

    xmlFreeDoc(doc);

    // Change this later?
    return "";
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

	try {
	    BESDEBUG("besdaemon", "besdaemon: cmd: " << ss.str() << endl);
	    // runs the command(s); throws on an error. The 'response' is often
	    // empty.
	    string response = execute_command(ss.str());

	    XMLWriter writer;
	    // when response is empty, return 'OK'
	    if (response.empty()) {
		if (xmlTextWriterStartElement(writer.get_writer(), (const xmlChar*) "hai:OK") < 0)
		    throw BESInternalFatalError("Could not write <hai:OK> element ", __FILE__, __LINE__);
		if (xmlTextWriterEndElement(writer.get_writer()) < 0)
		    throw BESInternalFatalError("Could not end <hai:OK> element ", __FILE__, __LINE__);
	    }

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
		// Send the BESError
		// we are finished, send the last chunk
		fds.finish();

		// reset the streams buffer
		cout.rdbuf(holder);
		return; // EXIT; disconnects from client
		break;

	    case BES_SYNTAX_USER_ERROR:
		// cerr << "syntax error" << endl;
		BESDEBUG("besdaemon", "besdaemon: Syntax Error: " << ss.str() << endl);
		c->sendExtensions(extensions);
		// Send the BESError
		break;

	    default:
		BESDEBUG("besdaemon", "besdaemon: Error (unknown command): " << ss.str() << endl);
		extensions["exit"] = "true";
		c->sendExtensions(extensions);
		// Send the BESError
		break;

	    }

	    // we are finished, send the last chunk
	    fds.finish();

	    // reset the streams buffer
	    cout.rdbuf(holder);

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

