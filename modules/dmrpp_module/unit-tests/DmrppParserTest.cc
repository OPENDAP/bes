
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2016 OPeNDAP, Inc.
// Author: Nathan David Potter <ndp@opendap.org>, James Gallagher
// <jgallagher@opendap.org>
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

#include <memory>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <DMR.h>

#include <BESDebug.h>

#include "DmrppArray.h"
#include "DmrppByte.h"
#include "DmrppCommon.h"
#include "DmrppD4Enum.h"
#include "DmrppD4Group.h"
#include "DmrppD4Opaque.h"
#include "DmrppD4Sequence.h"
#include "DmrppFloat32.h"
#include "DmrppFloat64.h"
#include "DmrppInt16.h"
#include "DmrppInt32.h"
#include "DmrppInt64.h"
#include "DmrppInt8.h"
#include "DmrppModule.h"
#include "DmrppStr.h"
#include "DmrppStructure.h"
#include "DmrppUInt16.h"
#include "DmrppUInt32.h"
#include "DmrppUInt64.h"
#include "DmrppUrl.h"

#include "DmrppParserSax2.h"
#include "DmrppRequestHandler.h"
#include "DmrppTypeFactory.h"

#include "GetOpt.h"
#include "test_config.h"

using namespace libdap;

static bool debug = false;

namespace dmrpp {

class DmrppParserTest: public CppUnit::TestFixture {
private:
    DmrppParserSax2 parser;

public:
    // Called once before everything gets tested
    DmrppParserTest() :parser()
    {
    }

    // Called at the end of the test
    ~DmrppParserTest()
    {
    }

    // Called before each test
    void setUp()
    {
        if (debug) BESDebug::SetUp("cerr,dmrpp");
    }

    // Called after each test
    void tearDown()
    {
    }

    void test_integers()
    {
        auto_ptr<DMR> dmr(new DMR);
        DmrppTypeFactory dtf;
        dmr->set_factory(&dtf);

        string int_h5 = string(TEST_DATA_DIR).append("/").append("d_int.h5.dmrpp");
        BESDEBUG("dmrpp", "Opening: " << int_h5 << endl);

        ifstream in(int_h5);
        parser.intern(in, dmr.get(), debug);

        D4Group *root = dmr->root();
        CPPUNIT_ASSERT(root);

        CPPUNIT_ASSERT(root->var_end() - root->var_begin() == 4);

        D4Group::Vars_iter v = root->var_begin();
        BESDEBUG("dmrpp", "Looking at variable: " << (*v)->name() << endl);
        CPPUNIT_ASSERT((*v)->name() == "d16_1");
        DmrppCommon *dc = dynamic_cast<DmrppCommon*>(*v);
        CPPUNIT_ASSERT(dc->get_offset() == 2216);
        CPPUNIT_ASSERT(dc->get_size() == 4);

        v++;

        BESDEBUG("dmrpp", "Looking at variable: " << (*v)->name() << endl);
        CPPUNIT_ASSERT((*v)->name() == "d16_2");
        dc = dynamic_cast<DmrppCommon*>(*v);
        CPPUNIT_ASSERT(dc->get_offset() == 2220);
        CPPUNIT_ASSERT(dc->get_size() == 8);

        v++;

        BESDEBUG("dmrpp", "Looking at variable: " << (*v)->name() << endl);
        CPPUNIT_ASSERT((*v)->name() == "d32_1");
        dc = dynamic_cast<DmrppCommon*>(*v);
        CPPUNIT_ASSERT(dc->get_offset() == 2228);
        CPPUNIT_ASSERT(dc->get_size() == 32);

        v++;

        BESDEBUG("dmrpp", "Looking at variable: " << (*v)->name() << endl);
        CPPUNIT_ASSERT((*v)->name() == "d32_2");
        dc = dynamic_cast<DmrppCommon*>(*v);
        CPPUNIT_ASSERT(dc->get_offset() == 2260);
        CPPUNIT_ASSERT(dc->get_size() == 128);
    }

    CPPUNIT_TEST_SUITE( DmrppParserTest );

    CPPUNIT_TEST(test_integers);

    CPPUNIT_TEST_SUITE_END();
        
 };

CPPUNIT_TEST_SUITE_REGISTRATION(DmrppParserTest);

} // namespace dmrpp

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
            test = string("dmrpp::DmrppParserTest::") + argv[i++];

            cerr << endl << "Running test " << test << endl << endl;

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

