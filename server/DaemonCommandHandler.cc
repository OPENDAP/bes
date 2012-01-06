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
#include "BESFSFile.h"
#include "BESFSDir.h"
#include "TheBESKeys.h"

#include "BESXMLWriter.h"
#include "BESDaemonConstants.h"

// Defined in daemon.cc
extern void block_signals();
extern void unblock_signals();
extern int start_master_beslistener();
extern bool stop_all_beslisteners(int);
extern int master_beslistener_status;

void DaemonCommandHandler::load_include_files(vector<string> &files, const string &keys_file_name)
{
    vector<string>::iterator i = files.begin();
    while (i != files.end())
        load_include_file(*i++, keys_file_name);
}

/** Find the child config files
 *
 * Build up a map of file names and their paths. We use this info in the
 * implementation of the config editing feature of the HAI.
 *
 * @note Stolen from BESKeys.
 * @param files string representing a file or a regular expression
 * patter for 1 or more files
 */
void DaemonCommandHandler::load_include_file(const string &files, const string &keys_file_name)
{
    string newdir;
    BESFSFile allfiles(files);

    // If the files specified begin with a /, then use that directory
    // instead of the current keys file directory.
    if (!files.empty() && files[0] == '/')
    {
        newdir = allfiles.getDirName();
    }
    else
    {
        // determine the directory of the current keys file. All included
        // files will be relative to this file.
        BESFSFile currfile(keys_file_name);
        string currdir = currfile.getDirName();

        string alldir = allfiles.getDirName();

        if ((currdir == "./" || currdir == ".") && (alldir == "./" || alldir == "."))
        {
            newdir = "./";
        }
        else
        {
            if (alldir == "./" || alldir == ".")
            {
                newdir = currdir;
            }
            else
            {
                newdir = currdir + "/" + alldir;
            }
        }
    }

    // load the files one at a time. If the directory doesn't exist,
    // then don't load any configuration files
    BESFSDir fsd(newdir, allfiles.getFileName());
    BESFSDir::fileIterator i = fsd.beginOfFileList();
    BESFSDir::fileIterator e = fsd.endOfFileList();
    for (; i != e; i++)
    {
        d_pathnames.insert(make_pair((*i).getFileName(), (*i).getFullPath()));
    }
}

DaemonCommandHandler::DaemonCommandHandler(const string &config) :
    d_bes_conf(config)
{
    // There is always a bes.conf file, even it does not use that exact name.
    string d_bes_name = d_bes_conf.substr(d_bes_conf.find_last_of('/') + 1);
    d_pathnames.insert(make_pair(d_bes_name, d_bes_conf));

    {
        // There will likely be subordinate config files for each module
        vector<string> vals;
        bool found = false;
        TheBESKeys::TheKeys()->get_values("BES.Include", vals, found);
        BESDEBUG("besdaemon", "besdaemon: found BES.Include: " << found << endl);

        // Load the child config file/path names into d_pathnames.
        if (found)
        {
            load_include_files(vals, config);
        }
    }

    if (BESDebug::IsSet("besdaemon"))
    {
        map<string, string>::iterator i = d_pathnames.begin();
        while (i != d_pathnames.end())
        {
            BESDEBUG("besdaemon", "besdaemon: d_pathnames: [" << (*i).first << "]: " << d_pathnames[(*i).first] << endl);
            ++i;
        }
    }

    {
        bool found = false;
        TheBESKeys::TheKeys()->get_value("BES.LogName", d_log_file_name, found);
        if (!found)
            d_log_file_name = "";
    }
}

/**
 * Lookup the command and return a constant.
 * @param command The XML element that names a command
 * @return A constant that can be used in a switch stmt.
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

/** Read a file and store the result in a chunk of memory.
 * @param name The name of the file
 * @return a pointer to the memory that contains the file's bytes
 * @note The caller must free the memory
 */
static char *read_file(const string &name)
{
    char *memblock;
    ifstream::pos_type size;

    ifstream file(name.c_str(), ios::in | ios::binary | ios::ate);
    if (file.is_open())
    {
        size = file.tellg();
        memblock = new char[((unsigned long)size) + 1];
        file.seekg(0, ios::beg);
        file.read(memblock, size);
        file.close();

        memblock[size] = '\0';

        return memblock;
    }
    else
    {
        throw BESInternalError("Could not open config file:" + name, __FILE__, __LINE__);
    }
}

/** Write the contents of a string to a file. This function was designed to write updated
 * versions of the bes configuration files. It saves a backup of the version of the file
 * present when the server started - an attempt to preserve a human-generated file and
 * not over write it with the stuff sent from the HAI (which is human-written too, but...).
 * This function also tries to make the file write an atomic operation so that the configuration
 * is not lost if there's an error midway through writing.
 * @param name The file name
 * @param buffer The data, in a string
 */
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

#if 0

// This has an infinite loop in it under some circumstances.

/** This version of get_bes_log_lines() is an attempt to improve the efficiency
 * of the process using an estimate of the size of the log lines to skip over
 * the vast majority of the log file.
 *
 * @note if num_lines is zero, that means get the whole file. This will have
 * the awful side affect of allocating memory for the whole file, which may be
 * very large.
 *
 * @todo Change the estimated number of chars per log line dynamically.
 */
#define BES_LOG_CHARS_EST_PER_LINE 126

static char *get_bes_log_lines(const string &log_file_name, long num_lines)
{
    ifstream infile(log_file_name.c_str(), ios::in | ios::binary);
    if (!infile.is_open())
        throw BESInternalError("Could not open file for reading (" + log_file_name + ")", __FILE__, __LINE__);

    // This is used to save time counting lines in large files
    static ifstream::pos_type prev_end_pos = 0;
    // static long prev_line_count = 0;

    ifstream::pos_type start_from_pos = 0;
    // num_line == 0 is special value that means get all the lines
    if (num_lines > 0)
    {
        // Get size of file in bytes, then estimate where to start looking for
        // num_lines lines, then set the file pointer there.
        infile.seekg(0, ios::end);
        ifstream::pos_type end_pos = infile.tellg();
        long est_num_lines = end_pos / BES_LOG_CHARS_EST_PER_LINE;
        if (num_lines >= est_num_lines)
            infile.seekg(0, ios::beg);
         else
            infile.seekg((est_num_lines - num_lines) * BES_LOG_CHARS_EST_PER_LINE, ios::beg);
        ifstream::pos_type start_from_pos = infile.tellg();

        BESDEBUG("besdaemon", "beadaemon: end_pos: " << end_pos << " start_from_pos: " << start_from_pos << endl);

        // No laughing at my goto...
        retry:
        // start_from_pos points to where we start looking for num_lines
        long count = 0;
        while (!infile.eof() && !infile.fail())
        {
            infile.ignore(1024, '\n');  // Assume no line is > 1024
            ++count;
        }

        infile.clear(); // Needed to reset eof() or fail() so tellg/seekg work.

        // if the start_from_pos is too far along (there are not num_lines
        // left), scale it back and try again.
        if (count < num_lines) {
            BESDEBUG("besdaemon", "besdaemon: Retrying; Log length  (count)" << count << ", num_lines " << num_lines << endl);
            // 10 isa fudge factor...
            long size = start_from_pos;
            size -= ((num_lines - count + 10) * BES_LOG_CHARS_EST_PER_LINE);
            infile.seekg(size, ios::beg);
            start_from_pos = infile.tellg();
            goto retry;
        }

        infile.seekg(start_from_pos, ios::beg);

        BESDEBUG("besdaemon", "besdaemon: Log length  (count)" << count << endl);

        if (count > num_lines)
        {
            // Skip count - num-lines
            long skip = count - num_lines;
            while (skip > 0 && !infile.eof() && !infile.fail())
            {
                infile.ignore(1024, '\n');
                --skip;
            }

            infile.clear();
        }
    }

    // Read remaining lines as a block of stuff.
    ifstream::pos_type start_pos = infile.tellg();
    infile.seekg(0, ios::end);
    ifstream::pos_type end_pos = infile.tellg();

    unsigned long size = end_pos - start_pos;
    char *memblock = new char[size + 1];

    infile.seekg(start_pos, ios::beg);
    infile.read(memblock, size);
    infile.close();

    memblock[size] = '\0';

    return memblock;
}
#endif

#if 0
// This is an older version of get_bes_log_lines(). It's not as inefficient as
// the first version, but it's not great either. This version remembers how big
// the log was and so skips one of two reads of the entire log. It will still
// read the entire log just to print the last 200 lines (the log might be 1 GB).

// if num_lines is == 0, get all the lines; if num_lines < 0, also get all the
// lines, but this is really an error, should be trapped by caller.
static char *get_bes_log_lines(const string &log_file_name, long num_lines)
{
    ifstream infile(log_file_name.c_str(), ios::in | ios::binary);
    if (!infile.is_open())
        throw BESInternalError("Could not open file for reading (" + log_file_name + ")", __FILE__, __LINE__);

    // This is used to save time counting lines in large files
    static ifstream::pos_type prev_end_pos = 0;
    static long prev_line_count = 0;

    BESDEBUG("besdaemon", "besdaemon: prev_line_count: " << prev_line_count << endl);
    // num_lines == 0 is special value that means get all the lines
    if (num_lines > 0)
    {
        // static values saved from the previous run saves recounting
        infile.seekg(prev_end_pos, ios::beg);
        long count = prev_line_count;
        while (!infile.eof() && !infile.fail())
        {
            infile.ignore(1024, '\n');
            ++count;
        }

        infile.clear(); // Needed to reset eof() or fail() so tellg/seekg work.

        prev_end_pos = infile.tellg(); // Save the end pos
        prev_line_count = count - 1;    // The loop always adds one

        infile.seekg(0, ios::beg);

        BESDEBUG("besdaemon", "besdaemon: Log length  " << count << endl);

        if (count > num_lines)
        {
            // Skip count - num-lines
            long skip = count - num_lines;
            do
            {
                infile.ignore(1024, '\n');
                --skip;
            } while (skip > 0 && !infile.eof() && !infile.fail());

            infile.clear();
        }
    }

    // Read remaining lines as a block of stuff.
    ifstream::pos_type start_pos = infile.tellg();
    infile.seekg(0, ios::end);
    ifstream::pos_type end_pos = infile.tellg();

    unsigned long size = end_pos - start_pos;
    char *memblock = new char[size + 1];

    infile.seekg(start_pos, ios::beg);
    infile.read(memblock, size);
    infile.close();

    memblock[size] = '\0';

    return memblock;
}
#endif

#if 1
// Count forward 'lines', leave the file pointer at the place just past that
// and return the number of lines actually read (which might be less if eof
// is found before 'lines' lines are read.
static unsigned long move_forward_lines(ifstream &infile, unsigned long lines)
{
    unsigned long count = 0;
	while (count < lines && !infile.eof() && !infile.fail()) {
		infile.ignore(1024, '\n');
		++count;
	}

	infile.clear(); // Needed to reset eof() or fail() so tellg/seekg work.

	return count;
}

// Count the number of lines from pos to the end of the file
static unsigned long count_lines(ifstream &infile, ifstream::pos_type pos)
{
	infile.seekg(pos, ios::beg);
	unsigned long count = 0;
	while (!infile.eof() && !infile.fail()) {
		infile.ignore(1024, '\n');
		++count;
	}

	infile.clear(); // Needed to reset eof() or fail() so tellg/seekg work.

	return count;
}

// Starting at wherever the file pointer is at, read to the end and return
// the data in a char *. The caller must delete[] the memory.
static char *read_file_data(ifstream &infile)
{
    // Read remaining lines as a block of stuff.
    ifstream::pos_type start_pos = infile.tellg();
    infile.seekg(0, ios::end);
    ifstream::pos_type end_pos = infile.tellg();

    unsigned long size = (end_pos > start_pos) ? end_pos - start_pos : 0;
    char *memblock = new char[size + 1];

    infile.seekg(start_pos, ios::beg);
    infile.read(memblock, size);
    infile.close();

    memblock[size] = '\0';

    return memblock;
}

// These are used to save time counting lines in large files
static ifstream::pos_type last_start_pos = 0;
static unsigned long last_start_line = 0;

// This is an older version of get_bes_log_lines(). It's not as inefficient as
// the first version, but it's not great either. This version remembers how big
// the log was and so skips one of two reads of the entire log. It will still
// read the entire log just to print the last 200 lines (the log might be 1 MB).

// if num_lines is == 0, get all the lines; if num_lines < 0, also get all the
// lines, but this is really an error, should be trapped by caller.
static char *get_bes_log_lines(const string &log_file_name, unsigned long num_lines)
{
    ifstream infile(log_file_name.c_str(), ios::in | ios::binary);
    if (!infile.is_open())
        throw BESInternalError("Could not open file for reading (" + log_file_name + ")", __FILE__, __LINE__);
#if 0
    // This is used to save time counting lines in large files
    static ifstream::pos_type last_start_pos = 0;
    static unsigned long last_start_line = 0;
#endif
    BESDEBUG("besdaemon", "besdaemon: last_start_line " << last_start_line << endl);
    if (num_lines == 0) {
    	// return the whole file
    	infile.seekg(0, ios::beg);
    	return read_file_data(infile);
    }
    else {
    	// How many lines in the total file? Use last count info.
        unsigned long count = count_lines(infile, last_start_pos) + last_start_line;
        BESDEBUG("besdaemon", "besdaemon: Log length " << count << " (started at " << last_start_line << ")" << endl);
    	// last_start_pos is where last_start_line is, we need to advance to
    	// the line that is num_lines back from the end of the file
        unsigned long new_start_line = (count >= num_lines) ? count - num_lines + 1: 0;
    	// Now go back to the last_start_pos
    	infile.seekg(last_start_pos, ios::beg);
    	// and count forward to the line that starts this last num_lines
    	count = move_forward_lines(infile, new_start_line - last_start_line);
    	BESDEBUG("besdaemon", "besdaemon: count forward " << count << " lines." << endl);
    	// Save this point for the next time
    	last_start_line = new_start_line;
    	last_start_pos = infile.tellg();

    	return read_file_data(infile);
    }
}
#endif

/**
 *
 * @return A string containing the XML response document is returned in the
 * value-result param 'response'. The return code indicates
 * @param command The XML command
 */
void DaemonCommandHandler::execute_command(const string &command, BESXMLWriter &writer)
{
    LIBXML_TEST_VERSION;

    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;
    xmlNode *current_node = NULL;

    try {
        // set the default error function to my own
        vector<string> parseerrors;
        xmlSetGenericErrorFunc((void *) &parseerrors, BESXMLUtils::XMLErrorFunc);
#if 0
	// We would like this, but older versions of libxml don't use 'const'.
	// Older == 2.6.16. jhrg 12.13.11
        doc = xmlParseDoc((const xmlChar*) command.c_str());
#else
        doc = xmlParseDoc((xmlChar*) command.c_str());
#endif
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
                // ***
                // cerr << "Processing  command " << node_name << endl;

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
                        if (master_beslistener_status == BESLISTENER_RUNNING) {
                            throw BESSyntaxUserError("Received Start command but the master beslistener is already running", __FILE__, __LINE__);
                        }

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
                            // Whenever the master listener starts, it makes a new log file. Reset the counters used to
                            // record the 'last line read' position - these variables are part of an optimization
                            // to limit re-reading old sections of the log file.
                            last_start_pos = 0;
                            last_start_line = 0;

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

                            char *content = read_file(d_pathnames[(*i).first]);
                            try {
                            BESDEBUG("besdaemon_verbose", "besdaemon: content: " << content << endl);
                            if (xmlTextWriterWriteString(writer.get_writer(), (const xmlChar*)"\n") < 0)
                            throw BESInternalFatalError("Could not write newline", __FILE__, __LINE__);

                            if (xmlTextWriterWriteString(writer.get_writer(), (const xmlChar*)content) < 0)
                            throw BESInternalFatalError("Could not write line", __FILE__, __LINE__);

                            delete [] content; content = 0;
                            }
                            catch (...) {
                            	delete [] content; content = 0;
                            	throw;
                            }
                            
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

                        char *content = get_bes_log_lines(d_log_file_name, num_lines);
                        try {
                        BESDEBUG("besdaemon_verbose", "besdaemon: Returned lines: " << content << endl);
                        if (xmlTextWriterWriteString(writer.get_writer(), (const xmlChar*)"\n") < 0)
                            throw BESInternalFatalError("Could not write newline", __FILE__, __LINE__);

                        if (xmlTextWriterWriteString(writer.get_writer(), (const xmlChar*)content) < 0)
                            throw BESInternalFatalError("Could not write line", __FILE__, __LINE__);

                        delete [] content; content = 0;
                           }
                            catch (...) {
                            	delete [] content; content = 0;
                            	throw;
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

                         BESDEBUG("besdaemon", "besdaemon: before setting " << name << " to " << state << endl);
                         // ***
                         // cerr << "setting context  " << name << " to " << state << endl;

                         // Setting this here is all we have to do. This will
                         // change the debug/log settings for the daemon and
                         // (See damon.cc update_beslistener_args()) cause the
                         // new settings to be passed onto new beslisteners.
                         BESDebug::Set( name, state );

                         BESDEBUG("besdaemon", "besdaemon: after setting " << name << " to " << state << endl);

                         if (xmlTextWriterStartElement(writer.get_writer(), (const xmlChar*) "hai:OK") < 0)
                             throw BESInternalFatalError("Could not write <hai:OK> element ", __FILE__, __LINE__);
                         if (xmlTextWriterEndElement(writer.get_writer()) < 0)
                             throw BESInternalFatalError("Could not end <hai:OK> element ", __FILE__, __LINE__);

                         break;
                     }

                    default:
                        throw BESSyntaxUserError("Command " + node_name + " unknown.", __FILE__, __LINE__);
                }
                // ***
                // cerr << "Completed command: " << node_name << endl;
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

static void send_bes_error(BESXMLWriter &writer, BESError &e)
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

    BESXMLWriter writer;

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

