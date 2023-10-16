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


    CPPUNIT_TEST_SUITE( MemoryCacheTest );

    //CPPUNIT_TEST(test_inject_data_url_default);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(MemoryCacheTest);

} // namespace ngap


int main(int argc, char*argv[])
{
    bool status = bes_run_tests<ngap::MemoryCacheTest>(argc, argv, "cerr,cache");

    return status ? 0 : 1;
}
