//
// Created by James Gallagher on 2/25/22.
//

// This file is part of the BES.

// Copyright (c) 2022 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

/**
 * This file contains a templated function that serves as the body of main()
 * for a CppUnit-based unit test.
 *
 * @note How to use this:
 *     int main(int argc, char *argv[]) {
 *         return bes_run_tests<http::CurlUtilsTest>(argc, argv, "bes,http,curl") ? 0: 1;
 *     }
 * This main() supports -d, -D, -b and -h options (debug, debug2 and bes_debug).
 *
 * @note This is a WIP now but is intended to cut down on duplicated code in both
 * the BES and libdap4 software repositories.
 */

#ifndef HYRAX_GIT_RUN_TESTS_CPPUNIT_H
#define HYRAX_GIT_RUN_TESTS_CPPUNIT_H

#include <fstream>
#include <string>
#include <iterator>

#include <unistd.h>
#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include "BESDebug.h"

bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false)

bool debug2 = false;

#undef DBG2
#define DBG2(x) do { if (debug2) (x); } while(false)

void show_file(const std::string &filename)
{
    std::ifstream t(filename);

    if (t.is_open()) {
        std::string file_content((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        std::cout << "##################################################################" << std::endl;
        std::cout << "file: " << filename << std::endl;
        std::cout << ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << std::endl;
        std::cout << file_content << std::endl;
        std::cout << ". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . " << std::endl;
    }
}

/**
 * @brief Run the test(s)
 *
 * @note How to use this:
 * int main(int argc, char*argv[]) {
 *     return bes_run_tests<bes::TemporaryFileTest>(argc, argv, "dDh") ? 0 : 1;
 * }
 *
 * @tparam CLASS The CppUnit test class to run/test
 * @param argc The argc value passed to main
 * @param argv The command line parameters passed to main()
 * @return True if the test(s) passed, false otherwise.
 */
template<class CLASS>
bool bes_run_tests(int argc, char *argv[], const std::string &besdebug_contexts)
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    int option_char;
    while ((option_char = getopt(argc, argv, "dDbh")) != -1) {
        switch (option_char) {
            case 'd':
                debug = true;  // debug is a global
                break;
            case 'D':
                debug2 = true;  // debug2 is a global
                break;
            case 'b':
                BESDebug::SetUp(besdebug_contexts);
                break;
            case 'h': {     // help - show test names
                std::cerr << "Usage: the following tests can be run individually or in combination:" << std::endl;
                auto &tests = CLASS::suite()->getTests();
                unsigned int prefix_len = CLASS::suite()->getName().append("::").size();
                for (auto &t: tests) {
                    std::cerr << t->getName().replace(0, prefix_len, "") << std::endl;
                }
                exit(EXIT_SUCCESS);
            }
            default:
                break;
        }
    }

    argc -= optind;
    argv += optind;

    if (0 == argc) { // run them all
        return runner.run("");
    }
    else {
        bool wasSuccessful = true;
        int i = 0;
        while (i < argc) {
            std::string test = CLASS::suite()->getName().append("::").append(argv[i++]);
            if (debug) std::cerr << "Running " << test << std::endl;
            wasSuccessful = wasSuccessful && runner.run(test);
        }
        return wasSuccessful;
    }
}

#endif //HYRAX_GIT_RUN_TESTS_CPPUNIT_H
