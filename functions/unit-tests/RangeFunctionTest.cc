// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2012 OPeNDAP, Inc.
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

// Tests for the AISResources class.

#include "config.h"

#include <iostream>
#include <sstream>
#include <iterator>
#include <string>
#include <algorithm>
#include <limits>
#include <functional>

#include <cmath>
#if 0
#include <gdal.h>
#include <gdal_priv.h>
#include <gdal_utils.h>
#endif
#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <GetOpt.h>

#include <BaseType.h>
#include <Float32.h>
#include <Float64.h>
#include <Array.h>
#include <Structure.h>
#include <Grid.h>

#include <test/TestTypeFactory.h>

#include <util.h>
#include <debug.h>

#include "RangeFunction.h"

#include "test_config.h"
#include "test_utils.h"

static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

const string small_dds_file = "small.dds";
const string small_float64_dds_file = "small_float64.dds";
const string grid_float64_dds_file = "grid_float64.dds";

using namespace CppUnit;
using namespace libdap;
using namespace functions;
using namespace std;

int test_variable_sleep_interval = 0;

class RangeFunctionTest: public TestFixture {
private:
    DDS *small_float64_dds;
    DDS *grid_float64_dds;
    TestTypeFactory btf;

    string src_dir;
    const static int x_size = 9, y_size = 11;

    /**
     * @brief simple double equality test
     * @see http://stackoverflow.com/questions/17333/most-effective-way-for-float-and-double-comparison
     * @param a
     * @param b
     * @return True if they are within epsilon
     */
    static bool same_as(const double a, const double b)
    {
        // use float's epsilon since double's is too small for these tests
        return fabs(a - b) <= numeric_limits<float>::epsilon();
    }


public:
    RangeFunctionTest() : small_float64_dds(0), grid_float64_dds(0), src_dir(TEST_SRC_DIR)
    {
        src_dir.append("/scale");   // The test data for scale is just fine...
    }

    ~RangeFunctionTest()
    {
    }

    void setUp()
    {
        DBG(cerr << __func__ << "() - BEGIN" << endl);
        try {
            small_float64_dds = new DDS(&btf);
            small_float64_dds->parse(src_dir + "/" + small_float64_dds_file);

            vector<dods_float64> lat(x_size);
            load_var(dynamic_cast<Array*>(small_float64_dds->var("lat")), src_dir + "/" + "small_lat.txt", lat);
            vector<dods_float64> lon(y_size);
            load_var(dynamic_cast<Array*>(small_float64_dds->var("lon")), src_dir + "/" + "small_lon.txt", lon);

            vector<dods_float64> data(x_size * y_size);
            Array *a = dynamic_cast<Array*>(small_float64_dds->var("data"));
            load_var(a, src_dir + "/" + "small_data.txt", data);

            a->get_attr_table().append_attr("missing_value", "String", "-99");

            grid_float64_dds = new DDS(&btf);
            grid_float64_dds->parse(src_dir + "/" + grid_float64_dds_file);
            Grid *g = dynamic_cast<Grid*>(grid_float64_dds->var("data"));
            if (!g) throw InternalErr(__FILE__, __LINE__, " Could not find grid 'data'");

            // Cheat
            if (!dynamic_cast<Array*>(g->var("lat")))
                throw InternalErr(__FILE__, __LINE__, " Could not find 'lat' in 'data'");
            dynamic_cast<Array*>(g->var("lat"))->set_value(lat, lat.size());

            if (!dynamic_cast<Array*>(g->var("lon")))
                throw InternalErr(__FILE__, __LINE__, " Could not find 'lon' in 'data'");
            dynamic_cast<Array*>(g->var("lon"))->set_value(lon, lon.size());

            if (!dynamic_cast<Array*>(g->var("data")))
                throw InternalErr(__FILE__, __LINE__, " Could not find 'arraydata' in 'grid data'");
            dynamic_cast<Array*>(g->var("data"))->set_value(data, data.size());
        }
        catch (Error & e) {
            cerr << "SetUp (Error): " << e.get_error_message() << endl;
            throw;
        }
        catch (std::exception &e) {
            cerr << "SetUp (std::exception): " << e.what() << endl;
            throw;
        }

        DBG(cerr << __func__ << "() - END" << endl);
    }

    void tearDown()
    {
        delete small_float64_dds;
        small_float64_dds = 0;
    }

    void test_reading_data()
    {
        DBG(cerr << __func__ << "() - BEGIN" << endl);

        vector<dods_float64> lat_buf(x_size);
        Array *lat = dynamic_cast<Array*>(small_float64_dds->var("lat"));
        lat->value(&lat_buf[0]);
        DBG(cerr << "lat: ");
        DBG(copy(lat_buf.begin(), lat_buf.end(), ostream_iterator<double>(cerr, " ")));
        DBG(cerr << endl);

        CPPUNIT_ASSERT(lat_buf[0] == 4);
        CPPUNIT_ASSERT(lat_buf[x_size - 1] == -4);

        vector<dods_float64> lon_buf(y_size);
        Array *lon = dynamic_cast<Array*>(small_float64_dds->var("lon"));
        lon->value(&lon_buf[0]);
        DBG(cerr << "lon: ");
        DBG(copy(lon_buf.begin(), lon_buf.end(), ostream_iterator<double>(cerr, " ")));
        DBG(cerr << endl);

        CPPUNIT_ASSERT(lon_buf[0] == -0.5);
        CPPUNIT_ASSERT(lon_buf[y_size - 1] == 0.5);

        Array *d = dynamic_cast<Array*>(small_float64_dds->var("data"));
        const int data_size = x_size * y_size;
        vector<dods_float64> data(data_size);
        d->value(&data[0]);
        DBG(cerr << "data: ");
        DBG(copy(data.begin(), data.end(), ostream_iterator<double>(cerr, " ")));
        DBG(cerr << endl);
        CPPUNIT_ASSERT(data[0] == -99);
        CPPUNIT_ASSERT(data[data_size - 1] == -99);

        DBG(cerr << "data[" << 4 * x_size + 0 << "]: " << data[4 * x_size + 0] << endl);
        DBG(cerr << "data[" << 4 * x_size + 4 << "]: " << data[4 * x_size + 4] << endl);
        CPPUNIT_ASSERT(same_as(data[4 * x_size + 0], -99)); // accounts for rounding error
        CPPUNIT_ASSERT(data[4 * x_size + 4] == 3.5);

        DBG(cerr << __func__ << "() - END" << endl);
    }

    void test_find_min_max_1()
    {
        DBG(cerr << __func__ << "() - BEGIN" << endl);

        // Test the initial state of the little response structure
        min_max_t v;
        DBG(cerr << "v: " << v << endl);
        CPPUNIT_ASSERT(v.min_val == DODS_DBL_MAX);
        CPPUNIT_ASSERT(v.max_val == -DODS_DBL_MAX);
        CPPUNIT_ASSERT(v.monotonic == true);

        DBG(cerr << __func__ << "() - END" << endl);
    }

    void test_find_min_max_2()
    {
        DBG(cerr << __func__ << "() - BEGIN" << endl);

        // Test one and two element vectors
        vector<double> data;
        data.push_back(-100);

        min_max_t v = find_min_max(&data[0], data.size(), false, 0);
        DBG(cerr << "v: " << v << endl);
        CPPUNIT_ASSERT(v.min_val == -100);
        CPPUNIT_ASSERT(v.max_val == -100);
        CPPUNIT_ASSERT(v.monotonic == true);

        data.push_back(100);

        v = find_min_max(&data[0], data.size(), false, 0);
        DBG(cerr << "v: " << v << endl);
        CPPUNIT_ASSERT(v.min_val == -100);
        CPPUNIT_ASSERT(v.max_val == 100);
        CPPUNIT_ASSERT(v.monotonic == true);

        DBG(cerr << __func__ << "() - END" << endl);
    }

    void test_find_min_max_3()
    {
        DBG(cerr << __func__ << "() - BEGIN" << endl);

        // Test one and two element vectors
        vector<double> data;
        data.push_back(-100);
        data.push_back(100);
        data.push_back(-50);
        data.push_back(50);
        data.push_back(-101);
        data.push_back(101);
        data.push_back(-9999);

        min_max_t v = find_min_max(&data[0], data.size(), false, 0);
        DBG(cerr << "v: " << v << endl);
        CPPUNIT_ASSERT(v.min_val == -9999);
        CPPUNIT_ASSERT(v.max_val == 101);
        CPPUNIT_ASSERT(v.monotonic == false);

        v = find_min_max(&data[0], data.size(), true, -9999);
        DBG(cerr << "v: " << v << endl);
        CPPUNIT_ASSERT(v.min_val == -101);
        CPPUNIT_ASSERT(v.max_val == 101);
        CPPUNIT_ASSERT(v.monotonic == false);

        DBG(cerr << __func__ << "() - END" << endl);
    }

    void test_monotonicity_edge_cases()
    {
        DBG(cerr << __func__ << "() - BEGIN" << endl);

        vector<double> data;
        data.push_back(100);

        // One value
        min_max_t v = find_min_max(&data[0], data.size(), false, 0);
        DBG(cerr << "v: " << v << endl);
        CPPUNIT_ASSERT(v.min_val ==  100);
        CPPUNIT_ASSERT(v.max_val == 100);
        CPPUNIT_ASSERT(v.monotonic == true);

        data.push_back(10);

        // Two value
        v = find_min_max(&data[0], data.size(), false, 0);
        DBG(cerr << "v: " << v << endl);
        CPPUNIT_ASSERT(v.min_val ==  10);
        CPPUNIT_ASSERT(v.max_val == 100);
        CPPUNIT_ASSERT(v.monotonic == true);

        data.push_back(20);
        data.push_back(30);
        data.push_back(40);
        data.push_back(50);

        // Third value directtion change
        v = find_min_max(&data[0], data.size(), false, 0);
        DBG(cerr << "v: " << v << endl);
        CPPUNIT_ASSERT(v.min_val ==  10);
        CPPUNIT_ASSERT(v.max_val == 100);
        CPPUNIT_ASSERT(v.monotonic == false);


        // Last value direction change.
        data[0] = 1;
        data.push_back(-9999);
        v = find_min_max(&data[0], data.size(), false, 0);
        DBG(cerr << "v: " << v << endl);
        CPPUNIT_ASSERT(v.min_val ==  -9999);
        CPPUNIT_ASSERT(v.max_val == 50);
        CPPUNIT_ASSERT(v.monotonic == false);


        v = find_min_max(&data[0], data.size(), true, -9999);
        DBG(cerr << "v: " << v << endl);
        CPPUNIT_ASSERT(v.min_val == 1);
        CPPUNIT_ASSERT(v.max_val == 50);
        CPPUNIT_ASSERT(v.monotonic == true);

        DBG(cerr << __func__ << "() - END" << endl);
    }

    // Test arrays - two tests
    void test_range_worker_1()
    {
        DBG(cerr << __func__ << "() - BEGIN" << endl);

        Structure *result = dynamic_cast<Structure*>(range_worker(small_float64_dds->var("data"), -99, true));
        CPPUNIT_ASSERT(result);
        CPPUNIT_ASSERT(result->name() == "range_result_unwrap");

        Float64 *min = dynamic_cast<Float64*>(result->var("min"));
        CPPUNIT_ASSERT(min);
        DBG(cerr << "min: " << min->value() << endl);
        CPPUNIT_ASSERT(min->value() == 1);

        Float64 *max = dynamic_cast<Float64*>(result->var("max"));
        CPPUNIT_ASSERT(max);
        DBG(cerr << "max: " << max->value() << endl);
        CPPUNIT_ASSERT(max->value() == 6.9);

        DBG(cerr << __func__ << "() - END" << endl);
    }

    void test_range_worker_2()
    {
        DBG(cerr << __func__ << "() - BEGIN" << endl);

        Structure *result = dynamic_cast<Structure*>(range_worker(small_float64_dds->var("data"), 0, false));
        CPPUNIT_ASSERT(result);
        CPPUNIT_ASSERT(result->name() == "range_result_unwrap");

        Float64 *min = dynamic_cast<Float64*>(result->var("min"));
        CPPUNIT_ASSERT(min);
        DBG(cerr << "min: " << min->value() << endl);
        CPPUNIT_ASSERT(min->value() == -99);

        Float64 *max = dynamic_cast<Float64*>(result->var("max"));
        CPPUNIT_ASSERT(max);
        DBG(cerr << "max: " << max->value() << endl);
        CPPUNIT_ASSERT(max->value() == 6.9);

        DBG(cerr << __func__ << "() - END" << endl);
    }

    // Test grids - two tests
    void test_range_worker_3()
    {
        DBG(cerr << __func__ << "() - BEGIN" << endl);
        // grid_float64_dds; previous two tests use small_float64_dds and that's
        // the only difference.
        Structure *result = dynamic_cast<Structure*>(range_worker(grid_float64_dds->var("data"), -99, true));
        CPPUNIT_ASSERT(result);
        CPPUNIT_ASSERT(result->name() == "range_result_unwrap");

        Float64 *min = dynamic_cast<Float64*>(result->var("min"));
        CPPUNIT_ASSERT(min);
        DBG(cerr << "min: " << min->value() << endl);
        CPPUNIT_ASSERT(min->value() == 1);

        Float64 *max = dynamic_cast<Float64*>(result->var("max"));
        CPPUNIT_ASSERT(max);
        DBG(cerr << "max: " << max->value() << endl);
        CPPUNIT_ASSERT(max->value() == 6.9);

        DBG(cerr << __func__ << "() - END" << endl);
    }

    void test_range_worker_4()
    {
        DBG(cerr << __func__ << "() - BEGIN" << endl);

        Structure *result = dynamic_cast<Structure*>(range_worker(grid_float64_dds->var("data"), 0, false));
        CPPUNIT_ASSERT(result);
        CPPUNIT_ASSERT(result->name() == "range_result_unwrap");

        Float64 *min = dynamic_cast<Float64*>(result->var("min"));
        CPPUNIT_ASSERT(min);
        DBG(cerr << "min: " << min->value() << endl);
        CPPUNIT_ASSERT(min->value() == -99);

        Float64 *max = dynamic_cast<Float64*>(result->var("max"));
        CPPUNIT_ASSERT(max);
        DBG(cerr << "max: " << max->value() << endl);
        CPPUNIT_ASSERT(max->value() == 6.9);

        DBG(cerr << __func__ << "() - END" << endl);
    }


    CPPUNIT_TEST_SUITE( RangeFunctionTest );

    CPPUNIT_TEST(test_reading_data);
    CPPUNIT_TEST(test_find_min_max_1);
    CPPUNIT_TEST(test_find_min_max_2);
    CPPUNIT_TEST(test_find_min_max_3);
    CPPUNIT_TEST(test_monotonicity_edge_cases);


    CPPUNIT_TEST(test_range_worker_1);
    CPPUNIT_TEST(test_range_worker_2);
    CPPUNIT_TEST(test_range_worker_3);
    CPPUNIT_TEST(test_range_worker_4);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(RangeFunctionTest);

int main(int argc, char*argv[])
{
    GetOpt getopt(argc, argv, "dbh");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;

        case 'b':
            bes_debug = 1;  // debug is a static global
            break;

        case 'h': {     // help - show test names
            cerr << "Usage: RangeFunctionTest has the following tests:" << endl;
            const std::vector<Test*> &tests = RangeFunctionTest::suite()->getTests();
            unsigned int prefix_len = RangeFunctionTest::suite()->getName().append("::").length();
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
            test = RangeFunctionTest::suite()->getName().append("::").append(argv[i++]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
