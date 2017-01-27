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

#include "GetOpt.h"

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

namespace libdap {

class SleepFunctionTest: public CppUnit::TestFixture {
private:
    BaseTypeFactory btf;
    DDS *testDDS;

public:
    // Called once before everything gets tested
    SleepFunctionTest()
    {

    }

    // Called at the end of the test
    ~SleepFunctionTest()
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

CPPUNIT_TEST_SUITE( SleepFunctionTest );

    CPPUNIT_TEST(sleepFunctionTest);

    CPPUNIT_TEST_SUITE_END()
    ;

    void sleepFunctionTest()
    {
        DBG(cerr << endl << "sleepFunctionTest() - BEGIN." << endl);
     
        debug_function::SleepFunc sleepFunc;
        
        libdap::btp_func sleep_function=sleepFunc.get_btp_func();
           
        libdap::Int32 time("time");
        time.set_value(3000);
        libdap::BaseType *argv[] = { &time };
        libdap::BaseType *result = 0;
        libdap::BaseType **btpp = &result;
        
        sleep_function(1, argv, *testDDS, btpp);      

        if(debug){
            (*btpp)->print_val(cerr,"",false);
            cerr << endl;
        }

        CPPUNIT_ASSERT(true);
        
        DBG(cerr << "sleepFunctionTest() - END." << endl);
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION(SleepFunctionTest);

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
