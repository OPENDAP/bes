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
#include "BESInternalError.h"
#include "BESSyntaxUserError.h"
#include "BESDebug.h"
#include "TheBESKeys.h"

#include "XMLWriter.h"
#include "BESDaemonConstants.h"

// Defined in daemon.cc
extern void block_signals();
extern void unblock_signals();
extern int start_master_beslistener();
extern bool stop_all_beslisteners(int);
extern int master_beslistener_status;

DaemonCommandHandler::DaemonCommandHandler(const string &config) :
    d_bes_conf(config)
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

    map<string, string>::iterator i = d_pathnames.begin();
    while (i != d_pathnames.end()) {
        BESDEBUG("besdaemon", "besdaemon: d_pathnames: [" << (*i).first << "]: " << d_pathnames[(*i).first] << endl);
        ++i;
    }

    bool found = false;
    TheBESKeys::TheKeys()->get_value("BES.LogName", d_log_file_name, found);
    if (!found)
        d_log_file_name = "";

#if 0
    // There will likely be subordinate config files for each module
    string module_regex;
    bool found = false;

    BESDEBUG("besdaemon", "besdaemon: keys file name: " << TheBESKeys::TheKeys()->keys_file_name() << endl);
    TheBESKeys::TheKeys()->get_value("BES.Include", module_regex, found);
    BESDEBUG("besdaemon", "besdaemon: found BES.Include: " << module_regex << endl);

    if (found) {
        BESDEBUG("besdaemon", "besdaemon: 2 found BES.Include: " << module_regex << endl);
    }
    else {
        BESDEBUG("besdaemon", "besdaemon: did not find BES.Include " << endl);
    }
#endif
}

/**
 * Lookup the command and return a constant.
 * @param command
 * @return
 */
DaemonCommandHandler::hai_command DaemonCommandHandler::lookup_command(const string &command)
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
    else if (command == "TailLog")
        return HAI_TAIL_LOG;
    else if (command == "GetLogContexts")
        return HAI_GET_LOG_CONTEXTS;
    else if (command == "SetLogContext")
        return HAI_SET_LOG_CONTEXT;
    else
        return HAI_UNKNOWN;
}

#if 0
// This doesn't work with our text files. There may be an issue with seekg and
// ifstreams.
static char *read_file(const string &name)
{
    char * buffer;
    long size;

    ifstream infile (name.c_str(), ifstream::binary);

    // get size of file
    infile.seekg(0, ifstream::end);
    size=infile.tellg();
    infile.seekg(0);

    // allocate memory for file content
    buffer = new char [size];

    // read content of infile
    infile.read (buffer,size);

    infile.close();

    return buffer;
}
#endif

static void write_file(const string &name, const string &buffer)
{
    // First write the new text to a temporary file
    string tmp_name = name + ".tmp";
    ofstream outfile(tmp_name.c_str(), std::ios_base::out);
    if (outfile.is_open()) {
        // write to outfile
        outfile.write(buffer.data(), buffer.length());

        outfile.close();
    }
    else {
        throw BESInternalError("Could not open config file:" + name, __FILE__, __LINE__);
    }

    // Now see if the original file should be backed up. For any given
    // instance of the server, only back up on the initial attempt to write a
    // new version of the file.
    ostringstream backup_name;
    backup_name << name << "." << getpid();
    if (access(backup_name.str().c_str(), F_OK) == -1) {
        BESDEBUG("besdaemon", "besdaemon: No backup file yet" << endl);
        // Backup does not exist for this instance of the server; backup name
        if (rename(name.c_str(), backup_name.str().c_str()) == -1) {
            BESDEBUG("besdaemon", "besdaemon: Could not backup file " << name << " to " << backup_name.str() << endl);
            ostringstream err;
            err << "(" << errno << ") " << strerror(errno);
            throw BESInternalError("Could not backup config file: " + name + ": " + err.str(), __FILE__, __LINE__);
        }
    }

    // Now move the '.tmp' file to <name>
    if (rename(tmp_name.c_str(), name.c_str()) == -1) {
        BESDEBUG("besdaemon", "besdaemon: Could not complete write " << name << " to " << backup_name.str() << endl);
        ostringstream err;
        err << "(" << errno << ") " << strerror(errno);
        throw BESInternalError("Could not write config file:" + name + ": " + err.str(), __FILE__, __LINE__);
    }
}

static vector<string> file2strings(const string &file_name)
{
    vector<string> file;
    string line;
    file.clear();
    ifstream infile(file_name.c_str(), std::ios_base::in);
    if (!infile.is_open())
        throw BESInternalError("Could not open file for reading (" + file_name + ")", __FILE__, __LINE__);

    while (getline(infile, line, '\n')) {
        file.push_back(line);
    }

    return file;
}

// if num_lines is == 0, get all the lines; if num_lines < 0, also get all the
// lines, but this is really an error, should be trapped by caller.
static vector<string> get_bes_log_lines(const string &log_file_name, long num_lines)
{
    vector<string> lines;
    string line;
    lines.clear();
    ifstream infile(log_file_name.c_str(), std::ios_base::in);
    if (!infile.is_open())
        throw BESInternalError("Could not open file for reading (" + log_file_name + ")", __FILE__, __LINE__);

    // Count the lines; there has to be a better way...
    long count = 0;
    do {
        infile.ignore(1024, '\n');
        ++count;
    } while (!infile.eof() && !infile.fail());

    // Why doesn't "infile.seekg (0);" work?
    infile.close();
    infile.open(log_file_name.c_str(), std::ios_base::in);

    BESDEBUG("besdaemon", "besdaemon: Log length  " << count << endl);

    // num_line == 0 is special value that means get all the lines
    if (num_lines > 0 && count > num_lines) {
        // Skip count - num-lines
        long skip = count - num_lines;
        BESDEBUG("besdaemon", "besdaemon: skipping  " << skip << endl);
        do {
            infile.ignore(1024, '\n');
            --skip;
        } while (skip > 0 && !infile.eof() && !infile.fail());
    }

    // Read what's left
    while (getline(infile, line, '\n')) {
        lines.push_back(line);
    }

    return lines;
}

/**
 *
 * @return A string containing the XML response document is returned in the
 * value-result param 'response'. The return code indicates
 * @param command The XML command
 */
void DaemonCommandHandler::execute_command(const string &command, XMLWriter &writer)
{
    LIBXML_TEST_VERSION;

    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;
    xmlNode *current_node = NULL;

    try {
        // set the default error function to my own
        vector<string> parseerrors;
        xmlSetGenericErrorFunc((void *) &parseerrors, BESXMLUtils::XMLErrorFunc);

        // FIXME This cast is legal?
        doc = xmlParseDoc((xmlChar*) command.c_str());
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
            string err = (string) "The root element should be a BesAdminCmd element, name is " + (char *) root_element->name;
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
                        map<string, string>::iterator i = d_pathnames.begin();
                        while (i != d_pathnames.end()) {
                            BESDEBUG("besdaemon", "besdaemon: d_pathnames: [" << (*i).first << "]: " << d_pathnames[(*i).first] << endl);

                            if (xmlTextWriterStartElement(writer.get_writer(), (const xmlChar*) "hai:BesConfig") < 0)
                                throw BESInternalFatalError("Could not write <hai:Config> element ", __FILE__, __LINE__);

                            if (xmlTextWriterWriteAttribute(writer.get_writer(), (const xmlChar*) "module", (const xmlChar*) (*i).first.c_str()) < 0)
                                throw BESInternalFatalError("Could not write fileName attribute ", __FILE__, __LINE__);
#if 0
                            // You'd think this would work, but there seems to be
                            // issue with the files that breaks it.
                            char *content = read_file(d_pathnames[(*i).first]);
                            BESDEBUG("besdaemon", "besdaemon: content: " << content << endl);
                            if (xmlTextWriterWriteString(writer.get_writer(), (const xmlChar*)"\n") < 0)
                            throw BESInternalFatalError("Could not write newline", __FILE__, __LINE__);

                            if (xmlTextWriterWriteString(writer.get_writer(), (const xmlChar*)content) < 0)
                            throw BESInternalFatalError("Could not write line", __FILE__, __LINE__);

                            delete [] content; content = 0;
#endif
#if 1
                            vector<string> lines = file2strings(d_pathnames[(*i).first]);
                            vector<string>::iterator j = lines.begin();
                            while (j != lines.end()) {
                                if (xmlTextWriterWriteString(writer.get_writer(), (const xmlChar*) "\n") < 0)
                                    throw BESInternalFatalError("Could not write newline", __FILE__, __LINE__);

                                if (xmlTextWriterWriteString(writer.get_writer(), (const xmlChar*) (*j++).c_str()) < 0)
                                    throw BESInternalFatalError("Could not write line", __FILE__, __LINE__);
                            }
#endif

                            if (xmlTextWriterEndElement(writer.get_writer()) < 0)
                                throw BESInternalFatalError("Could not end <hai:BesConfig> element ", __FILE__, __LINE__);
                            ++i;
                        }

                        break;
                    }

                    case HAI_SET_CONFIG: {
                        BESDEBUG("besdaemon", "besdaemon: Received SetConfig" << endl);
                        xmlChar *xml_char_module = xmlGetProp(current_node, (const xmlChar*) "module");
                        if (!xml_char_module) {
                            throw BESSyntaxUserError("SetConfig missing module ", __FILE__, __LINE__);
                        }
                        string module = (const char *) xml_char_module;
                        xmlFree(xml_char_module);

                        BESDEBUG("besdaemon", "besdaemon: Received SetConfig; module: " << module << endl);

                        xmlChar *file_content = xmlNodeListGetString(doc, current_node->children, /* inLine = */true);
                        if (!file_content) {
                            throw BESInternalFatalError("SetConfig missing content, no changes made ", __FILE__, __LINE__);
                        }
                        string content = (const char *) file_content;
                        xmlFree(file_content);
                        BESDEBUG("besdaemon_verbose", "besdaemon: Received SetConfig; content: " << endl << content << endl);

                        write_file(d_pathnames[module], content);

                        if (xmlTextWriterStartElement(writer.get_writer(), (const xmlChar*) "hai:OK") < 0)
                            throw BESInternalFatalError("Could not write <hai:OK> element ", __FILE__, __LINE__);

                        if (xmlTextWriterWriteString(writer.get_writer(), (const xmlChar*) "\nPlease restart the server for these changes to take affect.\n") < 0)
                            throw BESInternalFatalError("Could not write newline", __FILE__, __LINE__);

                        if (xmlTextWriterEndElement(writer.get_writer()) < 0)
                            throw BESInternalFatalError("Could not end <hai:OK> element ", __FILE__, __LINE__);

                        break;
                    }

                    case HAI_TAIL_LOG: {
                        BESDEBUG("besdaemon", "besdaemon: Received TailLog" << endl);

                        xmlChar *xml_char_lines = xmlGetProp(current_node, (const xmlChar*) "lines");
                        if (!xml_char_lines) {
                            throw BESSyntaxUserError("TailLog missing lines attribute ", __FILE__, __LINE__);
                        }

                        char *endptr;
                        long num_lines = strtol((const char *) xml_char_lines, &endptr, 10 /*base*/);
                        if (num_lines == 0 && endptr == (const char *) xml_char_lines) {
                            ostringstream err;
                            err << "(" << errno << ") " << strerror(errno);
                            throw BESSyntaxUserError("TailLog lines attribute bad value: " + err.str(), __FILE__, __LINE__);
                        }

                        if (xmlTextWriterStartElement(writer.get_writer(), (const xmlChar*) "hai:BesLog") < 0)
                            throw BESInternalFatalError("Could not write <hai:BesLog> element ", __FILE__, __LINE__);

                        BESDEBUG("besdaemon", "besdaemon: TailLog: log file:" << d_log_file_name << ", lines: " << num_lines << endl);

                        vector<string> lines = get_bes_log_lines(d_log_file_name, num_lines);
                        BESDEBUG("besdaemon", "besdaemon: Returned lines:" << lines.end() - lines.begin() << endl);
                        vector<string>::iterator j = lines.begin();
                        while (j != lines.end()) {
                            if (xmlTextWriterWriteString(writer.get_writer(), (const xmlChar*) "\n") < 0)
                                throw BESInternalFatalError("Could not write newline", __FILE__, __LINE__);

                            if (xmlTextWriterWriteString(writer.get_writer(), (const xmlChar*) (*j++).c_str()) < 0)
                                throw BESInternalFatalError("Could not write line", __FILE__, __LINE__);
                        }

                        if (xmlTextWriterEndElement(writer.get_writer()) < 0)
                            throw BESInternalFatalError("Could not end <hai:BesLog> element ", __FILE__, __LINE__);

                        break;
                    }

                    case HAI_GET_LOG_CONTEXTS: {
                         BESDEBUG("besdaemon", "besdaemon: Received GetLogContexts" << endl);

                         BESDEBUG("besdaemon", "besdaemon: There are " << BESDebug::debug_map().size() << " Contexts" << endl);
                         if (BESDebug::debug_map().size()) {
                            BESDebug::debug_citer i = BESDebug::debug_map().begin();
                            while (i != BESDebug::debug_map().end()) {
                                if (xmlTextWriterStartElement(writer.get_writer(), (const xmlChar*) "hai:LogContext") < 0)
                                    throw BESInternalFatalError("Could not write <hai:LogContext> element ", __FILE__, __LINE__);

                                if (xmlTextWriterWriteAttribute(writer.get_writer(), (const xmlChar*) "name", (const xmlChar*) (*i).first.c_str()) < 0)
                                    throw BESInternalFatalError("Could not write 'name' attribute ", __FILE__, __LINE__);

                                string state = (*i).second ? "on" : "off";
                                if (xmlTextWriterWriteAttribute(writer.get_writer(), (const xmlChar*) "state", (const xmlChar*) state.c_str()) < 0)
                                    throw BESInternalFatalError("Could not write 'state' attribute ", __FILE__, __LINE__);

                                if (xmlTextWriterEndElement(writer.get_writer()) < 0)
                                    throw BESInternalFatalError("Could not end <hai:LogContext> element ", __FILE__, __LINE__);

                                ++i;
                            }
                        }

                         break;
                     }

                    case HAI_SET_LOG_CONTEXT: {
                         BESDEBUG("besdaemon", "besdaemon: Received SetLogContext" << endl);

                         xmlChar *xml_char_module = xmlGetProp(current_node, (const xmlChar*)"name");
                         if (!xml_char_module) {
                             throw BESSyntaxUserError("SetLogContext missing name ", __FILE__, __LINE__);
                         }
                         string name = (const char *)xml_char_module;
                         xmlFree(xml_char_module);

                         xml_char_module = xmlGetProp(current_node, (const xmlChar*)"state");
                         if (!xml_char_module) {
                             throw BESSyntaxUserError("SetLogContext missing state ", __FILE__, __LINE__);
                         }
                         bool state = strcmp((const char *)xml_char_module, "on") == 0;
                         xmlFree(xml_char_module);

                         cerr << "besdaemon: before setting " << name << " to " << state << endl << flush;
                         //BESDEBUG("besdaemon", "besdaemon: before setting " << name << " to " << state << endl);
                         BESDebug::Set( name, state );
                         cerr << "besdaemon: after setting " << name << " to " << state << endl;
                         //BESDEBUG("besdaemon", "besdaemon: after setting " << name << " to " << state << endl);

                         if (xmlTextWriterStartElement(writer.get_writer(), (const xmlChar*) "hai:OK") < 0)
                             throw BESInternalFatalError("Could not write <hai:OK> element ", __FILE__, __LINE__);
                         if (xmlTextWriterEndElement(writer.get_writer()) < 0)
                             throw BESInternalFatalError("Could not end <hai:OK> element ", __FILE__, __LINE__);

                         break;
                     }

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
    catch (...) {
        xmlFreeDoc(doc);
        throw;
    }

    xmlFreeDoc(doc);
}

static void send_bes_error(XMLWriter &writer, BESError &e)
{
    if (xmlTextWriterStartElement(writer.get_writer(), (const xmlChar*) "hai:BESError") < 0)
        throw BESInternalFatalError("Could not write <hai:OK> element ", __FILE__, __LINE__);

    ostringstream oss;
    oss << e.get_error_type() << std::ends;
    if (xmlTextWriterWriteElement(writer.get_writer(), (const xmlChar*) "hai:Type", (const xmlChar*) oss.str().c_str()) < 0)
        throw BESInternalFatalError("Could not write <hai:Type> element ", __FILE__, __LINE__);

    if (xmlTextWriterWriteElement(writer.get_writer(), (const xmlChar*) "hai:Message", (const xmlChar*) e.get_message().c_str()) < 0)
        throw BESInternalFatalError("Could not write <hai:Message> element ", __FILE__, __LINE__);

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
#if 0
    for (;;) {
#endif
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
                BESDEBUG("besdaemon", "besdaemon: Internal/Fatal Error: " << e.get_message() << endl);
                extensions["exit"] = "true";
                c->sendExtensions(extensions);
                send_bes_error(writer, e);
                // Send the BESError
#if 0
                // This seemed like a good idea, but really, no error is
                // fatal, at least not yet.
                cout << writer.get_doc() << endl;
                fds.finish(); // we are finished, send the last chunk
                cout.rdbuf(holder); // reset the streams buffer
                return; // EXIT; disconnects from client
#endif
                break;

            case BES_SYNTAX_USER_ERROR:
                // cerr << "syntax error" << endl;
                BESDEBUG("besdaemon", "besdaemon: Syntax Error: " << e.get_message() << endl);
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
        fds.finish(); // we are finished, send the last chunk
        cout.rdbuf(holder); // reset the streams buffer
    }
#if 0
}
#endif
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void DaemonCommandHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "DaemonCommandHandler::dump - (" << (void *) this << ")" << endl;
}

