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
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace CppUnit;

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <fstream>

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;

#include "BESUtil.h"
#include "BESError.h"
#include "TheBESKeys.h"
#include <test_config.h>
#include <GetOpt.h>

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

class checkT: public TestFixture {
private:

public:
    checkT()
    {
    }
    ~checkT()
    {
    }

    void setUp()
    {
        string bes_conf = (string) TEST_SRC_DIR + "/bes.conf";
        TheBESKeys::ConfigFile = bes_conf;
    }

    void tearDown()
    {
    }

CPPUNIT_TEST_SUITE( checkT );

    CPPUNIT_TEST(do_test);

    CPPUNIT_TEST_SUITE_END()
    ;

    void do_test()
    {
        cout << "*****************************************" << endl;
        cout << "Entered checkT::run" << endl;

        cout << "*****************************************" << endl;
        cout << "create the desired directory structure" << endl;
        // testdir
        // testdir/nc
        // testdir/link_to_nc -> testdir/nc
        // testdir/nc/testfile.nc
        // testdir/nc/link_to_testfile.nc -> testdir/nc/testfile.nc
        cout << "    creating ./testdir/" << endl;
        int ret = mkdir("./testdir", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        int myerrno = errno;
        CPPUNIT_ASSERT(ret == 0 || myerrno == EEXIST);

        cout << "    creating ./testdir/nc/" << endl;
        ret = mkdir("./testdir/nc", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        myerrno = errno;
        CPPUNIT_ASSERT(ret == 0 || myerrno == EEXIST);

        cout << "    creating ./testdir/nc/testfile.nc" << endl;
        FILE *fp = fopen("./testdir/nc/testfile.nc", "w+");
        CPPUNIT_ASSERT(fp);

        cout << "    creating symlink ./testdir/nc/link_to_nc" << endl;
        fprintf(fp, "This is a test file");
        fclose(fp);
        ret = symlink("./nc", "./testdir/link_to_nc");
        myerrno = errno;
        CPPUNIT_ASSERT(ret == 0 || myerrno == EEXIST);

        cout << "    creating symlink ./testdir/nc/link_to_testfile.nc" << endl;
        ret = symlink("./testfile.nc", "./testdir/nc/link_to_testfile.nc");
        myerrno = errno;
        CPPUNIT_ASSERT(ret == 0 || myerrno == EEXIST);

        cout << "*****************************************" << endl;
        cout << "checking /testdir/" << endl;
        try {
            BESUtil::check_path("/testdir/", "./", true);
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"check failed for /testdir/");
        }

        cout << "*****************************************" << endl;
        cout << "checking /testdir/nc/" << endl;
        try {
            BESUtil::check_path("/testdir/nc/", "./", true);
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"check failed for /testdir/nc/");
        }

        cout << "*****************************************" << endl;
        cout << "checking /testdir/nc/testfile.nc" << endl;
        try {
            BESUtil::check_path("/testdir/nc/testfile.nc", "./", true);
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"check failed for /testdir/nc/testfile.nc");
        }

        cout << "*****************************************" << endl;
        cout << "checking /testdir/link_to_nc/" << endl;
        try {
            BESUtil::check_path("/testdir/link_to_nc/", "./", true);
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"check failed for /testdir/link_to_nc/");
        }

        cout << "*****************************************" << endl;
        cout << "checking /testdir/link_to_nc/link_to_testfile.nc" << endl;
        try {
            BESUtil::check_path("/testdir/link_to_nc/link_to_testfile.nc", "./", true);
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"check failed for /testdir/link_to_nc/link_to_testfile.nc");
        }

        cout << "*****************************************" << endl;
        cout << "checking /testdir/, folllow syms = false" << endl;
        try {
            BESUtil::check_path("/testdir/", "./", false);
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"check failed for /testdir/");
        }

        cout << "*****************************************" << endl;
        cout << "checking /testdir/nc/, folllow syms = false" << endl;
        try {
            BESUtil::check_path("/testdir/nc/", "./", false);
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"check failed for /testdir/nc/");
        }

        cout << "*****************************************" << endl;
        cout << "checking /testdir/nc/testfile.nc, folllow syms = false" << endl;
        try {
            BESUtil::check_path("/testdir/nc/testfile.nc", "./", false);
        }
        catch (BESError &e) {
            cerr << e.get_message() << endl;
            CPPUNIT_ASSERT(!"check failed for /testdir/nc/testfile.nc");
        }

        cout << "*****************************************" << endl;
        cout << "checking /testdir/link_to_nc/, follow_syms = false" << endl;
        try {
            BESUtil::check_path("/testdir/link_to_nc/", "./", false);
            CPPUNIT_ASSERT(!"check succeeded");
        }
        catch (BESError &e) {
        }

        cout << "*****************************************" << endl;
        cout << "checking /testdir/link_to_nc/link_to_testfile.nc, " << "follow syms = false" << endl;
        try {
            BESUtil::check_path("/testdir/link_to_nc/link_to_testfile.nc", "./", false);
            CPPUNIT_ASSERT(!"check succeeded");
        }
        catch (BESError &e) {
        }

        cout << "*****************************************" << endl;
        cout << "checking /testdir/nc/link_to_testfile.nc, " << "follow syms = false" << endl;
        try {
            BESUtil::check_path("/testdir/nc/link_to_testfile.nc", "./", false);
            CPPUNIT_ASSERT(!"check succeeded");
        }
        catch (BESError &e) {
        }

        cout << "*****************************************" << endl;
        cout << "checking /nodir/" << endl;
        try {
            BESUtil::check_path("/nodir/", "./", true);
            CPPUNIT_ASSERT(!"check succeeded");
        }
        catch (BESError &e) {
        }

        cout << "*****************************************" << endl;
        cout << "checking /nodir/, follow syms = false" << endl;
        try {
            BESUtil::check_path("/nodir/", "./", false);
            CPPUNIT_ASSERT(!"check succeeded");
        }
        catch (BESError &e) {
        }

        cout << "*****************************************" << endl;
        cout << "checking /testdir/nodir/" << endl;
        try {
            BESUtil::check_path("/testdir/nodir/", "./", true);
            CPPUNIT_ASSERT(!"check succeeded");
        }
        catch (BESError &e) {
        }

        cout << "*****************************************" << endl;
        cout << "checking /testdir/nc/nofile.nc" << endl;
        try {
            BESUtil::check_path("/testdir/nc/nofile.nc", "./", true);
            CPPUNIT_ASSERT(!"check succeeded");
        }
        catch (BESError &e) {
        }

        cout << "*****************************************" << endl;
        cout << "checking /testdir/link_to_nc/nofile.nc" << endl;
        try {
            BESUtil::check_path("/testdir/link_to_nc/nofile.nc", "./", true);
            CPPUNIT_ASSERT(!"check succeeded");
        }
        catch (BESError &e) {
        }

        cout << "*****************************************" << endl;
        cout << "checking /testdir/link_to_nc/nofile.nc, follow syms = false" << endl;
        try {
            BESUtil::check_path("/testdir/link_to_nc/nofile.nc", "./", false);
            CPPUNIT_ASSERT(!"check succeeded");
        }
        catch (BESError &e) {
        }

        cout << "*****************************************" << endl;
        cout << "Returning from checkT::run" << endl;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(checkT);

int main(int argc, char*argv[])
{

    GetOpt getopt(argc, argv, "dh");
    int option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: checkT has the following tests:" << endl;
            const std::vector<Test*> &tests = checkT::suite()->getTests();
            unsigned int prefix_len = checkT::suite()->getName().append("::").length();
            for (std::vector<Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                cerr << (*i)->getName().replace(0, prefix_len, "") << endl;
            }
            break;
        }
        default:
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
            test = checkT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

