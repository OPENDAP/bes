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

#include <gdal.h>
#include <gdal_priv.h>
#include <gdal_utils.h>

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

#include "ScaleGrid.h"

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

class ScaleUtilTest: public TestFixture {
private:
    DDS *small_dds;
    TestTypeFactory btf;

    CPLErrorHandler orig_err_handler;

    string src_dir;
    const static int x_size = 11, y_size = 9;

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
    ScaleUtilTest() : small_dds(0), src_dir(TEST_SRC_DIR)
    {
        src_dir.append("/scale");
        GDALAllRegister();
        OGRRegisterAll();

        orig_err_handler = CPLSetErrorHandler(CPLQuietErrorHandler);
    }

    ~ScaleUtilTest()
    {
    }

    void setUp()
    {
        DBG(cerr << "setUp() - BEGIN" << endl);
        try {
            small_dds = new DDS(&btf);
            string dds_file = src_dir + "/" + small_dds_file;
            small_dds->parse(dds_file);

            vector<dods_float32> lat(y_size);
            load_var(dynamic_cast<Array*>(small_dds->var("lat")), src_dir + "/" + "small_lat.txt", lat);
            vector<dods_float32> lon(x_size);
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
        vector<dods_float32> lat_buf(y_size);
        Array *lat = dynamic_cast<Array*>(small_dds->var("lat"));
        lat->value(&lat_buf[0]);
        DBG(cerr << "lat: ");
        DBG(copy(lat_buf.begin(), lat_buf.end(), ostream_iterator<double>(cerr, " ")));
        DBG(cerr << endl);

        CPPUNIT_ASSERT(lat_buf[0] == 4);
        CPPUNIT_ASSERT(lat_buf[y_size - 1] == -4);

        vector<dods_float32> lon_buf(x_size);
        Array *lon = dynamic_cast<Array*>(small_dds->var("lon"));
        lon->value(&lon_buf[0]);
        DBG(cerr << "lon: ");
        DBG(copy(lon_buf.begin(), lon_buf.end(), ostream_iterator<double>(cerr, " ")));
        DBG(cerr << endl);

        CPPUNIT_ASSERT(lon_buf[0] == -0.5);
        CPPUNIT_ASSERT(lon_buf[x_size - 1] == 0.5);

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
        CPPUNIT_ASSERT(double_eq(data[4 * x_size + 4],3.5));
    }

    void test_get_size_box()
    {
        SizeBox sb = get_size_box(dynamic_cast<Array*>(small_dds->var("lon")),
            dynamic_cast<Array*>(small_dds->var("lat")));

        CPPUNIT_ASSERT(sb.x_size == x_size);
        CPPUNIT_ASSERT(sb.y_size == y_size);
    }

    void test_get_geotransform_data()
    {
        try {
            Array *lat = dynamic_cast<Array*>(small_dds->var("lat"));
            Array *lon = dynamic_cast<Array*>(small_dds->var("lon"));
            CPPUNIT_ASSERT(lat);
            CPPUNIT_ASSERT(lon);

            SizeBox sb = get_size_box(lon, lat);
            CPPUNIT_ASSERT(sb.x_size == x_size);
            CPPUNIT_ASSERT(sb.y_size == y_size);

            // Xgeo = GT(0) + Xpixel*GT(1) + Yline*GT(2)
            // Ygeo = GT(3) + Xpixel*GT(4) + Yline*GT(5)
            // X == lon, Y is lat
            vector<double> gt = get_geotransform_data(lat, lon);

            DBG(cerr << "gt values: ");
            DBG(copy(gt.begin(), gt.end(), std::ostream_iterator<double>(std::cerr, " ")));
            DBG(cerr << endl);

            CPPUNIT_ASSERT(gt[0] == 4.0);  // min lon
            CPPUNIT_ASSERT(gt[1] == -1.0);   // resolution of lon; i.e., pixel width
            CPPUNIT_ASSERT(gt[2] == 0.0);   // north-south parallel to y axis
            CPPUNIT_ASSERT(gt[3] == -0.5);   // max lat
            CPPUNIT_ASSERT(gt[4] == 0.0);   // 0 if east-west is parallel to x axis
            CPPUNIT_ASSERT(gt[5] == 0.1);  // resolution of lat; neg for north up data
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
    }

    void test_get_gcp_data()
    {
        Array *lat = dynamic_cast<Array*>(small_dds->var("lat"));
        Array *lon = dynamic_cast<Array*>(small_dds->var("lon"));
        CPPUNIT_ASSERT(lat);
        CPPUNIT_ASSERT(lon);

        SizeBox sb = get_size_box(lon, lat);
        CPPUNIT_ASSERT(sb.x_size == x_size);
        CPPUNIT_ASSERT(sb.y_size == y_size);

        vector<GDAL_GCP> gcp_list = get_gcp_data(lon, lat);

        // Get the affine transform from the GCPs
        vector<double> gt(6);
        int status = GDALGCPsToGeoTransform(gcp_list.size(), &gcp_list[0], &gt[0], 1 /* cp_list*/);
        CPPUNIT_ASSERT(status == true);

        // Xgeo = GT(0) + Xpixel*GT(1) + Yline*GT(2)
        // Ygeo = GT(3) + Xpixel*GT(4) + Yline*GT(5)
        DBG(cerr << "gt values: ");
        DBG(copy(gt.begin(), gt.end(), std::ostream_iterator<double>(std::cerr, " ")));
        DBG(cerr << endl);

        // -0.5 0.1 -4.49921e-17 4 6.32138e-16 -1
        CPPUNIT_ASSERT(same_as(gt[0], -0.5));  // min lon
        CPPUNIT_ASSERT(same_as(gt[1], 0.1));   // resolution of lon; i.e., pixel width
        CPPUNIT_ASSERT(same_as(gt[2], -4.49921e-17));   // north-south parallel to y axis
        CPPUNIT_ASSERT(same_as(gt[3], 4));   // max lat
        CPPUNIT_ASSERT(same_as(gt[4], 6.32138e-16));   // 0 if east-west is parallel to x axis
        CPPUNIT_ASSERT(same_as(gt[5], -1));  // resolution of lat; neg for north up data

        int sample = 4; // For these data, 5 will fail.

        gcp_list = get_gcp_data(lon, lat, sample, sample);
        status = GDALGCPsToGeoTransform(gcp_list.size(), &gcp_list[0], &gt[0], 0 /* ApproxOK */);
        CPPUNIT_ASSERT(status == true);

        // Xgeo = GT(0) + Xpixel*GT(1) + Yline*GT(2)
        // Ygeo = GT(3) + Xpixel*GT(4) + Yline*GT(5)
        DBG(cerr << "gt values (sample " << sample << ") : ");
        DBG(copy(gt.begin(), gt.end(), std::ostream_iterator<double>(std::cerr, " ")));
        DBG(cerr << endl);

        // -0.5 0.1 0 4 0 -1
        CPPUNIT_ASSERT(same_as(gt[0], -0.5));  // min lon
        CPPUNIT_ASSERT(same_as(gt[1], 0.1));   // resolution of lon; i.e., pixel width
        CPPUNIT_ASSERT(same_as(gt[2], 0.0));   // north-south parallel to y axis
        CPPUNIT_ASSERT(same_as(gt[3], 4));   // max lat
        CPPUNIT_ASSERT(same_as(gt[4], 0.0));   // 0 if east-west is parallel to x axis
        CPPUNIT_ASSERT(same_as(gt[5], -1));  // resolution of lat; neg for north up data
    }

    void test_read_band_data()
    {
        try {
            Array *d = dynamic_cast<Array*>(small_dds->var("data"));

            GDALDataType gdal_type = get_array_type(d);

            GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("MEM");
            if (!driver) throw Error(string("Could not get the Memory driver for GDAL: ") + CPLGetLastErrorMsg());

            // The MEM driver takes no creation options (I think) jhrg 10/6/16
            auto_ptr<GDALDataset> ds(driver->Create("result", x_size, y_size, 1 /* nBands*/, gdal_type,
            NULL /* driver_options */));

            // The MEM format is one of the few that supports the AddBand() method. The AddBand()
            // method supports DATAPOINTER, PIXELOFFSET and LINEOFFSET options to reference an
            // existing memory array.

            // Get the one band for this dataset and load it with data
            GDALRasterBand *band = ds->GetRasterBand(1);
            if (!band)
                throw Error("Could not get the GDALRasterBand for Array '" + d->name() + "': " + CPLGetLastErrorMsg());

            CPLErr error = CE_None;
            // Without this call, the min value accessed below will be -99
            // error = band->SetNoDataValue(-99);

            read_band_data(d, band);

            CPPUNIT_ASSERT(band->GetXSize() == x_size);
            CPPUNIT_ASSERT(band->GetYSize() == y_size);
            CPPUNIT_ASSERT(band->GetBand() == 1);
            CPPUNIT_ASSERT(band->GetRasterDataType() == gdal_type); // tautology?

            // This is just interesting...
            int block_x, block_y;
            band->GetBlockSize(&block_x, &block_y);
            DBG(cerr << "Block size: " << block_y << ", " << block_x << endl);

            double min, max;
            error = band->GetStatistics(false, true, &min, &max, NULL, NULL);
            DBG(cerr << "min: " << min << ", max: " << max << " (error: " << error << ")" << endl);
            CPPUNIT_ASSERT(same_as(min, -99));
            CPPUNIT_ASSERT(double_eq(max, 8.9));

            band->SetNoDataValue(-99.0);
            band->ComputeStatistics(false, &min, &max, NULL/*mean*/, NULL/*stddev*/, NULL/*prog*/, NULL/*prog_arg*/);
            DBG(cerr << "min, max: " << min << ", " << max << endl);
            CPPUNIT_ASSERT(min = 1.0);
            CPPUNIT_ASSERT(double_eq(max, 8.9));
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
    }

    void test_add_band_data()
    {
        try {
            Array *d = dynamic_cast<Array*>(small_dds->var("data"));

            GDALDataType gdal_type = get_array_type(d);

            GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("MEM");
            if (!driver) throw Error(string("Could not get the Memory driver for GDAL: ") + CPLGetLastErrorMsg());

            // The MEM driver takes no creation options (I think) jhrg 10/6/16
            auto_ptr<GDALDataset> ds(driver->Create("result", x_size, y_size, 0 /* nBands*/, gdal_type,
            NULL /* driver_options */));

            // The MEM format is one of the few that supports the AddBand() method. The AddBand()
            // method supports DATAPOINTER, PIXELOFFSET and LINEOFFSET options to reference an
            // existing memory array.

            add_band_data(d, ds.get());

            CPPUNIT_ASSERT(ds->GetRasterCount() == 1);

            // Get the one band for this dataset
            GDALRasterBand *band = ds->GetRasterBand(1);
            if (!band)
                throw Error("Could not get the GDALRasterBand for Array '" + d->name() + "': " + CPLGetLastErrorMsg());

            CPPUNIT_ASSERT(band->GetXSize() == x_size);
            CPPUNIT_ASSERT(band->GetYSize() == y_size);
            CPPUNIT_ASSERT(band->GetBand() == 1);
            CPPUNIT_ASSERT(band->GetRasterDataType() == gdal_type); // tautology?

            // This is just interesting...
            int block_x, block_y;
            band->GetBlockSize(&block_x, &block_y);
            DBG(cerr << "Block size: " << block_y << ", " << block_x << endl);

            double min, max;
            CPLErr error = band->GetStatistics(false, true, &min, &max, NULL, NULL);
            DBG(cerr << "min: " << min << ", max: " << max << " (error: " << error << ")" << endl);
            CPPUNIT_ASSERT(same_as(min, -99));
            CPPUNIT_ASSERT(double_eq(max, 8.9));

            band->SetNoDataValue(-99.0);
            band->ComputeStatistics(false, &min, &max, NULL/*mean*/, NULL/*stddev*/, NULL/*prog*/, NULL/*prog_arg*/);
            DBG(cerr << "min, max: " << min << ", " << max << endl);
            CPPUNIT_ASSERT(min = 1.0);
            CPPUNIT_ASSERT(double_eq(max, 8.9));
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
    }

    // This tests the get_missing_data_value() function too (indirectly)
    void test_build_src_dataset()
    {
        try {
            Array *data = dynamic_cast<Array*>(small_dds->var("data"));
            Array *lon = dynamic_cast<Array*>(small_dds->var("lon"));
            Array *lat = dynamic_cast<Array*>(small_dds->var("lat"));

            auto_ptr<GDALDataset> ds = build_src_dataset(data, lon, lat);

            GDALRasterBand *band = ds->GetRasterBand(1);
            if (!band)
                throw Error(string("Could not get the GDALRasterBand for the GDALDataset: ") + CPLGetLastErrorMsg());

            CPPUNIT_ASSERT(band->GetXSize() == x_size);
            CPPUNIT_ASSERT(band->GetYSize() == y_size);
            CPPUNIT_ASSERT(band->GetBand() == 1);
            CPPUNIT_ASSERT(band->GetRasterDataType() == get_array_type(data));

            // This is just interesting...
            int block_x, block_y;
            band->GetBlockSize(&block_x, &block_y);
            DBG(cerr << "Block size: " << block_y << ", " << block_x << endl);

            double min, max;
            CPLErr error = band->GetStatistics(false, true, &min, &max, NULL, NULL);
            DBG(cerr << "min: " << min << ", max: " << max << " (error: " << error << ")" << endl);
            CPPUNIT_ASSERT(same_as(min, 1.0));  // This is 1 and not -99 because the no data value is set.
            CPPUNIT_ASSERT(double_eq(max, 8.9));

            vector<double> gt(6);
            ds->GetGeoTransform(&gt[0]);

            DBG(cerr << "gt values: ");
            DBG(copy(gt.begin(), gt.end(), std::ostream_iterator<double>(std::cerr, " ")));
            DBG(cerr << endl);

            CPPUNIT_ASSERT(gt[0] == -0.5);  // min lon
            CPPUNIT_ASSERT(gt[1] == 0.1);   // resolution of lon; i.e., pixel width
            CPPUNIT_ASSERT(gt[2] == 0.0);   // north-south parallel to y axis
            CPPUNIT_ASSERT(gt[3] == 4);   // max lat
            CPPUNIT_ASSERT(gt[4] == 0.0);   // 0 if east-west is parallel to x axis
            CPPUNIT_ASSERT(gt[5] == -1);  // resolution of lat; neg for north up data
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
    }

    void test_scaling_with_gdal()
    {
        try {
            Array *data = dynamic_cast<Array*>(small_dds->var("data"));
            Array *lon = dynamic_cast<Array*>(small_dds->var("lon"));
            Array *lat = dynamic_cast<Array*>(small_dds->var("lat"));

            auto_ptr<GDALDataset> src = build_src_dataset(data, lon, lat);

            const int dst_size = 22;
            SizeBox size(dst_size, dst_size);
            auto_ptr<GDALDataset> dst = scale_dataset(src, size);

            CPPUNIT_ASSERT(dst->GetRasterCount() == 1);

            GDALRasterBand *band = dst->GetRasterBand(1);
            if (!band)
                throw Error(string("Could not get the GDALRasterBand for the GDALDataset: ") + CPLGetLastErrorMsg());

            CPPUNIT_ASSERT(band->GetXSize() == dst_size);
            CPPUNIT_ASSERT(band->GetYSize() == dst_size);

            CPPUNIT_ASSERT(band->GetBand() == 1);
            CPPUNIT_ASSERT(band->GetRasterDataType() == get_array_type(data));

            // This is just interesting...
            int block_x, block_y;
            band->GetBlockSize(&block_x, &block_y);
            DBG(cerr << "Block size: " << block_y << ", " << block_x << endl);

            double min, max;
            CPLErr error = band->GetStatistics(false, true, &min, &max, NULL, NULL);
            DBG(cerr << "min: " << min << ", max: " << max << " (error: " << error << ")" << endl);
            CPPUNIT_ASSERT(same_as(min, 1.0));  // This is 1 and not -99 because the no data value is set.
            CPPUNIT_ASSERT(double_eq(max, 8.9));

            vector<double> gt(6);
            dst->GetGeoTransform(&gt[0]);

            DBG(cerr << "gt values: ");
            DBG(copy(gt.begin(), gt.end(), std::ostream_iterator<double>(std::cerr, " ")));
            DBG(cerr << endl);

            // gt values: -0.5 0.05 0 4 0 -0.409091
            CPPUNIT_ASSERT(gt[0] == -0.5);  // min lon
            CPPUNIT_ASSERT(gt[1] == 0.05);  // resolution of lon; i.e., pixel width
            CPPUNIT_ASSERT(gt[2] == 0.0);  // north-south parallel to y axis
            CPPUNIT_ASSERT(gt[3] == 4);  // max lat
            CPPUNIT_ASSERT(gt[4] == 0.0);  // 0 if east-west is parallel to x axis
            CPPUNIT_ASSERT(same_as(gt[5], -0.409091));  // resolution of lat; neg for north up data

            // Extract the data now.
            vector<dods_float32> buf(dst_size * dst_size);
            error = band->RasterIO(GF_Read, 0, 0, dst_size, dst_size, &buf[0], dst_size, dst_size, get_array_type(data),
                0, 0);
            if (error != CPLE_None)
                throw Error(string("Could not extract data for translated GDAL Dataset.") + CPLGetLastErrorMsg());

            if (debug) {
                cerr << "buf:" << endl;
                for (int y = 0; y < dst_size; ++y) {
                    for (int x = 0; x < dst_size; ++x) {
                        cerr << buf[y * dst_size + x] << " ";
                    }
                    cerr << endl;
                }
            }

            CPPUNIT_ASSERT(buf[0 * dst_size + 4] == 2.0);
            CPPUNIT_ASSERT(buf[0 * dst_size + 6] == 3.0);
            CPPUNIT_ASSERT(same_as(buf[0 * dst_size + 11], 4.9));
            CPPUNIT_ASSERT(double_eq(buf[6 * dst_size + 11], 4.7));

            GDALClose(dst.release());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
    }

    void test_build_array_from_gdal_dataset()
    {
        try {
            Array *data = dynamic_cast<Array*>(small_dds->var("data"));
            Array *lon = dynamic_cast<Array*>(small_dds->var("lon"));
            Array *lat = dynamic_cast<Array*>(small_dds->var("lat"));

            auto_ptr<GDALDataset> src = build_src_dataset(data, lon, lat);

            auto_ptr<Array> result(build_array_from_gdal_dataset(src.get(), data));

            vector<dods_float32> buf(x_size * y_size);
            result->value(&buf[0]);

            if (debug) {
                cerr << "buf:" << endl;
                for (int y = 0; y < y_size; ++y) {
                    for (int x = 0; x < x_size; ++x) {
                        cerr << buf[y * x_size + x] << " ";
                    }
                    cerr << endl;
                }
            }

            DBG(cerr << "buf[" << 0 * x_size + 4 << "]: " << buf[0 * x_size + 4] << endl);
            DBG(cerr << "buf[" << 4 * x_size + 4 << "]: " << buf[4 * x_size + 4] << endl);
            CPPUNIT_ASSERT(same_as(buf[0 * x_size + 4], 3.1)); // accounts for rounding error
            CPPUNIT_ASSERT(double_eq(buf[4 * x_size + 4],3.5));

        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
    }

    void test_build_maps_from_gdal_dataset()
    {
        Array *data = dynamic_cast<Array*>(small_dds->var("data"));
        Array *lon = dynamic_cast<Array*>(small_dds->var("lon"));
        Array *lat = dynamic_cast<Array*>(small_dds->var("lat"));

        auto_ptr<GDALDataset> src = build_src_dataset(data, lon, lat);

        auto_ptr<Array> built_lon(new Array("built_lon", new Float32("built_lon")));
        auto_ptr<Array> built_lat(new Array("built_lat", new Float32("built_lat")));

        build_maps_from_gdal_dataset(src.get(), built_lon.get(), built_lat.get());

        // Check the lon map
        CPPUNIT_ASSERT(built_lon->dimensions() == 1);
        unsigned long x = built_lon->dimension_size(built_lon->dim_begin());
        CPPUNIT_ASSERT(x == x_size);

        vector<dods_float32> buf_lon(x);
        built_lon->value(&buf_lon[0]);

        if (debug) {
            cerr << "buf_lon:" << endl;
            for (unsigned long i = 0; i < x_size; ++i)
                cerr << buf_lon[i] << " ";
            cerr << endl;
        }

        vector<dods_float32> orig_lon(x_size);
        lon->value(&orig_lon[0]);

        if (debug) {
            cerr << "orig_lon:" << endl;
            for (unsigned long i = 0; i < x_size; ++i)
                cerr << orig_lon[i] << " ";
            cerr << endl;
        }

        CPPUNIT_ASSERT(equal(buf_lon.begin(), buf_lon.end(), orig_lon.begin(), same_as));

        // Check the lat map
        unsigned long y = built_lon->dimension_size(built_lat->dim_begin());
        CPPUNIT_ASSERT(y == y_size);

        vector<dods_float32> buf_lat(y);
        built_lat->value(&buf_lat[0]);

        if (debug) {
            cerr << "buf_lat:" << endl;
            for (unsigned long i = 0; i < y_size; ++i)
                cerr << buf_lat[i] << " ";
            cerr << endl;
        }

        vector<dods_float32> orig_lat(y_size);
        lat->value(&orig_lat[0]);

        if (debug) {
            cerr << "orig_lat:" << endl;
            for (unsigned long i = 0; i < y_size; ++i)
                cerr << orig_lat[i] << " ";
            cerr << endl;
        }

        CPPUNIT_ASSERT(buf_lat == orig_lat);
    }

    CPPUNIT_TEST_SUITE( ScaleUtilTest );

    CPPUNIT_TEST(test_reading_data);
    CPPUNIT_TEST(test_get_size_box);
    CPPUNIT_TEST(test_get_geotransform_data);
    CPPUNIT_TEST(test_read_band_data);
    CPPUNIT_TEST(test_add_band_data);
    CPPUNIT_TEST(test_build_src_dataset);
    CPPUNIT_TEST(test_scaling_with_gdal);
    CPPUNIT_TEST(test_build_array_from_gdal_dataset);
    CPPUNIT_TEST(test_build_maps_from_gdal_dataset);

    CPPUNIT_TEST(test_get_gcp_data);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ScaleUtilTest);

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
            cerr << "Usage: ScaleUtilTest has the following tests:" << endl;
            const std::vector<Test*> &tests = ScaleUtilTest::suite()->getTests();
            unsigned int prefix_len = ScaleUtilTest::suite()->getName().append("::").length();
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
            test = ScaleUtilTest::suite()->getName().append("::").append(argv[i++]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
