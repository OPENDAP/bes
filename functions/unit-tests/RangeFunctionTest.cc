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
#include <Array.h>
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

using namespace CppUnit;
using namespace libdap;
using namespace functions;
using namespace std;

int test_variable_sleep_interval = 0;

class RangeFunctionTest: public TestFixture {
private:
    DDS *small_dds;
    TestTypeFactory btf;

    //CPLErrorHandler orig_err_handler;

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
    RangeFunctionTest() : small_dds(0), src_dir(TEST_SRC_DIR)
    {
        src_dir.append("/scale");   // The test data for scale is just fine...
    }

    ~RangeFunctionTest()
    {
    }

    void setUp()
    {
        DBG(cerr << "setUp() - BEGIN" << endl);
        try {
            small_dds = new DDS(&btf);
            string dds_file = src_dir + "/" + small_dds_file;
            small_dds->parse(dds_file);

            vector<dods_float32> lat(x_size);
            load_var(dynamic_cast<Array*>(small_dds->var("lat")), src_dir + "/" + "small_lat.txt", lat);
            vector<dods_float32> lon(y_size);
            load_var(dynamic_cast<Array*>(small_dds->var("lon")), src_dir + "/" + "small_lon.txt", lon);

            vector<dods_float32> data(x_size * y_size);
            Array *a = dynamic_cast<Array*>(small_dds->var("data"));
            load_var(a, src_dir + "/" + "small_data.txt", data);

            a->get_attr_table().append_attr("missing_value", "String", "-99");

        }
        catch (Error & e) {
            cerr << "SetUp (Error): " << e.get_error_message() << endl;
            throw;
        }
        catch (std::exception &e) {
            cerr << "SetUp (std::exception): " << e.what() << endl;
            throw;
        }

        DBG(cerr << "setUp() - END" << endl);
    }

    void tearDown()
    {
        delete small_dds;
        small_dds = 0;
    }

    void test_reading_data()
    {
        vector<dods_float32> lat_buf(x_size);
        Array *lat = dynamic_cast<Array*>(small_dds->var("lat"));
        lat->value(&lat_buf[0]);
        DBG(cerr << "lat: ");
        DBG(copy(lat_buf.begin(), lat_buf.end(), ostream_iterator<double>(cerr, " ")));
        DBG(cerr << endl);

        CPPUNIT_ASSERT(lat_buf[0] == 4);
        CPPUNIT_ASSERT(lat_buf[x_size - 1] == -4);

        vector<dods_float32> lon_buf(y_size);
        Array *lon = dynamic_cast<Array*>(small_dds->var("lon"));
        lon->value(&lon_buf[0]);
        DBG(cerr << "lon: ");
        DBG(copy(lon_buf.begin(), lon_buf.end(), ostream_iterator<double>(cerr, " ")));
        DBG(cerr << endl);

        CPPUNIT_ASSERT(lon_buf[0] == -0.5);
        CPPUNIT_ASSERT(lon_buf[y_size - 1] == 0.5);

        Array *d = dynamic_cast<Array*>(small_dds->var("data"));
        const int data_size = x_size * y_size;
        vector<dods_float32> data(data_size);
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
    }

    CPPUNIT_TEST_SUITE( RangeFunctionTest );

    CPPUNIT_TEST(test_reading_data);

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
            test = RangeFunctionTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
