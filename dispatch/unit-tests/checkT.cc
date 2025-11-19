// checkT.C

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

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include "config.h"

// #include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cerrno>
// #include <cstdlib>
#include <iostream>
// #include <fstream>

#include "BESUtil.h"
// #include "BESError.h"
#include "BESForbiddenError.h"
#include "BESNotFoundError.h"
#include "TheBESKeys.h"
#include "test_config.h"

using namespace std;
using namespace CppUnit;

static bool debug = false;
static bool debug2 = false;

#undef DBG
#define DBG(x)                                                                                                         \
    do {                                                                                                               \
        if (debug)                                                                                                     \
            (x);                                                                                                       \
    } while (false)

#undef DBG2
#define DBG2(x)                                                                                                        \
    do {                                                                                                               \
        if (debug2)                                                                                                    \
            (x);                                                                                                       \
    } while (false)

class checkT : public TestFixture {

public:
    checkT() = default;

    virtual ~checkT() = default;

    virtual void setUp() {
        string bes_conf = (string)TEST_BUILD_DIR + "/bes.conf";
        TheBESKeys::ConfigFile = bes_conf;

        DBG2(cerr << "*****************************************" << endl);
        DBG2(cerr << "create the desired directory structure" << endl);
        // testdir
        // testdir/nc
        // testdir/link_to_nc -> testdir/nc
        // testdir/nc/testfile.nc
        // testdir/nc/link_to_testfile.nc -> testdir/nc/testfile.nc
        DBG2(cerr << "    creating ./testdir/" << endl);
        int ret = mkdir("./testdir", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        int myerrno = errno;
        CPPUNIT_ASSERT(ret == 0 || myerrno == EEXIST);

        DBG2(cerr << "    creating ./testdir/nc/" << endl);
        ret = mkdir("./testdir/nc", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        myerrno = errno;
        CPPUNIT_ASSERT(ret == 0 || myerrno == EEXIST);

        DBG2(cerr << "    creating ./testdir/nc/testfile.nc" << endl);
        FILE *fp = fopen("./testdir/nc/testfile.nc", "w+");
        CPPUNIT_ASSERT(fp);

        DBG2(cerr << "    creating symlink ./testdir/nc/link_to_nc" << endl);
        fprintf(fp, "This is a test file");
        fclose(fp);
        ret = symlink("./nc", "./testdir/link_to_nc");
        myerrno = errno;
        CPPUNIT_ASSERT(ret == 0 || myerrno == EEXIST);

        DBG2(cerr << "    creating symlink ./testdir/nc/link_to_testfile.nc" << endl);
        ret = symlink("./testfile.nc", "./testdir/nc/link_to_testfile.nc");
        myerrno = errno;
        CPPUNIT_ASSERT(ret == 0 || myerrno == EEXIST);
    }

    CPPUNIT_TEST_SUITE(checkT);

    CPPUNIT_TEST(test_slash_path);
    CPPUNIT_TEST(test_slash_path_no_sym_links);
    CPPUNIT_TEST(test_empty_path);
    CPPUNIT_TEST(test_empty_path_no_sym_links);

    CPPUNIT_TEST(test_slash_root);
    CPPUNIT_TEST(test_empty_root);
    CPPUNIT_TEST(test_slash_root_no_sym_links);
    CPPUNIT_TEST(test_empty_root_no_sym_links);

    CPPUNIT_TEST(test_one_dir);
    CPPUNIT_TEST(test_two_dirs);
    CPPUNIT_TEST(test_file);
    CPPUNIT_TEST(test_file_abs_root);
    CPPUNIT_TEST(test_sym_link_when_allowed);
    CPPUNIT_TEST(test_sym_link_to_file_allowed);
    CPPUNIT_TEST(test_sym_link_dir_not_allowed);
    CPPUNIT_TEST(test_dirs_sym_link_not_allowed);
    CPPUNIT_TEST(test_file_sym_link_not_allowed);
    CPPUNIT_TEST(test_sym_link_sym_links_not_allowed);
    CPPUNIT_TEST(test_link_to_file_in_sym_link_dir_sym_links_not_allowed);
    CPPUNIT_TEST(test_file_in_sym_link_dir_sym_links_not_allowed);
    CPPUNIT_TEST(test_link_to_file_syn_link_not_allowed);
    CPPUNIT_TEST(test_no_dir_syn_link_allowed_1);
    CPPUNIT_TEST(test_no_dir_syn_link_not_allowed);
    CPPUNIT_TEST(test_no_dir_syn_link_allowed_2);
    CPPUNIT_TEST(test_no_file_syn_link_allowed);
    CPPUNIT_TEST(test_no_file_linked_dir_syn_link_allowed);
    CPPUNIT_TEST(test_no_file_linked_dir_syn_link_not_allowed);

    CPPUNIT_TEST_SUITE_END();

    // NB: void BESUtil::check_path(const string &path, const string &root, bool follow_sym_links)
    // check_path() throws when the root + path item does not exist or is a sym link if follow_sym_links
    // is false. Otherwise, it simply returns.

    void test_slash_path() {
        DBG(cerr << "checking /" << endl);
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("checking /", BESUtil::check_path("/", "./", true));
    }

    void test_slash_path_no_sym_links() {
        DBG(cerr << "checking /, follow_sym_links == false" << endl);
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("checking /, follow_sym_links == false", BESUtil::check_path("/", "./", false));
    }

    void test_empty_path() {
        DBG(cerr << "checking /" << endl);
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("checking /", BESUtil::check_path("", "./", true));
    }

    void test_empty_path_no_sym_links() {
        DBG(cerr << "checking /, follow_sym_links == false" << endl);
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("checking /, follow_sym_links == false", BESUtil::check_path("", "./", false));
    }

    void test_slash_root() {
        DBG(cerr << "checking /" << endl);
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("checking /", BESUtil::check_path("/", TEST_BUILD_DIR, true));
    }

    void test_empty_root() {
        DBG(cerr << "checking /" << endl);
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("checking /", BESUtil::check_path("", TEST_BUILD_DIR, true));
    }

    void test_slash_root_no_sym_links() {
        DBG(cerr << "checking /" << endl);
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("checking /", BESUtil::check_path("/", TEST_BUILD_DIR, false));
    }

    void test_empty_root_no_sym_links() {
        DBG(cerr << "checking /" << endl);
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("checking /", BESUtil::check_path("", TEST_BUILD_DIR, false));
    }

    void test_one_dir() {
        DBG(cerr << "checking /testdir/" << endl);
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("checking /testdir/", BESUtil::check_path("/testdir/", "./", true));
    }

    void test_two_dirs() {
        DBG(cerr << "checking /testdir/nc/" << endl);
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("checking /testdir/nc/", BESUtil::check_path("/testdir/nc/", "./", true));
    }

    void test_file() {
        DBG(cerr << "checking /testdir/nc/testfile.nc" << endl);
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("checking /testdir/nc/testfile.nc",
                                        BESUtil::check_path("/testdir/nc/testfile.nc", "./", true));
    }

    void test_file_abs_root() {
        DBG(cerr << "checking /testdir/nc/testfile.nc" << endl);
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("checking /testdir/nc/testfile.nc",
                                        BESUtil::check_path("/testdir/nc/testfile.nc", TEST_BUILD_DIR, true));
    }

    void test_sym_link_when_allowed() {
        DBG(cerr << "checking /testdir/link_to_nc/" << endl);
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("checking /testdir/link_to_nc/",
                                        BESUtil::check_path("/testdir/link_to_nc/", "./", true));
    }

    void test_sym_link_to_file_allowed() {
        DBG(cerr << "checking /testdir/link_to_nc/link_to_testfile.nc" << endl);
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("checking /testdir/link_to_nc/link_to_testfile.nc",
                                        BESUtil::check_path("/testdir/link_to_nc/link_to_testfile.nc", "./", true));
    }

    void test_sym_link_dir_not_allowed() {
        DBG(cerr << "checking /testdir/, follow syms = false" << endl);
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("checking /testdir/, follow syms = false",
                                        BESUtil::check_path("/testdir/", "./", false));
    }

    void test_dirs_sym_link_not_allowed() {
        DBG(cerr << "checking /testdir/nc/, follow syms = false" << endl);
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("checking /testdir/nc/, follow syms = false",
                                        BESUtil::check_path("/testdir/nc/", "./", false));
    }

    void test_file_sym_link_not_allowed() {
        DBG(cerr << "checking /testdir/nc/testfile.nc, follow syms = false" << endl);
        CPPUNIT_ASSERT_NO_THROW_MESSAGE("checking /testdir/nc/testfile.nc, follow syms = false",
                                        BESUtil::check_path("/testdir/nc/testfile.nc", "./", false));
    }

    void test_sym_link_sym_links_not_allowed() {
        DBG(cerr << "checking /testdir/link_to_nc/, follow_syms = false" << endl);
        CPPUNIT_ASSERT_THROW_MESSAGE("checking /testdir/link_to_nc/, follow_syms = false",
                                     BESUtil::check_path("/testdir/link_to_nc/", "./", false), BESForbiddenError);
    }

    void test_link_to_file_in_sym_link_dir_sym_links_not_allowed() {
        DBG(cerr << "checking /testdir/link_to_nc/link_to_testfile.nc, " << "follow syms = false" << endl);
        CPPUNIT_ASSERT_THROW_MESSAGE("checking /testdir/link_to_nc/link_to_testfile.nc, follow syms = false",
                                     BESUtil::check_path("/testdir/link_to_nc/link_to_testfile.nc", "./", false),
                                     BESForbiddenError);
    }

    void test_file_in_sym_link_dir_sym_links_not_allowed() {
        DBG(cerr << "checking /testdir/link_to_nc/testfile.nc, " << "follow syms = false" << endl);
        CPPUNIT_ASSERT_THROW_MESSAGE("checking /testdir/link_to_nc/testfile.nc, follow syms = false",
                                     BESUtil::check_path("/testdir/link_to_nc/testfile.nc", "./", false),
                                     BESForbiddenError);
    }

    void test_link_to_file_syn_link_not_allowed() {
        DBG(cerr << "checking /testdir/nc/link_to_testfile.nc, " << "follow syms = false" << endl);
        CPPUNIT_ASSERT_THROW_MESSAGE("checking /testdir/nc/link_to_testfile.nc, follow syms = false",
                                     BESUtil::check_path("/testdir/nc/link_to_testfile.nc", "./", false),
                                     BESForbiddenError);
    }

    void test_no_dir_syn_link_allowed_1() {
        DBG(cerr << "checking /nodir/" << endl);
        CPPUNIT_ASSERT_THROW_MESSAGE("checking /nodir/", BESUtil::check_path("/nodir/", "./", true), BESNotFoundError);
    }

    void test_no_dir_syn_link_not_allowed() {
        DBG(cerr << "checking /nodir/, follow syms = false" << endl);
        CPPUNIT_ASSERT_THROW_MESSAGE("checking /nodir/, follow syms = false",
                                     BESUtil::check_path("/nodir/", "./", false), BESNotFoundError);
    }

    void test_no_dir_syn_link_allowed_2() {
        DBG(cerr << "checking /testdir/nodir/" << endl);
        CPPUNIT_ASSERT_THROW_MESSAGE("checking /testdir/nodir/", BESUtil::check_path("/testdir/nodir/", "./", true),
                                     BESNotFoundError);
    }
    void test_no_file_syn_link_allowed() {
        DBG(cerr << "checking /testdir/nc/nofile.nc" << endl);
        CPPUNIT_ASSERT_THROW_MESSAGE("checking /testdir/nc/nofile.nc",
                                     BESUtil::check_path("/testdir/nc/nofile.nc", "./", true), BESNotFoundError);
    }

    void test_no_file_linked_dir_syn_link_allowed() {
        // This and the next test return NotFoundError because that is checked
        // before the tests for the sym link (which, for this case would return
        // BESForbiddenError). jhrg 12/31/21
        DBG(cerr << "checking /testdir/link_to_nc/nofile.nc" << endl);
        CPPUNIT_ASSERT_THROW_MESSAGE("checking /testdir/link_to_nc/nofile.nc",
                                     BESUtil::check_path("/testdir/link_to_nc/nofile.nc", "./", true),
                                     BESNotFoundError);
    }

    void test_no_file_linked_dir_syn_link_not_allowed() {
        DBG(cerr << "checking /testdir/link_to_nc/nofile.nc, follow syms = false" << endl);
        CPPUNIT_ASSERT_THROW_MESSAGE("checking /testdir/link_to_nc/nofile.nc, follow syms = false",
                                     BESUtil::check_path("/testdir/link_to_nc/nofile.nc", "./", false),
                                     BESNotFoundError);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(checkT);

int main(int argc, char *argv[]) {
    int option_char;
    while ((option_char = getopt(argc, argv, "dDh")) != EOF)
        switch (option_char) {
        case 'd':
            debug = true; // debug is a static global
            break;
        case 'D':
            debug2 = true; // debug is a static global
            break;
        case 'h': { // help - show test names
            cerr << "Usage: checkT has the following tests:" << endl;
            const std::vector<Test *> &tests = checkT::suite()->getTests();
            unsigned int prefix_len = checkT::suite()->getName().append("::").size();
            for (auto test : tests) {
                cerr << test->getName().replace(0, prefix_len, "") << endl;
            }

            return 0;
        }
        default:
            break;
        }

    argc -= optind;
    argv += optind;

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    string test = "";
    if (0 == argc) {
        // run them all
        wasSuccessful = runner.run("");
    } else {
        int i = 0;
        while (i < argc) {
            DBG(cerr << "Running " << argv[i] << endl);
            test = checkT::suite()->getName().append("::").append(argv[i++]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
