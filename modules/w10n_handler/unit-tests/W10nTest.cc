// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Author: Nathan David Potter <ndp@opendap.org>
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

#include <cstdio>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include <fstream>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include "GetOpt.h"

#include "BESDebug.h"
#include "BESInternalError.h"
#include "BESForbiddenError.h"
#include "BESNotFoundError.h"
#include "util.h"

#include "test_config.h"
#include "w10n_utils.h"

static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

class W10nTest: public CppUnit::TestFixture {

private:
    string d_tmpDir;
    string d_testDir;

    void eval_w10n_id(string resourceId, string catalogRoot, string expectedValidPath, string expectedRemainder,
        bool follow_sym_links)
    {
        string validPath;
        string remainder;
        bool isDir, isFile;
        DBG(cerr << "eval_w10n_id() - ##########################################################" << endl);
        DBG(cerr << "eval_w10n_id() - ResourceId:         " << resourceId << endl);
        DBG(cerr << "eval_w10n_id() - expectedValidPath:  " << expectedValidPath << endl);
        DBG(cerr << "eval_w10n_id() - expectedRemainder:  " << expectedRemainder << endl);
        DBG(cerr << "eval_w10n_id() - follow_sym_links:   " << (follow_sym_links ? "true" : "false") << endl);
        w10n::eval_resource_path(resourceId, catalogRoot, follow_sym_links, validPath, isFile, isDir, remainder);
        DBG(cerr << "eval_w10n_id() - Returned validPath: " << validPath << endl);
        DBG(cerr << "eval_w10n_id() - Returned remainder: " << remainder << endl);
        CPPUNIT_ASSERT(validPath == expectedValidPath);
        CPPUNIT_ASSERT(remainder == expectedRemainder);
    }

public:

    // Called once before everything gets tested
    W10nTest() :
        // both tmp and testdir are generated and thus in the build dir. jhrg 1/23/18
        d_tmpDir(string(TEST_BUILD_DIR) + "/tmp"), d_testDir(string(TEST_BUILD_DIR) + "/testdir")
    {
    }

    // Called at the end of the test
    ~W10nTest()
    {
        // DBG(cerr << "W10nTest - Destructor" << endl);
    }

    // Called before each test
    void setUp()
    {
        DBG(cerr << endl);
        if (bes_debug) BESDebug::SetUp("cerr,all");

        DBG(cerr << "W10nTest::setUp() - d_tmpDir:" << d_tmpDir << endl);
        DBG(cerr << "W10nTest::setUp() - d_testDir:" << d_testDir << endl);

    }

    // Called after each test
    void tearDown()
    {
        DBG(cerr << "W10nTest::tearDown()" << endl);
    }

CPPUNIT_TEST_SUITE( W10nTest );

    CPPUNIT_TEST(eval_w10_path_to_directory);
    CPPUNIT_TEST(eval_w10_path_to_file);
    CPPUNIT_TEST(eval_w10_path_to_file_with_variable);
    CPPUNIT_TEST(eval_w10_path_to_linked_file_with_variable);
    CPPUNIT_TEST(eval_w10_path_to_forbidden_linked_file_with_variable);
    CPPUNIT_TEST(eval_w10_path_to_bad_linked_file_with_variable);
#if 0
    CPPUNIT_TEST(eval_w10_path_to_forbidden_bad_linked_file_with_variable);
#endif
    CPPUNIT_TEST(eval_w10_forbidden_up_traversal_path);

    CPPUNIT_TEST_SUITE_END()
    ;

    void eval_w10_path_to_directory()
    {
        string w10nResourceId = "/nc/";
        string expectedPath = "/nc";
        string expectedRemainder = "";
        eval_w10n_id(w10nResourceId, d_testDir, expectedPath, expectedRemainder, true);
    }

    void eval_w10_path_to_file()
    {
        string w10nResourceId = "/nc/testfile.txt";
        string expectedPath = "/nc/testfile.txt";
        string expectedRemainder = "";
        eval_w10n_id(w10nResourceId, d_testDir, expectedPath, expectedRemainder, true);
    }

    void eval_w10_path_to_file_with_variable()
    {
        string w10nResourceId = "/nc/testfile.txt/sst/lat";
        string expectedPath = "/nc/testfile.txt";
        string expectedRemainder = "sst/lat";
        eval_w10n_id(w10nResourceId, d_testDir, expectedPath, expectedRemainder, true);
    }

    void eval_w10_path_to_linked_file_with_variable()
    {
        string w10nResourceId = "/link_to_nc/link_to_testfile.txt/sst/";
        string expectedPath = "/link_to_nc/link_to_testfile.txt";
        string expectedRemainder = "sst";
        eval_w10n_id(w10nResourceId, d_testDir, expectedPath, expectedRemainder, true);
    }

    void eval_w10_path_to_forbidden_linked_file_with_variable()
    {
        string w10nResourceId = "/link_to_nc/link_to_testfile.txt/sst/";
        string expectedPath = "";
        string expectedRemainder = "";
        try {
            eval_w10n_id(w10nResourceId, d_testDir, expectedPath, expectedRemainder, false);
        }
        catch (BESForbiddenError &e) {
            DBG(
                cerr << "test_path_eval() - Caught expected BESForbiddenError. Message: '" << e.get_message() << "'"
                    << endl);
            CPPUNIT_ASSERT(true);
        }
    }

    void eval_w10_path_to_bad_linked_file_with_variable()
    {
        string w10nResourceId = "/nc/bad_link/sst/";
        string expectedPath = "/nc";
        string expectedRemainder = "bad_link/sst";
        eval_w10n_id(w10nResourceId, d_testDir, expectedPath, expectedRemainder, true);
    }

#if 0
    void eval_w10_path_to_forbidden_bad_linked_file_with_variable()
    {
        string w10nResourceId = "/nc/bad_link/sst/";
        string expectedPath = "";
        string expectedRemainder = "";
        try {
            eval_w10n_id(w10nResourceId, d_testDir, expectedPath, expectedRemainder, false);
        }
        catch (BESForbiddenError &e) {
            DBG(
                cerr << "test_path_eval() - Caught expected BESForbiddenError. Message: '" << e.get_message() << "'"
                    << endl);
            CPPUNIT_ASSERT(true);
        }
    }
#endif

    void eval_w10_forbidden_up_traversal_path()
    {
        string w10nResourceId = "/nc/../../../sst/";
        string expectedPath = "";
        string expectedRemainder = "";
        try {
            eval_w10n_id(w10nResourceId, d_testDir, expectedPath, expectedRemainder, true);
        }
        catch (BESForbiddenError &e) {
            DBG(
                cerr << "test_path_eval() - Caught expected BESForbiddenError. Message: '" << e.get_message() << "'"
                    << endl);
            CPPUNIT_ASSERT(true);
        }

    }

};

CPPUNIT_TEST_SUITE_REGISTRATION(W10nTest);

int main(int argc, char*argv[])
{

    GetOpt getopt(argc, argv, "dbh");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = true;  // debug is a static global
            cerr << "##### DEBUG is ON" << endl;
            break;
        case 'b':
            bes_debug = true;  // bes_debug is a static global
            cerr << "##### BES DEBUG is ON" << endl;
            break;
        case 'h': {     // help - show test names
            std::cerr << "Usage: W10nTest has the following tests:" << std::endl;
            const std::vector<CppUnit::Test*> &tests = W10nTest::suite()->getTests();
            unsigned int prefix_len = W10nTest::suite()->getName().append("::").length();
            for (std::vector<CppUnit::Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                std::cerr << (*i)->getName().replace(0, prefix_len, "") << std::endl;
            }
            break;
        }
        default:
            cerr << "##### DEBUG is OFF" << endl;
            break;
        }

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    string test = "";
    int i = getopt.optind;
    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = W10nTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}

