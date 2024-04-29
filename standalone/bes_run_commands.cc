
// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2020 University Corporation for Atmospheric Research
// Author: Nathan Potter <ndp@opendap.org>
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

#include "config.h"

#include <iostream>
#include <fstream>
#include <vector>

#include <cstdlib>
#include <getopt.h>
#include <fstream>
#include <sstream>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "BESModuleApp.h"
#include "BESError.h"

using namespace std;

class StandAloneApp : public BESModuleApp {
private:
    StandAloneClient *_client {nullptr};
    std::vector<std::string> _command_file_names;
    std::string _cmd;
    std::ofstream *_outputStrm {nullptr};
    int _repeat {0};

    void showVersion();

    void showUsage();

public:
    StandAloneApp() = default;

    ~StandAloneApp() override = default;

    int initialize(int argC, char **argV) override;

    int run() override;

    int terminate(int sig = 0) override;

    void dump(std::ostream &strm) const override;

    StandAloneClient *client() { return _client; }
};

void show_version()
{
    cout << "bes_run_commands: version 1.0\n";
}

void show_usage()
{
    cout << '\n';
    cout << appName() << ": the following options are available:\n";
    cout << "    -c <file>, --config=<file> - BES configuration file\n";
    cout << "    -x <command>, --execute=<command> - command for the server \n";
    cout << "       to execute\n";
    cout << "    -i <file>, --inputfile=<file> - file with a sequence of input \n";
    cout << "       commands, may be used multiple times.\n";
    cout << "    -f <file>, --outputfile=<file> - write output to this file\n";
    cout << "    -d, --debug - turn on debugging for the client session\n";
    cout << "    -r <num>, --repeat=<num> - repeat the command(s) <num> times\n";
    cout << "    -v, --version - return version information\n";
    cout << "    -?, --help - display help information\n";
    cout << '\n';
    cout << "Note: You may provide a bes command with -x OR you may provide one \n";
    cout << "      or more BES command file names. One or the other, not neither, not both.\n";
    cout << '\n';

    BESDebug::Help(cout);
}

int initialize(int argc, char **argv)
{
    static struct option longopts[] = {
            {     "config", 1, 0, 'c' },
            {      "debug", 0, 0, 'd' },
            {    "version", 0, 0, 'v' },
            { "outpu-tfile", 1, 0, 'f' },
            {  "input-file", 1, 0, 'i' },
            {       "help", 0, 0, '?' },
            {            0, 0, 0,  0  }
    };

    string outputStr;
    string repeatStr;
    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "?vc:d:f:i:", longopts, &option_index)) != -1) {
        switch (c) {
            case 'c':
                TheBESKeys::ConfigFile = optarg;
                break;
            case 'd':
                BESDebug::SetUp(optarg);
                break;
            case 'f':
                outputStr = optarg;
                break;
            case 'i':
                _command_file_names.emplace_back(optarg);
                break;
            case 'v':
                show_version();
                exit(0);
            case '?':
            default:
                show_usage();
                exit(0);
        }
    }

    bool badUsage = false;
    if (_command_file_names.empty()) {
            cerr << "You must either specify an input file\n";
            badUsage = true;
        }

    if (!outputStr.empty()) {
        _outputStrm = new ofstream(outputStr.c_str());
        if (!(*_outputStrm)) {
            cerr << "could not open the output file " << outputStr << endl;
            badUsage = true;
        }
    }

    if (badUsage) {
        showUsage();
        return 1;
    }

    try {
        BESDefaultModule::initialize(argc, argv);
        BESXMLDefaultCommands::initialize(argc, argv);
        int retval = BESModuleApp::initialize(argc, argv);
        if (retval) return retval;
    }
    catch (BESError &e) {
        cerr << prolog << "Failed to initialize stand alone app. Message : " << e.get_message() << '\n';
        return 1;
    }

    return 0;
}


/**
 *
 */
int run()
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
        BESDEBUG(MODULE, prolog << "StandAloneClient instance created.\n");
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
            BESDEBUG(MODULE, prolog << "Found " << _command_file_names.size() << " command files.\n");
            unsigned int commands = 0;
            for (auto &command_filename: _command_file_names) {
                commands++;
                BESDEBUG(MODULE, prolog << "Processing BES command file: " << command_filename << '\n');
                if (!command_filename.empty()) {
                    ifstream cmdStrm(command_filename);
                    if (!cmdStrm.is_open()) {
                        cerr << prolog << "FAILED to open the input file '" << command_filename << "' SKIPPING.\n";
                    }
                    else {
                        try {
                            sac.executeCommands(cmdStrm, _repeat);
                            if (commands < _command_file_names.size()) {
                                sac.getOutput() << "\nNext-Response:\n" << flush;
                            }
                        }
                        catch (BESError &e) {
                            cerr << prolog << "Error processing commands. Message: " << e.get_message() << '\n';
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
        cerr << prolog << "Error processing commands. Message: " << e.get_message() << '\n';
    }

    return 0;
}

/** @brief clean up after the application
 *
 * @param sig return value from run that should be returned from method
 * @returns signal or return value passed to the terminate method
 */
int terminate(int sig)
{
    BESModuleApp::terminate(sig);
    BESXMLDefaultCommands::terminate();
    BESDefaultModule::terminate();

    xmlCleanupParser();

    return sig;
}

int main(int argc, char **argv)
{
    try {
        return main(argc, argv);
    }
    catch (const BESError &e) {
        std::cerr << "Caught BES Error while starting the command processor: " << e.get_message() << '\n';
        return 1;
    }
    catch (const std::exception &e) {
        std::cerr << "Caught C++ error while starting the command processor: " << e.what() << '\n';
        return 2;
    }
    catch (...) {
        std::cerr << "Caught unknown error while starting the command processor.\n";
        return 3;
    }
}
