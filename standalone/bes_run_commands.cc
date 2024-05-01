
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
#include <sstream>

#include <libxml/parser.h>

#include "BESModuleApp.h"
#include "BESDefaultModule.h"
#include "BESXMLDefaultCommands.h"
#include "TheBESKeys.h"
#include "BESXMLInterface.h"
#include "BESStopWatch.h"

using namespace std;

class StandAloneApp : public BESModuleApp {
private:
    std::vector<std::string> d_command_file_names;
    std::ofstream d_output_strm;
    bool output_file = false;

    static void show_version(const string &app_name) {
        cout << app_name << ": version 1.0\n";
    }

    static void show_usage(const string &app_name) {
        cout << '\n';
        cout << app_name << ": the following options are available:\n";
        cout << "    -c <file>, --config=<file> - BES configuration file\n";
        cout << "    -i <file>, --inputfile=<file> - file with a sequence of input \n";
        cout << "       commands, may be used multiple times.\n";
        cout << "    -f <file>, --outputfile=<file> - write output to this file\n";
        cout << "    -d, --debug - turn on debugging for the client session\n";
        cout << "    -v, --version - return version information\n";
        cout << "    -h,?, --help - display help information\n";
        cout << '\n';

        BESDebug::Help(cout);
    }

public:
    StandAloneApp() = default;

    ~StandAloneApp() override = default;

    int initialize(int argC, char **argV) override;

    int run() override;

    int terminate(int sig = 0) override {
        BESModuleApp::terminate(sig);
        BESXMLDefaultCommands::terminate();
        BESDefaultModule::terminate();

        xmlCleanupParser();

        return sig;
    }

    void run_command(const std::string &cmd);
};

int StandAloneApp::initialize(int argc, char **argv)
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
    while ((c = getopt_long(argc, argv, "c:d:f:i:vh?", longopts, &option_index)) != -1) {
        switch (c) {
            case 'c':
                TheBESKeys::ConfigFile = optarg;
                break;
            case 'd':
                BESDebug::SetUp(optarg);
                break;
            case 'f':
                outputStr = optarg;
                output_file = true;
                break;
            case 'i':
                d_command_file_names.emplace_back(optarg);
                break;
            case 'v':
                show_version(argv[0]);
                exit(0);

            default:    // ? and h
                show_usage(argv[0]);
                exit(0);
        }
    }

    bool badUsage = false;
    if (d_command_file_names.empty()) {
        cerr << "You must either specify an input file\n";
        badUsage = true;
    }

    if (!outputStr.empty()) {
        d_output_strm.open(outputStr, ios::out | ios::trunc);
        if (!d_output_strm.is_open()) {
            cerr << "could not open the output file " << outputStr << endl;
            badUsage = true;
        }
    }

    if (badUsage) {
        show_usage(argv[0]);
        return 1;
    }

    try {
        BESDefaultModule::initialize(argc, argv);
        BESXMLDefaultCommands::initialize(argc, argv);
        int retval = BESModuleApp::initialize(argc, argv);
        if (retval) return retval;
    }
    catch (BESError &e) {
        cerr << "Failed to initialize stand alone app. Message: " << e.get_message() << '\n';
        return 1;
    }

    return 0;
}

void StandAloneApp::run_command(const std::string &cmd)
{
    BESStopWatch sw;
    if (BESDebug::IsSet(TIMING_LOG_KEY)) sw.start("StandAloneApp::run_command");

    ostream &o_strm = d_output_strm.is_open() ? d_output_strm : cout;

    BESXMLInterface interface(cmd, &o_strm);

    int status = interface.execute_request("standalone");

    o_strm << flush;

    // Put the call to finish() here because we're not sending chunked responses back
    // to a client over PPT. In the BESServerHandler.cc code, we must do that and hence,
    // break up the call to finish() for the error and no-error cases.
    status = interface.finish(status);

    if (status != 0) {
        // an error has occurred.
        switch (status) {
            case BES_INTERNAL_FATAL_ERROR: {
                cerr << "Status not OK, dispatcher returned value " << status << '\n';
                exit(1);
            }

            case BES_INTERNAL_ERROR:
            case BES_SYNTAX_USER_ERROR:
            case BES_FORBIDDEN_ERROR:
            case BES_NOT_FOUND_ERROR: {
                cerr << "Status not OK, dispatcher returned value " << status << ", continuing\n";
                break;
            }

            default:
                break;
        }
    }
}

int StandAloneApp::run()
{
    try {
        ostream &o_strm = d_output_strm.is_open() ? d_output_strm : cout;
        unsigned int commands = 0;
        for (auto const &command_filename: d_command_file_names) {
            commands++;
            ifstream cmd_strm(command_filename);
            if (!cmd_strm.is_open())
                cerr << "FAILED to open the input file '" << command_filename << "' SKIPPING.\n";
            else {
                std::string cmd;

                // Allocate string memory up front
                cmd_strm.seekg(0, std::ios::end);
                cmd.reserve(cmd_strm.tellg());
                cmd_strm.seekg(0, std::ios::beg);

                // Read the file into the string. Note 'extra' parentheses around the
                // std::istreambuf_iterator<char> constructor. They are mandatory.
                // They prevent the problem known as the "most vexing parse", which
                // in this case won't actually give you a compile error like it usually
                // does, but will give you interesting (read: wrong) results. See Meyers'
                // 'Most vexing parse.'
                cmd.assign((std::istreambuf_iterator<char>(cmd_strm)),
                           std::istreambuf_iterator<char>());

                run_command(cmd);
                if (commands < d_command_file_names.size()) {
                    o_strm << "\nNext-Response:\n" << flush;
                }
            }
        }
    }
    catch (const BESError &e) {
        cerr << "Error processing commands. Message: " << e.get_message() << '\n';
    }

    return 0;
}

int main(int argc, char **argv)
{
    try {
        StandAloneApp app;
        if (app.initialize(argc, argv) == 0)
            return app.run();
        else {
            cerr << "Failed to initialize the command processor.\n";
            return 1;
        }
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
