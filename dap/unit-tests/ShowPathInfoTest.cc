// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2017 OPeNDAP, Inc.
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

#include <GetOpt.h>
#include <util.h>

#include "BESDebug.h"
#include "BESInternalError.h"
#include "BESForbiddenError.h"
#include "BESNotFoundError.h"
#include "ShowPathInfoResponseHandler.h"

#include "test_config.h"

static bool debug = false;
static bool bes_debug = false;

using std::cerr;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

class ShowPathInfoTest: public CppUnit::TestFixture {

private:
    string d_tmpDir;
    string d_testDir;

    void eval_resource_path(string resourceId, string catalogRoot, string expectedValidPath,
        string expectedRemainder, bool follow_sym_links)
    {
        ShowPathInfoResponseHandler spirh("ShowPathInfoResponseHandler-Unit-Test");

        string validPath;
        string remainder;
        long long size, time;
        bool isDir, isFile, canRead;
        DBG(cerr << __func__ << "() - ##########################################################" << endl);
        DBG(cerr << __func__ << "() - catalogRoot:         " << catalogRoot << endl);
        DBG(cerr << __func__ << "() - ResourceId:         " << resourceId << endl);
        DBG(cerr << __func__ << "() - expectedValidPath:  " << expectedValidPath << endl);
        DBG(cerr << __func__ << "() - expectedRemainder:  " << expectedRemainder << endl);
        DBG(cerr << __func__ << "() - follow_sym_links:   " << (follow_sym_links ? "true" : "false") << endl);

        spirh.eval_resource_path(
            resourceId,
            catalogRoot,
            follow_sym_links,
            validPath,
            isFile,
            isDir,
            size,
            time,
            canRead,
            remainder);

        DBG(cerr << __func__ << "() - Returned validPath: " << validPath << endl);
        DBG(cerr << __func__ << "() - isDir:              " << (isDir ? "true" : "false") << endl);
        DBG(cerr << __func__ << "() - isFile:             " << (isFile ? "true" : "false") << endl);
        DBG(cerr << __func__ << "() - canRead:            " << (canRead ? "true" : "false") << endl);
        DBG(cerr << __func__ << "() - Returned size:      " << size << endl);
        DBG(cerr << __func__ << "() - Returned lmt:       " << time << endl);
        DBG(cerr << __func__ << "() - Returned remainder: " << remainder << endl);
        CPPUNIT_ASSERT(validPath == expectedValidPath);
        CPPUNIT_ASSERT(remainder == expectedRemainder);
    }
    void eval_resource_path(
        const string &resource_path,
        const string &catalog_root,
        const bool follow_sym_links,
        string &validPath,
        bool &isFile,
        bool &isDir,
        long long &size,
        long long &lastModifiedTime,
        bool &canRead,
        string &remainder);

public:

    // Called once before everything gets tested
    ShowPathInfoTest() :
        d_tmpDir(string(TEST_SRC_DIR) + "/tmp"), d_testDir(string(TEST_SRC_DIR) + "/input-files")
    {
    }

    // Called at the end of the test
    ~ShowPathInfoTest()
    {
        // DBG(cerr << "ShowPathInfoTest - Destructor" << endl);
    }

    // Called before each test
    void setUp()
    {
        DBG(cerr << endl);
        if (bes_debug) BESDebug::SetUp("cerr,all");

        DBG(cerr << "ShowPathInfoTest::setUp() - d_tmpDir:" << d_tmpDir << endl);
        DBG(cerr << "ShowPathInfoTest::setUp() - d_testDir:" << d_testDir << endl);

    }

    // Called after each test
    void tearDown()
    {
        DBG(cerr << "ShowPathInfoTest::tearDown()" << endl);
    }

CPPUNIT_TEST_SUITE( ShowPathInfoTest );

    CPPUNIT_TEST(eval_resource_path_to_directory);
    CPPUNIT_TEST(eval_resource_path_to_file);
    CPPUNIT_TEST(eval_resource_path_to_file_with_variable);
    CPPUNIT_TEST(eval_resource_path_to_linked_file_with_variable);
    CPPUNIT_TEST(eval_resource_path_to_forbidden_linked_file_with_variable);
    CPPUNIT_TEST(eval_resource_path_to_bad_linked_file_with_variable);
#if 0
    CPPUNIT_TEST(eval_resource_path_to_forbidden_bad_linked_file_with_variable);
#endif
    CPPUNIT_TEST(eval_resource_path_forbidden_up_traversal);
    CPPUNIT_TEST(eval_resource_path_to_file_with_a_dap_suffix);
    CPPUNIT_TEST(eval_resource_path_to_file_with_a_dap_suffix_and_path);

    CPPUNIT_TEST_SUITE_END()
    ;

    void eval_resource_path_to_directory()
    {
        string resourceId = "/nc/";
        string expectedPath = "/nc";
        string expectedRemainder = "";
        eval_resource_path(resourceId, d_testDir, expectedPath, expectedRemainder, true);
    }

    void eval_resource_path_to_file()
    {
        string resourceId = "/nc/testfile.txt";
        string expectedPath = "/nc/testfile.txt";
        string expectedRemainder = "";
        eval_resource_path(resourceId, d_testDir, expectedPath, expectedRemainder, true);
    }

    void eval_resource_path_to_file_with_variable()
    {
        string resourceId = "/nc/testfile.txt/sst/lat";
        string expectedPath = "/nc/testfile.txt";
        string expectedRemainder = "sst/lat";
        eval_resource_path(resourceId, d_testDir, expectedPath, expectedRemainder, true);
    }

    void eval_resource_path_to_file_with_a_dap_suffix()
    {
        string resourceId = "/nc/testfile.txt.dmr.xml";
        string expectedPath = "/nc/testfile.txt";
        string expectedRemainder = ".dmr.xml";
        eval_resource_path(resourceId, d_testDir, expectedPath, expectedRemainder, true);
    }

    void eval_resource_path_to_file_with_a_dap_suffix_and_path()
    {
        string resourceId = "/nc/testfile.txt.dmr.xml/look/nothing/here";
        string expectedPath = "/nc";
        string expectedRemainder = "testfile.txt.dmr.xml/look/nothing/here";
        eval_resource_path(resourceId, d_testDir, expectedPath, expectedRemainder, true);
    }

    void eval_resource_path_to_linked_file_with_variable()
    {
        string resourceId = "/link_to_nc/link_to_testfile.txt/sst/";
        string expectedPath = "/link_to_nc/link_to_testfile.txt";
        string expectedRemainder = "sst/";
        eval_resource_path(resourceId, d_testDir, expectedPath, expectedRemainder, true);
    }

    void eval_resource_path_to_forbidden_linked_file_with_variable()
    {
        string resourceId = "/link_to_nc/link_to_testfile.txt/sst/";
        string expectedPath = "";
        string expectedRemainder = "";
        try {
            eval_resource_path(resourceId, d_testDir, expectedPath, expectedRemainder, false);
        }
        catch (BESForbiddenError &e) {
            DBG(
                cerr << "test_path_eval() - Caught expected BESForbiddenError. Message: '" << e.get_message() << "'"
                    << endl);
            CPPUNIT_ASSERT(true);
        }
    }

    void eval_resource_path_to_bad_linked_file_with_variable()
    {
        string resourceId = "/nc/bad_link/sst/";
        string expectedPath = "/nc";
        string expectedRemainder = "bad_link/sst/";
        eval_resource_path(resourceId, d_testDir, expectedPath, expectedRemainder, true);
    }

    void eval_resource_path_to_forbidden_bad_linked_file_with_variable()
    {
        string resourceId = "/nc/bad_link/sst/";
        string expectedPath = "";
        string expectedRemainder = "";
        try {
            eval_resource_path(resourceId, d_testDir, expectedPath, expectedRemainder, false);
        }
        catch (BESForbiddenError &e) {
            DBG(
                cerr << "test_path_eval() - Caught expected BESForbiddenError. Message: '" << e.get_message() << "'"
                    << endl);
            CPPUNIT_ASSERT(true);
        }
    }
    void eval_resource_path_forbidden_up_traversal()
    {
        string resourceId = "/nc/../../../sst/";
        string expectedPath = "";
        string expectedRemainder = "";
        try {
            eval_resource_path(resourceId, d_testDir, expectedPath, expectedRemainder, true);
        }
        catch (BESForbiddenError &e) {
            DBG(
                cerr << "test_path_eval() - Caught expected BESForbiddenError. Message: '" << e.get_message() << "'"
                    << endl);
            CPPUNIT_ASSERT(true);
        }

    }

};

CPPUNIT_TEST_SUITE_REGISTRATION(ShowPathInfoTest);

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
            const std::vector<CppUnit::Test*> &tests = ShowPathInfoTest::suite()->getTests();
            unsigned int prefix_len = ShowPathInfoTest::suite()->getName().append("::").length();
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
            test = ShowPathInfoTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            i++;
        }
    }

    return wasSuccessful ? 0 : 1;
}

