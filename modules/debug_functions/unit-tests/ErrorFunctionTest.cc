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

#include "util.h"
#include "debug.h"
#include "Array.h"
#include "Int32.h"
#include "Float64.h"
#include "DebugFunctions.h"

#include <BaseTypeFactory.h>
#include <BESInternalError.h>
#include <BESInternalFatalError.h>
#include <BESSyntaxUserError.h>
#include <BESForbiddenError.h>
#include <BESNotFoundError.h>

#include "GetOpt.h"

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

namespace libdap {

class ErrorFunctionTest: public CppUnit::TestFixture {
private:
    BaseTypeFactory btf;
    DDS *testDDS;

public:
    // Called once before everything gets tested
    ErrorFunctionTest() :testDDS(0)
    {

    }

    // Called at the end of the test
    ~ErrorFunctionTest()
    {
    }

    // Called before each test
    void setUp()
    {
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

    CPPUNIT_TEST_SUITE_END()
    ;

    void internalErrorFunctionTest()
    {
        DBG(cerr << endl << "internalErrorFunctionTest() - BEGIN." << endl);
        
        debug_function::ErrorFunc errorFunc;
        
        libdap::btp_func error_function=errorFunc.get_btp_func();
        
        libdap::Int32 error_type("error_type");
        error_type.set_value(BES_INTERNAL_ERROR);
        libdap::BaseType *argv[] = { &error_type };
        libdap::BaseType *result = 0;
        libdap::BaseType **btpp = &result;
        
        try {
            error_function(1, argv, *testDDS, btpp);      
            CPPUNIT_ASSERT(false);
        }
        catch(BESInternalError e){
            DBG(cerr << "internalErrorFunctionTest() - Caught BESInternalError. msg: " << e.get_message() << endl);
            CPPUNIT_ASSERT(true);
        }
        
        DBG(cerr << "internalErrorFunctionTest() - END." << endl);
    }
    
    
    void internalFatalErrorFunctionTest()
    {
        DBG(cerr << endl << "internalFatalErrorFunctionTest() - BEGIN." << endl);
        
        debug_function::ErrorFunc errorFunc;
        
        libdap::btp_func error_function=errorFunc.get_btp_func();
        
        libdap::Int32 error_type("error_type");
        error_type.set_value(BES_INTERNAL_FATAL_ERROR);
        libdap::BaseType *argv[] = { &error_type };
        libdap::BaseType *result = 0;
        libdap::BaseType **btpp = &result;
        
        try {
            error_function(1, argv, *testDDS, btpp);      
            CPPUNIT_ASSERT(false);
        }
        catch(BESInternalFatalError &e){
            DBG(cerr << "internalFatalErrorFunctionTest() - Caught BESInternalFatalError. msg: " << e.get_message() << endl);
            CPPUNIT_ASSERT(true);
        }

        DBG(cerr << "internalFatalErrorFunctionTest() - END." << endl);
    }

    void syntaxUserErrorFunctionTest()
    {
        DBG(cerr << endl << "syntaxUserErrorFunctionTest() - BEGIN." << endl);
        
        debug_function::ErrorFunc errorFunc;
        
        libdap::btp_func error_function=errorFunc.get_btp_func();
        
        libdap::Int32 error_type("error_type");
        error_type.set_value(BES_SYNTAX_USER_ERROR);
        libdap::BaseType *argv[] = { &error_type };
        libdap::BaseType *result = 0;
        libdap::BaseType **btpp = &result;
        
        try {
            error_function(1, argv, *testDDS, btpp);      
            CPPUNIT_ASSERT(false);
        }
        catch(BESSyntaxUserError &e){
            DBG(cerr << "syntaxUserErrorFunctionTest() - Caught BESSyntaxUserError. msg: " << e.get_message() << endl);
            CPPUNIT_ASSERT(true);
        }
        
        DBG(cerr << "syntaxUserErrorFunctionTest() - END." << endl);
    }

    void forbiddenErrorFunctionTest()
    {
        DBG(cerr << endl << "forbiddenErrorFunctionTest() - BEGIN." << endl);
        
        debug_function::ErrorFunc errorFunc;
        
        libdap::btp_func error_function=errorFunc.get_btp_func();
        
        libdap::Int32 error_type("error_type");
        error_type.set_value(BES_FORBIDDEN_ERROR);
        libdap::BaseType *argv[] = { &error_type };
        libdap::BaseType *result = 0;
        libdap::BaseType **btpp = &result;
        
        try {
            error_function(1, argv, *testDDS, btpp);      
            CPPUNIT_ASSERT(false);
        }
        catch(BESForbiddenError &e){
            DBG(cerr << "forbiddenErrorFunctionTest() - Caught BESForbiddenError. msg: " << e.get_message() << endl);
            CPPUNIT_ASSERT(true);
        }
        
        DBG(cerr << "forbiddenErrorFunctionTest() - END." << endl);
    }

    void notFoundErrorFunctionTest()
    {
        DBG(cerr << endl << "notFoundErrorFunctionTest() - BEGIN." << endl);
        
        debug_function::ErrorFunc errorFunc;
        
        libdap::btp_func error_function=errorFunc.get_btp_func();
        
        libdap::Int32 error_type("error_type");
        error_type.set_value(BES_NOT_FOUND_ERROR);
        libdap::BaseType *argv[] = { &error_type };
        libdap::BaseType *result = 0;
        libdap::BaseType **btpp = &result;
        
        try {
            error_function(1, argv, *testDDS, btpp);      
            CPPUNIT_ASSERT(false);
        }
        catch(BESNotFoundError e){
            DBG(cerr << "notFoundErrorFunctionTest() - Caught BESNotFoundError. msg: " << e.get_message() << endl);
            CPPUNIT_ASSERT(true);
        }
        
        DBG(cerr << "notFoundErrorFunctionTest() - END." << endl);
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION(ErrorFunctionTest);

} /* namespace libdap */

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "d");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = true;  // debug is a static global
            BESDebug::SetUp("cerr,ugrid");
            break;
        default:
            break;
        }

    bool wasSuccessful = true;
    string test = "";
    int i = getopt.optind;
    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        while (i < argc) {
            test = string("libdap::NDimArrayTest::") + argv[i++];

            DBG(cerr << endl << "Running test " << test << endl << endl);

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
