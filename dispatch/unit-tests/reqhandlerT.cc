// reqhandlerT.C

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

using namespace CppUnit;

#include <fcntl.h>
#include <iostream>
#include <libdap/util.h>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

using std::cerr;
using std::cout;
using std::endl;

#include "BESContextManager.h"
#include "BESFileContainer.h"
#include "BESNotFoundError.h"
#include "BESRequestHandler.h"
#include "BESRequestHandlerList.h"
#include "TestRequestHandler.h"
#include <unistd.h>

#include "test_config.h"

static bool debug = false;

#undef DBG
#define DBG(x)                                                                                                         \
    do {                                                                                                               \
        if (debug)                                                                                                     \
            (x);                                                                                                       \
    } while (false);

class reqhandlerT : public TestFixture {
private:
    BESRequestHandler d_handler;
    string tmp;

public:
    reqhandlerT() : d_handler("test_handler"), tmp(string(TEST_BUILD_DIR) + "/tmp") {
        BESRequestHandlerList::TheList()->add_handler("test_handler", &d_handler);
        mkdir(tmp.c_str(), 0755);
    }

    ~reqhandlerT() { rmdir(tmp.c_str()); }

    void setUp() {}

    void tearDown() {}

    void get_lmt_test_1() { // test for file that has HAS NOT been modified since it was created
        string relative_file = "/tmp/temp_01.dmr";
        string real_name = string(TEST_BUILD_DIR) + relative_file;
        BESFileContainer cont("cont", real_name, "test_handler");
        cont.set_relative_name(relative_file);

        DBG(cerr << "cont.get_real_name: " << cont.get_real_name() << endl);
        DBG(cerr << "cont.get_relative_name: " << cont.get_relative_name() << endl);

        BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler("test_handler");
        CPPUNIT_ASSERT(besRH != nullptr);

        try {
            int fd = open(real_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
            CPPUNIT_ASSERT(fd != -1);

            struct stat statbuf;
            if (stat(real_name.c_str(), &statbuf) == -1) {
                throw BESNotFoundError(strerror(errno), __FILE__, __LINE__);
            } // end if

            time_t ctime = statbuf.st_ctime;
            DBG(cerr << "ctime: " << ctime << endl);
            time_t mtime = besRH->get_lmt(real_name);
            DBG(cerr << "mtime: " << mtime << endl);

            bool test = ((mtime - ctime) < 2);
            CPPUNIT_ASSERT(test);
        } catch (BESError &e) {
            unlink(real_name.c_str());
            ostringstream oss;
            oss << "Error: " << e.get_message() << " " << e.get_file() << ":" << e.get_line();
            CPPUNIT_FAIL(oss.str());
        } catch (...) {
            unlink(real_name.c_str());
            throw;
        }

        unlink(real_name.c_str());
    } // get_lmt_test_1()

    void get_lmt_test_2() { // test for file that has HAS been modified since it was created
        string relative_file = "/tmp/temp_01.dmr";
        string real_name = string(TEST_BUILD_DIR) + relative_file;
        BESFileContainer cont("cont", real_name, "test_handler");
        cont.set_relative_name(relative_file);

        DBG(cerr << "cont.get_real_name: " << cont.get_real_name() << endl);
        DBG(cerr << "cont.get_relative_name: " << cont.get_relative_name() << endl);

        BESRequestHandler *besRH = BESRequestHandlerList::TheList()->find_handler("test_handler");
        CPPUNIT_ASSERT(besRH != nullptr);

        try {
            int fd = open(real_name.c_str(), O_RDWR | O_CREAT, 00664 /*mode = rw rw r*/);
            CPPUNIT_ASSERT(fd != -1);

            struct stat statbuf;
            if (stat(real_name.c_str(), &statbuf) == -1) {
                throw BESNotFoundError(strerror(errno), __FILE__, __LINE__);
            } // end if

            time_t ctime = statbuf.st_ctime;
            DBG(cerr << "ctime: " << ctime << endl);

            sleep(4);

            write(fd, "Test String\n", strlen("Test String\n"));

            time_t mtime = besRH->get_lmt(real_name);
            DBG(cerr << "mtime: " << mtime << endl);

            bool test = ((mtime - ctime) < 2);
            CPPUNIT_ASSERT(!test);
        } catch (BESError &e) {
            unlink(real_name.c_str());
            ostringstream oss;
            oss << "Error: " << e.get_message() << " " << e.get_file() << ":" << e.get_line();
            CPPUNIT_FAIL(oss.str());
        } catch (...) {
            unlink(real_name.c_str());
            throw;
        }

        unlink(real_name.c_str());
    } // get_lmt_test_2()

    CPPUNIT_TEST_SUITE(reqhandlerT);

    CPPUNIT_TEST(get_lmt_test_1);
    CPPUNIT_TEST(get_lmt_test_2);

    CPPUNIT_TEST(do_test);

    CPPUNIT_TEST_SUITE_END();

    void do_test() {
        cout << "*****************************************" << endl;
        cout << "Entered reqhandlerT::run" << endl;

        TestRequestHandler trh("test");
        (void)trh.test();

        cout << "*****************************************" << endl;
        cout << "Returning from reqhandlerT::run" << endl;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(reqhandlerT);

int main(int argc, char *argv[]) {
    int option_char;
    while ((option_char = getopt(argc, argv, "dh")) != EOF)
        switch (option_char) {
        case 'd':
            debug = true; // debug is a static global
            break;
        case 'h': { // help - show test names
            cerr << "Usage: reqhandlerT has the following tests:" << endl;
            const std::vector<Test *> &tests = reqhandlerT::suite()->getTests();
            unsigned int prefix_len = reqhandlerT::suite()->getName().append("::").size();
            for (auto test : tests) {
                cerr << test->getName().replace(0, prefix_len, "") << endl;
            }
            break;
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
            if (debug)
                cerr << "Running " << argv[i] << endl;
            test = reqhandlerT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
