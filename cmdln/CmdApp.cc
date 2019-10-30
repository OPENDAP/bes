// ClientMain.cc

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

#include <signal.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

using std::cout;
using std::cerr;
using std::endl;
using std::flush;
using std::string;
using std::ofstream;
using std::ifstream;
using std::ostream ;

#include "CmdApp.h"
#include "CmdClient.h"
#include "CmdTranslation.h"
#include "BESError.h"
#include "BESDebug.h"

#define BES_CMDLN_DEFAULT_TIMEOUT 5

// We got tired of typing in this all the time... jhrg 10/30/13
#define DEFAULT_PORT 10022
#define DEFAULT_HOST "localhost"

CmdApp::CmdApp() :
    BESApp(), _client(0), _hostStr(DEFAULT_HOST), _unixStr(""), _portVal(DEFAULT_PORT), _outputStrm(0), _inputStrm(
        0), _createdInputStrm(false), _timeout(0), _repeat(0)
{
}

CmdApp::~CmdApp()
{
    if (_client) {
        delete _client;
        _client = 0;
    }
}

void CmdApp::showVersion()
{
    cout << appName() << ": version 2.0" << endl;
}

void CmdApp::showUsage()
{
    cout << endl;
    cout << appName() << ": the following flags are available:" << endl;
    cout << "    -h <host> - specifies a host for TCP/IP connection" << endl;
    cout << "    -p <port> - specifies a port for TCP/IP connection" << endl;
    cout << "    -u <unixSocket> - specifies a unix socket for connection. " << endl;
    cout << "    -x <command> - specifies a command for the server to execute" << endl;
    cout << "    -i <inputFile> - specifies a file name for a sequence of input commands" << endl;
    cout << "    -f <outputFile> - specifies a file name to output the results of the input" << endl;
    cout << "    -t <timeoutVal> - specifies an optional timeout value in seconds" << endl;
    cout << "    -d - sets the optional debug flag for the client session" << endl;
    cout << "    -r <num> - repeat the command(s) num times" << endl;
    cout << "    -? - display this list of flags" << endl;
    cout << endl;
    BESDebug::Help(cout);
}

void CmdApp::signalCannotConnect(int sig)
{
    if (sig == SIGCONT) {
        CmdApp *app = dynamic_cast<CmdApp *>(BESApp::TheApplication());
        if (app) {
            CmdClient *client = app->client();
            if (client && !client->isConnected()) {
                cout << BESApp::TheApplication()->appName() << ": No response, server may be down or "
                    << "busy with another incoming connection. exiting!\n";
                exit(1);
            }
        }
    }
}

void CmdApp::signalInterrupt(int sig)
{
    if (sig == SIGINT) {
        cout << BESApp::TheApplication()->appName() << ": Please type exit to terminate the session" << endl;
    }
    if (signal(SIGINT, CmdApp::signalInterrupt) == SIG_ERR) {
        cerr << BESApp::TheApplication()->appName() << ": Could not re-register signal\n";
    }
}

void CmdApp::signalTerminate(int sig)
{
    if (sig == SIGTERM) {
        cout << BESApp::TheApplication()->appName() << ": Please type exit to terminate the session" << endl;
    }
    if (signal(SIGTERM, CmdApp::signalTerminate) == SIG_ERR) {
        cerr << BESApp::TheApplication()->appName() << ": Could not re-register signal\n";
    }
}

void CmdApp::signalBrokenPipe(int sig)
{
    if (sig == SIGPIPE) {
        cout << BESApp::TheApplication()->appName() << ": got a broken pipe, server may be down or the port invalid."
            << endl << "Please check parameters and try again" << endl;
        CmdApp *app = dynamic_cast<CmdApp *>(BESApp::TheApplication());
        if (app) {
            CmdClient *client = app->client();
            if (client) {
                client->brokenPipe();
                client->shutdownClient();
                delete client;
                client = 0;
            }
        }
        exit(1);
    }
}

void CmdApp::registerSignals()
{
    // Registering SIGCONT for connection unblocking
    BESDEBUG("cmdln", "CmdApp: Registering signal SIGCONT ... " << endl);
    if (signal( SIGCONT, signalCannotConnect) == SIG_ERR) {
        BESDEBUG("cmdln", "FAILED" << endl);
        cerr << appName() << "Failed to register signal SIGCONT" << endl;
        exit(1);
    }
    BESDEBUG("cmdln", "OK" << endl);

    // Registering SIGINT to disable Ctrl-C from the user in order to avoid
    // server instability
    BESDEBUG("cmdln", "CmdApp: Registering signal SIGINT ... " << endl);
    if (signal( SIGINT, signalInterrupt) == SIG_ERR) {
        BESDEBUG("cmdln", "FAILED" << endl);
        cerr << appName() << "Failed to register signal SIGINT" << endl;
        exit(1);
    }
    BESDEBUG("cmdln", "OK" << endl);

    // Registering SIGTERM to disable kill from the user in order to avoid
    // server instability
    BESDEBUG("cmdln", "CmdApp: Registering signal SIGTERM ... " << endl);
    if (signal( SIGTERM, signalTerminate) == SIG_ERR) {
        BESDEBUG("cmdln", "FAILED" << endl);
        cerr << appName() << "Failed to register signal SIGTERM" << endl;
        exit(1);
    }
    BESDEBUG("cmdln", "OK" << endl);

    // Registering SIGPIE for broken pipes managment.
    BESDEBUG("cmdln", "CmdApp: Registering signal SIGPIPE ... " << endl);
    if (signal( SIGPIPE, CmdApp::signalBrokenPipe) == SIG_ERR) {
        BESDEBUG("cmdln", "FAILED" << endl);
        cerr << appName() << "Failed to register signal SIGPIPE" << endl;
        exit(1);
    }
    BESDEBUG("cmdln", "OK" << endl);
}

int CmdApp::initialize(int argc, char **argv)
{
    int retVal = BESApp::initialize(argc, argv);
    if (retVal != 0) return retVal;

    CmdTranslation::initialize(argc, argv);

    string portStr = "";
    string outputStr = "";
    string inputStr = "";
    string timeoutStr = "";
    string repeatStr = "";

    bool badUsage = false;

    int c;

    while ((c = getopt(argc, argv, "?vd:h:p:t:u:x:f:i:r:")) != -1) {
        switch (c) {
        case 't':
            timeoutStr = optarg;
            break;
        case 'h':
            _hostStr = optarg;
            break;
        case 'd':
            BESDebug::SetUp(optarg);
            break;
        case 'v': {
            showVersion();
            exit(0);
        }
            break;
        case 'p':
            portStr = optarg;
            break;
        case 'u':
            _unixStr = optarg;
            break;
        case 'x':
            _cmd = optarg;
            break;
        case 'f':
            outputStr = optarg;
            break;
        case 'i':
            inputStr = optarg;
            break;
        case 'r':
            repeatStr = optarg;
            break;
        case '?': {
            showUsage();
            exit(0);
        }
            break;
        }
    }

    if (!portStr.empty() && !_unixStr.empty()) {
        cerr << "cannot use both a port number and a unix socket" << endl;
        badUsage = true;
    }

    if (!portStr.empty()) {
        _portVal = atoi(portStr.c_str());
    }

    if (!timeoutStr.empty()) {
        _timeout = atoi(timeoutStr.c_str());
    }
    else {
        _timeout = BES_CMDLN_DEFAULT_TIMEOUT;
    }

    if (outputStr != "") {
        if (_cmd == "" && inputStr == "") {
            cerr << "When specifying an output file you must either " << "specify a command or an input file" << endl;
            badUsage = true;
        }
        else if (_cmd != "" && inputStr != "") {
            cerr << "You must specify either a command or an input file on " << "the command line, not both" << endl;
            badUsage = true;
        }
    }

    if (badUsage == true) {
        showUsage();
        return 1;
    }

    if (outputStr != "") {
        _outputStrm = new ofstream(outputStr.c_str());
        if (!(*_outputStrm)) {
            cerr << "could not open the output file " << outputStr << endl;
            badUsage = true;
        }
    }

    if (inputStr != "") {
        _inputStrm = new ifstream(inputStr.c_str());
        if (!(*_inputStrm)) {
            cerr << "could not open the input file " << inputStr << endl;
            badUsage = true;
        }
        _createdInputStrm = true;
    }

    if (!repeatStr.empty()) {
        _repeat = atoi(repeatStr.c_str());
        if (!_repeat && repeatStr != "0") {
            cerr << "repeat number invalid: " << repeatStr << endl;
            badUsage = true;
        }
        if (!_repeat) {
            _repeat = 1;
        }
    }

    if (badUsage == true) {
        showUsage();
        return 1;
    }

    registerSignals();

    BESDEBUG("cmdln", "CmdApp: initialized settings:" << endl << *this);

    return 0;
}

int CmdApp::run()
{
    try {
        _client = new CmdClient();
        // Since hostStr and portVal have non-null/zero default values, see if unixStr
        // was given and, if so, use it.
        if (!_unixStr.empty()) {
            BESDEBUG("cmdln", "CmdApp: Connecting to unix socket: " << _unixStr << " ... " << endl);
            _client->startClient(_unixStr, _timeout);
        }
        else {
            BESDEBUG("cmdln",
                "CmdApp: Connecting to host: " << _hostStr << " at port: " << _portVal << " ... " << endl);
            _client->startClient(_hostStr, _portVal, _timeout);
        }

        if (_outputStrm) {
            _client->setOutput(_outputStrm, true);
        }
        else {
            _client->setOutput(&cout, false);
        }
        BESDEBUG("cmdln", "OK" << endl);
    }
    catch (BESError &e) {
        if (_client) {
            _client->shutdownClient();
            delete _client;
            _client = 0;
        }
        BESDEBUG("cmdln", "FAILED" << endl);
        cerr << "error starting the client" << endl;
        cerr << e.get_message() << endl;
        exit(1);
    }

    bool do_exit = false;
    try {
        if (_cmd != "") {
            do_exit = _client->executeCommands(_cmd, _repeat);
        }
        else if (_inputStrm) {
            do_exit = _client->executeCommands(*_inputStrm, _repeat);
        }
        else {
            do_exit = _client->interact();
        }
    }
    catch (BESError &e) {
        cerr << "error processing commands" << endl;
        cerr << e.get_message() << endl;
    }

    try {
        BESDEBUG("cmdln", "CmdApp: shutting down client ... " << endl);
        if (_client) {
            // if do_exit is set, then the client has shut itself down
            if (!do_exit) _client->shutdownClient();
            delete _client;
            _client = 0;
        }
        BESDEBUG("cmdln", "OK" << endl);

        BESDEBUG("cmdln", "CmdApp: closing input stream ... " << endl);
        if (_createdInputStrm && _inputStrm) {
            _inputStrm->close();
            delete _inputStrm;
            _inputStrm = 0;
        }
        BESDEBUG("cmdln", "OK" << endl);
    }
    catch (BESError &e) {
        BESDEBUG("cmdln", "FAILED" << endl);
        cerr << "error closing the client" << endl;
        cerr << e.get_message() << endl;
        return 1;
    }

    return 0;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void CmdApp::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "CmdApp::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    if (_client) {
        strm << BESIndent::LMarg << "client: " << endl;
        BESIndent::Indent();
        _client->dump(strm);
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "client: null" << endl;
    }
    strm << BESIndent::LMarg << "host: " << _hostStr << endl;
    strm << BESIndent::LMarg << "unix socket: " << _unixStr << endl;
    strm << BESIndent::LMarg << "port: " << _portVal << endl;
    strm << BESIndent::LMarg << "command: " << _cmd << endl;
    strm << BESIndent::LMarg << "output stream: " << (void *) _outputStrm << endl;
    strm << BESIndent::LMarg << "input stream: " << (void *) _inputStrm << endl;
    strm << BESIndent::LMarg << "created input stream? " << _createdInputStrm << endl;
    strm << BESIndent::LMarg << "timeout: " << _timeout << endl;
    BESApp::dump(strm);
    BESIndent::UnIndent();
}

int main(int argc, char **argv)
{
    CmdApp app;
    return app.main(argc, argv);
}

