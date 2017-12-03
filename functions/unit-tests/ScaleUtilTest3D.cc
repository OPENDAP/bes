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
#include <vector>

#include <cmath>

#include <gdal.h>
#include <gdal_priv.h>
#include <gdal_utils.h>
#include <ogr_spatialref.h>
#include <gdalwarper.h>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <GetOpt.h>

#include <BaseType.h>
#include <Float32.h>
#include <Array.h>
#include <Grid.h>
#include <assert.h>

#include <test/TestTypeFactory.h>

#include <util.h>
#include <debug.h>

#include "ScaleGrid.h"

#include "test_config.h"
#include "test_utils.h"

#include <util.h>
#include <Error.h>
#include <BESDebug.h>
#include <BESError.h>
#include <BESDapError.h>
#include <dods-datatypes.h>

static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

const string test3D_dds_file = "test3D.dds";

using namespace CppUnit;
using namespace libdap;
using namespace functions;
using namespace std;

int test_variable_sleep_interval = 0;

class ScaleUtilTest3D: public TestFixture {
private:
    DDS *test3D_dds;
    TestTypeFactory btf;

    CPLErrorHandler orig_err_handler;

    string src_dir;
    const static int t_size = 2, y_size = 10, x_size = 15;

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
    ScaleUtilTest3D() :
        test3D_dds(0), src_dir(TEST_SRC_DIR)
    {
        src_dir.append("/scale");
        GDALAllRegister();
        OGRRegisterAll();

        orig_err_handler = CPLSetErrorHandler(CPLQuietErrorHandler);
    }

    ~ScaleUtilTest3D()
    {
    }

    void setUp()
    {
        DBG(cerr << "setUp() - BEGIN" << endl);
        try {
            test3D_dds = new DDS(&btf);
            string dds_file = src_dir + "/" + test3D_dds_file;
            test3D_dds->parse(dds_file);

            vector<dods_float32> time(t_size);
            //DBG(cerr << "time num = " << time.size() << endl);
            string time_file = src_dir + "/" + "test3D_time.txt";
            load_var(dynamic_cast<Array*>(test3D_dds->var("time")), time_file, time);

            vector<dods_float32> lat(y_size);
            string lat_file = src_dir + "/" + "test3D_lat.txt";
            load_var(dynamic_cast<Array*>(test3D_dds->var("lat")), lat_file, lat);

            vector<dods_float32> lon(x_size);
            string lon_file = src_dir + "/" + "test3D_lon.txt";
            load_var(dynamic_cast<Array*>(test3D_dds->var("lon")), lon_file, lon);

            vector<dods_float32> data(t_size * x_size * y_size);
            Array *a = dynamic_cast<Array*>(test3D_dds->var("data"));
            string data_file = src_dir + "/" + "test3D_data.txt";
            load_var(a, data_file, data);

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
        delete test3D_dds;
        test3D_dds = 0;
    }

    void test_reading_data()
    {
        DBG(cerr << "Run test_reading_data." << endl);
        vector<dods_float32> lat_buf(y_size);
        Array *lat = dynamic_cast<Array*>(test3D_dds->var("lat"));
        lat->value(&lat_buf[0]);
        DBG(cerr << "lat: ");
        DBG(copy(lat_buf.begin(), lat_buf.end(), ostream_iterator<double>(cerr, " ")));
        DBG(cerr << endl);

        CPPUNIT_ASSERT(lat_buf[0] == -89.0);
        CPPUNIT_ASSERT(lat_buf[y_size - 1] == -71.0);

        vector<dods_float32> lon_buf(x_size);
        Array *lon = dynamic_cast<Array*>(test3D_dds->var("lon"));
        lon->value(&lon_buf[0]);
        DBG(cerr << "lon: ");
        DBG(copy(lon_buf.begin(), lon_buf.end(), ostream_iterator<double>(cerr, " ")));
        DBG(cerr << endl);

        CPPUNIT_ASSERT(lon_buf[0] == 21.0);
        CPPUNIT_ASSERT(lon_buf[x_size - 1] == 49.0);

        Array *d = dynamic_cast<Array*>(test3D_dds->var("data"));
        const int data_size = t_size * x_size * y_size;
        vector<dods_float32> data(data_size);
        d->value(&data[0]);
        DBG(cerr << "data: ");
        DBG(copy(data.begin(), data.end(), ostream_iterator<double>(cerr, " ")));
        DBG(cerr << endl);
        DBG(cerr << "data[0] = " << data[0] << endl);
        DBG(cerr << "data[data_size - 1] = " << data[data_size - 1] << endl);
        CPPUNIT_ASSERT(double_eq(data[0], -0.11));
        CPPUNIT_ASSERT(double_eq(data[data_size - 1], -0.9));

        DBG(cerr << "data[" << y_size * x_size + 0 << "]: " << data[y_size * x_size + 0] << endl);
        DBG(cerr << "data[" << t_size * y_size * x_size - 4 << "]: " << data[t_size * y_size * x_size - 4] << endl);
        DBG(cerr << "data[" << 4 * x_size - 4 << "]: " << data[4 * x_size - 4] << endl);
        CPPUNIT_ASSERT(same_as(data[4 * x_size + 0], -0.6)); // accounts for rounding error
        CPPUNIT_ASSERT(double_eq(data[4 * x_size - 4], -0.8));

        DBG(cerr << "d[1:0:0]  = " << d[1] << endl);
        DBG(cerr << "d[1:0:0] dimension_size = " << d->dimension_size(d->dim_begin()) << endl);
        DBG(cerr << "d[1:0:0] dimension_name = " << d->dimension_name(d->dim_begin()) << endl);

        DBG(cerr << "------------------End test------------------" << endl);
    }

    void test_get_size_box()
    {
        DBG(cerr << "Run test_get_size_box." << endl);
        SizeBox sb = get_size_box(dynamic_cast<Array*>(test3D_dds->var("lon")),
            dynamic_cast<Array*>(test3D_dds->var("lat")));

        DBG(cerr << "SizeBox x_size = " << sb.x_size << endl);
        DBG(cerr << "SizeBox y_size = " << sb.y_size << endl);

        CPPUNIT_ASSERT(sb.x_size == x_size);
        CPPUNIT_ASSERT(sb.y_size == y_size);

        DBG(cerr << "------------------End test------------------" << endl);
    }

    #define ADD_BAND 0

    void test_build_src_dataset_3D()
    {
        try {
            DBG(cerr << "Run test_build_src_dataset_3D" << endl);

            Array *data = dynamic_cast<Array*>(test3D_dds->var("data"));
            Array *t = dynamic_cast<Array*>(test3D_dds->var("time"));
            Array *x = dynamic_cast<Array*>(test3D_dds->var("lon"));
            Array *y = dynamic_cast<Array*>(test3D_dds->var("lat"));
            string srs = "WGS84";

            DBG(cerr << "data: ");
            DBG(data->print_val(cerr));
            DBG(cerr << endl);
            DBG(cerr << "t: ");
            DBG(t->print_val(cerr));
            DBG(cerr << endl);
            DBG(cerr << "x: ");
            DBG(x->print_val(cerr))
            DBG(cerr << endl);
            DBG(cerr << "y: ");
            DBG(y->print_val(cerr))
            DBG(cerr << endl);

            auto_ptr<GDALDataset> ds = build_src_dataset_3D(data, t, x, y);
            DBG(cerr << "RasterCount = " << ds.get()->GetRasterCount() << endl);

#if 0
            GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("MEM");
            if (!driver) {
                string msg = string("Could not get the Memory driver for GDAL: ") + CPLGetLastErrorMsg();
                DBG(cerr << "ERROR build_src_dataset(): " << msg << endl);
                throw BESError(msg, BES_INTERNAL_ERROR, __FILE__, __LINE__);
            }

            SizeBox array_size = get_size_box(x, y);
            int nBands = data->dimension_size(data->dim_begin(), true);
            DBG(cerr << "nBands = " << nBands << endl);
            int nBytes = data->prototype()->width();

            auto_ptr<GDALDataset> ds(
                driver->Create("result", array_size.x_size, array_size.y_size, nBands, get_array_type(data),
                    NULL /* driver_options */));

            DBG(cerr << "GetRasterCount = " << ds.get()->GetRasterCount() << endl);


            const int data_size = x_size * y_size;
            DBG(cerr << "data_size = " << data_size << endl);
            unsigned int dsize = data_size * nBytes;
            DBG(cerr << "dsize = " << dsize << endl);

            data->read();
            data->set_read_p(true);

            // start band loop
            for(int i=1; i<=nBands; i++){
                GDALRasterBand *band = ds->GetRasterBand(i);
                if (!band)
                    throw Error(string("Could not get the GDALRasterBand for the GDALDataset: ") + CPLGetLastErrorMsg());
                DBG(cerr << "GDALRasterBand: " << band->GetBand() << endl);

                //Get data for one band
                #if !ADD_BAND
                    CPLErr error = band->RasterIO(GF_Write, 0, 0, x_size, y_size, data->get_buf() + dsize*(i-1), x_size, y_size, get_array_type(data), 0, 0);
                    if (error != CPLE_None)
                                            throw Error("Could not write data for band: " + long_to_string(i) + ": " + string(CPLGetLastErrorMsg()));
                #endif
                DBG(cerr << "band  = " << band->GetBand() << endl);
                DBG(cerr << "band x size = " << band->GetXSize() << endl);
                DBG(cerr << "band y size = " << band->GetYSize() << endl);
                DBG(cerr << "--------: " << endl);
            } // end band loop

            vector<double> geo_transform = get_geotransform_data(x, y);
            ds->SetGeoTransform(&geo_transform[0]);

            OGRSpatialReference native_srs;
            if (CE_None != native_srs.SetWellKnownGeogCS(srs.c_str())){
                string msg = "Could not set '" + srs + "' as the dataset native CRS.";
                DBG(cerr << "ERROR build_src_dataset(): " << msg << endl);
                throw BESError(msg,BES_SYNTAX_USER_ERROR,__FILE__,__LINE__);
            }

            // Connect the SRS/CRS to the GDAL Dataset
            char *pszSRS_WKT = NULL;
            native_srs.exportToWkt( &pszSRS_WKT );
            ds->SetProjection( pszSRS_WKT );
            CPLFree( pszSRS_WKT );

            DBG(cerr << "ProjectionRef = " << ds.get()->GetProjectionRef() << endl);
#endif
            DBG(cerr << "---------------------------------------test" << endl);
            int tBands = ds->GetRasterCount();
            DBG(cerr << "tBands = " << tBands << endl);
            for(int i=0; i<tBands; i++){
                GDALRasterBand *band = ds->GetRasterBand(i+1);
                if (!band)
                    throw Error(string("Could not get the GDALRasterBand for the GDALDataset: ") + CPLGetLastErrorMsg());
                DBG(cerr << "GDALRasterBand: " << band->GetBand() << endl);
                CPPUNIT_ASSERT(band->GetXSize() == x_size);
                CPPUNIT_ASSERT(band->GetYSize() == y_size);
                CPPUNIT_ASSERT(band->GetBand() == i+1);
                CPPUNIT_ASSERT(band->GetRasterDataType() == get_array_type(data));

                // This is just interesting...
                int block_x, block_y;
                band->GetBlockSize(&block_x, &block_y);
                DBG(cerr << "Block size: " << block_y << ", " << block_x << endl);

                double min, max;
                CPLErr error = band->GetStatistics(false, true, &min, &max, NULL, NULL);
                DBG(cerr << "min: " << min << ", max: " << max << " (error: " << error << ")" << endl);

                CPPUNIT_ASSERT(double_eq(min, -0.9));  // This is 1 and not -99 because the no data value is set.
                CPPUNIT_ASSERT(double_eq(max, 1.0));

                vector<double> gt(6);
                ds->GetGeoTransform(&gt[0]);

                DBG(cerr << "gt values: ");
                DBG(copy(gt.begin(), gt.end(), std::ostream_iterator<double>(std::cerr, " ")));
                DBG(cerr << endl);

                CPPUNIT_ASSERT(gt[0] == 21);  // min lon
                CPPUNIT_ASSERT(gt[1] == 2);   // resolution of lon; i.e., pixel width
                CPPUNIT_ASSERT(gt[2] == 0);   // north-south parallel to y axis
                CPPUNIT_ASSERT(gt[3] == -89);   // max lat
                CPPUNIT_ASSERT(gt[4] == 0);   // 0 if east-west is parallel to x axis
                CPPUNIT_ASSERT(gt[5] == 2);  // resolution of lat; neg for north up data
            }

            DBG(cerr << "------------------End test------------------" << endl);
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
    }

    void test_build_array_from_gdal_dataset_3D()
        {
            try {
                DBG(cerr << "Run test_build_array_from_gdal_dataset_3D" << endl);

                Array *data = dynamic_cast<Array*>(test3D_dds->var("data"));
                Array *t = dynamic_cast<Array*>(test3D_dds->var("time"));
                Array *x = dynamic_cast<Array*>(test3D_dds->var("lon"));
                Array *y = dynamic_cast<Array*>(test3D_dds->var("lat"));

                DBG(cerr << "original data: ");
                DBG(data->print_val(cerr));
                DBG(cerr << endl);

                // Array to gdal
                auto_ptr<GDALDataset> src = build_src_dataset_3D(data, t, x, y);

                //Get GDAL data
                int nBand = src.get()->GetRasterCount();
                DBG(cerr << "Band count = " << nBand << endl);
                for(int b = 0; b < nBand; b++){
                    // Extract the data now.
                    cerr << "----> GDAL data band " << b+1 << ":" << endl;
                    GDALRasterBand *band = src->GetRasterBand(b+1);
                    int x_size = band->GetXSize();
                    DBG(cerr << "band x_size: " << band->GetXSize() << endl);
                    int y_size = band->GetYSize();
                    DBG(cerr << "band y_size: " << band->GetYSize() << endl);

                    vector<dods_float32> buf(x->length() * y->length());
                    CPLErr error = band->RasterIO(GF_Read, 0, 0, x_size, y_size, &buf[0], x_size, y_size, get_array_type(data), 0, 0);
                    if (error != CPLE_None)
                        throw Error(string("Could not extract data for translated GDAL Dataset.") + CPLGetLastErrorMsg());

                    if (debug) {
                        for (int i = 0; i < y_size; ++i) {
                            for (int j = 0; j < x_size; ++j) {
                                cerr << buf[i * x_size + j] << " ";
                            }
                            cerr << endl;
                        }
                    }
                }

                // gdal to array
                auto_ptr<Array> result(build_array_from_gdal_dataset_3D(src.get(), data));

                cerr << endl;
                DBG(cerr << "new data: ");
                DBG(result->print_val(cerr));
                DBG(cerr << endl);

                DBG(cerr << "------------------End test------------------" << endl);
            }
            catch (Error &e) {
                CPPUNIT_FAIL(e.get_error_message());
            }
        }

#if 0
    void test_scaling_with_gdal()
    {
        try {

            DBG(cerr << "Run ");

            Array *d = dynamic_cast<Array*>(test3D_dds->var("data"));
            Array *t = dynamic_cast<Array*>(test3D_dds->var("time"));
            Array *lon = dynamic_cast<Array*>(test3D_dds->var("lon"));
            Array *lat = dynamic_cast<Array*>(test3D_dds->var("lat"));

            DBG(cerr << "d: ");
            DBG(d->print_val(cerr));
            DBG(cerr << endl);
            DBG(cerr << "t: ");
            DBG(t->print_val(cerr));
            DBG(cerr << endl);
            DBG(cerr << "lat: ");
            DBG(lat->print_val(cerr))
            DBG(cerr << endl);
            DBG(cerr << "lon: ");
            DBG(lon->print_val(cerr))
            DBG(cerr << endl);

            for (int i = 0; i < t->length(); i++) {

                DBG(cerr << "========= i = " << i << endl);

                const int data_size = x_size * y_size;

                DBG(cerr << "data_size = " << data_size << endl);
                dods_float32 *target = new dods_float32[data_size];
                unsigned int dsize = d->prototype()->width() * data_size;

                DBG(cerr << "dsize = " << dsize << endl);

                memcpy(target, d->get_buf() + i * dsize, dsize);

                string grd_name = "data_time";

                Array *arr = new Array("time_data", new Float32(grd_name));
                arr->set_value(target, data_size);
                arr->append_dim(y_size, "lat");
                arr->append_dim(x_size, "lon");
                //-------------------------------------------------------------------
                DBG(arr->print_val(cerr));
                DBG(cerr << endl);

                auto_ptr<GDALDataset> src = build_src_dataset(arr, lon, lat);

                const int dst_size_x = 22;
                const int dst_size_y = 15;
                SizeBox size(dst_size_x, dst_size_y);

                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                auto_ptr<GDALDataset> dst = scale_dataset(src, size);

                CPPUNIT_ASSERT(dst->GetRasterCount() == 1);

                GDALRasterBand *band = dst->GetRasterBand(1);
                if (!band)
                    throw Error(
                        string("Could not get the GDALRasterBand for the GDALDataset: ") + CPLGetLastErrorMsg());

                CPPUNIT_ASSERT(band->GetXSize() == dst_size_x);
                CPPUNIT_ASSERT(band->GetYSize() == dst_size_y);

                CPPUNIT_ASSERT(band->GetBand() == 1);
                CPPUNIT_ASSERT(band->GetRasterDataType() == get_array_type(arr));

                // This is just interesting...
                int block_x, block_y;
                band->GetBlockSize(&block_x, &block_y);
                DBG(cerr << "Block size: " << block_y << ", " << block_x << endl);

                double min, max;
                CPLErr error = band->GetStatistics(false, true, &min, &max, NULL, NULL);
                DBG(cerr << "min: " << min << ", max: " << max << " (error: " << error << ")" << endl);
                CPPUNIT_ASSERT(same_as(min, -0.9));  // This is 1 and not -99 because the no data value is set.
                CPPUNIT_ASSERT(same_as(max, 1.0));

                vector<double> gt(6);
                dst->GetGeoTransform(&gt[0]);

                DBG(cerr << "gt values: ");
                DBG(copy(gt.begin(), gt.end(), std::ostream_iterator<double>(std::cerr, " ")));
                DBG(cerr << endl);

                // gt values: 21 1.36364 0 -89 0 1.33333
                CPPUNIT_ASSERT(gt[0] == 21);  // min lon
                CPPUNIT_ASSERT(double_eq(gt[1], 1.36364));  // resolution of lon; i.e., pixel width
                CPPUNIT_ASSERT(gt[2] == 0.0);  // north-south parallel to y axis
                CPPUNIT_ASSERT(gt[3] == -89);  // max lat
                CPPUNIT_ASSERT(gt[4] == 0.0);  // 0 if east-west is parallel to x axis
                CPPUNIT_ASSERT(double_eq(gt[5], 1.33333));  // resolution of lat; neg for north up data

                // Extract the data now.
                vector<dods_float32> buf(dst_size_x * dst_size_y);
                error = band->RasterIO(GF_Read, 0, 0, dst_size_x, dst_size_y, &buf[0], dst_size_x, dst_size_y,
                    get_array_type(arr), 0, 0);
                if (error != CPLE_None)
                    throw Error(string("Could not extract data for translated GDAL Dataset.") + CPLGetLastErrorMsg());

                if (debug) {
                    cerr << "buf:" << endl;
                    for (int y = 0; y < dst_size_y; ++y) {
                        for (int x = 0; x < dst_size_x; ++x) {
                            cerr << buf[y * dst_size_x + x] << " ";
                        }
                        cerr << endl;
                    }
                }

                CPPUNIT_ASSERT(double_eq(buf[0 * dst_size_x + 4], -0.2));
                CPPUNIT_ASSERT(double_eq(buf[0 * dst_size_x + 9], -0.3));
                CPPUNIT_ASSERT(same_as(buf[1 * dst_size_x + 5], -0.2));
                CPPUNIT_ASSERT(same_as(buf[4 * dst_size_x + 11], -0.3));

                Array *out = build_array_from_gdal_dataset(dst.get(), arr);
                DBG(cerr << "out = " << out[0] << endl);

                GDALClose(dst.release());
            } //end time loop

        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
    }
#endif

    void test_build_maps_from_gdal_dataset_3D(){

        DBG(cerr << "Run test_build_maps_from_gdal_dataset_3D." << endl);

        Array *data = dynamic_cast<Array*>(test3D_dds->var("data"));
        Array *t = dynamic_cast<Array*>(test3D_dds->var("time"));
        Array *lon = dynamic_cast<Array*>(test3D_dds->var("lon"));
        Array *lat = dynamic_cast<Array*>(test3D_dds->var("lat"));

        DBG(cerr << "data: ");
        DBG(data->print_val(cerr));
        DBG(cerr << endl);
        DBG(cerr << "t: ");
        DBG(t->print_val(cerr));
        DBG(cerr << endl);
        DBG(cerr << "lat: ");
        DBG(lat->print_val(cerr))
        DBG(cerr << endl);
        DBG(cerr << "lon: ");
        DBG(lon->print_val(cerr))
        DBG(cerr << endl);

        auto_ptr<GDALDataset> src = build_src_dataset_3D(data, t, lon, lat);

        auto_ptr<Array> built_time(new Array("built_time", new Float32("built_time")));
        auto_ptr<Array> built_lon(new Array("built_lon", new Float32("built_lon")));
        auto_ptr<Array> built_lat(new Array("built_lat", new Float32("built_lat")));

        DBG(cerr << "built_time before: ");
        DBG(built_time->print_val(cerr))
        DBG(cerr << endl);

        // Build Time map
       int t_size = t->length();

       DBG(cerr << "t_size = " << t_size << endl);
       bool name_maps = false;

       if (name_maps) {
           built_time->append_dim(t_size, "Time");
       }
       else {
           built_time->append_dim(t_size);
       }
       vector<dods_float32> t_buf(t_size);
       t->value(&t_buf[0]);

       built_time->set_value(&t_buf[0], t_size);

        DBG(cerr << "built_time after: ");
        DBG(built_time.get()->print_val(cerr))
        DBG(cerr << endl);

        DBG(cerr << "------------------End test------------------" << endl);
    }

    void test_scaling_dap_array_3D(){
        try {
            DBG(cerr << "Run test_scaling_dap_array_3D." << endl);

            Array *data = dynamic_cast<Array*>(test3D_dds->var("data"));
            Array *t = dynamic_cast<Array*>(test3D_dds->var("time"));
            Array *lon = dynamic_cast<Array*>(test3D_dds->var("lon"));
            Array *lat = dynamic_cast<Array*>(test3D_dds->var("lat"));

            DBG(cerr << "data: ");
            DBG(data->print_val(cerr));
            DBG(cerr << endl);
            DBG(cerr << "t: ");
            DBG(t->print_val(cerr));
            DBG(cerr << endl);
            DBG(cerr << "lat: ");
            DBG(lat->print_val(cerr))
            DBG(cerr << endl);
            DBG(cerr << "lon: ");
            DBG(lon->print_val(cerr))
            DBG(cerr << endl);

            // build GDAL dataset
            auto_ptr<GDALDataset> src = build_src_dataset_3D(data, t, lon, lat);
            DBG(cerr << "src nBands = " << src->GetRasterCount() << endl);

            // result box
            const int x_dst_size = 10;
            const int y_dst_size = 7;

            SizeBox size(x_dst_size, y_dst_size);
            int data_size = x_dst_size * y_dst_size;

            DBG(cerr << "result data_size = " << data_size << endl);

            string crs = "WGS84";   // FIXME WGS84 assumes a certain order for lat and lon
            string interp = "nearest";

            // scale data set
            Grid *gr = scale_dap_array_3D(data, t, lon, lat, size, crs, interp);

            DBG(cerr << "gr = ");
            DBG(gr->print_val(cerr));
            DBG(cerr << endl);

            DBG(cerr << "------------------End test------------------" << endl);

        }catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
    }


CPPUNIT_TEST_SUITE( ScaleUtilTest3D );

    CPPUNIT_TEST(test_reading_data);
    CPPUNIT_TEST(test_get_size_box);
    CPPUNIT_TEST(test_build_src_dataset_3D);
    CPPUNIT_TEST(test_build_array_from_gdal_dataset_3D);
    CPPUNIT_TEST(test_build_maps_from_gdal_dataset_3D);
    CPPUNIT_TEST(test_scaling_dap_array_3D);

    CPPUNIT_TEST_SUITE_END()
    ;
};

CPPUNIT_TEST_SUITE_REGISTRATION(ScaleUtilTest3D);

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
            const std::vector<Test*> &tests = ScaleUtilTest3D::suite()->getTests();
            unsigned int prefix_len = ScaleUtilTest3D::suite()->getName().append("::").length();
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
            test = ScaleUtilTest3D::suite()->getName().append("::").append(argv[i++]);
            wasSuccessful = wasSuccessful && runner.run(test);
        }
    }

    return wasSuccessful ? 0 : 1;
}
