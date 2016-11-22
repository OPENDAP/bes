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

class DmrppTypeReadTest: public CppUnit::TestFixture {
private:
    DmrppParserSax2 parser;

public:
    // Called once before everything gets tested
    DmrppTypeReadTest() :
        parser()
    {
    }

    // Called at the end of the test
    ~DmrppTypeReadTest()
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

    /**
     * Evaluates a D4Group against exected values for the name, number of child
     * groups and the number of child variables.
     */
    void checkGroupsAndVars(D4Group *grp, string name, int expectedNumGrps, int expectedNumVars)
    {

        CPPUNIT_ASSERT(grp);
        BESDEBUG("dmrpp", "Checking D4Group '" << grp->name() << "'" << endl);

        CPPUNIT_ASSERT(grp->name() == name);

        int numGroups = grp->grp_end() - grp->grp_begin();
        BESDEBUG("dmrpp", "The D4Group '" << grp->name() << "' has " << numGroups << " child groups." << endl);
        CPPUNIT_ASSERT(numGroups == expectedNumGrps);

        int numVars = grp->var_end() - grp->var_begin();
        BESDEBUG("dmrpp", "The D4Group '" << grp->name() << "' has " << numVars << " child variables." << endl);
        CPPUNIT_ASSERT(numVars == expectedNumVars);

    }

    void test_integer_scalar() {
        auto_ptr<DMR> dmr(new DMR);
        DmrppTypeFactory dtf;
        dmr->set_factory(&dtf);

        string int_h5 = string(TEST_DATA_DIR).append("/").append("t_int_scalar.h5.dmrpp");
        BESDEBUG("dmrpp", "Opening: " << int_h5 << endl);

        ifstream in(int_h5);
        parser.intern(in, dmr.get(), debug);
        BESDEBUG("dmrpp", "Parsing complete"<< endl);

        D4Group *root = dmr->root();

        checkGroupsAndVars(root, "/", 0, 1);

        D4Group::Vars_iter v = root->var_begin();

        DmrppInt32 *di32 = dynamic_cast<DmrppInt32*>(*v);

        CPPUNIT_ASSERT(di32);

        try {
            di32->read();
        }
        catch (BESError &e) {
            CPPUNIT_FAIL(e.get_message());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
        catch (std::exception &e) {
            CPPUNIT_FAIL(e.what());
        }

        CPPUNIT_ASSERT("Passed");
     }

    CPPUNIT_TEST_SUITE( DmrppTypeReadTest );

    CPPUNIT_TEST(test_integer_scalar);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DmrppTypeReadTest);

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
            test = string("dmrpp::DmrppTypeReadTest::") + argv[i++];

            cerr << endl << "Running test " << test << endl << endl;

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

