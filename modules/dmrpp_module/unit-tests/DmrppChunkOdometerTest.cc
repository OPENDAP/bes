
// This file is part of bes, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2021 OPeNDAP, Inc.
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
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.//
// Created by James Gallagher on 5/5/22.
//

#include <vector>
#include <algorithm>

#include "DmrppChunkOdometer.h"

#include "run_tests_cppunit.h"
#include "test_config.h"

using namespace std;

#define prolog std::string("DmrppChunkOdometerTest::").append(__func__).append("() - ")

namespace dmrpp {

class DmrppChunkOdometerTest: public CppUnit::TestFixture {
private:
    //DmrppChunkOdometer d_odometer;

public:
    // Called once before everything gets tested
    DmrppChunkOdometerTest() = default;

    // Called at the end of the test
    ~DmrppChunkOdometerTest() override = default;

    // Called before each test
    void setUp() override {
        //TheBESKeys::ConfigFile = string(TEST_BUILD_DIR).append("/bes.conf");
    }

    void default_odometer_test() {
        DmrppChunkOdometer odometer;
        const DmrppChunkOdometer::shape &s = odometer.indices();
        CPPUNIT_ASSERT(s.empty());
        CPPUNIT_ASSERT(!odometer.next());
        CPPUNIT_ASSERT(!odometer.next());
    }

    void oneD_odometer_test() {
        vector<unsigned long long> array_size{40000};
        vector<unsigned long long> chunk_size{10000};
        DmrppChunkOdometer odometer(array_size, chunk_size);
        const DmrppChunkOdometer::shape &s = odometer.indices();
        CPPUNIT_ASSERT(s.size() == 1);
        CPPUNIT_ASSERT(s.at(0) == 0);
        CPPUNIT_ASSERT(odometer.next());
        const DmrppChunkOdometer::shape &s2 = odometer.indices();
        CPPUNIT_ASSERT(s2.at(0) == 10000);
        CPPUNIT_ASSERT(odometer.next());
        CPPUNIT_ASSERT(odometer.next());
        const DmrppChunkOdometer::shape &s3 = odometer.indices();
        CPPUNIT_ASSERT(s3.at(0) == 30000);
        CPPUNIT_ASSERT(!odometer.next());
        const DmrppChunkOdometer::shape &s4 = odometer.indices();
        CPPUNIT_ASSERT(s4.at(0) == 0);
    }

    void twoD_odometer_test() {
        vector<unsigned long long> array_size{40, 40};
        vector<unsigned long long> chunk_size{20, 20};
        DmrppChunkOdometer odometer(array_size, chunk_size);
        const DmrppChunkOdometer::shape &s = odometer.indices();
        CPPUNIT_ASSERT(s.size() == 2);
        CPPUNIT_ASSERT(s.at(0) == 0);
        CPPUNIT_ASSERT(s.at(1) == 0);
        CPPUNIT_ASSERT(odometer.next());
        const DmrppChunkOdometer::shape &s2 = odometer.indices();
        CPPUNIT_ASSERT(s2.at(0) == 0);
        CPPUNIT_ASSERT(s2.at(1) == 20);
        CPPUNIT_ASSERT(odometer.next());
        const DmrppChunkOdometer::shape &s3 = odometer.indices();
        CPPUNIT_ASSERT(s3.at(0) == 20);
        CPPUNIT_ASSERT(s3.at(1) == 0);
        CPPUNIT_ASSERT(odometer.next());
        const DmrppChunkOdometer::shape &s4 = odometer.indices();
        CPPUNIT_ASSERT(s4.at(0) == 20);
        CPPUNIT_ASSERT(s4.at(1) == 20);
        CPPUNIT_ASSERT(!odometer.next());
    }

    void oneD_asym_odometer_test() {
        vector<unsigned long long> array_size{40000};
        vector<unsigned long long> chunk_size{9501};
        DmrppChunkOdometer odometer(array_size, chunk_size);
        const DmrppChunkOdometer::shape &s = odometer.indices();
        CPPUNIT_ASSERT(s.size() == 1);
        CPPUNIT_ASSERT(s.at(0) == 0);
        CPPUNIT_ASSERT(odometer.next());
        const DmrppChunkOdometer::shape &s2 = odometer.indices();
        CPPUNIT_ASSERT(s2.at(0) == 9501);
        CPPUNIT_ASSERT(odometer.next());
        CPPUNIT_ASSERT(odometer.next());
        const DmrppChunkOdometer::shape &s3 = odometer.indices();
        CPPUNIT_ASSERT(s3.at(0) == 28503);
        CPPUNIT_ASSERT(odometer.next());
        const DmrppChunkOdometer::shape &s4 = odometer.indices();
        CPPUNIT_ASSERT(s4.at(0) == 38004);
        CPPUNIT_ASSERT(!odometer.next());
    }

    void threeD_asym_odometer_test() {
        vector<unsigned long long> array_size{100, 50, 200};
        vector<unsigned long long> chunk_size{41, 50, 53};
        DmrppChunkOdometer odometer(array_size, chunk_size);
        const DmrppChunkOdometer::shape &s = odometer.indices();
        CPPUNIT_ASSERT(s.size() == 3);
        CPPUNIT_ASSERT(s.at(0) == 0);
        CPPUNIT_ASSERT(s.at(1) == 0);
        CPPUNIT_ASSERT(s.at(2) == 0);
        CPPUNIT_ASSERT(odometer.next());
        CPPUNIT_ASSERT(odometer.next());
        CPPUNIT_ASSERT(odometer.next());
        const DmrppChunkOdometer::shape &s2 = odometer.indices();
        CPPUNIT_ASSERT(s2.at(0) == 0);
        CPPUNIT_ASSERT(s2.at(1) == 0);
        CPPUNIT_ASSERT(s2.at(2) == 159);
        CPPUNIT_ASSERT(odometer.next());
        const DmrppChunkOdometer::shape &s3 = odometer.indices();
        DBG(for_each(s3.begin(), s3.end(), [](unsigned long long x) { cerr << x << " "; }));
        CPPUNIT_ASSERT(s3.at(0) == 41);
        CPPUNIT_ASSERT(s3.at(1) == 0);
        CPPUNIT_ASSERT(s3.at(2) == 0);
        CPPUNIT_ASSERT(odometer.next());
        CPPUNIT_ASSERT(odometer.next());
        CPPUNIT_ASSERT(odometer.next());
        CPPUNIT_ASSERT(odometer.next());
        const DmrppChunkOdometer::shape &s4 = odometer.indices();
        DBG(for_each(s4.begin(), s4.end(), [](unsigned long long x) { cerr << x << " "; }));
        CPPUNIT_ASSERT(s4.at(0) == 82);
        CPPUNIT_ASSERT(s4.at(1) == 0);
        CPPUNIT_ASSERT(s4.at(2) == 0);
    }

    void threeD_loop_odometer_test() {
        vector<unsigned long long> array_size{100, 50, 200};
        vector<unsigned long long> chunk_size{41, 23, 53};
        DmrppChunkOdometer odometer(array_size, chunk_size);
        int count = 0;
        do {
            const DmrppChunkOdometer::shape &s = odometer.indices();
            DBG(for_each(s.begin(), s.end(), [](unsigned long long x) { cerr << x << " "; }));
            ++count;
        } while (odometer.next());

        CPPUNIT_ASSERT(count == 36);
    }

    CPPUNIT_TEST_SUITE( DmrppChunkOdometerTest );

    CPPUNIT_TEST(default_odometer_test);
    CPPUNIT_TEST(oneD_odometer_test);
    CPPUNIT_TEST(twoD_odometer_test);

    CPPUNIT_TEST(oneD_asym_odometer_test);
    CPPUNIT_TEST(threeD_asym_odometer_test);
    CPPUNIT_TEST(threeD_loop_odometer_test);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DmrppChunkOdometerTest);

} // namespace dmrpp

int main(int argc, char*argv[])
{
    return bes_run_tests<dmrpp::DmrppChunkOdometerTest>(argc, argv, "cerr,dmz") ? 0 : 1;
}
