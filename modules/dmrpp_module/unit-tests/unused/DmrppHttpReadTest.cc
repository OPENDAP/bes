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

#include <unistd.h>

#include <memory>
#include <iterator>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <libdap/DMR.h>

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

//#include "GetOpt.h"
#include "test_config.h"

using namespace libdap;

static bool debug = false;

namespace dmrpp {

class DmrppHttpReadTest: public CppUnit::TestFixture {
private:
    DmrppParserSax2 parser;

public:
    // Called once before everything gets tested
    DmrppHttpReadTest() :
        parser()
    {
    }

    // Called at the end of the test
    ~DmrppHttpReadTest()
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

        string int_h5 = string(TEST_DATA_DIR).append("/").append("http_t_int_scalar.h5.dmrpp");
        BESDEBUG("dmrpp", "Opening: " << int_h5 << endl);

        ifstream in(int_h5.c_str());
        parser.intern(in, dmr.get(), debug);
        BESDEBUG("dmrpp", "Parsing complete"<< endl);

        D4Group *root = dmr->root();

        checkGroupsAndVars(root, "/", 0, 1);

        D4Group::Vars_iter v = root->var_begin();

        DmrppInt32 *di32 = dynamic_cast<DmrppInt32*>(*v);

        CPPUNIT_ASSERT(di32);

        try {
            di32->read();

            BESDEBUG("dmrpp", "Value: " << di32->value() << endl);
            CPPUNIT_ASSERT(di32->value() == 45);
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

    void test_integer_arrays() {
        auto_ptr<DMR> dmr(new DMR);
        DmrppTypeFactory dtf;
        dmr->set_factory(&dtf);

        string int_h5 = string(TEST_DATA_DIR).append("/").append("http_d_int.h5.dmrpp");
        BESDEBUG("dmrpp", "Opening: " << int_h5 << endl);

        ifstream in(int_h5.c_str());
        parser.intern(in, dmr.get(), debug);
        BESDEBUG("dmrpp", "Parsing complete"<< endl);

        D4Group *root = dmr->root();

        checkGroupsAndVars(root, "/", 0, 4);

        D4Group::Vars_iter v = root->var_begin();

        try {
            DmrppArray *da = dynamic_cast<DmrppArray*>(*v);

            CPPUNIT_ASSERT(da);
            CPPUNIT_ASSERT(da->name() == "d16_1");

            da->read();

            CPPUNIT_ASSERT(da->size() == 2);

            vector<dods_int16> v16(da->size());
            da->value(v16.data());
            BESDEBUG("dmrpp", "v16[0]: " << v16[0] << ", v16[1]: " << v16[1] << endl);
            CPPUNIT_ASSERT(v16[0] == -32768);
            CPPUNIT_ASSERT(v16[1] == 32767);

            ++v;    // next
            da = dynamic_cast<DmrppArray*>(*v);

            CPPUNIT_ASSERT(da);
            CPPUNIT_ASSERT(da->name() == "d16_2");

            da->read();

            CPPUNIT_ASSERT(da->size() == 4);

            vector<dods_int16> v16_2(da->size());
            da->value(v16_2.data());

            if (debug) copy(v16_2.begin(), v16_2.end(), ostream_iterator<dods_int16>(cerr, " "));

            CPPUNIT_ASSERT(v16_2[0] == -32768);
            CPPUNIT_ASSERT(v16_2[3] == 32767);

            ++v;    // next
            da = dynamic_cast<DmrppArray*>(*v);

            CPPUNIT_ASSERT(da);
            CPPUNIT_ASSERT(da->name() == "d32_1");

            da->read();

            CPPUNIT_ASSERT(da->size() == 8);

            vector<dods_int32> v32(da->size());
            da->value(v32.data());

            if (debug) copy(v32.begin(), v32.end(), ostream_iterator<dods_int32>(cerr, " "));

            CPPUNIT_ASSERT(v32[0] == -2147483648);
            CPPUNIT_ASSERT(v32[7] == 2147483647);

            ++v;    // next
            da = dynamic_cast<DmrppArray*>(*v);

            CPPUNIT_ASSERT(da);
            CPPUNIT_ASSERT(da->name() == "d32_2");

            da->read();

            CPPUNIT_ASSERT(da->size() == 32);

            vector<dods_int32> v32_2(da->size());
            da->value(v32_2.data());

            if (debug) copy(v32_2.begin(), v32_2.end(), ostream_iterator<dods_int32>(cerr, " "));

            CPPUNIT_ASSERT(v32_2[0] == -2147483648);
            CPPUNIT_ASSERT(v32_2[31] == 2147483647);
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

    CPPUNIT_TEST_SUITE( DmrppHttpReadTest );

    CPPUNIT_TEST(test_integer_scalar);
    CPPUNIT_TEST(test_integer_arrays);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DmrppHttpReadTest);

} // namespace dmrpp

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    int option_char;
    while ((option_char = getopt(argc, argv, "db")) != -1) {
        switch (option_char) {
        case 'd':
            debug = true;  // debug is a static global
            break;
        default:
            break;
        }
    }

    bool wasSuccessful = true;
    string test = "";
    int i = optind;
    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            test = dmrpp::DmrppHttpReadTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        ++i;
        }
    }
    return wasSuccessful ? 0 : 1;
}

