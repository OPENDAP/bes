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
    MemoryCache<string> string_cache;

public:
    // Called once before everything gets tested
    MemoryCacheTest() = default;
    ~MemoryCacheTest() override = default;
    MemoryCacheTest(const MemoryCacheTest &src) = delete;
    const MemoryCacheTest &operator=(const MemoryCacheTest & rhs) = delete;

    // setUp; Called before each test; not used.
    void setUp() override {
        string_cache.initialize(5, 2);    // holds five things; purge removes two
        string_cache.put("one", "one_1");
        string_cache.put("two", "two_2");
        string_cache.put("three", "three_3");
        string_cache.put("four", "four_4");
        string_cache.put("five", "five_5");
    }

    // tearDown; Called after each test; not used.
    void tearDown() override {
        string_cache.clear();
    }

    void test_zero_items() {
        MemoryCache<string> broken_cache;
        bool status = broken_cache.initialize(0, 2);

        CPPUNIT_ASSERT_MESSAGE("The cache should not initialize with size of zero", status == false);
        CPPUNIT_ASSERT_MESSAGE("The cache invariant should be true", broken_cache.invariant());
    }

    void test_zero_purge() {
        MemoryCache<string> broken_cache;
        bool status = broken_cache.initialize(5, 0);

        CPPUNIT_ASSERT_MESSAGE("The cache should not initialize with purge size of zero", status == false);
        CPPUNIT_ASSERT_MESSAGE("The cache invariant should be true", broken_cache.invariant());
    }

    void test_negative_items() {
        MemoryCache<string> broken_cache;
        bool status = broken_cache.initialize(-5, 2);

        CPPUNIT_ASSERT_MESSAGE("The cache should not initialize with a negative size", status == false);
        CPPUNIT_ASSERT_MESSAGE("The cache invariant should be true", broken_cache.invariant());
    }

    void test_negative_purge_size() {
        MemoryCache<string> broken_cache;
        bool status = broken_cache.initialize(5, -2);

        CPPUNIT_ASSERT_MESSAGE("The cache should not initialize with a negative purge size", status == false);
        CPPUNIT_ASSERT_MESSAGE("The cache invariant should be true", broken_cache.invariant());
    }

    void test_put_one_item() {
        MemoryCache<string> local_cache;
        local_cache.put("one", "one");
        CPPUNIT_ASSERT_MESSAGE("The cache should have one item", local_cache.size() == 1);
        CPPUNIT_ASSERT_MESSAGE("The cache invariant should be true", local_cache.invariant());
    }

    void test_put_five_items() {
        CPPUNIT_ASSERT_MESSAGE("The cache should have five items", string_cache.size() == 5);
        CPPUNIT_ASSERT_MESSAGE("The cache invariant should be true", string_cache.invariant());
    }

    void test_put_six_items_should_purge() {
        string_cache.put("six", "six_6");

        // After adding the sixth item, the cache should have purged two items, leaving four.
        CPPUNIT_ASSERT_MESSAGE("The cache should have four items", string_cache.size() == 4);
        CPPUNIT_ASSERT_MESSAGE("The cache invariant should be true", string_cache.invariant());
        CPPUNIT_ASSERT_MESSAGE("The oldest entry in the fifo of keys should be 'two'",
                               *string_cache.d_fifo_keys.begin() == "three");
    }

    void test_put_three_items_one_purge() {
        MemoryCache<string> small_cache;
        small_cache.initialize(3, 1);    // holds three things; purge removes one
        small_cache.put("one", "one_1");
        small_cache.put("two", "two_2");
        small_cache.put("three", "three_3");
        small_cache.put("four", "four_4");
        small_cache.put("five", "five_5");
        small_cache.put("six", "six_6");

        // After adding the sixth item, the cache should have purged two items, leaving four.
        CPPUNIT_ASSERT_MESSAGE("The cache should have three items", small_cache.size() == 3);
        CPPUNIT_ASSERT_MESSAGE("The cache invariant should be true", small_cache.invariant());
        CPPUNIT_ASSERT_MESSAGE("The oldest entry in the fifo of keys should be 'four'",
                               *small_cache.d_fifo_keys.begin() == "four");
    }

    void test_get_one_item() {
        CPPUNIT_ASSERT_MESSAGE("The cache should have five items", string_cache.size() == 5);
        CPPUNIT_ASSERT_MESSAGE("The cache invariant should be true", string_cache.invariant());

        string value;
        bool status = string_cache.get("three", value);
        CPPUNIT_ASSERT_MESSAGE("The cache should have returned true", status == true);
        CPPUNIT_ASSERT_MESSAGE("The cache should have returned 'three_3'", value == "three_3");
    }

    void test_get_one_item_not_in_cache() {
        CPPUNIT_ASSERT_MESSAGE("The cache should have five items", string_cache.size() == 5);
        CPPUNIT_ASSERT_MESSAGE("The cache invariant should be true", string_cache.invariant());

        string value;
        bool status = string_cache.get("17", value);
        CPPUNIT_ASSERT_MESSAGE("The cache should have returned false - the item is not in the cache", status == false);
        CPPUNIT_ASSERT_MESSAGE("The cache should have returned the empty string", value == "");
    }

    void test_get_one_item_purged() {
        string_cache.put("six", "six_6");   // This triggers a purge of two items

        CPPUNIT_ASSERT_MESSAGE("The cache should have five items", string_cache.size() == 4);
        CPPUNIT_ASSERT_MESSAGE("The cache invariant should be true", string_cache.invariant());

        string value;
        bool status = string_cache.get("one", value);
        CPPUNIT_ASSERT_MESSAGE("The cache should have returned false - the item is not in the cache", status == false);
        CPPUNIT_ASSERT_MESSAGE("The cache should have returned the empty string", value == "");

        status = string_cache.get("two", value);
        CPPUNIT_ASSERT_MESSAGE("The cache should have returned false - the item is not in the cache", status == false);
        CPPUNIT_ASSERT_MESSAGE("The cache should have returned the empty string", value == "");

        status = string_cache.get("three", value);
        CPPUNIT_ASSERT_MESSAGE("The cache should have returned true - the item is in the cache", status == true);
        CPPUNIT_ASSERT_MESSAGE("The cache should have returned 'three_3'", value == "three_3");

        status = string_cache.get("six", value);
        CPPUNIT_ASSERT_MESSAGE("The cache should have returned true - the item is in the cache", status == true);
        CPPUNIT_ASSERT_MESSAGE("The cache should have returned 'six_6'", value == "six_6");
    }

    void test_no_initialize() {
        MemoryCache<string> local_cache;
        string value;
        bool status = local_cache.get("one", value);
        CPPUNIT_ASSERT_MESSAGE("The cache should have returned false - the item is not in the cache", status == false);
        CPPUNIT_ASSERT_MESSAGE("The cache should have returned the empty string", value == "");

        local_cache.put("one", "one");
        CPPUNIT_ASSERT_MESSAGE("The cache should have one item", local_cache.size() == 1);
        CPPUNIT_ASSERT_MESSAGE("The cache invariant should be true", local_cache.invariant());
    }

    CPPUNIT_TEST_SUITE( MemoryCacheTest );

    CPPUNIT_TEST(test_zero_items);
    CPPUNIT_TEST(test_zero_purge);
    CPPUNIT_TEST(test_negative_items);
    CPPUNIT_TEST(test_negative_purge_size);

    CPPUNIT_TEST(test_put_one_item);
    CPPUNIT_TEST(test_put_five_items);
    CPPUNIT_TEST(test_put_six_items_should_purge);
    CPPUNIT_TEST(test_put_three_items_one_purge);

    CPPUNIT_TEST(test_get_one_item);
    CPPUNIT_TEST(test_get_one_item_not_in_cache);
    CPPUNIT_TEST(test_get_one_item_purged);

    CPPUNIT_TEST(test_no_initialize);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(MemoryCacheTest);

} // namespace ngap

int main(int argc, char*argv[])
{
    bool status = bes_run_tests<ngap::MemoryCacheTest>(argc, argv, "cerr,cache");

    return status ? 0 : 1;
}
