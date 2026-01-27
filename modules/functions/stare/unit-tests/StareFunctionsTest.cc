// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2019 OPeNDAP, Inc.
// Authors: Kodi Neumiller <kneumiller@opendap.org>
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

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <libdap/BaseType.h>
#include <libdap/Float32.h>
#include <libdap/Array.h>
#include <libdap/Byte.h>
#include <libdap/Int32.h>
#include <libdap/UInt64.h>
#include <libdap/Structure.h>
#include <libdap/D4Group.h>
#include <libdap/D4RValue.h>
#include <libdap/DMR.h>
#include <test/D4TestTypeFactory.h>
#include <test/TestByte.h>
#include <test/TestArray.h>

#include <libdap/util.h>
#include <libdap/debug.h>

#include <TheBESKeys.h>
#include <BESError.h>
#include <BESDebug.h>

#include <STARE.h>

#include "StareFunctions.h"

#include "test_config.h"

using namespace CppUnit;
using namespace libdap;
using namespace std;
using namespace functions;

int test_variable_sleep_interval = 0;

static bool debug = false;
#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

static bool bes_debug = false;

class StareFunctionsTest : public TestFixture {
private:
    DMR *two_arrays_dmr;
    D4BaseTypeFactory *d4_btf;
public:
    StareFunctionsTest() : two_arrays_dmr(nullptr), d4_btf(nullptr) {
        TheBESKeys::ConfigFile = "bes.conf";
        // The key names and module variables used here are defined in StareFunctions.cc
        // These two lines duplicate DapFunctions Module behavior. jhrg 5/21/20
        stare_storage_path = TheBESKeys::TheKeys()->read_string_key(STARE_STORAGE_PATH_KEY, stare_storage_path);
        stare_sidecar_suffix = TheBESKeys::TheKeys()->read_string_key(STARE_SIDECAR_SUFFIX_KEY, stare_sidecar_suffix);

        if (bes_debug) BESDebug::SetUp("cerr,stare,geofile");
    }

    virtual ~StareFunctionsTest() = default;

    virtual void setUp() {
        d4_btf = new D4BaseTypeFactory();

        two_arrays_dmr = new DMR(d4_btf);
        two_arrays_dmr->set_name("test_dmr");

        string filename = string(TOP_SRC_DIR) + "/functions/stare/data/MYD09.A2019003_hacked.h5";
        // Old file name: "/MYD09.A2019003.2040.006.2019005020913.h5";

        two_arrays_dmr->set_filename(filename);
    }

    virtual void tearDown() {
        delete two_arrays_dmr;
        two_arrays_dmr = 0;
        delete d4_btf;
        d4_btf = 0;
    }

    CPPUNIT_TEST_SUITE(StareFunctionsTest);

    // Deprecated test - breaks distcheck CPPUNIT_TEST(test_get_sidecar_file_pathname);
    // jhrg 1.14.20
    CPPUNIT_TEST(test_target_in_dataset);
    CPPUNIT_TEST(test_target_in_dataset_2);
    CPPUNIT_TEST(test_target_in_dataset_3);

    CPPUNIT_TEST(test_count_1);
    CPPUNIT_TEST(test_count_2);
    CPPUNIT_TEST(test_count_3);

    CPPUNIT_TEST(test_stare_subset);

   // CPPUNIT_TEST(test_stare_get_sidecar_uint64_values);

    // fails; rewrite to use the new sidecar files. jhrg 6/17/21
    // CPPUNIT_TEST(intersection_function_test_2);
    // CPPUNIT_TEST(count_function_test_2);
    // CPPUNIT_TEST(subset_function_test_2);

    CPPUNIT_TEST(test_stare_subset_array_helper);

    CPPUNIT_TEST(test_stare_box_helper_1);
    CPPUNIT_TEST(test_stare_box_helper_2);
    CPPUNIT_TEST(test_stare_box_helper_3);
    CPPUNIT_TEST(test_stare_box_helper_4); // This takes ~ 1s
    CPPUNIT_TEST(test_stare_box_helper_5);
    CPPUNIT_TEST(test_stare_box_helper_6);
    CPPUNIT_TEST(test_stare_box_helper_7);

    CPPUNIT_TEST_SUITE_END();

    // STARE_SpatialIntervals stare_box_helper(const point &top_left, const point &bottom_right);

    void stare_box_extent(const STARE_SpatialIntervals &sids, point &tl, point &br) {
        tl.lat = -90, tl.lon = 0;
        br.lat = 90, br.lon = 360;
        
        STARE index(27, 6);
        for (STARE_ArrayIndexSpatialValue sid: sids) {
            DBG2(cerr << "SID: " << sid << endl);
            LatLonDegrees64 lat_lon = index.LatLonDegreesFromValue(sid);
            DBG2(cerr << "lat lon degrees: " << lat_lon.lat << ", " << lat_lon.lon << endl);
            if (lat_lon.lat > tl.lat) tl.lat = lat_lon.lat;
            if (lat_lon.lon > tl.lon) tl.lon = lat_lon.lon;
            if (lat_lon.lat < br.lat) br.lat = lat_lon.lat;
            if (lat_lon.lon < br.lon) br.lon = lat_lon.lon;
        }
    }

    bool d_eq(double t, double v, double delta = 0.001) {
        return abs(t - v) < delta;
    }

    bool i_eq(int t, int v, int delta = 1) {
        return abs(t - v) < delta;
    }

    // Values from the Ubuntu Travis system:
//    .--- test_stare_box_helper() test - BEGIN ---
//    Number of SIDs: 3
//    Box extent: 45,315: 43.0105,315
//    .--- test_stare_box_helper() test - BEGIN ---
//    Number of SIDs: 185
//    Box extent: 45.1242,315.175: 43.9394,313.876
//    .--- test_stare_box_helper() test - BEGIN ---
//    Number of SIDs: 1725
//    Box extent: 45.0155,315.022: 43.9935,313.98
//    .--- test_stare_box_helper() test - BEGIN ---
//    Number of SIDs: 7394
//    Box extent: 45.0049,315.005: 43.9987,313.995
//    .--- test_stare_box_helper() test - BEGIN ---
//    Number of SIDs: 1725
//    Box extent: 45.0155,315.022: 43.9935,313.98
//    .--- test_stare_box_helper() test - BEGIN ---
//    Number of SIDs: 9816
//    Box extent: 47.5207,359.135: 19.9817,279.89
//    .--- test_stare_box_helper() test - BEGIN ---
//    Number of SIDs: 359
//    Box extent: 76.1106,222.448: 71.9216,215.599

    void test_stare_box_helper_1() {
        DBG(cerr << "--- test_stare_box_helper() test - BEGIN ---" << endl);

        point pt1(45.0, 315);
        point pt2(44.0, 314);

        STARE_SpatialIntervals sids = stare_box_helper(pt1, pt2);
        DBG(cerr << "Number of SIDs: " << sids.size() << endl);
        CPPUNIT_ASSERT(sids.size() == 3);

        point tl, br;
        stare_box_extent(sids, tl, br);
        DBG(cerr << "Box extent: " << tl.lat << "," << tl.lon << ": " << br.lat << "," << br.lon << endl);
        CPPUNIT_ASSERT(d_eq(tl.lat, 45.0) && d_eq(tl.lon, 315.0));
        CPPUNIT_ASSERT(d_eq(br.lat, 43.0105) && d_eq(br.lon, 315));
    }

    void test_stare_box_helper_2() {
        DBG(cerr << "--- test_stare_box_helper() test - BEGIN ---" << endl);

        point pt1(45.0, 315.0); // aka 45, 315
        point pt2(44.0, 314.0); // 44, 314

        STARE_SpatialIntervals sids = stare_box_helper(pt1, pt2, 10);
        DBG(cerr << "Number of SIDs: " << sids.size() << endl);
        // different library versions and/or compilers seem to return slightly different
        // results. I don't understand why, but for now am paper overing the differences.
        // jhrg 8/6/21
        CPPUNIT_ASSERT(i_eq(sids.size(), 186, 29));

        point tl, br;
        stare_box_extent(sids, tl, br);
        DBG(cerr << "Box extent: " << tl.lat << "," << tl.lon << ": " << br.lat << "," << br.lon << endl);
        CPPUNIT_ASSERT(d_eq(tl.lat, 45.1242, 3.0) && d_eq(tl.lon, 315.175, 44.0));
        CPPUNIT_ASSERT(d_eq(br.lat, 43.9394, 5.0) && d_eq(br.lon, 313.876, 34.0));
    }

    void test_stare_box_helper_3() {
        DBG(cerr << "--- test_stare_box_helper() test - BEGIN ---" << endl);

        point pt1(45.0, 315.0); // 315 == -45 == 45w
        point pt2(44.0, 314.0);

        STARE_SpatialIntervals sids = stare_box_helper(pt1, pt2, 13);
        DBG(cerr << "Number of SIDs: " << sids.size() << endl);
        CPPUNIT_ASSERT(i_eq(sids.size(), 1729, 427));

        point tl, br;
        stare_box_extent(sids, tl, br);
        DBG(cerr << "Box extent: " << tl.lat << "," << tl.lon << ": " << br.lat << "," << br.lon << endl);
        CPPUNIT_ASSERT(d_eq(tl.lat, 45.0155) && d_eq(tl.lon, 315.022));
        CPPUNIT_ASSERT(d_eq(br.lat, 43.9935) && d_eq(br.lon, 313.98));
    }

    void test_stare_box_helper_4() {
        DBG(cerr << "--- test_stare_box_helper() test - BEGIN ---" << endl);

        point pt1(45.0, 315.0);
        point pt2(44.0, 314.0);

        STARE_SpatialIntervals sids = stare_box_helper(pt1, pt2, 15);
        DBG(cerr << "Number of SIDs: " << sids.size() << endl);
        CPPUNIT_ASSERT(i_eq(sids.size(), 7407, 1839));

        point tl, br;
        stare_box_extent(sids, tl, br);
        DBG(cerr << "Box extent: " << tl.lat << "," << tl.lon << ": " << br.lat << "," << br.lon << endl);
        CPPUNIT_ASSERT(d_eq(tl.lat, 45.0049) && d_eq(tl.lon, 315.005));
        CPPUNIT_ASSERT(d_eq(br.lat, 43.9987) && d_eq(br.lon, 313.995));
        // 45.0049,315.005: 43.9987,313.995
    }

    void test_stare_box_helper_5() {
        DBG(cerr << "--- test_stare_box_helper() test - BEGIN ---" << endl);

        vector<point> points;
        point pt;
        pt.lat = 45.0, pt.lon = 315.0;
        points.push_back(pt);
        pt.lat = 45.0, pt.lon = 314.0;
        points.push_back(pt);
        pt.lat = 44.0, pt.lon = 314.0;
        points.push_back(pt);
        pt.lat = 44.0, pt.lon = 315.0;
        points.push_back(pt);

        STARE_SpatialIntervals sids = stare_box_helper(points, 13);
        DBG(cerr << "Number of SIDs: " << sids.size() << endl);
        CPPUNIT_ASSERT(i_eq(sids.size(), 1729, 427));

        point tl, br;
        stare_box_extent(sids, tl, br);
        DBG(cerr << "Box extent: " << tl.lat << "," << tl.lon << ": " << br.lat << "," << br.lon << endl);
        CPPUNIT_ASSERT(d_eq(tl.lat, 45.0155) && d_eq(tl.lon, 315.022));
        CPPUNIT_ASSERT(d_eq(br.lat, 43.9935) && d_eq(br.lon, 313.98));
    }

    void test_stare_box_helper_6() {
        DBG(cerr << "--- test_stare_box_helper() test - BEGIN ---" << endl);

        point pt1(40.0, 280.0);
        point pt2(20.0, 359.0);

        STARE_SpatialIntervals sids = stare_box_helper(pt1, pt2, 10);
        DBG(cerr << "Number of SIDs: " << sids.size() << endl);
        CPPUNIT_ASSERT(i_eq(sids.size(), 9816, 98));

        point tl, br;
        stare_box_extent(sids, tl, br);
        DBG(cerr << "Box extent: " << tl.lat << "," << tl.lon << ": " << br.lat << "," << br.lon << endl);
        CPPUNIT_ASSERT(d_eq(tl.lat, 47.5207) && d_eq(tl.lon, 359.135));
        CPPUNIT_ASSERT(d_eq(br.lat, 19.9817) && d_eq(br.lon, 279.89));
        // Box extent: 47.5207,359.135: 19.9817,279.89
    }

    // This is a box we can use in regression tests with
    // data/sample_data/336-23xx.20201211/MOD05_L2.A2019336.2315.061.2019337071952.hdf
    void test_stare_box_helper_7() {
        DBG(cerr << "--- test_stare_box_helper() test - BEGIN ---" << endl);

        point pt1(76.0, -144);
        point pt2(72.0, -138.0);

        STARE_SpatialIntervals sids = stare_box_helper(pt1, pt2, 10);
        DBG(cerr << "Number of SIDs: " << sids.size() << endl);
        CPPUNIT_ASSERT(i_eq(sids.size(), 359, 3));

        point tl, br;
        stare_box_extent(sids, tl, br);
        DBG(cerr << "Box extent: " << tl.lat << "," << tl.lon << ": " << br.lat << "," << br.lon << endl);
        CPPUNIT_ASSERT(d_eq(tl.lat, 76.1106) && d_eq(tl.lon, 222.448));
        CPPUNIT_ASSERT(d_eq(br.lat, 71.9216) && d_eq(br.lon, 215.599));
        // Box extent: 76.1106,222.448: 71.9216,215.599
    }

    void test_stare_subset_array_helper() {
        DBG(cerr << "--- test_stare_subset_array_helper() test - BEGIN ---" << endl);

        vector<STARE_ArrayIndexSpatialValue> target_indices = {3440016191299518474, 3440016191299518400, 3440016191299518401};
        // // In these data indices, 3440012343008821258 overlaps 3440016191299518400, 3440016191299518401
        // // and 3440016191299518474 overlaps 3440016191299518474, 3440016191299518400, 3440016191299518401
        // // I think this is kind of a degenerate example since the three target indices seem to be at different
        // // levels. jhrg 1.14.20
        vector<STARE_ArrayIndexSpatialValue> data_indices = {9223372034707292159, 3440012343008821258, 3440016191299518474};

        vector<dods_int16> src_data{100, 200, 300};
        vector<dods_int16> result_data{0, 0, 0};    // result_data is initialized to the mask value

        stare_subset_array_helper(result_data, src_data, target_indices, data_indices);


        CPPUNIT_ASSERT(result_data[0] == 0);
        CPPUNIT_ASSERT(result_data[1] == 200);
        CPPUNIT_ASSERT(result_data[2] == 300);
    }


    void test_stare_subset() {
        DBG(cerr << "--- test_stare_subset() test - BEGIN ---" << endl);

        vector<STARE_ArrayIndexSpatialValue> target_indices = {3440016191299518474, 3440016191299518400, 3440016191299518401};
        // In these data indices, 3440012343008821258 overlaps 3440016191299518400, 3440016191299518401
        // and 3440016191299518474 overlaps 3440016191299518474, 3440016191299518400, 3440016191299518401
        // I think this is kind of a degenerate example since the three target indices seem to be at different
        // levels. jhrg 1.14.20
        vector<STARE_ArrayIndexSpatialValue> data_indices = {9223372034707292159, 3440012343008821258, 3440016191299518474, 3440016191299518528};

        unique_ptr<stare_matches> result = stare_subset_helper(target_indices, data_indices, 2, 2);

        DBG(cerr << "result->row_indices.size(): " << result->row_indices.size() << endl);

        CPPUNIT_ASSERT(result->row_indices.size() == 8);
        CPPUNIT_ASSERT(result->col_indices.size() == 8);
        CPPUNIT_ASSERT(result->stare_indices.size() == 8);

        DBG(cerr << *result << endl);

        CPPUNIT_ASSERT(result->stare_indices.at(0) == 3440012343008821258);
        CPPUNIT_ASSERT(result->row_indices.at(0) == 0);
        CPPUNIT_ASSERT(result->col_indices.at(0) == 1);

        CPPUNIT_ASSERT(result->stare_indices.at(2) == 3440016191299518474);
        CPPUNIT_ASSERT(result->row_indices.at(2) == 1);
        CPPUNIT_ASSERT(result->col_indices.at(2) == 0);

        CPPUNIT_ASSERT(result->stare_indices.at(5) == 3440016191299518528);
        CPPUNIT_ASSERT(result->row_indices.at(5) == 1);
        CPPUNIT_ASSERT(result->col_indices.at(5) == 1);
    }

#if 0
    void test_stare_get_sidecar_uint64_values() {
        DBG(cerr << "--- test_stare_get_sidecar_uint64_values() test - BEGIN ---" << endl);

        try {
            const string filename_1 = string(TOP_SRC_DIR) + "/functions/stare/data/t1.nc";
            Float32 *variable = new Float32("Solar_Zenith");
            vector<STARE_ArrayIndexSpatialValue> values;

            // Call our function.
            get_sidecar_uint64_values(filename_1, variable->name(), values);

            // Check the results.
            if (values.size() != 406 * 270)
                CPPUNIT_FAIL("test_stare_get_sidecar_uint64_values() test failed bad size");
            if (values.at(0) != 3461703427396677225)
                CPPUNIT_FAIL("test_stare_get_sidecar_uint64_values() test failed bad value");
        }
        catch (BESError &e) {
            DBG(cerr << e.get_verbose_message() << endl);
            cout << e.get_verbose_message() << endl;
            CPPUNIT_FAIL("test_stare_get_sidecar_uint64_values() test failed: " + e.get_verbose_message());
        }
    }
#endif

    // The one and only target index is in the 'dataset'
    void test_count_1() {
        DBG(cerr << "--- test_count_1() test - BEGIN ---" << endl);

        vector<STARE_ArrayIndexSpatialValue> target_indices = {3440016191299518474};
        vector<STARE_ArrayIndexSpatialValue> data_indices = {9223372034707292159, 3440012343008821258, 3440016191299518474};

        CPPUNIT_ASSERT(count(target_indices, data_indices) == 1);
    }

    // Of the four target_indices, two are in the 'dataset' and two are not
    void test_count_2() {
        DBG(cerr << "--- test_count_2() test - BEGIN ---" << endl);

        vector<STARE_ArrayIndexSpatialValue> target_indices = {3440016191299518474, 5440016191299518475, 3440016191299518400,
                                              3440016191299518401};
        vector<STARE_ArrayIndexSpatialValue> data_indices = {9223372034707292159, 3440012343008821258, 3440016191299518474};

        // 3440016191299518474 matches 3440016191299518474
        // 3440016191299518400 and 3440016191299518401 match 3440012343008821258
        // Thus three indices in the target set match the dataset indices.
        int matches = count(target_indices, data_indices);
        DBG(cerr << "test_count_2, count(target, dataset): " << matches << endl);

        CPPUNIT_ASSERT(matches == 3);
    }

    // Of the two target_indices, none are in the 'dataset.'
    void test_count_3() {
        DBG(cerr << "--- test_count_3() test - BEGIN ---" << endl);

        vector<STARE_ArrayIndexSpatialValue> target_indices = {3440016191299518400, 3440016191299518401};
        vector<STARE_ArrayIndexSpatialValue> data_indices = {9223372034707292159, 3440012343008821258, 3440016191299518474};

        DBG(cerr << "test_count_3, count(target, dataset): " << count(target_indices, data_indices) << endl);

        CPPUNIT_ASSERT(count(target_indices, data_indices) == 2);
    }

    // target in the 'dataset.'
    void test_target_in_dataset() {
        DBG(cerr << "--- test_target_in_dagtaset() test - BEGIN ---" << endl);

        vector<STARE_ArrayIndexSpatialValue> target_indices = {3440016191299518474};
        vector<STARE_ArrayIndexSpatialValue> data_indices = {9223372034707292159, 3440012343008821258, 3440016191299518474};

        CPPUNIT_ASSERT(target_in_dataset(target_indices, data_indices));
    }

    // target not in the 'dataset.'
    void test_target_in_dataset_2() {
        DBG(cerr << "--- test_target_in_dagtaset_2() test - BEGIN ---" << endl);

        vector<STARE_ArrayIndexSpatialValue> target_indices = {5440016191299518475};// {3440016191299518500};
        vector<STARE_ArrayIndexSpatialValue> data_indices = {9223372034707292159, 3440012343008821258, 3440016191299518474};

        CPPUNIT_ASSERT(!target_in_dataset(target_indices, data_indices));
    }

    // Second target in the 'dataset.'
    void test_target_in_dataset_3() {
        DBG(cerr << "--- test_target_in_dagtaset_3() test - BEGIN ---" << endl);

        vector<STARE_ArrayIndexSpatialValue> target_indices = {3440016191299518500, 3440016191299518474};
        vector<STARE_ArrayIndexSpatialValue> data_indices = {9223372034707292159, 3440012343008821258, 3440016191299518474};

        CPPUNIT_ASSERT(target_in_dataset(target_indices, data_indices));
    }

    void intersection_function_test() {
        DBG(cerr << "--- intersection_function_test() test - BEGIN ---" << endl);

        try {
            // 'a_var' is a dependent variable in the dataset.
            Array *a_var = new TestArray("a_var", new TestByte("a_var"));
            a_var->append_dim(10);

            two_arrays_dmr->root()->add_var_nocopy(a_var);

            //MYD09.A2019003.2040.006.2019005020913_sidecar.h5 values:
            //Lat - 32.2739, 32.2736, 32.2733, 32.2731, 32.2728, 32.2725, 32.2723, 32.272, 32.2718, 32.2715
            //Lon - -98.8324, -98.8388, -98.8452, -98.8516, -98.858, -98.8644, -98.8708, -98.8772, -98.8836, -98.8899
            //Stare - 3440016191299518474 x 10

            //The first index is an actual stare value from: MYD09.A2019003.2040.006.2019005020913_sidecar.h5
            //The final value is made up.
            //
            // This should be a vector<STARE_ArrayIndexSpatialValue> and on OSX that's fine but
            // on CentOS 7 there's an issue because it appears that dods_uint64 is not 'unsigned long long'
            // but instead 'unsigned long int'. This may be an issue all through the unit test code. See
            // below. jhrg 7/8/21
            vector<dods_uint64> target_indices = {3440016721727979534, 3440012343008821258, 3440016322296021006};

            D4RValueList params;
            params.add_rvalue(new D4RValue(a_var));
            params.add_rvalue(new D4RValue(target_indices));

            BaseType *checkHasValue = StareIntersectionFunction::stare_intersection_dap4_function(&params,
                                                                                                  *two_arrays_dmr);

            CPPUNIT_ASSERT(dynamic_cast<Int32 *> (checkHasValue)->value() == 1);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_FAIL("intersection_function_test() test failed");
        }
        catch (BESError &e) {
            DBG(cerr << e.get_verbose_message() << endl);
            CPPUNIT_FAIL("intersection_function_test() test failed");
        }
    }

#if 0
    void intersection_function_test_2() {
        DBG(cerr << "--- intersection_function_test_2() test - BEGIN ---" << endl);

        try {
            // 'a_var' is a dependent variable in the dataset.
            Array *a_var = new TestArray("a_var", new TestByte("a_var"));
            a_var->append_dim(10);

            two_arrays_dmr->root()->add_var_nocopy(a_var);

            //MYD09.A2019003.2040.006.2019005020913_sidecar.h5 values:
            //Lat - 32.2739, 32.2736, 32.2733, 32.2731, 32.2728, 32.2725, 32.2723, 32.272, 32.2718, 32.2715
            //Lon - -98.8324, -98.8388, -98.8452, -98.8516, -98.858, -98.8644, -98.8708, -98.8772, -98.8836, -98.8899
            //Stare - 3440016191299518474 x 10

            //The first index is an actual stare value from: MYD09.A2019003.2040.006.2019005020913_sidecar.h5
            //The final value is made up.
            vector<dods_uint64> target_indices = {3440016721727979534, 3440012343008821258, 3440016322296021006};

            D4RValueList params;
            params.add_rvalue(new D4RValue(a_var));
            params.add_rvalue(new D4RValue(target_indices));

            BaseType *checkHasValue = StareIntersectionFunction::stare_intersection_dap4_function(&params,
                                                                                                  *two_arrays_dmr);

            CPPUNIT_ASSERT(dynamic_cast<Int32 *> (checkHasValue)->value() == 1);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_FAIL("intersection_function_test() test failed");
        }
        catch (BESError &e) {
            DBG(cerr << e.get_verbose_message() << endl);
            CPPUNIT_FAIL("intersection_function_test() test failed");
        }
    }

    void count_function_test() {
        DBG(cerr << "--- count_function_test() test - BEGIN ---" << endl);

        try {
            Array *a_var = new TestArray("a_var", new TestByte("a_var"));
            a_var->append_dim(10);

            two_arrays_dmr->root()->add_var_nocopy(a_var);

            //MYD09.A2019003.2040.006.2019005020913_sidecar.h5 values:
            //Lat - 32.2739, 32.2736, 32.2733, 32.2731, 32.2728, 32.2725, 32.2723, 32.272, 32.2718, 32.2715
            //Lon - -98.8324, -98.8388, -98.8452, -98.8516, -98.858, -98.8644, -98.8708, -98.8772, -98.8836, -98.8899
            //Stare - 3440016191299518474 x 10

            //Array a_var - uint64 for stare indices
            //The first index is an actual stare value from: MYD09.A2019003.2040.006.2019005020913_stare.h5
            //The final value is made up.
            vector<dods_uint64> target_indices = {3440016721727979534, 3440012343008821258, 3440016322296021006};

            D4RValueList params;
            params.add_rvalue(new D4RValue(a_var));
            params.add_rvalue(new D4RValue(target_indices));

            BaseType *checkHasValue = StareCountFunction::stare_count_dap4_function(&params, *two_arrays_dmr);

            CPPUNIT_ASSERT(dynamic_cast<Int32 *> (checkHasValue)->value() == 3);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_FAIL("count_function_test() test failed");
        }
        catch (BESError &e) {
            DBG(cerr << e.get_verbose_message() << endl);
            CPPUNIT_FAIL("count_function_test() test failed");
        }
    }

    void subset_function_test() {
        DBG(cerr << "--- subset_function_test() test - BEGIN ---" << endl);

        try {
            Array *a_var = new TestArray("a_var", new TestByte("a_var"));
            a_var->append_dim(10);

            two_arrays_dmr->root()->add_var_nocopy(a_var);

            //MYD09.A2019003.2040.006.2019005020913_sidecar.h5 values:
            //Lat - 32.2739, 32.2736, 32.2733, 32.2731, 32.2728, 32.2725, 32.2723, 32.272, 32.2718, 32.2715
            //Lon - -98.8324, -98.8388, -98.8452, -98.8516, -98.858, -98.8644, -98.8708, -98.8772, -98.8836, -98.8899
            //Stare - 3440016191299518474 x 10

            //Array a_var - uint64 for stare indices
            //The first index is an actual stare value from: MYD09.A2019003.2040.006.2019005020913_sidecar.h5
            //The final value is made up.
            vector<dods_uint64> target_indices = {3440016721727979534, 3440012343008821258, 3440016322296021006};

            D4RValueList params;
            params.add_rvalue(new D4RValue(a_var));
            params.add_rvalue(new D4RValue(target_indices));

            BaseType *result = StareSubsetFunction::stare_subset_dap4_function(&params, *two_arrays_dmr);

            CPPUNIT_ASSERT(dynamic_cast<Structure *>(result) != nullptr);

            Structure *subset_result = dynamic_cast<Structure *>(result);
            Array *stare = dynamic_cast<Array *>(subset_result->var("stare"));

            CPPUNIT_ASSERT(stare != nullptr);
            vector<STARE_ArrayIndexSpatialValue> result_s_indices;
            stare->value(result_s_indices.data());

            DBG(cerr << "S Indices length: " << result_s_indices.size() << endl);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_FAIL("count_function_test() test failed");
        }
        catch (BESError &e) {
            DBG(cerr << e.get_verbose_message() << endl);
            CPPUNIT_FAIL("count_function_test() test failed");
        }
    }

    void subset_function_test_2() {
        DBG(cerr << "--- subset_function_test_2() test - BEGIN ---" << endl);

         try {
             Array *a_var = new TestArray("a_var", new TestByte("a_var"));
             a_var->append_dim(10);

             two_arrays_dmr->root()->add_var_nocopy(a_var);

             //MYD09.A2019003.2040.006.2019005020913_sidecar.h5 values:
             //Lat - 32.2739, 32.2736, 32.2733, 32.2731, 32.2728, 32.2725, 32.2723, 32.272, 32.2718, 32.2715
             //Lon - -98.8324, -98.8388, -98.8452, -98.8516, -98.858, -98.8644, -98.8708, -98.8772, -98.8836, -98.8899
             //Stare - 3440016191299518474 x 10

             //Array a_var - uint64 for stare indices
             //The first index is an actual stare value from: MYD09.A2019003.2040.006.2019005020913_sidecar.h5
             //The final value is made up.
             vector<STARE_ArrayIndexSpatialValue> target_indices = {3440016721727979534, 3440012343008821258, 3440016322296021006};

             D4RValueList params;
             params.add_rvalue(new D4RValue(a_var));
             params.add_rvalue(new D4RValue(target_indices));

             BaseType *result = StareSubsetFunction::stare_subset_dap4_function(&params, *two_arrays_dmr);

             CPPUNIT_ASSERT(dynamic_cast<Structure*>(result) != nullptr);

             Structure *subset_result = dynamic_cast<Structure*>(result);
             Array *stare = dynamic_cast<Array*>(subset_result->var("stare"));

             CPPUNIT_ASSERT(stare != nullptr);
             vector<STARE_ArrayIndexSpatialValue> result_s_indices;
             stare->value(result_s_indices.data());

             DBG(cerr << "S Indices length: " << result_s_indices.size() << endl);
         }
         catch(Error &e) {
             DBG(cerr << e.get_error_message() << endl);
             CPPUNIT_FAIL("count_function_test() test failed");
         }
         catch(BESError &e) {
             DBG(cerr << e.get_verbose_message() << endl);
             CPPUNIT_FAIL("count_function_test() test failed");
         }
    }
#endif
};

CPPUNIT_TEST_SUITE_REGISTRATION(StareFunctionsTest);

int main(int argc, char *argv[]) {

    int ch;

    while ((ch = getopt(argc, argv, "dDh")) != -1) {
        switch (ch) {
            case 'd':
                debug = true;
                break;
            case 'D':
                bes_debug = true;
                break;
            case 'h': {
                cerr << "StareFunctionsTest has the following tests: " << endl;
                const std::vector<Test *> &tests = StareFunctionsTest::suite()->getTests();
                unsigned int prefix_len = StareFunctionsTest::suite()->getName().append("::").size();
                for (std::vector<Test *>::const_iterator i = tests.begin(), e = tests.end(); i != e; ++i) {
                    cerr << (*i)->getName().replace(0, prefix_len, "") << endl;
                }
                break;
            }
            default:
                break;
        }
    }
    argc -= optind;
    argv += optind;

    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    bool wasSuccessful = true;
    if (argc == 0) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        int i = 0;
        while (i < argc) {
            if (debug) cerr << "Running " << argv[i] << endl;
            string test = StareFunctionsTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
            ++i;
        }
    }

    return wasSuccessful ? 0 : 1;
}
