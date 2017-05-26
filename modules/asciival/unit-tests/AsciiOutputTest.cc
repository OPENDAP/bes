// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of asciival, software which can return an ASCII
// representation of the data read from a DAP server.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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

// Tests for the DataDDS class.

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <DDS.h>
#include <debug.h>

#include "AsciiOutput.h"
#include "AsciiOutputFactory.h"
#include "test_config.h"
#include <GetOpt.h>

// These globals are defined in ascii_val.cc and are needed by the Ascii*
// classes. This code has to be linked with those so that the Ascii*
// specializations of Byte, ..., Grid will be instantiated by DDS when it
// parses a .dds file. Each of those subclasses is a child of AsciiOutput in
// addition to its regular lineage. This test code depends on being able to
// cast each variable to an AsciiOutput object. 01/24/03 jhrg
bool translate = false;
extern int ddsdebug;
using namespace CppUnit;

static bool debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

class AsciiOutputTest: public TestFixture {
private:
    DDS *dds;
    AsciiOutputFactory *aof;

public:
    AsciiOutputTest() {
    }
    ~AsciiOutputTest() {
    }

    void setUp() {
        try {
            DBG2(ddsdebug = 1);
            aof = new AsciiOutputFactory;
            dds = new DDS(aof, "ascii_output_test");
            string parsefile = (string) TEST_SRC_DIR + "/testsuite/AsciiOutputTest1.dds";
            DBG2(cerr << "parsefile: " << parsefile << endl);
            dds->parse(parsefile);
        } catch (Error &e) {
            cerr << "Error: " << e.get_error_message() << endl;
        }
    }

    void tearDown() {
        delete aof;
        aof = 0;
        delete dds;
        dds = 0;
    }

    CPPUNIT_TEST_SUITE(AsciiOutputTest);

    CPPUNIT_TEST(test_get_full_name);

    CPPUNIT_TEST_SUITE_END();

    void test_get_full_name() {
        CPPUNIT_ASSERT(dynamic_cast<AsciiOutput*>(dds->var("a"))->get_full_name() == "a");
        DBG(cerr << "full name: " << dynamic_cast<AsciiOutput*> (dds->var("e.c"))->get_full_name() << endl);

        CPPUNIT_ASSERT(dynamic_cast<AsciiOutput*>(dds->var("e.c"))->get_full_name() == "e.c");
        CPPUNIT_ASSERT(dynamic_cast<AsciiOutput*>(dds->var("f.c"))->get_full_name() == "f.c");
        CPPUNIT_ASSERT(dynamic_cast<AsciiOutput*>(dds->var("g.y"))->get_full_name() == "g.y");
        CPPUNIT_ASSERT(dynamic_cast<AsciiOutput*>(dds->var("k.h.i"))->get_full_name() == "k.h.i");
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(AsciiOutputTest);

int main(int argc, char*argv[]) {

    GetOpt getopt(argc, argv, "dh");
    char option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: AsciiOutputTest has the following tests:" << endl;
            const std::vector<Test*> &tests = AsciiOutputTest::suite()->getTests();
            unsigned int prefix_len = AsciiOutputTest::suite()->getName().append("::").length();
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
            test = AsciiOutputTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}

