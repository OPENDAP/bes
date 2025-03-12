// StandAloneClient.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.
//
// Copyright (c) 2014 OPeNDAP, Inc.
//
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
//
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>
//      hyoklee     Hyo-Kyung Lee <hyoklee@hdfgroup.org>

#include "config.h"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>

#ifdef HAVE_UNISTD_H

#include <unistd.h>

#endif

using std::cout;
using std::endl;
using std::cerr;
using std::ofstream;
using std::ios;
using std::flush;
using std::ostringstream;
using std::ostream;
using std::string;
using std::ifstream;

#ifdef HAVE_LIBREADLINE
#  if defined(HAVE_READLINE_READLINE_H)

#    include <readline/readline.h>

#  elif defined(HAVE_READLINE_H)
#    include <readline.h>
#  else                         /* !defined(HAVE_READLINE_H) */
extern "C" {
    char *readline(const char *);
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
extern "C" {
    int add_history(const char *);
    int write_history(const char *);
    int read_history(const char *);
}
#  endif                        /* defined(HAVE_READLINE_HISTORY_H) */
/* no history */
#endif       /* HAVE_READLINE_HISTORY */


#define SIZE_COMMUNICATION_BUFFER (4096*4096)

#include "BESXMLInterface.h"
#include "BESStopWatch.h"
#include "BESError.h"
#include "BESDebug.h"

#include "StandAloneClient.h"
#include "CmdTranslation.h"

#define MODULE "standalone"
#define prolog string("StandAloneClient::").append(__func__).append("() - ")


StandAloneClient::~StandAloneClient() {
    if (_strmCreated && _strm) {
        _strm->flush();
        delete _strm;
        _strm = nullptr;
    } else if (_strm) {
        _strm->flush();
    }
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
void StandAloneClient::setOutput(ostream *strm, bool created) {
    if (_strmCreated && _strm) {
        _strm->flush();
        delete _strm;
    } else if (_strm) {
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
void StandAloneClient::executeClientCommand(const string &cmd) {
    string suppress = "suppress";
    if (cmd.compare(0, suppress.size(), suppress) == 0) {
        setOutput(nullptr, false);
        return;
    }

    string output = "output to";
    if (cmd.compare(0, output.size(), output) == 0) {
        string subcmd = cmd.substr(output.size() + 1);
        string screen = "screen";
        if (subcmd.compare(0, screen.size(), screen) == 0) {
            setOutput(&cout, false);
        } else {
            // subcmd is the name of the file - then semicolon
            string file = subcmd.substr(0, subcmd.size() - 1);
            auto *fstrm = new ofstream(file.c_str(), ios::app);
            if (!(*fstrm)) {
                delete fstrm;
                cerr << "Unable to set client output to file " << file << endl;
            } else {
                setOutput(fstrm, true);
            }
        }
        return;
    }

    // load commands from an input file and run them
    string load = "load";
    if (cmd.compare(0, load.size(), load) == 0) {
        string file = cmd.substr(load.size() + 1, cmd.size() - load.size() - 2);
        ifstream fstrm(file.c_str());
        if (!fstrm) {
            cerr << "Unable to load commands from file " << file << ": file does not exist or failed to open file"
                 << endl;
        } else {
            executeCommands(fstrm, 1);
        }

        return;
    }

    cerr << "Improper client command " << cmd << endl;
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
void StandAloneClient::executeCommand(const string &cmd, int repeat) {
    string client = "client";
    if (cmd.compare(0, client.size(), client) == 0) {
        executeClientCommand(cmd.substr(client.size() + 1));
    } else {
        if (repeat < 1) repeat = 1;
        for (int i = 0; i < repeat; i++) {
            ostringstream *show_stream = nullptr;
            if (CmdTranslation::is_show()) {
                show_stream = new ostringstream;
            }

            BESDEBUG(MODULE, "StandAloneClient::executeCommand sending: " << cmd << endl);

            BES_STOPWATCH_START(MODULE, prolog + "Timing");

            BESXMLInterface *interface = nullptr;
            if (show_stream) {
                interface = new BESXMLInterface(cmd, show_stream);
            } else {
                interface = new BESXMLInterface(cmd, _strm);
            }

            int status = interface->execute_request("standalone");

            *_strm << flush;

            // Put the call to finish() here because we're not sending chunked responses back
            // to a client over PPT. In the BESServerHandler.cc code, we must do that and hence,
            // break up the call to finish() for the error and no-error cases.
            status = interface->finish(status);

            if (status == 0) {
                BESDEBUG(MODULE, "StandAloneClient::executeCommand - executed successfully" << endl);
            } else {
                // an error has occurred.
                BESDEBUG(MODULE, "StandAloneClient::executeCommand - error occurred" << endl);
                switch (status) {
                    case BES_INTERNAL_FATAL_ERROR: {
                        cerr << "Status not OK, dispatcher returned value " << status << endl;
                        exit(1);
                    }

                    case BES_INTERNAL_ERROR:
                    case BES_SYNTAX_USER_ERROR:
                    case BES_FORBIDDEN_ERROR:
                    case BES_NOT_FOUND_ERROR:
                    case BES_HTTP_ERROR:
                    default:
                        break;
                }
            }

            delete interface;
            interface = nullptr;

            if (show_stream) {
                *(_strm) << show_stream->str() << endl;
                delete show_stream;
                show_stream = nullptr;
            }

            _strm->flush();

            if (repeat > 1 && i < repeat - 1) {
                *_strm << "\nNext-Response:\n" << flush;
            }
        }
    }
}

/** @brief Send the command(s) specified to the BES server after wrapping in
 * request document
 *
 * This takes a command  or set of commands from the command line, wraps it
 * in the proper request document, and sends it to the server.
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
void StandAloneClient::executeCommands(const string &cmd_list, int repeat) {
    _isInteractive = true;
    if (repeat < 1) repeat = 1;

    CmdTranslation::set_show(false);
    try {
        string doc = CmdTranslation::translate(cmd_list);
        if (!doc.empty()) {
            executeCommand(doc, repeat);
        }
    }
    catch (BESError &e) {
        CmdTranslation::set_show(false);
        _isInteractive = false;
        throw e;
    }
    CmdTranslation::set_show(false);
    _isInteractive = false;
}

/** @brief Sends the xml request document from the specified file to the server
 *
 * The requests do not have to be one per line but can span multiple
 * lines and there can be more than one command per line.
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
void StandAloneClient::executeCommands(ifstream &istrm, int repeat) {
    _isInteractive = false;
    if (repeat < 1) repeat = 1;
    for (int i = 0; i < repeat; i++) {
        istrm.clear();
        istrm.seekg(0, ios::beg);

        string cmd;
        string line;
        while (getline(istrm, line)) {
            cmd += line;
        }

        this->executeCommand(cmd, 1);

        if (repeat > 1 && i < repeat - 1) {
            *_strm << "\nNext-Response:\n" << flush;
        }
    }
}

/** @brief An interactive BES client that takes BES requests on the command line.
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
void StandAloneClient::interact() {
    _isInteractive = true;

    cout << endl << endl << "Type 'exit' to exit the command line client and 'help' or '?' "
         << "to display the help screen" << endl << endl;

    bool done = false;
    while (!done) {
        string message;
        size_t len = this->readLine(message);
        if (message == "exit" || message == "exit;") {
            done = true;
        } else if (message == "help" || message == "help;" || message == "?") {
            this->displayHelp();
        } else if (message.size() > 6 && message.substr(0, 6) == "client") {
            this->executeCommand(message, 1);
        } else if (len != 0 && !message.empty()) {
            CmdTranslation::set_show(false);
            try {
                string doc = CmdTranslation::translate(message);
                if (!doc.empty()) {
                    this->executeCommand(doc, 1);
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
}

/** @brief Read a line from the interactive terminal using the readline library
 *
 * @param msg read the line into this string
 * @return number of characters read
 */
size_t StandAloneClient::readLine(string &msg) {
    size_t len = 0;
    char *buf = nullptr;
    buf = ::readline("BESClient> ");
    if (buf && *buf) {
        len = strlen(buf);
#ifdef HAVE_READLINE_HISTORY
        add_history(buf);
#endif
        if (len > SIZE_COMMUNICATION_BUFFER) {
            cerr << __FILE__ << __LINE__ <<
                 ": incoming data buffer exceeds maximum capacity with length " << len << endl;
            exit(1);
        } else {
            msg = buf;
        }
    } else {
        if (!buf) {
            // If a null buffer is returned then this means that EOF is
            // returned. This is different from the user just hitting enter,
            // which means a character buffer is returned, but is empty.

            // Problem: len is unsigned.
            len = -1;
        }
    }

    if (buf) {
        free(buf);
        buf = nullptr;
    }

    return len;
}

/** @brief display help information for the command line client
 */
void StandAloneClient::displayHelp() {
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
    cout << endl;
    cout << "Any commands beginning with 'client' must end with a semicolon" << endl;
    cout << endl;
    cout << "To display the list of commands available from the server " << "please type the command 'show help;'"
         << endl;
    cout << endl;
    cout << endl;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void StandAloneClient::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "StandAloneClient::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "stream: " << (void *) _strm << endl;
    strm << BESIndent::LMarg << "stream created? " << _strmCreated << endl;
    BESIndent::UnIndent();
}
