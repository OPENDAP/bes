// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
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

#include <vector>
#include <string>

#include "modules/common/run_tests_cppunit.h"
#include "test_config.h"

#include "FONcArray.h"

using namespace std;

class FONcArrayTest: public CppUnit::TestFixture {
    FONcArray fa;

public:
    // Called once before everything gets tested
    FONcArrayTest() = default;

    // Called at the end of the test
    ~FONcArrayTest() = default;

    // These equal length tests are not necessary.
#if 0
    // These tests don't define specializations of void setUp() OR void tearDown().
    // jhrg 2/25/22

    void test_equal_length_1() {
        vector<string> stuff;
        stuff.reserve(100);
        for (int i = 0; i < 100; ++i) {
            stuff.emplace_back("test_data");
        }

        CPPUNIT_ASSERT_MESSAGE("All the string are the same length", fa.equal_length(stuff));
    }

    void test_equal_length_2() {
        vector<string> stuff;
        stuff.reserve(100);
        for (int i = 0; i < 100; ++i) {
            stuff.emplace_back("test_data");
        }

        stuff.at(49) = "longer_test_data";

        CPPUNIT_ASSERT_MESSAGE("All the string are not the same length", !fa.equal_length(stuff));
    }

    void test_equal_length_3() {
        vector<string> stuff;
        stuff.reserve(100);
        for (int i = 0; i < 100; ++i) {
            stuff.emplace_back("test_data");
        }

        stuff.at(0) = "longer_test_data";

        CPPUNIT_ASSERT_MESSAGE("All the string are not the same length", !fa.equal_length(stuff));
    }

    void test_equal_length_4() {
        vector<string> stuff;
        stuff.reserve(100);
        for (int i = 0; i < 100; ++i) {
            stuff.emplace_back("test_data");
        }

        stuff.at(99) = "longer_test_data";

        CPPUNIT_ASSERT_MESSAGE("All the string are not the same length", !fa.equal_length(stuff));
    }

    void test_equal_length_for_profiler() {
        vector<string> stuff;
        stuff.reserve(100000000);
        for (int i = 0; i < 100; ++i) {
            stuff.emplace_back("test_data");
        }

        sleep(1);

        CPPUNIT_ASSERT_MESSAGE("All the string are the same length", fa.equal_length(stuff));
    }
#endif

    CPPUNIT_TEST_SUITE( FONcArrayTest );

#if 0
    CPPUNIT_TEST(test_equal_length_1);
    CPPUNIT_TEST(test_equal_length_2);
    CPPUNIT_TEST(test_equal_length_3);
    CPPUNIT_TEST(test_equal_length_4);
#endif

    // equal_length is so fast the profiler does not sample its call. jhrg 10/4/22
    // CPPUNIT_TEST(test_equal_length_for_profiler);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(FONcArrayTest);

int main(int argc, char *argv[])
{
    return bes_run_tests<FONcArrayTest>(argc, argv, "cerr,fonc") ? 0: 1;
}

