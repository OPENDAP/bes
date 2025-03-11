// CmdClient.cc

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

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

using std::cout;
using std::endl;
using std::cerr;
using std::ofstream;
using std::ostringstream;
using std::ios;
using std::map;
using std::ifstream;
using std::ostream;
using std::string;

#ifdef HAVE_LIBREADLINE
#  if defined(HAVE_READLINE_READLINE_H)
#    include <readline/readline.h>
#  elif defined(HAVE_READLINE_H)
#    include <readline.h>
#  else                         /* !defined(HAVE_READLINE_H) */
extern "C"
{
    char *readline( const char * );
}
#  endif                        /* !defined(HAVE_READLINE_H) */
char *cmdline = NULL;
#else                           /* !defined(HAVE_READLINE_READLINE_H) */
/* no readline */
#endif                          /* HAVE_LIBREADLINE */

#ifdef HAVE_READLINE_HISTORY
#  if defined(HAVE_READLINE_HISTORY_H)
#    include <readline/history.h>
#  elif defined(HAVE_HISTORY_H)
#    include <history.h>
#  else                         /* !defined(HAVE_HISTORY_H) */
extern "C"
{
    int add_history( const char * );
    int write_history( const char * );
    int read_history( const char * );
}
#  endif                        /* defined(HAVE_READLINE_HISTORY_H) */
/* no history */
#endif                          /* HAVE_READLINE_HISTORY */

#include <libxml/encoding.h>

#define SIZE_COMMUNICATION_BUFFER 4096*4096
#include "CmdClient.h"
#include "CmdTranslation.h"
#include "PPTClient.h"
#include "BESDebug.h"
#include "BESStopWatch.h"
#include "BESError.h"

#define MODULE "cmdln"
#define prolog string("CmdClient::").append(__func__).append("() - ")

CmdClient::~CmdClient()
{
    if (_strmCreated && _strm) {
        _strm->flush();
        delete _strm;
        _strm = 0;
    }
    else if (_strm) {
        _strm->flush();
    }
    if (_client) {
        delete _client;
        _client = 0;
    }
}

/** @brief Connect the BES client to the BES server.
 *
 * Connects to the BES server on the specified machine listening on
 * the specified port.
 *
 * @param  host     The name of the host machine where the server is
 *                  running.
 * @param  portVal  The port on which the server on the host hostStr is
 *                  listening for requests.
 * @param  timeout  Number of times to try an un-blocked read
 * @throws BESError Thrown if unable to connect to the specified host
 *                      machine given the specified port.
 * @see    BESError
 */
void CmdClient::startClient(const string & host, int portVal, int timeout)
{
    _client = new PPTClient(host, portVal, timeout);
    _client->initConnection();
}

/** @brief Connect the BES client to the BES server using the unix socket
 *
 * Connects to the BES server using the specified unix socket
 *
 * @param  unixStr  Full path to the unix socket
 * @param  timeout  Number of times to try an un-blocked read
 * @throws BESError Thrown if unable to connect to the BES server
 * @see    BESError
 */
void CmdClient::startClient(const string & unixStr, int timeout)
{
    _client = new PPTClient(unixStr, timeout);
    _client->initConnection();
}

/** @brief Closes the connection to the OpeNDAP server and closes the output stream.
 *
 * @throws BESError Thrown if unable to close the connection or close
 *                  the output stream.
 *                  machine given the specified port.
 * @see    OutputStream
 * @see    BESError
 */
void CmdClient::shutdownClient()
{
    if (_client) _client->closeConnection();
}

/** @brief Set the output stream for responses from the BES server.
 *
 * Specify where the response output from your BES request will be
 * sent. Set to null if you wish to ignore the response from the BES
 * server.
 *
 * @param  strm  an OutputStream specifying where to send responses from
 *               the BES server. If null then the output will not be
 *               output but will be thrown away.
 * @param created true of the passed stream was created and can be deleted
 *                either by being replaced ro in the destructor
 * @throws BESError catches any problems with opening or writing to
 *                      the output stream and creates a BESError
 * @see    OutputStream
 * @see    BESError
 */
void CmdClient::setOutput(ostream * strm, bool created)
{
    if (_strmCreated && _strm) {
        _strm->flush();
        delete _strm;
    }
    else if (_strm) {
        _strm->flush();
    }
    _strm = strm;
    _strmCreated = created;
}

/** @brief Executes a client side command
 *
 * Client side commands include
 * client suppress;
 * client output to screen;
 * client output to &lt;filename&gt;;
 * client load &lt;filename&gt;;
 *
 * @param  cmd  The BES client side command to execute
 * @see    BESError
 */
bool CmdClient::executeClientCommand(const string & cmd)
{
    bool do_exit = false;
    string suppress = "suppress";
    if (cmd.compare(0, suppress.size(), suppress) == 0) {
        setOutput(NULL, false);
        return do_exit;
    }

    string output = "output to";
    if (cmd.compare(0, output.size(), output) == 0) {
        string subcmd = cmd.substr(output.size() + 1);
        string screen = "screen";
        if (subcmd.compare(0, screen.size(), screen) == 0) {
            setOutput(&cout, false);
        }
        else {
            // subcmd is the name of the file - then semicolon
            string file = subcmd.substr(0, subcmd.size() - 1);
            ofstream *fstrm = new ofstream(file.c_str(), ios::app);
            if (fstrm && !(*fstrm)) {
                delete fstrm;
                cerr << "Unable to set client output to file " << file << endl;
            }
            else {
                setOutput(fstrm, true);
            }
        }
        return do_exit;
    }

    // load commands from an input file and run them
    string load = "load";
    if (cmd.compare(0, load.size(), load) == 0) {
        string file = cmd.substr(load.size() + 1, cmd.size() - load.size() - 2);
        ifstream fstrm(file.c_str());
        if (!fstrm) {
            cerr << "Unable to load commands from file " << file << ": file does not exist or failed to open file"
                    << endl;
        }
        else {
            do_exit = executeCommands(fstrm, 1);
        }

        return do_exit;
    }

    cerr << "Improper client command " << cmd << endl;

    return do_exit;
}

/** @brief Sends a single OpeNDAP request to the OpeNDAP server.
 *
 * The response is written to the output stream if one is specified,
 * otherwise the output is ignored.
 *
 * @param  cmd  The BES request that is sent to the BES server to handle.
 * @param repeat Number of times to repeat the command
 * @throws BESError Thrown if there is a problem sending the request
 *                      to the server or a problem receiving the response
 *                      from the server.
 * @see    BESError
 */
bool CmdClient::executeCommand(const string &cmd, int repeat)
{
    bool do_exit = false;
    const string client = "client";
    if (cmd.compare(0, client.size(), client) == 0) {
        do_exit = executeClientCommand(cmd.substr(client.size() + 1));
    }
    else {
        if (repeat < 1) repeat = 1;
        for (int i = 0; i < repeat && !do_exit; i++) {
            BESDEBUG(MODULE, prolog << "sending: " << cmd << endl);

            // BES_STOPWATCH_START_DHI(MODULE, prolog + "Elapsed Time To Transmit", d_dhi_ptr);
            BES_STOPWATCH_START(MODULE, prolog + "Elapsed Time To Transmit");

            map<string, string> extensions;
            _client->send(cmd, extensions);

            BESDEBUG(MODULE, prolog << "receiving " << endl);
            // keep reading till we get the last chunk, send to _strm
            bool done = false;
            ostringstream *show_stream = 0;
            while (!done) {
                if (CmdTranslation::is_show()) {
                    if (!show_stream) {
                        show_stream = new ostringstream;
                    }
                }
                if (show_stream) {
                    done = _client->receive(extensions, show_stream);
                }
                else {
                    done = _client->receive(extensions, _strm);
                }
                if (extensions["status"] == "error") {
                    // If there is an error, just flush what I have
                    // and continue on.
                    _strm->flush();

                    // let's also set show to true because we've gotten back
                    // an xml document (maybe)
                    if (_isInteractive) {
                        CmdTranslation::set_show(true);
                    }
                }
                if (extensions["exit"] == "true") {
                    do_exit = true;
                }
            }
            if (show_stream) {
                *(_strm) << show_stream->str() << endl;
                delete show_stream;
                show_stream = 0;
            }
            if (BESDebug::IsSet(MODULE)) {
                BESDEBUG(MODULE, "extensions:" << endl);
                map<string, string>::const_iterator i = extensions.begin();
                map<string, string>::const_iterator e = extensions.end();
                for (; i != e; i++) {
                    BESDEBUG(MODULE, "  " << (*i).first << " = " << (*i).second << endl);
                }
                BESDEBUG(MODULE, "cmdclient done receiving " << endl);
            }

            _strm->flush();
        }
    }
    return do_exit;
}

/** @brief Send the command(s) specified to the BES server after wrapping in
 * request document
 *
 * This takes a string command or set of string commands from the command
 * line, builds an xml document with the commands, and sends it to the
 * server.
 *
 * The response is written to the output stream if one is specified,
 * otherwise the output is ignored.
 *
 * @param  cmd_list  The BES commands to send to the BES server
 * @param  repeat    Number of times to repeat the command
 * @throws BESError  Thrown if there is a problem sending any of the
 *                   request to the server or a problem receiving any
 *                   of the responses from the server.
 * @see    BESError
 */
bool CmdClient::executeCommands(const string &cmd_list, int repeat)
{
    bool do_exit = false;
    _isInteractive = true;
    if (repeat < 1) repeat = 1;

    CmdTranslation::set_show(false);
    try {
        string doc = CmdTranslation::translate(cmd_list);
        if (!doc.empty()) {
            do_exit = this->executeCommand(doc, repeat);
        }
    }
    catch (BESError &e) {
        CmdTranslation::set_show(false);
        _isInteractive = false;
        throw e;
    }
    CmdTranslation::set_show(false);
    _isInteractive = false;
    return do_exit;
}

/** @brief Sends the xml request document from the specified file to the server
 *
 * The file can contain only one xml request document and must be well
 * formatted. If not, the server will respond with an exception
 *
 * The response is written to the output stream if one is specified,
 * otherwise the output is ignored.
 *
 * @param  istrm  The file holding the xml request document
 * @param repeat  Number of times to repeat the series of commands
 from the file.
 * @throws BESError Thrown if there is a problem opening the file to
 *                  read, reading the request  document from the file, sending
 *                  the request document to the server or a problem
 *                  receiving any of the responses from the server.
 * @see    File
 * @see    BESError
 */
bool CmdClient::executeCommands(ifstream & istrm, int repeat)
{
    bool do_exit = false;
    _isInteractive = false;
    if (repeat < 1) repeat = 1;
    for (int i = 0; i < repeat; i++) {
        istrm.clear();
        istrm.seekg(0, ios::beg);
        string cmd;
        while (!istrm.eof()) {
            char line[4096];
            line[0] = '\0';
            istrm.getline(line, 4096, '\n');
            cmd += line;
        }
        do_exit = this->executeCommand(cmd, 1);
    }
    return do_exit;
}

/** @brief An interactive BES client that takes BES requests on the command
 * line.
 *
 * The commands specified interactively are the old style command syntax, not
 * xml documents.
 *
 * There can be more than one command per line, but commands cannot span
 * multiple lines. The user will be prompted to enter a new BES request.
 *
 * OpenDAPClient:
 *
 * The response is written to the output stream if one is specified,
 * otherwise the output is ignored.
 *
 * @throws BESError Thrown if there is a problem sending any of the
 *                      requests to the server or a problem receiving any
 *                      of the responses from the server.
 * @see    BESError
 */
bool CmdClient::interact()
{
    bool do_exit = false;
    _isInteractive = true;

    cout << endl << endl << "Type 'exit' to exit the command line client and 'help' or '?' "
            << "to display the help screen" << endl << endl;

    bool done = false;
    while (!done && !do_exit) {
        string message = "";
        size_t len = this->readLine(message);
        // len is unsigned. jhrg 11/5/13
        if (/* len == -1 || */message == "exit" || message == "exit;") {
            done = true;
        }
        else if (message == "help" || message == "help;" || message == "?") {
            this->displayHelp();
        }
        else if (message.size() > 6 && message.substr(0, 6) == "client") {
            do_exit = this->executeCommand(message, 1);
        }
        else if (len != 0 && message != "") {
            CmdTranslation::set_show(false);
            try {
                string doc = CmdTranslation::translate(message);
                if (!doc.empty()) {
                    do_exit = this->executeCommand(doc, 1);
                }
            }
            catch (BESError &e) {
                CmdTranslation::set_show(false);
                _isInteractive = false;
                throw e;
            }
            CmdTranslation::set_show(false);
        }
    }
    _isInteractive = false;

    return do_exit;
}

/** @brief Read a line from the interactive terminal using the readline library
 *
 * @note I've changed this code so that it returns zero on EOF, not whatever -1
 * is when it is assigned to an unsigned int/long.
 * @param msg read the line into this string
 * @return number of characters read, zero for EOF
 */
size_t CmdClient::readLine(string &msg)
{
    size_t len = 0;
    char *buf = (char *) NULL;
    buf = ::readline("BESClient> ");
    if (buf && *buf) {
        len = strlen(buf);
#ifdef HAVE_READLINE_HISTORY
        add_history(buf);
#endif
        if (len > SIZE_COMMUNICATION_BUFFER) {
            cerr << __FILE__ << __LINE__ <<
            ": incoming data buffer exceeds maximum capacity with lenght " << len << endl;
            exit(1);
        }
        else {
            msg = buf;
        }
    }
    else {
        if (!buf) {
            // If a null buffer is returned then this means that EOF is
            // returned. This is different from the user just hitting enter,
            // which means a character buffer is returned, but is empty.

            // Problem: len is unsigned.
            // len = -1 ; I replaced this with the following. jhrg 1/4/12
            len = 0;
        }
    }
    if (buf) {
        free(buf);
        buf = (char *) NULL;
    }
    return len;
}

/** @brief display help information for the command line client
 */
void CmdClient::displayHelp()
{
    cout << endl;
    cout << endl;
    cout << "BES Command Line Client Help" << endl;
    cout << endl;
    cout << "Client commands available:" << endl;
    cout << "    exit                     - exit the command line interface" << endl;
    cout << "    help                     - display this help screen" << endl;
    cout << "    client suppress;         - suppress output from the server" << endl;
    cout << "    client output to screen; - display server output to the screen" << endl;
    cout << "    client output to <file>; - display server output to specified file" << endl;
    cout << "    client load <file>; - load xml document from file" << endl;
    cout << endl;
    cout << "Any commands beginning with 'client' must end with a semicolon" << endl;
    cout << endl;
    cout << "To display the list of commands available from the server " << "please type the command 'show help;'"
            << endl;
    cout << endl;
    cout << endl;
}

/** @brief return whether the client is connected to the BES
 *
 * @return whether the client is connected (true) or not (false)
 */
bool CmdClient::isConnected()
{
    if (_client) return _client->isConnected();
    return false;
}

/** @brief inform the server that there has been a borken pipe
 */
void CmdClient::brokenPipe()
{
    if (_client) _client->brokenPipe();
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void CmdClient::dump(ostream & strm) const
{
    strm << BESIndent::LMarg << "CmdClient::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    if (_client) {
        strm << BESIndent::LMarg << "client:" << endl;
        BESIndent::Indent();
        _client->dump(strm);
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "client: null" << endl;
    }
    strm << BESIndent::LMarg << "stream: " << (void *) _strm << endl;
    strm << BESIndent::LMarg << "stream created? " << _strmCreated << endl;
    BESIndent::UnIndent();
}
