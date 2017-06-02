// StandAloneApp.cc

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

#include <unistd.h>
#include <getopt.h>
#include <signal.h>

#include <iostream>
#include <string>
#include <fstream>

using std::cout;
using std::cerr;
using std::endl;
using std::flush;
using std::string;
using std::ofstream;

#include "StandAloneApp.h"
#include "StandAloneClient.h"
#include "BESError.h"
#include "BESDebug.h"
#include "BESDefaultModule.h"
#include "BESXMLDefaultCommands.h"
#include "TheBESKeys.h"
#include "BESCatalogUtils.h"
#include "CmdTranslation.h"

StandAloneApp::StandAloneApp() :
    BESModuleApp(), _client(0), _outputStrm(0), _inputStrm(0), _createdInputStrm( false), _repeat(0)
{
}

StandAloneApp::~StandAloneApp()
{
    if (_client) {
        delete _client;
        _client = 0;
    }

    delete TheBESKeys::TheKeys();

    BESCatalogUtils::delete_all_catalogs();
}

void StandAloneApp::showVersion()
{
    cout << appName() << ": version 2.0" << endl;
}

void StandAloneApp::showUsage()
{
    cout << endl;
    cout << appName() << ": the following options are available:" << endl;
    cout << "    -c <file>, --config=<file> - BES configuration file" << endl;
    cout << "    -x <command>, --execute=<command> - command for the server to execute" << endl;
    cout << "    -i <file>, --inputfile=<file> - file with a sequence of input commands" << endl;
    cout << "    -f <file>, --outputfile=<file> - write output to this file" << endl;
    cout << "    -d, --debug - turn on debugging for the client session" << endl;
    cout << "    -r <num>, --repeat=<num> - repeat the command(s) <num> times" << endl;
    cout << "    -v, --version - return version information" << endl;
    cout << "    -?, --help - display help information" << endl;
    cout << endl;
    BESDebug::Help(cout);
}

int StandAloneApp::initialize(int argc, char **argv)
{
    CmdTranslation::initialize(argc, argv);

    string outputStr = "";
    string inputStr = "";
    string repeatStr = "";

    bool badUsage = false;

    int c;

    static struct option longopts[] = { { "config", 1, 0, 'c' }, { "debug", 0, 0, 'd' }, { "version", 0, 0, 'v' }, {
        "execute", 1, 0, 'x' }, { "outputfile", 1, 0, 'f' }, { "inputfile", 1, 0, 'i' }, { "repeat", 1, 0, 'r' }, {
        "help", 0, 0, '?' }, { 0, 0, 0, 0 } };
    int option_index = 0;

    while ((c = getopt_long(argc, argv, "?vc:d:x:f:i:r:", longopts, &option_index)) != -1) {
        switch (c) {
        case 'c':
            TheBESKeys::ConfigFile = optarg;
            break;
        case 'd':
            BESDebug::SetUp(optarg);
            break;
        case 'v': {
            showVersion();
            exit(0);
        }
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

    try {
        BESDEBUG("standalone", "ServerApp: initializing default module ... " << endl);
        BESDefaultModule::initialize(argc, argv);
        BESDEBUG("standalone", "ServerApp: done initializing default module" << endl);

        BESDEBUG("standalone", "ServerApp: initializing default commands ... " << endl);
        BESXMLDefaultCommands::initialize(argc, argv);
        BESDEBUG("standalone", "ServerApp: done initializing default commands" << endl);

        BESDEBUG("standalone", "ServerApp: initializing loaded modules ... " << endl);
        int retval = BESModuleApp::initialize(argc, argv);
        BESDEBUG("standalone", "ServerApp: done initializing loaded modules" << endl);
        if (retval) return retval;
    }
    catch (BESError &e) {
        cerr << "Failed to initialize stand alone app" << endl;
        cerr << e.get_message() << endl;
        return 1;
    }

    BESDEBUG("standalone", "StandAloneApp: initialized settings:" << endl << *this);

    return 0;
}

int StandAloneApp::run()
{
    try {
        _client = new StandAloneClient;
        if (_outputStrm) {
            _client->setOutput(_outputStrm, true);
        }
        else {
            _client->setOutput(&cout, false);
        }
        BESDEBUG("standalone", "OK" << endl);
    }
    catch (BESError &e) {
        if (_client) {
            delete _client;
            _client = 0;
        }
        BESDEBUG("standalone", "FAILED" << endl);
        cerr << "error starting the client" << endl;
        cerr << e.get_message() << endl;
        exit(1);
    }

    try {
        if (_cmd != "") {
            _client->executeCommands(_cmd, _repeat);
        }
        else if (_inputStrm) {
            _client->executeCommands(*_inputStrm, _repeat);
        }
        else {
            _client->interact();
        }
    }
    catch (BESError &e) {
        cerr << "error processing commands" << endl;
        cerr << e.get_message() << endl;
    }

    try {
        BESDEBUG("standalone", "StandAloneApp: shutting down client ... " << endl);
        if (_client) {
            delete _client;
            _client = 0;
        }
        BESDEBUG("standalone", "OK" << endl);

        BESDEBUG("standalone", "StandAloneApp: closing input stream ... " << endl);
        if (_createdInputStrm && _inputStrm) {
            _inputStrm->close();
            delete _inputStrm;
            _inputStrm = 0;
        }
        BESDEBUG("standalone", "OK" << endl);
    }
    catch (BESError &e) {
        BESDEBUG("standalone", "FAILED" << endl);
        cerr << "error closing the client" << endl;
        cerr << e.get_message() << endl;
        return 1;
    }

    return 0;
}

/** @brief clean up after the application
 *
 * @param sig return value from run that should be returned from method
 * @returns signal or return value passed to the terminate method
 */
int StandAloneApp::terminate(int sig)
{
    BESDEBUG("standalone", "ServerApp: terminating loaded modules ... " << endl);
    BESModuleApp::terminate(sig);
    BESDEBUG("standalone", "ServerApp: done terminating loaded modules" << endl);

    BESDEBUG("standalone", "ServerApp: terminating default commands ...  " << endl);
    BESXMLDefaultCommands::terminate();
    BESDEBUG("standalone", "ServerApp: done terminating default commands" << endl);

    BESDEBUG("standalone", "ServerApp: terminating default module ... " << endl);
    BESDefaultModule::terminate();
    BESDEBUG("standalone", "ServerApp: done terminating default module" << endl);

    CmdTranslation::terminate();

    xmlCleanupParser();

    return sig;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void StandAloneApp::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "StandAloneApp::dump - (" << (void *) this << ")" << endl;
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
    strm << BESIndent::LMarg << "command: " << _cmd << endl;
    strm << BESIndent::LMarg << "output stream: " << (void *) _outputStrm << endl;
    strm << BESIndent::LMarg << "input stream: " << (void *) _inputStrm << endl;
    strm << BESIndent::LMarg << "created input stream? " << _createdInputStrm << endl;
    BESApp::dump(strm);
    BESIndent::UnIndent();
}

int main(int argc, char **argv)
{
    try {
        StandAloneApp app;
        return app.main(argc, argv);
    }
    catch (BESError &e) {
        cerr << "Caught BES Error while starting the command processor: " << e.get_message() << endl;
        return 1;
    }
    catch (std::exception &e) {
        cerr << "Caught C++ error while starting the command processor: " << e.what() << endl;
        return 2;
    }
    catch (...) {
        cerr << "Caught unknown error while starting the command processor." << endl;
        return 3;
    }
}

