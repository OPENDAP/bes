// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2006 OPeNDAP, Inc.
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

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <BaseType.h>
#include <Int32.h>
#include <Float64.h>
#include <Str.h>
#include <Array.h>
#include <Grid.h>
#include <DDS.h>
#include <DAS.h>
#include <GetOpt.h>
#include <GridGeoConstraint.h>
#include <util.h>
#include <debug.h>

#include "test/TestTypeFactory.h"

#include <test_config.h>

using namespace CppUnit;
using namespace libdap;
using namespace std;
using namespace functions;

int test_variable_sleep_interval = 0;

static bool debug = false;
static bool debug2 = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false)
#undef DBG2
#define DBG2(x) do { if (debug2) (x); } while(false)

namespace functions {

class GridGeoConstraintTest: public TestFixture {
private:
    TestTypeFactory btf;
    ConstraintEvaluator ce;

    DDS *geo_dds;
    DDS *geo_dds_3d;
    DDS *geo_dds_coads_lon;

public:
    GridGeoConstraintTest() :
        geo_dds(0), geo_dds_3d(0), geo_dds_coads_lon(0)
    {
    }
    ~GridGeoConstraintTest()
    {
    }

    void setUp()
    {
        // geo grid test data
        try {
            geo_dds = new DDS(&btf);
            geo_dds->parse((string) TEST_SRC_DIR + "/ce-functions-testsuite/geo_grid.dds");
            DAS das;
            das.parse((string) TEST_SRC_DIR + "/ce-functions-testsuite/geo_grid.das");
            geo_dds->transfer_attributes(&das);

            DBG2(geo_dds->print_xml(stderr, false, "No blob"));

            // Load values into the grid variables
            Grid & sst1 = dynamic_cast<Grid &>(*geo_dds->var("SST1"));

            Array & lon1 = dynamic_cast<Array &>(**sst1.map_begin());
            dods_float64 tmp_lon1[10] = { 0, 40, 80, 120, 160, 200, 240, 280, 320, 359 };
            lon1.val2buf(tmp_lon1);
            lon1.set_read_p(true);

            Array & lat1 = dynamic_cast<Array &>(**(sst1.map_begin() + 1));
            dods_float64 tmp_lat1[10] = { 40, 30, 20, 10, 0, -10, -20, -30, -40, -50 };
            lat1.val2buf(tmp_lat1);
            lat1.set_read_p(true);

            dods_byte tmp_data[10][10] = { { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }, { 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 },
                { 20, 21, 22, 23, 24, 25, 26, 27, 28, 29 }, { 30, 31, 32, 33, 34, 35, 36, 37, 38, 39 }, { 40, 41, 42,
                    43, 44, 45, 46, 47, 48, 49 }, { 50, 51, 52, 53, 54, 55, 56, 57, 58, 59 }, { 60, 61, 62, 63, 64, 65,
                    66, 67, 68, 69 }, { 70, 71, 72, 73, 74, 75, 76, 77, 78, 79 }, { 80, 81, 82, 83, 84, 85, 86, 87, 88,
                    89 }, { 90, 91, 92, 93, 94, 95, 96, 97, 98, 99 } };
            sst1.get_array()->val2buf((void*) tmp_data);
            sst1.get_array()->set_read_p(true);

            // Load values into the grid variables
            Grid & sst1_1 = dynamic_cast<Grid &>(*geo_dds->var("SST1_1"));

            Array & lat1_1 = dynamic_cast<Array &>(**sst1_1.map_begin());
            dods_float64 tmp_lat1_1[10] = { 40, 30, 20, 10, 0, -10, -20, -30, -40, -50 };
            lat1_1.val2buf(tmp_lat1_1);
            lat1_1.set_read_p(true);

            Array & lon1_1 = dynamic_cast<Array &>(**(sst1_1.map_begin() + 1));
            dods_float64 tmp_lon1_1[10] = { 0, 40, 80, 120, 160, 200, 240, 280, 320, 359 };
            lon1_1.val2buf(tmp_lon1_1);
            lon1_1.set_read_p(true);

            sst1_1.get_array()->val2buf((void*) tmp_data);
            sst1_1.get_array()->set_read_p(true);

            // Load values into the grid variables
            Grid & sst2 = dynamic_cast<Grid &>(*geo_dds->var("SST2"));

            Array & lon2 = dynamic_cast<Array &>(**sst2.map_begin());
            dods_float64 tmp_lon2[10] = { 0, 40, 80, 120, 160, -160, -120, -80, -40, -1 };
            //    { -180, -120, -80, -40, 0, 40, 80, 120, 160, 179 };
            lon2.val2buf(tmp_lon2);
            lon2.set_read_p(true);
            DBG2(cerr << "lon2[0]: " << dynamic_cast<Float64*>(lon2.var(0))->value() << endl);

            Array & lat2 = dynamic_cast<Array &>(**(sst2.map_begin() + 1));
            dods_float64 tmp_lat2[10] = { 40, 30, 20, 10, 0, -10, -20, -30, -40, -50 };
            lat2.val2buf(tmp_lat2);
            lat2.set_read_p(true);

            sst2.get_array()->val2buf((void*) tmp_data);
            sst2.get_array()->set_read_p(true);

            // Load values into the grid variables
            Grid & sst3 = dynamic_cast<Grid &>(*geo_dds->var("SST3"));

            Array & lat3 = dynamic_cast<Array &>(**sst3.map_begin());
            dods_float64 tmp_lat3[10] = { -40, -30, -20, -10, 0, 10, 20, 30, 40, 50 };
            lat3.val2buf(tmp_lat3);
            lat3.set_read_p(true);

            Array & lon3 = dynamic_cast<Array &>(**(sst3.map_begin() + 1));
            dods_float64 tmp_lon3[10] = { 20, 60, 100, 140, 180, 220, 260, 300, 340, 379 };
            lon3.val2buf(tmp_lon3);
            lon3.set_read_p(true);

            sst3.get_array()->val2buf((void*) tmp_data);
            sst3.get_array()->set_read_p(true);

            // Build the three dimensional grid
            geo_dds_3d = new DDS(&btf);
            geo_dds_3d->parse((string) TEST_SRC_DIR + "/ce-functions-testsuite/geo_grid_3d.dds");
            // Load values into the grid variables
            Grid & sst4 = dynamic_cast<Grid &>(*geo_dds_3d->var("SST4"));

            Array & time = dynamic_cast<Array&>(**sst4.map_begin());
            dods_int32 tmp_time[3] = { 0, 1, 2 };
            time.val2buf(tmp_time);
            time.set_read_p(true);

            Array & lon4 = dynamic_cast<Array &>(**(sst4.map_begin() + 1));
            dods_float64 tmp_lon4[5] = { 160, 200, 240, 280, 320 };
            lon4.val2buf(tmp_lon4);
            lon4.set_read_p(true);

            Array & lat4 = dynamic_cast<Array &>(**(sst4.map_begin() + 2));
            dods_float64 tmp_lat4[5] = { 40, 30, 20, 10, 0 };
            lat4.val2buf(tmp_lat4);
            lat4.set_read_p(true);

            dods_byte tmp_data4[3][5][5] = { { { 0, 1, 2, 3, 4 }, { 10, 11, 12, 13, 14 }, { 20, 21, 22, 23, 24 }, { 30,
                31, 32, 33, 34 }, { 40, 41, 42, 43, 44 } }, { { 100, 101, 102, 103, 104 }, { 110, 111, 112, 113, 114 },
                { 120, 121, 122, 123, 124 }, { 130, 131, 132, 133, 134 }, { 140, 141, 142, 143, 144 } }, { { 200, 201,
                202, 203, 204 }, { 210, 211, 212, 213, 214 }, { 220, 221, 222, 223, 224 }, { 230, 231, 232, 233, 234 },
                { 240, 241, 242, 243, 244 } } };
            sst4.get_array()->val2buf((void*) tmp_data4);
            sst4.get_array()->set_read_p(true);

            DBG2(sst4.print_val(stderr));

            geo_dds_coads_lon = new DDS(&btf);
            geo_dds_coads_lon->parse((string) TEST_SRC_DIR + "/ce-functions-testsuite/geo_grid_coads_lon.dds");
            Grid & sst5 = dynamic_cast<Grid &>(*geo_dds_coads_lon->var("SST5"));
            Array & lon5 = dynamic_cast<Array &>(**sst5.map_begin());
            dods_float64 tmp_lon5[15] = { 41, 81, 121, 161, 201, 241, 281, 321, 361, 365, 370, 375, 380, 385, 390 };
            lon5.val2buf(tmp_lon5);
            lon5.set_read_p(true);

            Array & lat5 = dynamic_cast<Array &>(**(sst5.map_begin() + 1));
            dods_float64 tmp_lat5[5] = { 20, 10, 0, -10, -20 };
            lat5.val2buf(tmp_lat5);
            lat5.set_read_p(true);

            dods_byte tmp_data5[15][5] = { { 0, 1, 2, 3, 4 }, { 10, 11, 12, 13, 14 }, { 20, 21, 22, 23, 24 }, { 30, 31,
                32, 33, 34 }, { 40, 41, 42, 43, 44 }, { 100, 101, 102, 103, 104 }, { 110, 111, 112, 113, 114 }, { 120,
                121, 122, 123, 124 }, { 130, 131, 132, 133, 134 }, { 140, 141, 142, 143, 144 }, { 200, 201, 202, 203,
                204 }, { 210, 211, 212, 213, 214 }, { 220, 221, 222, 223, 224 }, { 230, 231, 232, 233, 234 }, { 240,
                241, 242, 243, 244 } };
            sst5.get_array()->val2buf((void*) tmp_data5);
            sst5.get_array()->set_read_p(true);
            DBG2(sst5.print_val(stderr));
        }

        catch (Error & e) {
            cerr << "SetUp: " << e.get_error_message() << endl;
            throw;
        }
    }

    void tearDown()
    {
        delete geo_dds;
        delete geo_dds_3d;
        delete geo_dds_coads_lon;

    }

CPPUNIT_TEST_SUITE( GridGeoConstraintTest );

#if 1
    CPPUNIT_TEST(geoconstraint_build_lat_lon_maps_test);
    CPPUNIT_TEST(lat_lon_dimensions_ok_test);
    CPPUNIT_TEST(transform_longitude_to_pos_notation_test);
    CPPUNIT_TEST(find_longitude_indeces_test);
    CPPUNIT_TEST(categorize_latitude_test);
    CPPUNIT_TEST(find_latitude_indeces_test);
    CPPUNIT_TEST(set_array_using_double_test);
    CPPUNIT_TEST(reorder_longitude_map_test);
    // See the comment at the function...
    // CPPUNIT_TEST(reorder_data_longitude_axis_test);
    CPPUNIT_TEST(set_bounding_box_test1);
    CPPUNIT_TEST(set_bounding_box_test2);
    CPPUNIT_TEST(set_bounding_box_test3);
    CPPUNIT_TEST(set_bounding_box_test4);
    CPPUNIT_TEST(set_bounding_box_test5);
    CPPUNIT_TEST(set_bounding_box_test6);
    CPPUNIT_TEST(set_bounding_box_test7);
    CPPUNIT_TEST(apply_constriant_to_data_test);
    CPPUNIT_TEST(apply_constriant_to_data_test2);
    CPPUNIT_TEST(apply_constriant_to_data_test3);
    CPPUNIT_TEST(apply_constriant_to_data_test4);
#endif
    CPPUNIT_TEST(apply_constriant_to_data_test_three_arg);
    CPPUNIT_TEST(apply_constriant_to_data_test2_three_arg);
    CPPUNIT_TEST(apply_constriant_to_data_test3_three_arg);
    CPPUNIT_TEST(apply_constriant_to_data_test4_three_arg);

    CPPUNIT_TEST_SUITE_END()
    ;

    void geoconstraint_build_lat_lon_maps_test()
    {
        try {
            Grid *g = dynamic_cast<Grid*>(geo_dds->var("SST1"));
            CPPUNIT_ASSERT(g);
            DBG(cerr << "Grid: " << g->name());
            DBG(g->print_decl(cerr, "    ", true));
            GridGeoConstraint gc1(g);
            CPPUNIT_ASSERT(gc1.build_lat_lon_maps());

            g = dynamic_cast<Grid*>(geo_dds->var("SST2"));
            CPPUNIT_ASSERT(g);
            GridGeoConstraint gc2(g);
            CPPUNIT_ASSERT(gc2.build_lat_lon_maps());

            DBG(cerr << "geoconstraint_build_lat_lon_maps_test midpoint" << endl);

            g = dynamic_cast<Grid*>(geo_dds->var("SST3"));
            CPPUNIT_ASSERT(g);
            GridGeoConstraint gc3(g);
            CPPUNIT_ASSERT(gc3.build_lat_lon_maps());

            g = dynamic_cast<Grid*>(geo_dds_3d->var("SST4"));
            CPPUNIT_ASSERT(g);
            GridGeoConstraint gc4(g);
            CPPUNIT_ASSERT(gc4.build_lat_lon_maps());
            CPPUNIT_ASSERT(gc4.d_latitude == *(g->map_begin() + 2));
            CPPUNIT_ASSERT(gc4.d_longitude == *(g->map_begin() + 1));
        }
        catch (Error &e) {
            DBG(cerr << "Error: " << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"build_lat_lon_maps_test() failed");
        }
    }

    void lat_lon_dimensions_ok_test()
    {
        try {
            Grid *g = dynamic_cast<Grid*>(geo_dds->var("SST1"));
            CPPUNIT_ASSERT(g);
            GridGeoConstraint gc1(g);
            CPPUNIT_ASSERT(gc1.build_lat_lon_maps());
            CPPUNIT_ASSERT(gc1.lat_lon_dimensions_ok());
            CPPUNIT_ASSERT(gc1.is_longitude_rightmost() == false);

            Grid *g3 = dynamic_cast<Grid*>(geo_dds->var("SST3"));
            CPPUNIT_ASSERT(g3);
            GridGeoConstraint gc3(g3);
            CPPUNIT_ASSERT(gc3.build_lat_lon_maps());
            CPPUNIT_ASSERT(gc3.lat_lon_dimensions_ok());
            CPPUNIT_ASSERT(gc3.is_longitude_rightmost() == true);
        }
        catch (Error &e) {
            DBG(cerr << "Error: " << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"lat_lon_dimensions_ok_test() failed");
        }
    }

    void transform_longitude_to_pos_notation_test()
    {
        try {
            Grid *g = dynamic_cast<Grid*>(geo_dds->var("SST2"));
            CPPUNIT_ASSERT(g);
            GridGeoConstraint gc2(g);
            CPPUNIT_ASSERT(gc2.build_lat_lon_maps());

            CPPUNIT_ASSERT(gc2.d_lon[0] == 0);
            CPPUNIT_ASSERT(gc2.d_lon[gc2.d_lon_length - 1] == -1);

            GeoConstraint::Notation map_notation = gc2.categorize_notation(gc2.d_lon[0],
                gc2.d_lon[gc2.d_lon_length - 1]);
            CPPUNIT_ASSERT(map_notation == GeoConstraint::neg_pos);

            gc2.transform_longitude_to_pos_notation();
            DBG(cerr << "gc2.d_lon[0]: " << gc2.d_lon[0] << endl);
            CPPUNIT_ASSERT(gc2.d_lon[0] == 0);
            DBG(cerr << "gc2.d_lon[gc2.d_lon_length-1]: " << gc2.d_lon[gc2.d_lon_length - 1] << endl);
            CPPUNIT_ASSERT(gc2.d_lon[gc2.d_lon_length - 1] == 359);

        }
        catch (Error &e) {
            DBG(cerr << "Error: " << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"transform_map_to_pos_notation_test() failed");
        }
    }

    void find_longitude_indeces_test()
    {
        try {
            //  { 0, 40, 80, 120, 160, 200, 240, 280, 320, 359 };
            Grid *g = dynamic_cast<Grid*>(geo_dds->var("SST1"));
            CPPUNIT_ASSERT(g);
            GridGeoConstraint gc1(g);

            int left_i, right_i;
            gc1.find_longitude_indeces(40.0, 200.0, left_i, right_i);
            CPPUNIT_ASSERT(left_i == 1);
            CPPUNIT_ASSERT(right_i == 5);

            g = dynamic_cast<Grid*>(geo_dds->var("SST1"));
            CPPUNIT_ASSERT(g);
            GridGeoConstraint gc2(g);

            gc2.find_longitude_indeces(200, 40.0, left_i, right_i);
            CPPUNIT_ASSERT(left_i == 5);
            CPPUNIT_ASSERT(right_i == 1);

            g = dynamic_cast<Grid*>(geo_dds->var("SST1"));
            CPPUNIT_ASSERT(g);
            GridGeoConstraint gc3(g);

            gc3.find_longitude_indeces(5.0, 81.0, left_i, right_i);
            DBG(cerr << "left_i: " << left_i << endl);
            DBG(cerr << "right_i: " << right_i << endl);
            CPPUNIT_ASSERT(left_i == 0);
            CPPUNIT_ASSERT(right_i == 3);

            g = dynamic_cast<Grid*>(geo_dds->var("SST1"));
            CPPUNIT_ASSERT(g);
            GridGeoConstraint gc4(g);

            gc4.find_longitude_indeces(81.0, 5.0, left_i, right_i);
            DBG(cerr << "left_i: " << left_i << endl);
            DBG(cerr << "right_i: " << right_i << endl);
            CPPUNIT_ASSERT(left_i == 2);
            CPPUNIT_ASSERT(right_i == 1);

            // lon: { 41, 81, 121, 161, 201, 241, 281, 321, 361, 365, 370, 375, 380, 385, 390 };
            g = dynamic_cast<Grid*>(geo_dds_coads_lon->var("SST5"));
            CPPUNIT_ASSERT(g);
            GridGeoConstraint gc5(g);

            gc5.find_longitude_indeces(5.0, 81.0, left_i, right_i);
            DBG(cerr << "left_i: " << left_i << endl);
            DBG(cerr << "right_i: " << right_i << endl);
            CPPUNIT_ASSERT(left_i == 9);
            CPPUNIT_ASSERT(right_i == 1);

            g = dynamic_cast<Grid*>(geo_dds_coads_lon->var("SST5"));
            CPPUNIT_ASSERT(g);
            GridGeoConstraint gc6(g);

            gc6.find_longitude_indeces(81.0, 5.0, left_i, right_i);
            DBG(cerr << "left_i: " << left_i << endl);
            DBG(cerr << "right_i: " << right_i << endl);
            CPPUNIT_ASSERT(left_i == 1);
            CPPUNIT_ASSERT(right_i == 9);
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
    }

    void categorize_latitude_test()
    {
        Grid *g = dynamic_cast<Grid*>(geo_dds->var("SST1"));
        CPPUNIT_ASSERT(g);
        GridGeoConstraint gc1(g);

        CPPUNIT_ASSERT(gc1.categorize_latitude() == GeoConstraint::normal);

        Grid *g3 = dynamic_cast<Grid*>(geo_dds->var("SST3"));
        CPPUNIT_ASSERT(g3);
        GridGeoConstraint gc3(g3);

        CPPUNIT_ASSERT(gc3.categorize_latitude() == GeoConstraint::inverted);
    }

    void find_latitude_indeces_test()
    {
        // SST1 lat: { 40, 30, 20, 10, 0, -10, -20, -30, -40, -50 };
        Grid *g = dynamic_cast<Grid*>(geo_dds->var("SST1"));
        int top_i, bottom_i;

        CPPUNIT_ASSERT(g);

        GridGeoConstraint gc0(g);
        gc0.find_latitude_indeces(20, -20, GeoConstraint::normal, top_i, bottom_i);
        DBG(cerr << "SST1, top: " << top_i << ", bottom: " << bottom_i << endl);
        CPPUNIT_ASSERT(top_i == 2);
        CPPUNIT_ASSERT(bottom_i == 6);

        // SST1 lat: { 40, 30, 20, 10, 0, -10, -20, -30, -40, -50 };
        GridGeoConstraint gc1(g);
        gc1.find_latitude_indeces(35, 5, GeoConstraint::normal, top_i, bottom_i);
        DBG(cerr << "SST1, top: " << top_i << ", bottom: " << bottom_i << endl);
        CPPUNIT_ASSERT(top_i == 0);
        CPPUNIT_ASSERT(bottom_i == 4);

        // SST1 lat: { 40, 30, 20, 10, 0, -10, -20, -30, -40, -50 };
        GridGeoConstraint gc2(g);
        gc2.find_latitude_indeces(25, -25, GeoConstraint::normal, top_i, bottom_i);
        DBG(cerr << "SST1, top: " << top_i << ", bottom: " << bottom_i << endl);
        CPPUNIT_ASSERT(top_i == 1);
        CPPUNIT_ASSERT(bottom_i == 7);

        // SST1 lat: { 40, 30, 20, 10, 0, -10, -20, -30, -40, -50 };
        GridGeoConstraint gc3(g);
        gc3.find_latitude_indeces(-15, -35, GeoConstraint::normal, top_i, bottom_i);
        DBG(cerr << "SST1, top: " << top_i << ", bottom: " << bottom_i << endl);
        CPPUNIT_ASSERT(top_i == 5);
        CPPUNIT_ASSERT(bottom_i == 8);

        // SST3 lat: { -40, -30, -20, -10, 0, 10, 20, 30, 40, 50 };
        g = dynamic_cast<Grid*>(geo_dds->var("SST3"));
        CPPUNIT_ASSERT(g);

        GridGeoConstraint gc4(g);
        gc4.find_latitude_indeces(20, -20, GeoConstraint::inverted, top_i, bottom_i);
        DBG(cerr << "SST3, top: " << top_i << ", bottom: " << bottom_i << endl);
        CPPUNIT_ASSERT(top_i == 6);
        CPPUNIT_ASSERT(bottom_i == 2);

        // SST3 lat: { -40, -30, -20, -10, 0, 10, 20, 30, 40, 50 };
        GridGeoConstraint gc5(g);
        gc5.find_latitude_indeces(35, 5, GeoConstraint::inverted, top_i, bottom_i);
        DBG(cerr << "SST3, top: " << top_i << ", bottom: " << bottom_i << endl);
        CPPUNIT_ASSERT(top_i == 8);
        CPPUNIT_ASSERT(bottom_i == 4);

        // SST3 lat: { -40, -30, -20, -10, 0, 10, 20, 30, 40, 50 };
        GridGeoConstraint gc6(g);
        gc6.find_latitude_indeces(-5, -35, GeoConstraint::inverted, top_i, bottom_i);
        DBG(cerr << "SST3, top: " << top_i << ", bottom: " << bottom_i << endl);
        CPPUNIT_ASSERT(top_i == 4);
        CPPUNIT_ASSERT(bottom_i == 0);

        // SST3 lat: { -40, -30, -20, -10, 0, 10, 20, 30, 40, 50 };
        GridGeoConstraint gc7(g);
        gc7.find_latitude_indeces(25, -25, GeoConstraint::inverted, top_i, bottom_i);
        DBG(cerr << "SST3, top: " << top_i << ", bottom: " << bottom_i << endl);
        CPPUNIT_ASSERT(top_i == 7);
        CPPUNIT_ASSERT(bottom_i == 1);
    }

    void set_array_using_double_test()
    {
        try {
            Grid *g = dynamic_cast<Grid*>(geo_dds->var("SST1"));
            Array *lon = dynamic_cast<Array*>(*g->map_begin());
            double ten_values[10] = { -1, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
            set_array_using_double(lon, ten_values, 10);
            CPPUNIT_ASSERT(extract_double_value(lon->var(0)) == ten_values[0]);
            CPPUNIT_ASSERT(extract_double_value(lon->var(9)) == ten_values[9]);

            Int32 *i = new Int32("");
            Array *a = new Array("", i);
            a->append_dim(10);
            int dummy_values[10] = { 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 };
            a->val2buf((void*) dummy_values);
            a->set_read_p(true);
            CPPUNIT_ASSERT(extract_double_value(a->var(0)) == 10.0);
            CPPUNIT_ASSERT(extract_double_value(a->var(9)) == 19.0);
            set_array_using_double(a, ten_values, 10);
            CPPUNIT_ASSERT(extract_double_value(a->var(0)) == ten_values[0]);
            CPPUNIT_ASSERT(extract_double_value(a->var(9)) == ten_values[9]);
        }
        catch (Error &e) {
            DBG(cerr << "Error: " << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Error in set_array_using_double_test.");
        }
    }

    void reorder_longitude_map_test()
    {
        Grid *g = dynamic_cast<Grid*>(geo_dds->var("SST1"));
        CPPUNIT_ASSERT(g);
        GridGeoConstraint gc1(g);
        // Longitude map: { 0, 40, 80, 120, 160, 200, 240, 280, 320, 359 }

        gc1.reorder_longitude_map(7);

        CPPUNIT_ASSERT(gc1.d_lon[0] == 280);
        CPPUNIT_ASSERT(gc1.d_lon[2] == 359);
        CPPUNIT_ASSERT(gc1.d_lon[3] == 0);
        CPPUNIT_ASSERT(gc1.d_lon[6] == 120);
        CPPUNIT_ASSERT(gc1.d_lon[9] == 240);
    }
#if 0
    // This test is broken because reorder...() uses read and I haven't worked
    // out how to get the data used here into the grid so that read() called
    // elsewhere will return it. Might try using the series_values property to
    // create a predictable set of values...
    void reorder_data_longitude_axis_test()
    {
        try {
            Grid *g = dynamic_cast<Grid*>(geo_dds->var("SST1"));
            CPPUNIT_ASSERT(g);
            GridGeoConstraint gc1(g);
            /* Data values for Grid SST1:
             { { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
             { 10,11,12,13,14,15,16,17,18,19},
             { 20,21,22,23,24,25,26,27,28,29},
             { 30,31,32,33,34,35,36,37,38,39},
             { 40,41,42,43,44,45,46,47,48,49},
             { 50,51,52,53,54,55,56,57,58,59},
             { 60,61,62,63,64,65,66,67,68,69},
             { 70,71,72,73,74,75,76,77,78,79},
             { 80,81,82,83,84,85,86,87,88,89},
             { 90,91,92,93,94,95,96,97,98,99} };
             */
            gc1.d_longitude_index_left = 5;
            gc1.d_longitude_index_right = 1;

            cerr << "Before gc1.reorder_data_longitude_axis();" << endl;
            gc1.reorder_data_longitude_axis();
            cerr << "past gc1.reorder_data_longitude_axis();" << endl;

            // Read the data out into local storage
            dods_byte *tmp_data2=0;
            dods_byte **tmp_data2_ptr = &tmp_data2;
            int size = g->get_array()->buf2val((void**)tmp_data2_ptr);
            cerr << "size = " << size << endl;

            cerr << "tmp_data2[0]: " << (int)tmp_data2[0] << endl;
            CPPUNIT_ASSERT(tmp_data2[0] == 5);
            CPPUNIT_ASSERT(tmp_data2[9] == 4);
            CPPUNIT_ASSERT(tmp_data2[10] == 15);
            CPPUNIT_ASSERT(tmp_data2[90] == 95);
            CPPUNIT_ASSERT(tmp_data2[99] == 94);
        }
        catch (Error &e) {
            DBG(cerr << "Error: " << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Error in reorder_data_longitude_axis_test.");
        }
    }
#endif

    void set_bounding_box_test1()
    {
        try {
            // SST1 uses pos notation; constraint uses pos
            Grid *g = dynamic_cast<Grid*>(geo_dds->var("SST1"));
            CPPUNIT_ASSERT(g);
            GridGeoConstraint gc1(g);
            // lat: { 40, 30, 20, 10, 0, -10, -20, -30, -40, -50 };
            // This should be lon 1 to 5 and lat 0 to 3
            gc1.set_bounding_box(40.0, 40.0, 10.0, 200.0);

            CPPUNIT_ASSERT(gc1.d_longitude_index_left == 1);
            CPPUNIT_ASSERT(gc1.d_longitude_index_right == 5);

            CPPUNIT_ASSERT(gc1.d_latitude_index_top == 0);
            CPPUNIT_ASSERT(gc1.d_latitude_index_bottom == 3);

        }
        catch (Error &e) {
            DBG(cerr << "Error: " << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Error in set_bounding_box_test.");
        }
    }

    void set_bounding_box_test2()
    {
        try {
            Grid *g2 = dynamic_cast<Grid*>(geo_dds->var("SST1"));
            CPPUNIT_ASSERT(g2);
            GridGeoConstraint gc2(g2);
            // SST1 with a constraint that uses neg_pos notation for lon
            gc2.set_bounding_box(40.0, 40.0, 10.0, -160.0);
            CPPUNIT_ASSERT(gc2.d_longitude_index_left == 1);
            CPPUNIT_ASSERT(gc2.d_longitude_index_right == 5);

            CPPUNIT_ASSERT(gc2.d_latitude_index_top == 0);
            CPPUNIT_ASSERT(gc2.d_latitude_index_bottom == 3);

        }
        catch (Error &e) {
            DBG(cerr << "Error: " << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Error in set_bounding_box_test.");
        }
    }

    void set_bounding_box_test3()
    {
        try {
            Grid *g3 = dynamic_cast<Grid*>(geo_dds_3d->var("SST4"));
            CPPUNIT_ASSERT(g3);
            GridGeoConstraint gc3(g3);
            // SST1 with a constraint that uses neg_pos notation for lon
            gc3.set_bounding_box(30.0, 200.0, 20.0, 280.0);
            CPPUNIT_ASSERT(gc3.d_longitude_index_left == 1);
            CPPUNIT_ASSERT(gc3.d_longitude_index_right == 3);

            CPPUNIT_ASSERT(gc3.d_latitude_index_top == 1);
            CPPUNIT_ASSERT(gc3.d_latitude_index_bottom == 2);

        }
        catch (Error &e) {
            DBG(cerr << "Error: " << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Error in set_bounding_box_test.");
        }
    }

    void set_bounding_box_test4()
    {
        // lon: { 160, 200, 240, 280, 320 }
        // lat: { 40, 30, 20, 10, 0 }
        try {
            Grid *g3 = dynamic_cast<Grid*>(geo_dds_3d->var("SST4"));
            CPPUNIT_ASSERT(g3);
            GridGeoConstraint gc3(g3);
            // SST1 with a constraint that uses neg_pos notation for lon
            gc3.set_bounding_box(30.0, 0, 20.0, 150);
            CPPUNIT_ASSERT(!"Should fail since the BB box contains no data");

        }
        catch (Error &e) {
            DBG(cerr << "Error: " << e.get_error_message() << endl);
            CPPUNIT_ASSERT("Caught Error.");
        }
    }

    void set_bounding_box_test5()
    {
        // lon: { 160, 200, 240, 280, 320 }
        // lat: { 40, 30, 20, 10, 0 }
        try {
            Grid *g3 = dynamic_cast<Grid*>(geo_dds_3d->var("SST4"));
            CPPUNIT_ASSERT(g3);
            GridGeoConstraint gc3(g3);
            // SST1 with a constraint that uses neg_pos notation for lon
            gc3.set_bounding_box(30.0, 0, 20.0, 170);
            CPPUNIT_ASSERT("Should not fail since the BB box contains data");

        }
        catch (Error &e) {
            DBG(cerr << "Error: " << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Should not throw Error.");
        }
    }

    void set_bounding_box_test6()
    {
        // lon: { 160, 200, 240, 280, 320 }
        // lat: { 40, 30, 20, 10, 0 }
        try {
            Grid *g3 = dynamic_cast<Grid*>(geo_dds_3d->var("SST4"));
            CPPUNIT_ASSERT(g3);
            GridGeoConstraint gc3(g3);
            // SST1 with a constraint that uses neg_pos notation for lon
            gc3.set_bounding_box(60.0, 170, 50.0, 270);
            CPPUNIT_FAIL("Should fail since the BB box contains no data");

        }
        catch (Error &e) {
            DBG(cerr << "Error: " << e.get_error_message() << endl);
            CPPUNIT_ASSERT("Cought Error.");
        }
    }

    void set_bounding_box_test7()
    {
        // lon: 20 --> 359
        // lat: { -40, -30, -20, -10, 0, 10, 20, 30, 40, 50 };
        try {
            Grid *g3 = dynamic_cast<Grid*>(geo_dds->var("SST3"));
            CPPUNIT_ASSERT(g3);
            GridGeoConstraint gc3(g3);
            // SST1 with a constraint that uses neg_pos notation for lon
            gc3.set_bounding_box(70.0, 170, 60.0, 270);
            CPPUNIT_ASSERT(!"Should fail since the BB box contains no data");

        }
        catch (Error &e) {
            DBG(cerr << "Error: " << e.get_error_message() << endl);
            CPPUNIT_ASSERT("Cought Error.");
        }
    }

    void apply_constriant_to_data_test()
    {
        try {
            Grid *g2 = dynamic_cast<Grid*>(geo_dds->var("SST1"));
            CPPUNIT_ASSERT(g2);
            g2->set_send_p(true);

            DBG(g2->print_decl(cerr, "    ", true, false, true));
            DBG(g2->print_val(cerr, "    ", false));
            DBG(cerr << endl << endl);

            GridGeoConstraint gc2(g2);
            // top, left, bottom, right
            gc2.set_bounding_box(30, 40, 10, 120);

            /* lat: { 40, 30, 20, 10, 0, -10, -20, -30, -40, -50 };
             lon: { 0, 40, 80, 120, 160, 200, 240, 280, 320, 359 };

             Data values for Grid SST1:
             { { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
             { 10,11,12,13,14,15,16,17,18,19},
             { 20,21,22,23,24,25,26,27,28,29},
             { 30,31,32,33,34,35,36,37,38,39},
             { 40,41,42,43,44,45,46,47,48,49},
             { 50,51,52,53,54,55,56,57,58,59},
             { 60,61,62,63,64,65,66,67,68,69},
             { 70,71,72,73,74,75,76,77,78,79},
             { 80,81,82,83,84,85,86,87,88,89},
             { 90,91,92,93,94,95,96,97,98,99} };
             */

            gc2.apply_constraint_to_data();

            CPPUNIT_ASSERT(gc2.d_latitude->length() == 3);
            CPPUNIT_ASSERT(gc2.d_longitude->length() == 3);

            double *lats = 0;
            double **lats_ptr = &lats;
            gc2.d_latitude->buf2val((void**) lats_ptr);
            CPPUNIT_ASSERT(lats[0] == 30.0);
            CPPUNIT_ASSERT(lats[2] == 10.0);

            double *lons = 0;
            double **lons_ptr = &lons;
            gc2.d_longitude->buf2val((void**) lons_ptr);
            CPPUNIT_ASSERT(lons[0] == 40.0);
            CPPUNIT_ASSERT(lons[2] == 120.0);

            DBG(gc2.get_constrained_grid()->print_decl(cerr, "    ", true, false, true));
            DBG(gc2.get_constrained_grid()->print_val(cerr, "    ", false));
            DBG(cerr << endl << endl);
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
    }

    void apply_constriant_to_data_test_three_arg()
    {
        try {
            Grid *g2 = dynamic_cast<Grid*>(geo_dds->var("SST1"));
            CPPUNIT_ASSERT(g2);
            g2->set_send_p(true);

            DBG(g2->print_decl(cerr, "    ", true, false, true));
            DBG(g2->print_val(cerr, "    ", false));
            DBG(cerr << endl << endl);

            Array *lat2 = dynamic_cast<Array*>(geo_dds->var("SST1.lat"));
            CPPUNIT_ASSERT(lat2);
            Array *lon2 = dynamic_cast<Array*>(geo_dds->var("SST1.lon"));
            CPPUNIT_ASSERT(lon2);

            GridGeoConstraint gc2(g2, lat2, lon2);
            // top, left, bottom, right
            gc2.set_bounding_box(30, 40, 10, 120);

            /* lat: { 40, 30, 20, 10, 0, -10, -20, -30, -40, -50 };
             lon: { 0, 40, 80, 120, 160, 200, 240, 280, 320, 359 };

             Data values for Grid SST1:
             { { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
             { 10,11,12,13,14,15,16,17,18,19},
             { 20,21,22,23,24,25,26,27,28,29},
             { 30,31,32,33,34,35,36,37,38,39},
             { 40,41,42,43,44,45,46,47,48,49},
             { 50,51,52,53,54,55,56,57,58,59},
             { 60,61,62,63,64,65,66,67,68,69},
             { 70,71,72,73,74,75,76,77,78,79},
             { 80,81,82,83,84,85,86,87,88,89},
             { 90,91,92,93,94,95,96,97,98,99} };
             */

            gc2.apply_constraint_to_data();

            CPPUNIT_ASSERT(gc2.d_latitude->length() == 3);
            CPPUNIT_ASSERT(gc2.d_longitude->length() == 3);

            double *lats = 0;
            double **lats_ptr = &lats;
            gc2.d_latitude->buf2val((void**) lats_ptr);
            CPPUNIT_ASSERT(lats[0] == 30.0);
            CPPUNIT_ASSERT(lats[2] == 10.0);

            double *lons = 0;
            double **lons_ptr = &lons;
            gc2.d_longitude->buf2val((void**) lons_ptr);
            CPPUNIT_ASSERT(lons[0] == 40.0);
            CPPUNIT_ASSERT(lons[2] == 120.0);

            DBG(gc2.get_constrained_grid()->print_decl(cerr, "    ", true, false, true));
            DBG(gc2.get_constrained_grid()->print_val(cerr, "    ", false));
            DBG(cerr << endl << endl);
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
    }

    void apply_constriant_to_data_test2()
    {
        try {
            Grid *g = dynamic_cast<Grid*>(geo_dds_3d->var("SST4"));
            CPPUNIT_ASSERT(g);
            g->set_send_p(true);

            GridGeoConstraint gc(g);
            // top, left, bottom, right
            gc.set_bounding_box(30.0, 200.0, 20.0, 280.0);

            /* time[3] = { 0, 1, 2 };
             lon4[5] = { 160, 200, 240, 280, 320 };
             lat4[5] = { 40, 30, 20, 10, 0 };
             dods_byte tmp_data4[3][5][5] =
             {
             { { 0, 1, 2, 3, 4},
             { 10,11,12,13,14},
             { 20,21,22,23,24},
             { 30,31,32,33,34},
             { 40,41,42,43,44}  },
             { { 100, 101, 102, 103, 104},
             { 110, 111, 112, 113, 114},
             { 120, 121, 122, 123, 124},
             { 130, 131, 132, 133, 134},
             { 140, 141, 142, 143, 144}  },
             { { 200, 201, 202, 203, 204},
             { 210, 211, 212, 213, 214},
             { 220, 221, 222, 223, 224},
             { 230, 231, 232, 233, 234},
             { 240, 241, 242, 243, 244}  }
             };
             */

            gc.apply_constraint_to_data();

            CPPUNIT_ASSERT(gc.d_latitude->length() == 2);
            CPPUNIT_ASSERT(gc.d_longitude->length() == 3);

            double *lats = 0;
            double **lats_ptr = &lats;
            gc.d_latitude->buf2val((void**) lats_ptr);
            CPPUNIT_ASSERT(lats[0] == 30.0);
            CPPUNIT_ASSERT(lats[1] == 20.0);

            double *lons = 0;
            double **lons_ptr = &lons;
            gc.d_longitude->buf2val((void**) lons_ptr);
            CPPUNIT_ASSERT(lons[0] == 200.0);
            CPPUNIT_ASSERT(lons[2] == 280.0);

            DBG(gc.get_constrained_grid()->print_decl(cerr, "    ", true, false, true));
            DBG(gc.get_constrained_grid()->print_val(cerr, "    ", false));
            DBG(cerr << endl << endl);
        }
        catch (Error &e) {
            DBG(cerr << "Error: " << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"apply_constriant_to_data_test caught Error");
        }
    }

    void apply_constriant_to_data_test2_three_arg()
    {
        try {
            Grid *g = dynamic_cast<Grid*>(geo_dds_3d->var("SST4"));
            CPPUNIT_ASSERT(g);
            g->set_send_p(true);

            Array *lat = dynamic_cast<Array*>(geo_dds_3d->var("SST4.lat"));
            CPPUNIT_ASSERT(lat);
            Array *lon = dynamic_cast<Array*>(geo_dds_3d->var("SST4.lon"));
            CPPUNIT_ASSERT(lon);

            GridGeoConstraint gc(g, lat, lon);

            gc.set_bounding_box(30.0, 200.0, 20.0, 280.0);

            /* time[3] = { 0, 1, 2 };
             lon4[5] = { 160, 200, 240, 280, 320 };
             lat4[5] = { 40, 30, 20, 10, 0 };
             dods_byte tmp_data4[3][5][5] =
             {
             { { 0, 1, 2, 3, 4},
             { 10,11,12,13,14},
             { 20,21,22,23,24},
             { 30,31,32,33,34},
             { 40,41,42,43,44}  },
             { { 100, 101, 102, 103, 104},
             { 110, 111, 112, 113, 114},
             { 120, 121, 122, 123, 124},
             { 130, 131, 132, 133, 134},
             { 140, 141, 142, 143, 144}  },
             { { 200, 201, 202, 203, 204},
             { 210, 211, 212, 213, 214},
             { 220, 221, 222, 223, 224},
             { 230, 231, 232, 233, 234},
             { 240, 241, 242, 243, 244}  }
             };
             */

            gc.apply_constraint_to_data();

            CPPUNIT_ASSERT(gc.d_latitude->length() == 2);
            CPPUNIT_ASSERT(gc.d_longitude->length() == 3);

            double *lats = 0;
            double **lats_ptr = &lats;
            gc.d_latitude->buf2val((void**) lats_ptr);
            CPPUNIT_ASSERT(lats[0] == 30.0);
            CPPUNIT_ASSERT(lats[1] == 20.0);

            double *lons = 0;
            double **lons_ptr = &lons;
            gc.d_longitude->buf2val((void**) lons_ptr);
            CPPUNIT_ASSERT(lons[0] == 200.0);
            CPPUNIT_ASSERT(lons[2] == 280.0);

            DBG(gc.get_constrained_grid()->print_decl(cerr, "    ", true, false, true));
            DBG(gc.get_constrained_grid()->print_val(cerr, "    ", false));
            DBG(cerr << endl << endl);
        }
        catch (Error &e) {
            DBG(cerr << "Error: " << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"apply_constriant_to_data_test caught Error");
        }
    }

    // This tests the 'stitch' feature of the GridGeoConstraint/GeoConstraint
    void apply_constriant_to_data_test3()
    {
        try {
            Grid *g2 = dynamic_cast<Grid*>(geo_dds->var("SST3"));
            CPPUNIT_ASSERT(g2);
            g2->set_send_p(true);

            GridGeoConstraint gc2(g2);

            gc2.set_bounding_box(30, 300, 30, 60);

            /* lat:  { -40, -30, -20, -10, 0, 10, 20, 30, 40, 50 };
             lon: { 20, 60, 100, 140, 180, 220, 260, 300, 340, 379 };

             Data values for Grid SST1:
             { { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
             { 10,11,12,13,14,15,16,17,18,19},
             { 20,21,22,23,24,25,26,27,28,29},
             { 30,31,32,33,34,35,36,37,38,39},
             { 40,41,42,43,44,45,46,47,48,49},
             { 50,51,52,53,54,55,56,57,58,59},
             { 60,61,62,63,64,65,66,67,68,69},
             { 70,71,72,73,74,75,76,77,78,79},
             { 80,81,82,83,84,85,86,87,88,89},
             { 90,91,92,93,94,95,96,97,98,99} };
             */

            gc2.apply_constraint_to_data();

            DBG(cerr << "gc2.d_latitude->length(): " << gc2.d_latitude->length() << endl);
            DBG(cerr << "gc2.d_longitude->length(): " << gc2.d_longitude->length() << endl);

            double *lats = 0;
            double **lats_ptr = &lats;
            gc2.d_latitude->buf2val((void**) lats_ptr);
            DBG(cerr << "lats[0]: " << lats[0] << endl);

            double *lons = 0;
            double **lons_ptr = &lons;
            gc2.d_longitude->buf2val((void**) lons_ptr);
            DBG(cerr << "lons[0]: " << lons[0] << endl);
            DBG(cerr << "lons[gc2.d_longitude->length()-1]: " << lons[gc2.d_longitude->length() - 1] << endl);
            //CPPUNIT_ASSERT(lons[0] == 260.0);
            //CPPUNIT_ASSERT(lons[4] == 20.0);

            CPPUNIT_ASSERT(gc2.d_latitude->length() == 1);
            CPPUNIT_ASSERT(gc2.d_longitude->length() == 5);

            CPPUNIT_ASSERT(lats[0] == 30.0);

            CPPUNIT_ASSERT(lons[0] == 300.0);
            CPPUNIT_ASSERT(lons[4] == 60.0);

            DBG(gc2.get_constrained_grid()->print_decl(cerr, "    ", true, false, true));
            DBG(gc2.get_constrained_grid()->print_val(cerr, "    ", false));
            DBG(cerr << endl << endl);
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
    }

    // This tests the 'stitch' feature of the GridGeoConstraint/GeoConstraint
    void apply_constriant_to_data_test3_three_arg()
    {
        try {
            Grid *g2 = dynamic_cast<Grid*>(geo_dds->var("SST3"));
            CPPUNIT_ASSERT(g2);
            g2->set_send_p(true);

            Array *lat2 = dynamic_cast<Array*>(geo_dds->var("SST3.lat"));
            CPPUNIT_ASSERT(lat2);
            Array *lon2 = dynamic_cast<Array*>(geo_dds->var("SST3.lon"));
            CPPUNIT_ASSERT(lon2);

            GridGeoConstraint gc2(g2, lat2, lon2);

            gc2.set_bounding_box(30, 300, 30, 60);

            /* lat:  { -40, -30, -20, -10, 0, 10, 20, 30, 40, 50 };
             lon: { 20, 60, 100, 140, 180, 220, 260, 300, 340, 379 };

             Data values for Grid SST1:
             { { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
             { 10,11,12,13,14,15,16,17,18,19},
             { 20,21,22,23,24,25,26,27,28,29},
             { 30,31,32,33,34,35,36,37,38,39},
             { 40,41,42,43,44,45,46,47,48,49},
             { 50,51,52,53,54,55,56,57,58,59},
             { 60,61,62,63,64,65,66,67,68,69},
             { 70,71,72,73,74,75,76,77,78,79},
             { 80,81,82,83,84,85,86,87,88,89},
             { 90,91,92,93,94,95,96,97,98,99} };
             */

            gc2.apply_constraint_to_data();

            DBG(cerr << "gc2.d_latitude->length(): " << gc2.d_latitude->length() << endl);
            DBG(cerr << "gc2.d_longitude->length(): " << gc2.d_longitude->length() << endl);

            double *lats = 0;
            double **lats_ptr = &lats;
            gc2.d_latitude->buf2val((void**) lats_ptr);
            DBG(cerr << "lats[0]: " << lats[0] << endl);

            double *lons = 0;
            double **lons_ptr = &lons;
            gc2.d_longitude->buf2val((void**) lons_ptr);
            DBG(cerr << "lons[0]: " << lons[0] << endl);
            DBG(cerr << "lons[gc2.d_longitude->length()-1]: " << lons[gc2.d_longitude->length() - 1] << endl);
            //CPPUNIT_ASSERT(lons[0] == 260.0);
            //CPPUNIT_ASSERT(lons[4] == 20.0);

            CPPUNIT_ASSERT(gc2.d_latitude->length() == 1);
            CPPUNIT_ASSERT(gc2.d_longitude->length() == 5);

            CPPUNIT_ASSERT(lats[0] == 30.0);

            CPPUNIT_ASSERT(lons[0] == 300.0);
            CPPUNIT_ASSERT(lons[4] == 60.0);
            DBG(gc2.get_constrained_grid()->print_decl(cerr, "    ", true, false, true));
            DBG(gc2.get_constrained_grid()->print_val(cerr, "    ", false));
            DBG(cerr << endl << endl);
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
    }

    // This tests the 'stitch' feature of the GridGeoConstraint/GeoConstraint
    void apply_constriant_to_data_test4()
    {
        try {
            Grid *g2 = dynamic_cast<Grid*>(geo_dds->var("SST1_1"));
            CPPUNIT_ASSERT(g2);
            g2->set_send_p(true);

            GridGeoConstraint gc2(g2);

            gc2.set_bounding_box(30, 280, 30, 40);

            /* lat: { 40, 30, 20, 10, 0, -10, -20, -30, -40, -50 };
             lon: { 0, 40, 80, 120, 160, 200, 240, 280, 320, 359 };
             Data values for Grid SST1_1:
             { { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
             { 10,11,12,13,14,15,16,17,18,19},
             { 20,21,22,23,24,25,26,27,28,29},
             { 30,31,32,33,34,35,36,37,38,39},
             { 40,41,42,43,44,45,46,47,48,49},
             { 50,51,52,53,54,55,56,57,58,59},
             { 60,61,62,63,64,65,66,67,68,69},
             { 70,71,72,73,74,75,76,77,78,79},
             { 80,81,82,83,84,85,86,87,88,89},
             { 90,91,92,93,94,95,96,97,98,99} };
             */

            gc2.apply_constraint_to_data();

            DBG(cerr << "gc2.d_latitude->length(): " << gc2.d_latitude->length() << endl);
            DBG(cerr << "gc2.d_longitude->length(): " << gc2.d_longitude->length() << endl);

            double *lats = 0;
            double **lats_ptr = &lats;
            gc2.d_latitude->buf2val((void**) lats_ptr);
            DBG(cerr << "lats[0]: " << lats[0] << endl);
            //CPPUNIT_ASSERT(lats[3] == -40.0);

            double *lons = 0;
            double **lons_ptr = &lons;
            gc2.d_longitude->buf2val((void**) lons_ptr);
            DBG(cerr << "lons[0]: " << lons[0] << endl);
            DBG(cerr << "lons[3]: " << lons[4] << endl);
            //CPPUNIT_ASSERT(lons[0] == 260.0);
            //CPPUNIT_ASSERT(lons[4] == 20.0);

            CPPUNIT_ASSERT(gc2.d_latitude->length() == 1);
            CPPUNIT_ASSERT(gc2.d_longitude->length() == 5);

            CPPUNIT_ASSERT(lats[0] == 30.0);
            //CPPUNIT_ASSERT(lats[3] == -40.0);

            CPPUNIT_ASSERT(lons[0] == 280.0);
            CPPUNIT_ASSERT(lons[4] == 40.0);

            DBG(gc2.get_constrained_grid()->print_decl(cerr, "    ", true, false, true));
            DBG(gc2.get_constrained_grid()->print_val(cerr, "    ", false));
            DBG(cerr << endl << endl);
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
    }

    void apply_constriant_to_data_test4_three_arg()
    {
        try {
            Grid *g2 = dynamic_cast<Grid*>(geo_dds->var("SST1_1"));
            CPPUNIT_ASSERT(g2);
            g2->set_send_p(true);

            Array *lat2 = dynamic_cast<Array*>(geo_dds->var("SST1_1.lat"));
            CPPUNIT_ASSERT(lat2);
            Array *lon2 = dynamic_cast<Array*>(geo_dds->var("SST1_1.lon"));
            CPPUNIT_ASSERT(lon2);

            GridGeoConstraint gc2(g2, lat2, lon2);

            gc2.set_bounding_box(30, 280, 30, 40);

            /* lat: { 40, 30, 20, 10, 0, -10, -20, -30, -40, -50 };
             lon: { 0, 40, 80, 120, 160, 200, 240, 280, 320, 359 };
             Data values for Grid SST1_1:
             { { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
             { 10,11,12,13,14,15,16,17,18,19},
             { 20,21,22,23,24,25,26,27,28,29},
             { 30,31,32,33,34,35,36,37,38,39},
             { 40,41,42,43,44,45,46,47,48,49},
             { 50,51,52,53,54,55,56,57,58,59},
             { 60,61,62,63,64,65,66,67,68,69},
             { 70,71,72,73,74,75,76,77,78,79},
             { 80,81,82,83,84,85,86,87,88,89},
             { 90,91,92,93,94,95,96,97,98,99} };
             */

            gc2.apply_constraint_to_data();

            DBG(cerr << "gc2.d_latitude->length(): " << gc2.d_latitude->length() << endl);
            DBG(cerr << "gc2.d_longitude->length(): " << gc2.d_longitude->length() << endl);

            double *lats = 0;
            double **lats_ptr = &lats;
            gc2.d_latitude->buf2val((void**) lats_ptr);
            DBG(cerr << "lats[0]: " << lats[0] << endl);
            //CPPUNIT_ASSERT(lats[3] == -40.0);

            double *lons = 0;
            double **lons_ptr = &lons;
            gc2.d_longitude->buf2val((void**) lons_ptr);
            DBG(cerr << "lons[0]: " << lons[0] << endl);
            DBG(cerr << "lons[3]: " << lons[4] << endl);
            //CPPUNIT_ASSERT(lons[0] == 260.0);
            //CPPUNIT_ASSERT(lons[4] == 20.0);

            CPPUNIT_ASSERT(gc2.d_latitude->length() == 1);
            CPPUNIT_ASSERT(gc2.d_longitude->length() == 5);

            CPPUNIT_ASSERT(lats[0] == 30.0);
            //CPPUNIT_ASSERT(lats[3] == -40.0);

            CPPUNIT_ASSERT(lons[0] == 280.0);
            CPPUNIT_ASSERT(lons[4] == 40.0);

            DBG(gc2.get_constrained_grid()->print_decl(cerr, "    ", true, false, true));
            DBG(gc2.get_constrained_grid()->print_val(cerr, "    ", false));
            DBG(cerr << endl << endl);
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION(GridGeoConstraintTest);

} // namespace functions

int main(int argc, char*argv[])
{

    GetOpt getopt(argc, argv, "dDh");
    char option_char;
    while ((option_char = getopt()) != EOF)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;
        case 'D':
            debug2 = 1;
            break;
        case 'h': {     // help - show test names
            cerr << "Usage: GridGeoConstraintTest has the following tests:" << endl;
            const std::vector<Test*> &tests = GridGeoConstraintTest::suite()->getTests();
            unsigned int prefix_len = GridGeoConstraintTest::suite()->getName().append("::").length();
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
            test = GridGeoConstraintTest::suite()->getName().append("::").append(argv[i]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
