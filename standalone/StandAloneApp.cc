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


#include "config.h"

#include <getopt.h>

#include <iostream>
#include <string>
#include <vector>

#include <libxml/parser.h>

#include "StandAloneApp.h"
#include "StandAloneClient.h"
#include "BESError.h"
#include "BESDebug.h"
#include "BESDefaultModule.h"
#include "BESXMLDefaultCommands.h"
#include "TheBESKeys.h"
#include "CmdTranslation.h"

#define MODULE "standalone"
#define prolog string("StandAloneApp::").append(__func__).append("() - ")

using namespace std;

void StandAloneApp::showVersion()
{
    cout << appName() << ": version 3.0" << endl;
}

void StandAloneApp::showUsage()
{
    cout << endl;
    cout << appName() << ": the following options are available:" << endl;
    cout << "    -c <file>, --config=<file> - BES configuration file" << endl;
    cout << "    -x <command>, --execute=<command> - command for the server " << endl;
    cout << "       to execute" << endl;
    cout << "    -i <file>, --inputfile=<file> - file with a sequence of input " << endl;
    cout << "       commands, may be used multiple times." << endl;
    cout << "    -f <file>, --outputfile=<file> - write output to this file" << endl;
    cout << "    -d, --debug - turn on debugging for the client session" << endl;
    cout << "    -r <num>, --repeat=<num> - repeat the command(s) <num> times" << endl;
    cout << "    -v, --version - return version information" << endl;
    cout << "    -?, --help - display help information" << endl;
    cout << endl;
    cout << "Note: You may provide a bes command with -x OR you may provide one " << endl;
    cout << "      or more BES command file names. One or the other, not neither, not both." << endl;
    cout << endl;
    BESDebug::Help(cout);
}

int StandAloneApp::initialize(int argc, char **argv)
{
    CmdTranslation::initialize(argc, argv);

    static struct option longopts[] = {
            {     "config", 1, 0, 'c' },
            {      "debug", 0, 0, 'd' },
            {    "version", 0, 0, 'v' },
            {    "execute", 1, 0, 'x' },
            { "outputfile", 1, 0, 'f' },
            {  "inputfile", 1, 0, 'i' },
            {     "repeat", 1, 0, 'r' },
            {       "help", 0, 0, '?' },
            {            0, 0, 0,  0  }
    };

    string outputStr;
    string repeatStr;
    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "?vc:d:x:f:i:r:", longopts, &option_index)) != -1) {
        switch (c) {
            case 'c':
                TheBESKeys::ConfigFile = optarg;
                break;
            case 'd':
                BESDebug::SetUp(optarg);
                break;
            case 'x':
                _cmd = optarg;
                break;
            case 'f':
                outputStr = optarg;
                break;
            case 'i':
                _command_file_names.emplace_back(optarg);
                break;
            case 'r':
                repeatStr = optarg;
                break;
            case 'v':
                showVersion();
                exit(0);
            case '?':
            default:
                showUsage();
                exit(0);
        }
    }

    bool badUsage = false;
    if (!outputStr.empty()) {
        if (_cmd.empty() && _command_file_names.empty()) {
            cerr << "When specifying an output file you must either specify a command or an input file" << endl;
            badUsage = true;
        }
        else if (!_cmd.empty() &&  !_command_file_names.empty()) {
            cerr << "You must specify either a command or an input file on the command line, not both" << endl;
            badUsage = true;
        }
    }

    if (badUsage) {
        showUsage();
        return 1;
    }

    if (!outputStr.empty()) {
        _outputStrm = new ofstream(outputStr.c_str());
        if (!(*_outputStrm)) {
            cerr << "could not open the output file " << outputStr << endl;
            badUsage = true;
        }
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

    if (badUsage) {
        showUsage();
        return 1;
    }

    try {
        BESDEBUG(MODULE, prolog << "Initializing default module ... " << endl);
        BESDefaultModule::initialize(argc, argv);
        BESDEBUG(MODULE, prolog << "Done initializing default module" << endl);

        BESDEBUG(MODULE, prolog << "Initializing default commands ... " << endl);
        BESXMLDefaultCommands::initialize(argc, argv);
        BESDEBUG(MODULE, prolog << "Done initializing default commands" << endl);

        BESDEBUG(MODULE, prolog << "Initializing loaded modules ... " << endl);
        int retval = BESModuleApp::initialize(argc, argv);
        BESDEBUG(MODULE, prolog << "Done initializing loaded modules" << endl);
        if (retval) return retval;
    }
    catch (BESError &e) {
        cerr << prolog << "Failed to initialize stand alone app. Message : " << e.get_message() << endl;
        return 1;
    }

    BESDEBUG(MODULE, prolog << "Initialized settings:" << endl << *this);

    return 0;
}


/**
 *
 */
int StandAloneApp::run()
{
    StandAloneClient sac;
    string msg;
    try {
        if (_outputStrm) {
            sac.setOutput(_outputStrm, true);
        }
        else {
            sac.setOutput(&cout, false);
        }
        BESDEBUG(MODULE, prolog << "StandAloneClient instance created." << endl);
    }
    catch (BESError &e) {
        msg = prolog + "FAILED to start StandAloneClient instance. Message: " + e.get_message();
        BESDEBUG(MODULE, msg << endl);
        cerr << msg << endl;
        return 1;
    }

    try {
        if (!_cmd.empty()) {
            sac.executeCommands(_cmd, _repeat);
        }
        else if (!_command_file_names.empty()) {
            BESDEBUG(MODULE, prolog << "Found " << _command_file_names.size() << " command files." << endl);
            unsigned int commands = 0;
            for (auto &command_filename: _command_file_names) {
                commands++;
                BESDEBUG(MODULE, prolog << "Processing BES command file: " << command_filename<< endl);
                if (!command_filename.empty()) {
                    ifstream cmdStrm(command_filename);
                    if (!cmdStrm.is_open()) {
                        cerr << prolog << "FAILED to open the input file '" << command_filename << "' SKIPPING." << endl;
                    }
                    else {
                        try {
                            sac.executeCommands(cmdStrm, _repeat);
                            if (commands < _command_file_names.size()) {
                                sac.getOutput() << "\nNext-Response:\n" << flush;
                            }
                        }
                        catch (BESError &e) {
                            cerr << prolog << "Error processing commands. Message: " << e.get_message() << endl;
                        }
                    }
                }
            }
        }
        else {
            sac.interact();
        }
    }
    catch (BESError &e) {
        cerr << prolog << "Error processing commands. Message: " << e.get_message() << endl;
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
    BESDEBUG(MODULE, "ServerApp: terminating loaded modules ... " << endl);
    BESModuleApp::terminate(sig);
    BESDEBUG(MODULE, "ServerApp: done terminating loaded modules" << endl);

    BESDEBUG(MODULE, "ServerApp: terminating default commands ...  " << endl);
    BESXMLDefaultCommands::terminate();
    BESDEBUG(MODULE, "ServerApp: done terminating default commands" << endl);

    BESDEBUG(MODULE, "ServerApp: terminating default module ... " << endl);
    BESDefaultModule::terminate();
    BESDEBUG(MODULE, "ServerApp: done terminating default module" << endl);

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

    strm << BESIndent::LMarg << "command: " << _cmd << endl;
    strm << BESIndent::LMarg << "output stream: " << (void *) _outputStrm << endl;
    if(_command_file_names.empty()){
        strm << BESIndent::LMarg << "No command filenames were identified." << endl;
    }
    else {
        strm << BESIndent::LMarg << "Found " << _command_file_names.size() << " command file names." << endl;
        BESIndent::Indent();
        for (unsigned index = 0; index < _command_file_names.size(); index++) {
            strm << BESIndent::LMarg << "command_filename["<<index<<"]: "<< _command_file_names[index] << endl;
        }
        BESIndent::UnIndent();
    }
    BESApp::dump(strm);
    BESIndent::UnIndent();
}

