// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2005 OPeNDAP, Inc.
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

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#define DODS_DEBUG

#include <BESDebug.h>

#include <libdap/util.h>
#include <libdap/debug.h>
#include <libdap/Array.h>
#include <libdap/Int32.h>
#include <libdap/Float64.h>
#include "DebugFunctions.h"

#include <libdap/BaseTypeFactory.h>
#include <BESInternalError.h>
#include <BESInternalFatalError.h>
#include <BESSyntaxUserError.h>
#include <BESForbiddenError.h>
#include <BESNotFoundError.h>

#include <HttpError.h>

#include <unistd.h>

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

#define prolog std::string("# ErrorFunctionTest::").append(__func__).append("() - ")

namespace libdap {

class ErrorFunctionTest: public CppUnit::TestFixture {
private:
    BaseTypeFactory btf;
    DDS *testDDS;

public:
    // Called once before everything gets tested
    ErrorFunctionTest() :
        testDDS(0)
    {

    }

    // Called at the end of the test
    ~ErrorFunctionTest()
    {
    }

    // Called before each test
    void setUp()
    {
        DBG(cerr << prolog << "\n");
        
        try {
            testDDS = new DDS(&btf);
        }
        catch (Error & e) {
            cerr << "SetUp: " << e.get_error_message() << endl;
            throw;
        }
    }

    // Called after each test
    void tearDown()
    {
        delete testDDS;
    }

CPPUNIT_TEST_SUITE( ErrorFunctionTest );

    CPPUNIT_TEST(internalErrorFunctionTest);
    CPPUNIT_TEST(internalFatalErrorFunctionTest);
    CPPUNIT_TEST(syntaxUserErrorFunctionTest);
    CPPUNIT_TEST(forbiddenErrorFunctionTest);
    CPPUNIT_TEST(notFoundErrorFunctionTest);
    CPPUNIT_TEST(httpErrorFunctionTest);

    CPPUNIT_TEST_SUITE_END();

    void internalErrorFunctionTest()
    {
        DBG(cerr << prolog << "BEGIN." << endl);

        debug_function::ErrorFunc errorFunc;

        libdap::btp_func error_function = errorFunc.get_btp_func();

        libdap::Int32 error_type("error_type");
        error_type.set_value(BES_INTERNAL_ERROR);
        libdap::BaseType *argv[] = { &error_type };
        libdap::BaseType *result = 0;
        libdap::BaseType **btpp = &result;

        try {
            error_function(1, argv, *testDDS, btpp);
            CPPUNIT_ASSERT(false);
        }
        catch (BESInternalError &e) {
            DBG(cerr << prolog << "Caught BESInternalError. msg: " << e.get_message() << endl);
            CPPUNIT_ASSERT(true);
        }

        DBG(cerr << prolog << "END." << endl);
    }

    void internalFatalErrorFunctionTest()
    {
        DBG(cerr << prolog << "BEGIN." << endl);

        debug_function::ErrorFunc errorFunc;

        libdap::btp_func error_function = errorFunc.get_btp_func();

        libdap::Int32 error_type("error_type");
        error_type.set_value(BES_INTERNAL_FATAL_ERROR);
        libdap::BaseType *argv[] = { &error_type };
        libdap::BaseType *result = 0;
        libdap::BaseType **btpp = &result;

        try {
            error_function(1, argv, *testDDS, btpp);
            CPPUNIT_ASSERT(false);
        }
        catch (BESInternalFatalError &e) {
            DBG(
                cerr << "Caught BESInternalFatalError. msg: " << e.get_message()
                    << endl);
            CPPUNIT_ASSERT(true);
        }

        DBG(cerr << prolog << "END." << endl);
    }

    void syntaxUserErrorFunctionTest()
    {
        DBG(cerr << prolog << "BEGIN." << endl);

        debug_function::ErrorFunc errorFunc;

        libdap::btp_func error_function = errorFunc.get_btp_func();

        libdap::Int32 error_type("error_type");
        error_type.set_value(BES_SYNTAX_USER_ERROR);
        libdap::BaseType *argv[] = { &error_type };
        libdap::BaseType *result = 0;
        libdap::BaseType **btpp = &result;

        try {
            error_function(1, argv, *testDDS, btpp);
            CPPUNIT_ASSERT(false);
        }
        catch (BESSyntaxUserError &e) {
            DBG(cerr << prolog << "Caught BESSyntaxUserError. msg: " << e.get_message() << endl);
            CPPUNIT_ASSERT(true);
        }

        DBG(cerr << prolog << "END." << endl);
    }

    void forbiddenErrorFunctionTest()
    {
        DBG(cerr << prolog << "BEGIN." << endl);

        debug_function::ErrorFunc errorFunc;

        libdap::btp_func error_function = errorFunc.get_btp_func();

        libdap::Int32 error_type("error_type");
        error_type.set_value(BES_FORBIDDEN_ERROR);
        libdap::BaseType *argv[] = { &error_type };
        libdap::BaseType *result = 0;
        libdap::BaseType **btpp = &result;

        try {
            error_function(1, argv, *testDDS, btpp);
            CPPUNIT_ASSERT(false);
        }
        catch (BESForbiddenError &e) {
            DBG(cerr << prolog << "Caught BESForbiddenError. msg: " << e.get_message() << endl);
            CPPUNIT_ASSERT(true);
        }

        DBG(cerr << prolog << "END." << endl);
    }

    void notFoundErrorFunctionTest()
    {
        DBG(cerr << prolog << "BEGIN." << endl);

        debug_function::ErrorFunc errorFunc;

        libdap::btp_func error_function = errorFunc.get_btp_func();

        libdap::Int32 error_type("error_type");
        error_type.set_value(BES_NOT_FOUND_ERROR);
        libdap::BaseType *argv[] = { &error_type };
        libdap::BaseType *result = 0;
        libdap::BaseType **btpp = &result;

        try {
            error_function(1, argv, *testDDS, btpp);
            CPPUNIT_ASSERT(false);
        }
        catch (BESNotFoundError &e) {
            DBG(cerr << prolog << "Caught BESNotFoundError. msg: " << e.get_message() << endl);
            CPPUNIT_ASSERT(true);
        }

        DBG(cerr << prolog << "END." << endl);
    }

    void httpErrorFunctionTest()
    {
        DBG(cerr << prolog << "BEGIN." << endl);

        debug_function::ErrorFunc errorFunc;

        libdap::btp_func error_function = errorFunc.get_btp_func();

        libdap::Int32 error_type("error_type");
        error_type.set_value(BES_HTTP_ERROR);
        libdap::Int32 http_status("http_status");
        http_status.set_value(501);
        libdap::BaseType *argv[] = { &error_type, &http_status  };
        libdap::BaseType *result = nullptr;
        libdap::BaseType **btpp = &result;

        try {
            error_function(2, argv, *testDDS, btpp);
            CPPUNIT_ASSERT(false);
        }
        catch (http::HttpError &e) {
            DBG(cerr << prolog << "Caught HttpError...\n");
            DBG(cerr << prolog << "    message: " << e.get_message() << "\n");
            DBG(cerr << prolog << "    dump: \n" << e.dump() << "\n");
            CPPUNIT_ASSERT(true);
        }

        DBG(cerr << prolog << "END." << endl);
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION(ErrorFunctionTest);

} /* namespace libdap */

int main(int argc, char*argv[])
{
    int option_char;
    while ((option_char = getopt(argc, argv, "dh")) != -1)
        switch (option_char) {
        case 'd':
            debug = true;  // debug is a static global
            BESDebug::SetUp("cerr,ugrid");
            break;
        case 'h': {     // help - show test names
            std::cerr << "Usage: ErrorFunctionTest has the following tests:" << std::endl;
            const std::vector<CppUnit::Test*> &tests = libdap::ErrorFunctionTest::suite()->getTests();
            unsigned int prefix_len = libdap::ErrorFunctionTest::suite()->getName().append("::").size();
            for (std::vector<CppUnit::Test*>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                std::cerr << (*i)->getName().replace(0, prefix_len, "") << std::endl;
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
    }
    else {
        int i = 0;
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = libdap::ErrorFunctionTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            i++;
        }
    }

    return wasSuccessful ? 0 : 1;
}
