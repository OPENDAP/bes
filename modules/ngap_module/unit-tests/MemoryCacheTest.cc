// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES component of the Hyrax Data Server.

// Copyright (c) 2023 OPeNDAP, Inc.
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

#include "config.h"

#include <memory>
#include <string>

#include "BESDebug.h"
#include "MemoryCache.h"

#include "test_config.h"
#include "run_tests_cppunit.h"

using namespace std;

#define prolog string("MemoryCacheTest::").append(__func__).append("() - ")

namespace ngap {

class MemoryCacheTest: public CppUnit::TestFixture {

public:
    // Called once before everything gets tested
    MemoryCacheTest() = default;
    ~MemoryCacheTest() override = default;
    MemoryCacheTest(const MemoryCacheTest &src) = delete;
    const MemoryCacheTest &operator=(const MemoryCacheTest & rhs) = delete;

    // setUp; Called before each test; not used.

    // tearDown; Called after each test; not used.

    void test_put_one_item() {
        MemoryCache<string> string_cache(5, 2);    // holds five things; purge removes two
        string_cache.put("one", "one");
        CPPUNIT_ASSERT_MESSAGE("The cache should have one item", string_cache.size() == 1);
        CPPUNIT_ASSERT_MESSAGE("The cache invariant should be true", string_cache.invariant());
    }

    void test_put_five_items() {
        MemoryCache<string> string_cache(5, 2);    // holds five things; purge removes two
        string_cache.put("one", "one_1");
        string_cache.put("two", "two_2");
        string_cache.put("three", "three_3");
        string_cache.put("four", "four_4");
        string_cache.put("five", "five_5");
        CPPUNIT_ASSERT_MESSAGE("The cache should have five items", string_cache.size() == 5);
        CPPUNIT_ASSERT_MESSAGE("The cache invariant should be true", string_cache.invariant());
    }

    void test_put_six_items_should_purge() {
        MemoryCache<string> string_cache(5, 2);    // holds five things; purge removes two
        string_cache.put("one", "one_1");
        string_cache.put("two", "two_2");
        string_cache.put("three", "three_3");
        string_cache.put("four", "four_4");
        string_cache.put("five", "five_5");
        string_cache.put("six", "six_6");

        // After adding the sixth item, the cache should have purged two items, leaving four.
        CPPUNIT_ASSERT_MESSAGE("The cache should have five items", string_cache.size() == 4);
        CPPUNIT_ASSERT_MESSAGE("The cache invariant should be true", string_cache.invariant());
        CPPUNIT_ASSERT_MESSAGE("The oldest entry in the fifo of keys should be 'two'",
                               *string_cache.d_fifo_keys.begin() == "three");
    }

    CPPUNIT_TEST_SUITE( MemoryCacheTest );

    CPPUNIT_TEST(test_put_one_item);
    CPPUNIT_TEST(test_put_five_items);
    CPPUNIT_TEST(test_put_six_items_should_purge);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(MemoryCacheTest);

} // namespace ngap


int main(int argc, char*argv[])
{
    bool status = bes_run_tests<ngap::MemoryCacheTest>(argc, argv, "cerr,cache");

    return status ? 0 : 1;
}
