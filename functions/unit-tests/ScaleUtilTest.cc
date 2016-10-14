
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

class ScaleUtilTest : public TestFixture
{
private:
    DDS *small_dds;
    TestTypeFactory btf;

    CPLErrorHandler orig_err_handler;

    string src_dir;
    const static int small_dim_size = 11;

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

    /**
     * @brief Read data from a text file
     *
     * Read data from a text file where those values are listed on one or more
     * lines. Each value is separated by a space, comma, or something that C++'s
     * istringstream won't confuse with a character that's part of the value.
     *
     * Assume the text file starts with a line that should be ignored.
     *
     * @param file Name of the input file
     * @param size Number of values to read
     * @param dest Chunk of memory with sizeof(T) * size bytes
     */
    template <typename T>
    void read_data_from_file(const string &file, unsigned int size, T *dest)
    {
        fstream input(file.c_str(), fstream::in);
        if (input.eof() || input.fail()) throw Error(string(__FUNCTION__) + ": Could not open data (" + file +").");

        // Read a line of text to get to the start of the data.
        string line;
        getline(input, line);
        if (input.eof() || input.fail()) throw Error(string(__FUNCTION__) + ": Could not read data.");

        // Get data line by line and load it into 'dest.' Assume that the last line
        // might not have as many values as the others.
        getline(input, line);
        if (input.eof() || input.fail())
            throw Error(string(__FUNCTION__) + ": Could not read data.");

        while (!(input.eof() || input.fail())) {
            DBG(cerr << "line: " << line << endl);
            istringstream iss(line);
            while (!iss.eof()) {
                iss >> (*dest++);

                if (!size--)
                    throw Error(string(__FUNCTION__) + ": Too many values in the data file.");
            }

            getline(input, line);
            if (input.bad())   // in the loop we only care about I/O failures, not logical errors like reading EOF.
                throw Error(string(__FUNCTION__) + ": Could not read data.");
        }
    }

    template<typename T>
    void load_var(Array *var, const string &file, vector<T> &buf)
    {
        if (!var) throw Error(string(__FUNCTION__) + ": The Array variable was null.");
        string data_file = src_dir + "/" + file;
        read_data_from_file(data_file, buf.size(), &buf[0]);
        if (!var) throw Error(string(__FUNCTION__) + ": Could not find '" + var->name() + "'.");
        if (!var->set_value(buf, buf.size())) throw Error(string(__FUNCTION__) + ": Could not set '" + var->name() + "'.");
        var->set_read_p(true);
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
    {}

    void setUp()
    {
        DBG(cerr << "setUp() - BEGIN" << endl);
        try {
            small_dds = new DDS(&btf);
            string dds_file = src_dir + "/" + small_dds_file;
            small_dds->parse(dds_file);

            vector<dods_float32> buf(small_dim_size);
            load_var(dynamic_cast<Array*>(small_dds->var("lat")), "small_lat.txt", buf);
            load_var(dynamic_cast<Array*>(small_dds->var("lon")), "small_lon.txt", buf);

            vector<dods_float32> data(small_dim_size * small_dim_size);
            Array *a = dynamic_cast<Array*>(small_dds->var("data"));
            load_var(a, "small_data.txt", data);

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
        delete small_dds; small_dds = 0;
    }

    void test_reading_data() {
        vector<dods_float32> f32(small_dim_size);
        Array *lat = dynamic_cast<Array*>(small_dds->var("lat"));
        lat->value(&f32[0]);
        DBG(cerr << "lat: ");
        DBG(copy(f32.begin(), f32.end(), ostream_iterator<double>(cerr, " ")));
        DBG(cerr << endl);

        CPPUNIT_ASSERT(f32[0] == 5);
        CPPUNIT_ASSERT(f32[small_dim_size - 1] == -5);

        Array *lon = dynamic_cast<Array*>(small_dds->var("lon"));
        lon->value(&f32[0]);
        DBG(cerr << "lon: ");
        DBG(copy(f32.begin(), f32.end(), ostream_iterator<double>(cerr, " ")));
        DBG(cerr << endl);

        CPPUNIT_ASSERT(f32[0] == -0.5);
        CPPUNIT_ASSERT(f32[small_dim_size - 1] == 0.5);

        Array *d = dynamic_cast<Array*>(small_dds->var("data"));
        const int data_size = small_dim_size * small_dim_size;
        vector<dods_float32> data(data_size);
        d->value(&data[0]);
        DBG(cerr << "data: ");
        DBG(copy(data.begin(), data.end(), ostream_iterator<double>(cerr, " ")));
        DBG(cerr << endl);
        CPPUNIT_ASSERT(data[0] == -99);
        CPPUNIT_ASSERT(data[small_dim_size - 1] == -99);

        DBG(cerr << "data[" << 5*small_dim_size + 0 << "]: " << data[5*small_dim_size + 0] << endl);
        DBG(cerr << "data[" << 5*small_dim_size + 4 << "]: " << data[5*small_dim_size + 4] << endl);
        CPPUNIT_ASSERT(same_as(data[5*small_dim_size + 0], 3.1)); // accounts for rounding error
        CPPUNIT_ASSERT(data[5*small_dim_size + 4] == 3.5);
    }

    void test_get_size_box() {
        SizeBox sb = get_size_box(dynamic_cast<Array*>(small_dds->var("lat")), dynamic_cast<Array*>(small_dds->var("lon")));

        CPPUNIT_ASSERT(sb.x_size == small_dim_size);
        CPPUNIT_ASSERT(sb.y_size == small_dim_size);
    }

    // vector<double> get_geotransform_data(Array *lat, Array *lon, const SizeBox &size)
    void test_get_geotransform_data(){
        Array *lat = dynamic_cast<Array*>(small_dds->var("lat"));
        Array *lon = dynamic_cast<Array*>(small_dds->var("lon"));
        CPPUNIT_ASSERT(lat);
        CPPUNIT_ASSERT(lon);

        SizeBox sb = get_size_box(lat, lon);
        CPPUNIT_ASSERT(sb.x_size == small_dim_size);
        CPPUNIT_ASSERT(sb.y_size == small_dim_size);

        // Xgeo = GT(0) + Xpixel*GT(1) + Yline*GT(2)
        // Ygeo = GT(3) + Xpixel*GT(4) + Yline*GT(5)
        // X == lon, Y is lat
        vector<double> gt = get_geotransform_data(lat, lon);

        DBG(cerr << "gt values: ");
        DBG(copy(gt.begin(), gt.end(), std::ostream_iterator<double>(std::cerr, " ")));
        DBG(cerr << endl);

        CPPUNIT_ASSERT(gt[0] == -0.5);  // min lon
        CPPUNIT_ASSERT(gt[1] == 0.1);   // resolution of lon; i.e., pixel width
        CPPUNIT_ASSERT(gt[2] == 0.0);   // north-south parallel to y axis
        CPPUNIT_ASSERT(gt[3] == 5.0);   // max lat
        CPPUNIT_ASSERT(gt[4] == 0.0);   // 0 if east-west is parallel to x axis
        CPPUNIT_ASSERT(gt[5] == -1.0);  // resolution of lat; neg for north up data
    }

    void test_read_band_data()
    {
        try {
            Array *d = dynamic_cast<Array*>(small_dds->var("data"));

            GDALDataType gdal_type = get_array_type(d);

            GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("MEM");
            if (!driver) throw Error(string("Could not get the Memory driver for GDAL: ") + CPLGetLastErrorMsg());

            // The MEM driver takes no creation options (I think) jhrg 10/6/16
            auto_ptr<GDALDataset> ds(driver->Create("result", small_dim_size, small_dim_size, 1 /* nBands*/, gdal_type,
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

            SizeBox size(small_dim_size, small_dim_size);
            read_band_data(d, band);

            CPPUNIT_ASSERT(band->GetXSize() == small_dim_size);
            CPPUNIT_ASSERT(band->GetYSize() == small_dim_size);
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
            CPPUNIT_ASSERT(same_as(max, 6.9));

            band->SetNoDataValue(-99.0);
            band->ComputeStatistics(false, &min, &max, NULL/*mean*/, NULL/*stddev*/, NULL/*prog*/, NULL/*prog_arg*/);
            DBG(cerr << "min, max: " << min << ", " << max << endl);
            CPPUNIT_ASSERT(min = 1.0);
            CPPUNIT_ASSERT(same_as(max, 6.9));
        }
        catch(Error &e) {
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
            auto_ptr<GDALDataset> ds(driver->Create("result", small_dim_size, small_dim_size, 0 /* nBands*/, gdal_type,
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

            CPPUNIT_ASSERT(band->GetXSize() == small_dim_size);
            CPPUNIT_ASSERT(band->GetYSize() == small_dim_size);
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
            CPPUNIT_ASSERT(same_as(max, 6.9));

            band->SetNoDataValue(-99.0);
            band->ComputeStatistics(false, &min, &max, NULL/*mean*/, NULL/*stddev*/, NULL/*prog*/, NULL/*prog_arg*/);
            DBG(cerr << "min, max: " << min << ", " << max << endl);
            CPPUNIT_ASSERT(min = 1.0);
            CPPUNIT_ASSERT(same_as(max, 6.9));
        }
        catch(Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
    }

    // This tests the get_missing_data_value() function too (indirectly)
    void test_build_src_dataset() {
        Array *data = dynamic_cast<Array*>(small_dds->var("data"));
        Array *lon = dynamic_cast<Array*>(small_dds->var("lon"));
        Array *lat = dynamic_cast<Array*>(small_dds->var("lat"));

        auto_ptr<GDALDataset> ds = build_src_dataset(data, lon, lat);

        GDALRasterBand *band = ds->GetRasterBand(1);
        if (!band)
            throw Error(string("Could not get the GDALRasterBand for the GDALDataset: ") + CPLGetLastErrorMsg());

        CPPUNIT_ASSERT(band->GetXSize() == small_dim_size);
        CPPUNIT_ASSERT(band->GetYSize() == small_dim_size);
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
        CPPUNIT_ASSERT(same_as(max, 6.9));

        vector<double> gt(6);
        ds->GetGeoTransform(&gt[0]);

        DBG(cerr << "gt values: ");
        DBG(copy(gt.begin(), gt.end(), std::ostream_iterator<double>(std::cerr, " ")));
        DBG(cerr << endl);

        CPPUNIT_ASSERT(gt[0] == -0.5);  // min lon
        CPPUNIT_ASSERT(gt[1] == 0.1);   // resolution of lon; i.e., pixel width
        CPPUNIT_ASSERT(gt[2] == 0.0);   // north-south parallel to y axis
        CPPUNIT_ASSERT(gt[3] == 5.0);   // max lat
        CPPUNIT_ASSERT(gt[4] == 0.0);   // 0 if east-west is parallel to x axis
        CPPUNIT_ASSERT(gt[5] == -1.0);  // resolution of lat; neg for north up data
    }

    void test_scaling_with_gdal() {
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
            CPPUNIT_ASSERT(same_as(max, 6.9));

            vector<double> gt(6);
            dst->GetGeoTransform(&gt[0]);

            DBG(cerr << "gt values: ");
            DBG(copy(gt.begin(), gt.end(), std::ostream_iterator<double>(std::cerr, " ")));
            DBG(cerr << endl);

            CPPUNIT_ASSERT(gt[0] == -0.5);  // min lon
            CPPUNIT_ASSERT(gt[1] == 0.05);// resolution of lon; i.e., pixel width
            CPPUNIT_ASSERT(gt[2] == 0.0);// north-south parallel to y axis
            CPPUNIT_ASSERT(gt[3] == 5.0);// max lat
            CPPUNIT_ASSERT(gt[4] == 0.0);// 0 if east-west is parallel to x axis
            CPPUNIT_ASSERT(gt[5] == -0.5);// resolution of lat; neg for north up data

            // Extract the data now.
            vector<dods_float32> buf(dst_size * dst_size);
            error = band->RasterIO(GF_Read, 0, 0, dst_size, dst_size, &buf[0], dst_size, dst_size, get_array_type(data), 0, 0);
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

            CPPUNIT_ASSERT(buf[5 * dst_size + 0] == 1.0);
            CPPUNIT_ASSERT(buf[6 * dst_size + 0] == 2.0);
            CPPUNIT_ASSERT(same_as(buf[10 * dst_size + 0], 3.1));
            CPPUNIT_ASSERT(same_as(buf[10 * dst_size + 6], 3.4));

            GDALClose(dst.release());
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
    }

    void test_build_array_from_gdal_dataset() {
        Array *data = dynamic_cast<Array*>(small_dds->var("data"));
        Array *lon = dynamic_cast<Array*>(small_dds->var("lon"));
        Array *lat = dynamic_cast<Array*>(small_dds->var("lat"));

        auto_ptr<GDALDataset> src = build_src_dataset(data, lon, lat);

        auto_ptr<Array> result(build_array_from_gdal_dataset(src, data));

        vector<dods_float32> buf(small_dim_size * small_dim_size);
        result->value(&buf[0]);

        if (debug) {
            cerr << "buf:" << endl;
            for (int y = 0; y < small_dim_size; ++y) {
                for (int x = 0; x < small_dim_size; ++x) {
                    cerr << buf[y * small_dim_size + x] << " ";
                }
                cerr << endl;
            }
        }

        DBG(cerr << "buf[" << 5*small_dim_size + 0 << "]: " << buf[5*small_dim_size + 0] << endl);
        DBG(cerr << "buf[" << 5*small_dim_size + 4 << "]: " << buf[5*small_dim_size + 4] << endl);
        CPPUNIT_ASSERT(same_as(buf[5*small_dim_size + 0], 3.1)); // accounts for rounding error
        CPPUNIT_ASSERT(buf[5*small_dim_size + 4] == 3.5);
    }

    void test_build_maps_from_gdal_dataset() {
        Array *data = dynamic_cast<Array*>(small_dds->var("data"));
        Array *lon = dynamic_cast<Array*>(small_dds->var("lon"));
        Array *lat = dynamic_cast<Array*>(small_dds->var("lat"));

        auto_ptr<GDALDataset> src = build_src_dataset(data, lon, lat);

        auto_ptr<Array> built_lon(new Array("built_lon", new Float32("built_lon")));
        auto_ptr<Array> built_lat(new Array("built_lat", new Float32("built_lat")));

        build_maps_from_gdal_dataset(src.get(), built_lon.get(), built_lat.get());

#if 0
        Array *built_lon = new Array("built_lon", new Float32("built_lon"));
        Array *built_lat = new Array("built_lat", new Float32("built_lat"));

        build_maps_from_gdal_dataset(src.get(), built_lon, built_lat);
#endif
        // Check the lon map

        CPPUNIT_ASSERT(built_lon->dimensions() == 1);
        unsigned long x = built_lon->dimension_size(built_lon->dim_begin());
        CPPUNIT_ASSERT(x == small_dim_size);

        vector<dods_float32> buf_lon(x);
        built_lon->value(&buf_lon[0]);

        if (debug) {
            cerr << "buf_lon:" << endl;
            for (unsigned long i = 0; i < small_dim_size; ++i)
                cerr << buf_lon[i] << " ";
            cerr << endl;
        }

        vector<dods_float32> orig_lon(small_dim_size);
        lon->value(&orig_lon[0]);

        if (debug) {
             cerr << "orig_lon:" << endl;
             for (unsigned long i = 0; i < small_dim_size; ++i)
                 cerr << orig_lon[i] << " ";
             cerr << endl;
         }

        CPPUNIT_ASSERT(equal(buf_lon.begin(), buf_lon.end(), orig_lon.begin(), same_as));

        // Check the lat map
        unsigned long y = built_lon->dimension_size(built_lon->dim_begin());
        CPPUNIT_ASSERT(y == small_dim_size);

        vector<dods_float32> buf_lat(y);
        built_lat->value(&buf_lat[0]);

        if (debug) {
            cerr << "buf_lat:" << endl;
            for (unsigned long i = 0; i < small_dim_size; ++i)
                cerr << buf_lat[i] << " ";
            cerr << endl;
        }

        vector<dods_float32> orig_lat(small_dim_size);
        lat->value(&orig_lat[0]);

        if (debug) {
            cerr << "orig_lat:" << endl;
            for (unsigned long i = 0; i < small_dim_size; ++i)
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

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ScaleUtilTest);

int main(int argc, char*argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    GetOpt getopt(argc, argv, "db");
    int option_char;
    while ((option_char = getopt()) != -1)
        switch (option_char) {
        case 'd':
            debug = 1;  // debug is a static global
            break;

        case 'b':
            bes_debug = 1;  // debug is a static global
            break;

        default:
            break;
        }

    bool wasSuccessful = true;
    string test = "";
    int i = getopt.optind;
    if (i == argc) {
        // run them all
        wasSuccessful = runner.run("");
    }
    else {
        while (i < argc) {
            test = string("ReprojFunctionsTest::") + argv[i++];

            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
