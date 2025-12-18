// resplistT.C

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

#include <exception>
#include <iostream>
#include <memory>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include <libdap/Error.h>
#include <unistd.h>

#include "BESError.h"
#include "BESResponseHandlerList.h"
#include "TheBESKeys.h"

#include "TestResponseHandler.h"
#include "test_config.h"

using namespace CppUnit;
using namespace libdap;
using namespace std;

static bool debug = false;

#undef DBG
#define DBG(x)                                                                                                         \
    do {                                                                                                               \
        if (debug)                                                                                                     \
            (x);                                                                                                       \
    } while (false);

class resplistT : public TestFixture {
private:
    BESResponseHandlerList *handler_list;

public:
    resplistT() : handler_list(nullptr) {
        TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");
        DBG(cerr << "TheBESKeys::ConfigFile: " << TheBESKeys::ConfigFile << endl);
    }
    ~resplistT() = default;

    void setUp() {
        handler_list = BESResponseHandlerList::TheList();
        // this really need mutex protection
        if (handler_list->_handler_list.empty()) {
            char num[10];
            for (int i = 0; i < 5; i++) {
                sprintf(num, "resp%d", i);
                DBG(cerr << "Adding handler: " << num << endl);
                CPPUNIT_ASSERT(handler_list->add_handler(num, TestResponseHandler::TestResponseBuilder) == true);
            }
        }
    }

    void tearDown() {}

    CPPUNIT_TEST_SUITE(resplistT);

    CPPUNIT_TEST(test_duplicate_add);
    CPPUNIT_TEST(test_find_all_handlers);
    CPPUNIT_TEST(test_nonexistant_handler);
    CPPUNIT_TEST(test_remove_and_add_handler);

    CPPUNIT_TEST(test_ctor);

    CPPUNIT_TEST_SUITE_END();

    void test_duplicate_add() {
        try {
            CPPUNIT_ASSERT(handler_list->add_handler("resp3", TestResponseHandler::TestResponseBuilder) == false);
        } catch (BESError &e) {
            cerr << "Caught BESError: " << e.get_verbose_message() << endl;
            CPPUNIT_FAIL("BESError");
        } catch (Error &e) {
            cerr << "Caught libdap::Error: " << e.get_error_message() << endl;
            CPPUNIT_FAIL("libdap::Error");
        } catch (std::exception &e) {
            cerr << "Caught std::exception: " << e.what() << endl;
            CPPUNIT_FAIL("std::exception");
        }
    }

    void test_find_all_handlers() {
        try {
            BESResponseHandler *rh = nullptr;
            char num[10];
            for (int i = 4; i >= 0; i--) {
                sprintf(num, "resp%d", i);
                DBG(cerr << "    finding " << num << endl);
                rh = handler_list->find_handler(num);
                CPPUNIT_ASSERT(rh);
            }
        } catch (BESError &e) {
            cerr << "Caught BESError: " << e.get_verbose_message() << endl;
            CPPUNIT_FAIL("BESError");
        } catch (Error &e) {
            cerr << "Caught libdap::Error: " << e.get_error_message() << endl;
            CPPUNIT_FAIL("libdap::Error");
        } catch (std::exception &e) {
            cerr << "Caught std::exception: " << e.what() << endl;
            CPPUNIT_FAIL("std::exception");
        }
    }

    void test_nonexistant_handler() {
        try {
            CPPUNIT_ASSERT(!handler_list->find_handler("not_there"));
        } catch (BESError &e) {
            cerr << "Caught BESError: " << e.get_verbose_message() << endl;
            CPPUNIT_FAIL("BESError");
        } catch (Error &e) {
            cerr << "Caught libdap::Error: " << e.get_error_message() << endl;
            CPPUNIT_FAIL("libdap::Error");
        } catch (std::exception &e) {
            cerr << "Caught std::exception: " << e.what() << endl;
            CPPUNIT_FAIL("std::exception");
        }
    }

    void test_remove_and_add_handler() {
        try {
            DBG(cerr << "Removing resp2" << endl);
            CPPUNIT_ASSERT(handler_list->remove_handler("resp2") == true);

            BESResponseHandler *rh = handler_list->find_handler("resp2");
            CPPUNIT_ASSERT(!rh);

            DBG(cerr << "Add resp2 back" << endl);
            CPPUNIT_ASSERT(handler_list->add_handler("resp2", TestResponseHandler::TestResponseBuilder) == true);

            rh = handler_list->find_handler("resp2");
            CPPUNIT_ASSERT(rh);
        } catch (BESError &e) {
            cerr << "Caught BESError: " << e.get_verbose_message() << endl;
            CPPUNIT_FAIL("BESError");
        } catch (Error &e) {
            cerr << "Caught libdap::Error: " << e.get_error_message() << endl;
            CPPUNIT_FAIL("libdap::Error");
        } catch (std::exception &e) {
            cerr << "Caught std::exception: " << e.what() << endl;
            CPPUNIT_FAIL("std::exception");
        }
    }

    void test_ctor() {
        try {
            unique_ptr<BESResponseHandler> handler(TestResponseHandler::TestResponseBuilder("test"));

            // These values are set in bes.conf which is built from bes.conf.in
            DBG(cerr << "handler->d_annotation_service_url: " << handler->d_annotation_service_url << endl);
            CPPUNIT_ASSERT(handler->d_annotation_service_url == "http://localhost:8083/Feedback/form");
        } catch (BESError &e) {
            cerr << "Caught BESError: " << e.get_verbose_message() << endl;
            CPPUNIT_FAIL("BESError");
        } catch (Error &e) {
            cerr << "Caught libdap::Error: " << e.get_error_message() << endl;
            CPPUNIT_FAIL("libdap::Error");
        } catch (std::exception &e) {
            cerr << "Caught std::exception: " << e.what() << endl;
            CPPUNIT_FAIL("std::exception");
        }
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(resplistT);

int main(int argc, char *argv[]) {
    int option_char;
    while ((option_char = getopt(argc, argv, "dh")) != EOF)
        switch (option_char) {
        case 'd':
            debug = true; // debug is a static global
            break;
        case 'h': { // help - show test names
            cerr << "Usage: resplistT has the following tests:" << endl;
            const std::vector<Test *> &tests = resplistT::suite()->getTests();
            unsigned int prefix_len = resplistT::suite()->getName().append("::").size();
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
            test = resplistT::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
