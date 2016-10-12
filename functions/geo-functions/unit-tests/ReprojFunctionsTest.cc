
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

#include <cmath>

#include <cpl_port.h>
#include <cpl_error.h>

#include <gdal.h>
#include <gdal_priv.h>
#include <ogr_spatialref.h>
#include <gdalwarper.h>

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

#include <GetOpt.h>

#include <BaseType.h>
#include <Array.h>
#include <Grid.h>

#include <test/TestTypeFactory.h>

#include <util.h>
#include <debug.h>

#include "scale_util.h"

#include "test_config.h"

static bool debug = false;
static bool bes_debug = false;

#undef DBG
#define DBG(x) do { if (debug) (x); } while(false);

const string small_dds_file = "small.dds";

using namespace CppUnit;
using namespace libdap;
using namespace std;

int test_variable_sleep_interval = 0;

class ReprojFunctionsTest : public TestFixture
{
private:
    DDS *small_dds;
    TestTypeFactory btf;

    CPLErrorHandler orig_err_handler;

    const string src_dir;
    const static int small_dim_size = 11;

    /**
     * @brief simple double equality test
     * @param a
     * @param b
     * @return True if they are within epsilon
     */
    bool same_as(double a, double b)
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
    ReprojFunctionsTest() : small_dds(0), src_dir(TEST_SRC_DIR)
    {
        GDALAllRegister();
        OGRRegisterAll();

        orig_err_handler = CPLSetErrorHandler(CPLQuietErrorHandler);
    }
    ~ReprojFunctionsTest()
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

    void test_warp_raster() {
        try {
        Array *data = dynamic_cast<Array*>(small_dds->var("data"));
        Array *lon = dynamic_cast<Array*>(small_dds->var("lon"));
        Array *lat = dynamic_cast<Array*>(small_dds->var("lat"));

        auto_ptr<GDALDataset> src = build_src_dataset(data, lon, lat);

        // Lets build the dst using the same element type as the src.
        SizeBox dst_size(small_dim_size, small_dim_size);
        auto_ptr<GDALDataset> dst = build_dst_dataset(dst_size, get_array_type(data));

        // The dst GDALDataset is modified as a side effect
        warp_raster(src.get(), dst.get());

        CPPUNIT_ASSERT(dst->GetRasterCount() == 1);

        GDALRasterBand *band = dst->GetRasterBand(1);
        if (!band)
            throw Error(string("Could not get the GDALRasterBand for the GDALDataset: ") + CPLGetLastErrorMsg());

        CPPUNIT_ASSERT(band->GetXSize() == 21);
        CPPUNIT_ASSERT(band->GetYSize() == 21);
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
#if 0
        CPPUNIT_ASSERT(gt[0] == -0.5);  // min lon
        CPPUNIT_ASSERT(gt[1] == 0.1);   // resolution of lon; i.e., pixel width
        CPPUNIT_ASSERT(gt[2] == 0.0);   // north-south parallel to y axis
        CPPUNIT_ASSERT(gt[3] == 5.0);   // max lat
        CPPUNIT_ASSERT(gt[4] == 0.0);   // 0 if east-west is parallel to x axis
        CPPUNIT_ASSERT(gt[5] == -1.0);  // resolution of lat; neg for north up data
#endif
        }
        catch (Error &e) {
            CPPUNIT_FAIL(e.get_error_message());
        }
    }

    CPPUNIT_TEST_SUITE( ReprojFunctionsTest );

    CPPUNIT_TEST(test_reading_data);
    CPPUNIT_TEST(test_get_size_box);
    CPPUNIT_TEST(test_get_geotransform_data);
    CPPUNIT_TEST(test_read_band_data);
    CPPUNIT_TEST(test_build_src_dataset);
    CPPUNIT_TEST(test_warp_raster);

    CPPUNIT_TEST_SUITE_END();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ReprojFunctionsTest);

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


// Old code //

// Used to initialize value in setup
#if 0
            dim_0_size = 135;
            dim_1_size = 90;

            small_dds = new DDS(&btf);
            string dds_file = (string)TEST_SRC_DIR + "/" + THREE_ARRAY_1_DDS;
            small_dds->parse(dds_file);
            DAS das;
            string das_file = (string)TEST_SRC_DIR + "/" + THREE_ARRAY_1_DAS;
            das.parse(das_file);
            small_dds->transfer_attributes(&das);

            DBG(cerr<<"setUp() - Loaded DDS:"<< endl);
            DBG(small_dds->print_xml(stderr, false, "noBlob"));

            // Load values into the array variables
            Array & t = dynamic_cast < Array & >(*small_dds->var("BT_diff_SO2"));
            Array & lon = dynamic_cast < Array & >(*small_dds->var("Longitude"));
            Array & lat = dynamic_cast < Array & >(*small_dds->var("Latitude"));

            DBG(cerr<<"setUp() - Synthesizing data values..."<< endl);

            dods_float32 t_vals[dim_0_size][dim_1_size];
            for (int i = 0; i < dim_0_size; ++i)
            for (int j = 0; j < dim_1_size; ++j)
            t_vals[i][j] = ((double)j) + (i * 10.0);
            t.set_value(&t_vals[0][0], dim_0_size*dim_1_size);
            t.set_read_p(true);

            // Read real lat/lon values from a Level 1B file ascii dump
            fstream input("three_array_1.txt", fstream::in);
            if (input.eof() || input.fail())
            throw Error("Could not open lat/lon data in SetUp.");
            // Read a line of text to get to the start of the data.
            string line;
            getline(input, line);
            if (input.eof() || input.fail())
            throw Error("Could not read lat/lon data in SetUp.");

            dods_float64 lon_vals[dim_0_size][dim_1_size];
            for (int i = 0; i < dim_0_size; ++i) {
                getline(input, line);
                if (input.eof() || input.fail())
                throw Error("Could not read lon data from row " + long_to_string(i) + " in SetUp.");
                vector<string> vals = split(line, ',');
                for (unsigned int j = 1; j < vals.size(); ++j) {
                    DBG2(cerr << "loading in lon value: " << vals[j] << "' " << atof(vals[j].c_str()) << endl;)
                    lon_vals[i][j-1] = atof(vals[j].c_str());
                }
            }
            lon.set_value(&lon_vals[0][0], dim_0_size*dim_1_size);
            lon.set_read_p(true);

            dods_float64 lat_vals[dim_0_size][dim_1_size];
            for (int i = 0; i < dim_0_size; ++i) {
                getline(input, line);
                if (input.eof() || input.fail())
                throw Error("Could not read lat data from row " + long_to_string(i) + " in SetUp.");
                vector<string> vals = split(line, ',');
                for (unsigned int j = 1; j < vals.size(); ++j) {
                    DBG2(cerr << "loading in lat value: " << vals[j] << "' " << atof(vals[j].c_str()) << endl;)
                    lat_vals[i][j-1] = atof(vals[j].c_str());
                }
            }
            lat.set_value(&lat_vals[0][0], dim_0_size*dim_1_size);
            lat.set_read_p(true);
#endif
#if 0
            small_dds = new DDS(&btf);
            string dds_file = /*(string)TEST_SRC_DIR + "/" +*/THREE_ARRAY_1_DDS;
            small_dds->parse(dds_file);
            DAS das;
            string das_file = /*(string)TEST_SRC_DIR + "/" +*/THREE_ARRAY_1_DAS;
            das.parse(das_file);
            small_dds->transfer_attributes(&das);

            DBG(cerr<<"setUp() - Loaded DDS:"<< endl);
            DBG(small_dds->print_xml(stderr, false, "noBlob"));

            // Load values into the array variables
            Array & t = dynamic_cast < Array & >(*small_dds->var("t"));
            Array & lon = dynamic_cast < Array & >(*small_dds->var("lon"));
            Array & lat = dynamic_cast < Array & >(*small_dds->var("lat"));

            dods_float64 t_vals[10][10];
            for (int i = 0; i < 10; ++i)
            for (int j = 0; j < 10; ++j)
            t_vals[i][j] = j + (i * 10);
            t.set_value(&t_vals[0][0], 100);
            t.set_read_p(true);

            double val=-5.0;

            DBG(cerr<<"setUp() - Building lon array...\n");
            dods_float64 lon_vals[10][10];
            for (unsigned i = 0; i < 10; ++i) {
                val = -5.0;
                for (unsigned int j = 0; j < 10; ++j) {
                    DBG2(cerr << "loading in lon value: " << val << "' " << endl;)
                    lon_vals[i][j] = val+=1.0;

                }
            }
            lon.set_value(&lon_vals[0][0], 100);
            lon.set_read_p(true);

            DBG(cerr<<"setUp() - Building lat array...\n");
            val=-5.0;
            dods_float64 lat_vals[10][10];
            for (int i = 0; i < 10; ++i) {
                val = -5.0;
                for (unsigned int j = 0; j < 10; ++j) {
                    DBG2(cerr << "loading in lat value: " << val << "' " << endl;)
                    lat_vals[i][j] = val+=1.0;
                }
            }
            lat.set_value(&lat_vals[0][0], 100);
            lat.set_read_p(true);

#endif


// Old tests

#if 0
    void no_arguments_test()
    {
        DBG(cerr<<"no_arguments_test() - BEGIN" << endl);
        try {
            BaseType *btp = 0;
            function_swath2array(0, 0, *small_dds, &btp);
            CPPUNIT_ASSERT(true);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("no_arguments_test(): " + e.get_error_message());
        }
        DBG(cerr<<"no_arguments_test() - END" << endl);
    }

    void array_return_test()
    {
        DBG(cerr<<"array_return_test() - BEGIN" << endl);
        try {
            BaseType *argv[3];
            argv[0] = small_dds->var("BT_diff_SO2");
            argv[1] = small_dds->var("Longitude");
            argv[2] = small_dds->var("Latitude");

            BaseType *btp = 0;
            function_swath2array(3, argv, *dds, &btp);

            DBG(cerr << "btp->name(): " << btp->name() << endl);
            CPPUNIT_ASSERT(btp->name() == "BT_diff_SO2");
            CPPUNIT_ASSERT(btp->type() == dods_array_c);

            // Extract data; I know it's 135x154 from debugging output
            dods_float64 values[135][154];
            Array *a = static_cast<Array*>(btp);
            a->value(&values[0][0]);
        }
        catch (Error &e) {
            CPPUNIT_FAIL("array_return_test: " + e.get_error_message());
        }
        DBG(cerr<<"array_return_test() - END" << endl);
    }



    void one_argument_not_a_grid_test()
    {
        try {
            BaseType *argv[1];
            argv[0] = small_dds->var("lat");
            CPPUNIT_ASSERT(argv[0] && "dds->var should find this, although it is not a grid");
            BaseType *btp = 0;
            function_grid(1, argv, *dds, &btp);
            CPPUNIT_ASSERT(!"one_argument_not_a_grid_test() should have failed");
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(true);
        }
    }

    void map_not_in_grid_test()
    {
        try {
            BaseType *argv[2];
            argv[0] = small_dds->var("a");
            CPPUNIT_ASSERT(argv[0] && "small_dds->var should find this");
            argv[1] = new Str("");
            string expression = "3<second<=7";
            dynamic_cast<Str*>(argv[1])->val2buf(&expression);
            dynamic_cast<Str*>(argv[1])->set_read_p(true);
            BaseType *btp = 0;
            function_grid(2, argv, *dds, &btp);
            CPPUNIT_ASSERT(!"map_not_in_grid_test() should have failed");
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(true);
        }
    }

    void one_dim_grid_test()
    {
        try {
            BaseType *argv[2];
            argv[0] = small_dds->var("a");
            CPPUNIT_ASSERT(argv[0] && "small_dds->var should find this");
            argv[1] = new Str("");
            string expression = "3<first<=7";
            dynamic_cast<Str*>(argv[1])->val2buf(&expression);
            dynamic_cast<Str*>(argv[1])->set_read_p(true);

            BaseType *btp = 0;
            function_grid(2, argv, *small_dds, &btp);
            Grid &g = dynamic_cast<Grid&>(*btp);

            //Grid &g = dynamic_cast<Grid&>(*argv[0]);
            Array &m = dynamic_cast<Array&>(**g.map_begin());
            CPPUNIT_ASSERT(m.dimension_start(m.dim_begin(), true) == 4);
            CPPUNIT_ASSERT(m.dimension_stop(m.dim_begin(), true) == 7);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"one_dim_grid_test() should have worked");
        }
    }

    void one_dim_grid_two_expressions_test()
    {
        try {
            BaseType *argv[3];
            argv[0] = small_dds->var("a");
            CPPUNIT_ASSERT(argv[0] && "small_dds->var should find this");

            argv[1] = new Str("");
            string expression = "first>3";
            dynamic_cast<Str*>(argv[1])->val2buf(&expression);
            dynamic_cast<Str*>(argv[1])->set_read_p(true);

            argv[2] = new Str("");
            expression = "first<=7";
            dynamic_cast<Str*>(argv[2])->val2buf(&expression);
            dynamic_cast<Str*>(argv[2])->set_read_p(true);

            //function_grid(3, argv, *small_dds);
            BaseType *btp = 0;
            function_grid(3, argv, *small_dds, &btp);
            Grid &g = dynamic_cast<Grid&>(*btp);

            //Grid &g = dynamic_cast<Grid&>(*function_grid(3, argv, *dds));
            //Grid &g = dynamic_cast<Grid&>(*argv[0]);
            Array &m = dynamic_cast<Array&>(**g.map_begin());
            CPPUNIT_ASSERT(m.dimension_start(m.dim_begin(), true) == 4);
            CPPUNIT_ASSERT(m.dimension_stop(m.dim_begin(), true) == 7);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"one_dim_grid_two_expressions_test() should have worked");
        }
    }

    void one_dim_grid_descending_test()
    {
        try {
            BaseType *argv[2];
            argv[0] = small_dds->var("b");
            CPPUNIT_ASSERT(argv[0] && "small_dds->var should find this");
            argv[1] = new Str("");
            string expression = "3<first<=7";
            dynamic_cast<Str*>(argv[1])->val2buf(&expression);
            dynamic_cast<Str*>(argv[1])->set_read_p(true);

            BaseType *btp = 0;
            function_grid(2, argv, *small_dds, &btp);
            Grid &g = dynamic_cast<Grid&>(*btp);

            //function_grid(2, argv, *dds);
            //Grid &g = dynamic_cast<Grid&>(*function_grid(2, argv, *dds));
            //Grid &g = dynamic_cast<Grid&>(*argv[0]);
            Array &m = dynamic_cast<Array&>(**g.map_begin());
            CPPUNIT_ASSERT(m.dimension_start(m.dim_begin(), true) == 2);
            CPPUNIT_ASSERT(m.dimension_stop(m.dim_begin(), true) == 5);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"one_dim_grid_test() should have worked");
        }
    }

    void one_dim_grid_two_expressions_descending_test()
    {
        try {
            BaseType *argv[3];
            argv[0] = small_dds->var("b");
            CPPUNIT_ASSERT(argv[0] && "small_dds->var should find this");

            argv[1] = new Str("");
            string expression = "first>3";
            dynamic_cast<Str*>(argv[1])->val2buf(&expression);
            dynamic_cast<Str*>(argv[1])->set_read_p(true);

            argv[2] = new Str("");
            expression = "first<=7";
            dynamic_cast<Str*>(argv[2])->val2buf(&expression);
            dynamic_cast<Str*>(argv[2])->set_read_p(true);

            BaseType *btp = 0;
            function_grid(3, argv, *small_dds, &btp);
            Grid &g = dynamic_cast<Grid&>(*btp);

            //function_grid(3, argv, *dds);
            //Grid &g = dynamic_cast<Grid&>(*function_grid(3, argv, *dds));
            //Grid &g = dynamic_cast<Grid&>(*argv[0]);
            Array &m = dynamic_cast<Array&>(**g.map_begin());
            CPPUNIT_ASSERT(m.dimension_start(m.dim_begin(), true) == 2);
            CPPUNIT_ASSERT(m.dimension_stop(m.dim_begin(), true) == 5);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"one_dim_grid_two_expressions_test() should have worked");
        }
    }

    void one_dim_grid_noninclusive_values_test()
    {
        try {
            BaseType *argv[2];
            argv[0] = small_dds->var("a");
            CPPUNIT_ASSERT(argv[0] && "small_dds->var should find this");
            argv[1] = new Str("");
            string expression = "7<first<=3";
            dynamic_cast<Str*>(argv[1])->val2buf(&expression);
            dynamic_cast<Str*>(argv[1])->set_read_p(true);

            BaseType *btp = 0;
            function_grid(2, argv, *dds, &btp);
            //Grid &g = dynamic_cast<Grid&>(*btp);

            // function_grid(2, argv, *dds);

            CPPUNIT_ASSERT(!"one_dim_grid_noninclusive_values_test() should not have worked");
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(true);
        }
    }

    // grid() is not required to handle this case. This test is not used.
    void values_outside_map_range_test()
    {
        try {
            BaseType *argv[2];
            argv[0] = small_dds->var("a");
            CPPUNIT_ASSERT(argv[0] && "small_dds->var should find this");
            argv[1] = new Str("");
            string expression = "3<=first<20";
            dynamic_cast<Str*>(argv[1])->val2buf(&expression);
            dynamic_cast<Str*>(argv[1])->set_read_p(true);

            BaseType *btp = 0;
            function_grid(2, argv, *dds, &btp);
            //Grid &g = dynamic_cast<Grid&>(*btp);

            // function_grid(2, argv, *dds);

            CPPUNIT_ASSERT(!"values_outside_map_range_test() should not have worked");
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(true);
        }
    }

    // linear_scale tests
    void linear_scale_args_test() {
        try {
            BaseType *btp = 0;
            function_linear_scale(0, 0, *small_dds, &btp);
            CPPUNIT_ASSERT(true);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"linear_scale_args_test: should not throw Error");
        }
    }

    void linear_scale_array_test() {
        try {
            Array *a = dynamic_cast<Grid&>(*small_dds->var("a")).get_array();
            CPPUNIT_ASSERT(a);
            BaseType *argv[3];
            argv[0] = a;
            argv[1] = new Float64("");
            dynamic_cast<Float64*>(argv[1])->set_value(0.1);//m
            argv[2] = new Float64("");
            dynamic_cast<Float64*>(argv[2])->set_value(10);//b
            BaseType *scaled = 0;
            function_linear_scale(3, argv, *small_dds, &scaled);
            CPPUNIT_ASSERT(scaled->type() == dods_array_c
                           && scaled->var()->type() == dods_float64_c);
            double *values = extract_double_array(dynamic_cast<Array*>(scaled));
            CPPUNIT_ASSERT(values[0] == 10);
            CPPUNIT_ASSERT(values[1] == 10.1);
            CPPUNIT_ASSERT(values[9] == 10.9);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Error in linear_scale_grid_test()");
        }
    }

    void linear_scale_grid_test() {
        try {
            Grid *g = dynamic_cast<Grid*>(small_dds->var("a"));
            CPPUNIT_ASSERT(g);
            BaseType *argv[3];
            argv[0] = g;
            argv[1] = new Float64("");
            dynamic_cast<Float64*>(argv[1])->set_value(0.1);
            argv[2] = new Float64("");
            dynamic_cast<Float64*>(argv[2])->set_value(10);
            BaseType *scaled = 0;
            function_linear_scale(3, argv, *small_dds, &scaled);
            CPPUNIT_ASSERT(scaled->type() == dods_grid_c);
            Grid *g_s = dynamic_cast<Grid*>(scaled);
            CPPUNIT_ASSERT(g_s->get_array()->var()->type() == dods_float64_c);
            double *values = extract_double_array(g_s->get_array());
            CPPUNIT_ASSERT(values[0] == 10);
            CPPUNIT_ASSERT(values[1] == 10.1);
            CPPUNIT_ASSERT(values[9] == 10.9);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Error in linear_scale_grid_test()");
        }
    }

    void linear_scale_grid_attributes_test() {
        try {
            Grid *g = dynamic_cast<Grid*>(small_dds->var("a"));
            CPPUNIT_ASSERT(g);
            BaseType *argv[1];
            argv[0] = g;
            BaseType *scaled = 0;
            function_linear_scale(1, argv, *small_dds, &scaled);
            CPPUNIT_ASSERT(scaled->type() == dods_grid_c);
            Grid *g_s = dynamic_cast<Grid*>(scaled);
            CPPUNIT_ASSERT(g_s->get_array()->var()->type() == dods_float64_c);
            double *values = extract_double_array(g_s->get_array());
            CPPUNIT_ASSERT(values[0] == 10);
            CPPUNIT_ASSERT(values[1] == 10.1);
            CPPUNIT_ASSERT(values[9] == 10.9);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Error in linear_scale_grid_test()");
        }
    }

    // This tests the case where attributes are not found
    void linear_scale_grid_attributes_test2() {
        try {
            Grid *g = dynamic_cast<Grid*>(small_dds->var("b"));
            CPPUNIT_ASSERT(g);
            BaseType *argv[1];
            argv[0] = g;
            BaseType *btp = 0;
            function_linear_scale(1, argv, *dds, &btp);
            CPPUNIT_FAIL("Should not get here; no params passed and no attributes set for grid 'b'");
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT("Caught exception");
        }
    }

    void linear_scale_scalar_test() {
        try {
            Int32 *i = new Int32("linear_scale_test_int32");
            CPPUNIT_ASSERT(i);
            i->set_value(1);
            BaseType *argv[3];
            argv[0] = i;
            argv[1] = new Float64("");
            dynamic_cast<Float64*>(argv[1])->set_value(0.1);//m
            argv[2] = new Float64("");
            dynamic_cast<Float64*>(argv[2])->set_value(10);//b
            BaseType *scaled = 0;
            function_linear_scale(3, argv, *small_dds, &scaled);
            CPPUNIT_ASSERT(scaled->type() == dods_float64_c);

            CPPUNIT_ASSERT(dynamic_cast<Float64*>(scaled)->value() == 10.1);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT(!"Error in linear_scale_scalar_test()");
        }
    }


    void function_dap_1_test() {
        try {
            Int32 *i = new Int32("function_dap_1_test_int32");
            CPPUNIT_ASSERT(i);
            i->set_value(2);
            BaseType *argv[1];
            argv[0] = i;

            ConstraintEvaluator unused;
            function_dap(1, argv, *small_dds, unused);

            CPPUNIT_ASSERT(small_dds->get_dap_major() == 2);
            CPPUNIT_ASSERT(small_dds->get_dap_minor() == 0);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_FAIL("Error in function_dap_1_test(): " + e.get_error_message());
        }
    }

    void function_dap_2_test() {
        try {
            Float64 *d = new Float64("function_dap_1_test_float64");
            CPPUNIT_ASSERT(d);
            d->set_value(3.2);
            BaseType *argv[1];
            argv[0] = d;

            ConstraintEvaluator unused;
            function_dap(1, argv, *small_dds, unused);

            CPPUNIT_ASSERT(small_dds->get_dap_major() == 3);
            CPPUNIT_ASSERT(small_dds->get_dap_minor() == 2);
        }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_FAIL("Error in function_dap_2_test(): " + e.get_error_message());
        }
    }

    void function_dap_3_test() {
        try {
            cerr <<"In function_dap_3_test" << endl;
            ConstraintEvaluator unused;
            function_dap(0, 0, *dds, unused);

            CPPUNIT_FAIL("Should have thrown an exception on no args");
                    }
        catch (Error &e) {
            DBG(cerr << e.get_error_message() << endl);
            CPPUNIT_ASSERT("Pass: Caught exception");
        }
    }

    void grid_return_test()
    {
        DBG(cerr<<"grid_return_test() - BEGIN" << endl);
        try {
            BaseType *argv[3];
            argv[0] = small_dds->var("BT_diff_SO2");
            argv[1] = small_dds->var("Longitude");
            argv[2] = small_dds->var("Latitude");


            DBG(cerr << "Input values:" << endl);
            dods_float32 data_vals[dim_0_size][dim_1_size];
            Array *a = static_cast<Array*>(argv[0]);
            a->value(&data_vals[0][0]);
            for (int i = 0; i < dim_0_size; ++i) {
                for (int j = 0; j < dim_1_size; ++j) {
                    DBG2(cerr << "BT_diff_SO2[" << i << "][" << j << "]: " <<  data_vals[i][j] << endl);
                }
            }

            DBG(cerr<<"grid_return_test() - Calling function_swath2grid()" << endl);

            BaseType *btp = 0;
            function_swath2grid(3, argv, *dds, &btp);
            DBG(cerr<<"grid_return_test() - Completed function_swath2grid()" << endl);

            DBG(cerr << "btp->name(): " << btp->name() << endl);
            CPPUNIT_ASSERT(btp->name() == "t");
            CPPUNIT_ASSERT(btp->type() == dods_grid_c);

            // Extract data; I know it's 135x154 from debugging output
            dods_float64 values[135][154];
            Grid *g = static_cast<Grid*>(btp);
            g->get_array()->value(&values[0][0]);

            Grid::Map_iter m = g->map_begin();
            dods_float64 lat[135];
            static_cast<Array*>(*m)->value(&lat[0]);

            ++m;
            dods_float64 lon[154];
            static_cast<Array*>(*m)->value(&lon[0]);

            cerr << "Output values:" << endl;
            for (int i = 0; i < 135; ++i) {
                for (int j = 0; j < 154; ++j) {
                    DBG2(cerr << "t[" << i << "][" << j << "] == lon: " << lon[j] << ", lat: " << lat[i] << " val: " << values[i][j] << endl);
                }
            }
        }
        catch (Error &e) {
            CPPUNIT_FAIL("gid_return_test: " + e.get_error_message());
        }
        DBG(cerr<<"grid_return_test() - END" << endl);
    }
#endif

