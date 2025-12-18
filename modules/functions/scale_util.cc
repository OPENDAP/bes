// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
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

#include <iostream>
#include <vector>
#include <memory>
#include <limits>
#include <sstream>
#include <cassert>

#include <gdal.h>
#include <gdal_priv.h>
#include <gdal_utils.h>
#include <ogr_spatialref.h>
#include <gdalwarper.h>

#include <libdap/Float32.h>
#include <libdap/Int16.h>
#include <libdap/Array.h>
#include <libdap/Grid.h>

#include <libdap/util.h>
#include <libdap/Error.h>
#include <BESDebug.h>
#include <BESError.h>

#include "ScaleGrid.h"

#define DEBUG_KEY "geo"

using namespace std;
using namespace libdap;

namespace functions {
/**
 * @brief return a SizeBox circumscribing the two DAP Arrays
 * @param x
 * @param y
 * @return A SizeBox
 */
SizeBox get_size_box(Array *x, Array *y) {
    int src_x_size = x->dimension_size(x->dim_begin(), true);
    int src_y_size = y->dimension_size(y->dim_begin(), true);

    return SizeBox(src_x_size, src_y_size);
}

/**
 * @brief simple double equality test
 * @see http://stackoverflow.com/questions/17333/most-effective-way-for-float-and-double-comparison
 * @param a
 * @param b
 * @return True if they are within epsilon
 */
static bool same_as(const double a, const double b) {
    // use float's epsilon since double's is too small for these tests
    return fabs(a - b) <= numeric_limits<float>::epsilon();
}

/**
 * @brief Test an array of doubles to see if its values are monotonic and uniform
 * @param values The array
 * @param res The uniform offset between elements
 * @return True if the array is monotonic and uniform, otherwise false.
 */
bool monotonic_and_uniform(const vector<double> &values, double res) {
    vector<double>::size_type end_index = values.size() - 1;
    for (vector<double>::size_type i = 0; i < end_index; ++i) {
        //        BESDEBUG(DEBUG_KEY, "values[" << i+1 << "]: " << values[i+1] << " - values[" << i << "]: " << values[i] << endl);
        //        BESDEBUG(DEBUG_KEY, values[i+1] - values[i] <<" != res: " << res << endl);
        if (!same_as((values[i + 1] - values[i]), res))
            return false;
    }

    return true;
}

/**
 * @brief Extract the geo-transform coordinates from a DP2 Grid
 *
 * @note Side effect: Data are read into the x and y Arrays
 *
 * @param x
 * @param y
 * @param test_maps Should the x and y arrays be tested to ensure they are monotonic
 * and uniform? By default, false unless the source has been built with --enable-developer
 * @return A vector<double> of length 6 with the GDAL geo-transform parameters
 */
vector<double> get_geotransform_data(Array *x, Array *y, bool test_maps /* default: false*/) {
#ifndef NDEBUG
    test_maps = true;
#endif

    SizeBox size = get_size_box(x, y);

    y->read();
    vector<double> y_values(size.y_size);
    extract_double_array(y, y_values);

    double res_y = (y_values[y_values.size() - 1] - y_values[0]) / (y_values.size() - 1);

    if (test_maps && !monotonic_and_uniform(y_values, res_y)) {
        string msg = "The grids maps/dimensions must be monotonic and uniform (" + y->name() + ").";
        BESDEBUG(DEBUG_KEY, "ERROR get_geotransform_data(): " << msg << endl);
        throw BESError(msg,BES_SYNTAX_USER_ERROR,__FILE__,__LINE__);
    }
    x->read();
    vector<double> x_values(size.x_size);
    extract_double_array(x, x_values);

    double res_x = (x_values[x_values.size() - 1] - x_values[0]) / (x_values.size() - 1);

    if (test_maps && !monotonic_and_uniform(x_values, res_x)) {
        string msg = "The grids maps/dimensions must be monotonic and uniform (" + x->name() + ").";
        BESDEBUG(DEBUG_KEY, "ERROR get_geotransform_data(): " << msg << endl);
        throw BESError(msg,BES_SYNTAX_USER_ERROR,__FILE__,__LINE__);
    }

    // Original comment:
    // In case of north up images, the GT(2) and GT(4) coefficients are zero,
    // and the GT(1) is pixel width, and GT(5) is pixel height. The (GT(0),GT(3))
    // position is the top left corner of the top left pixel of the raster.
    //
    // Note that for an inverted dataset, use min_y and res_y for GT(3) and GT(5)
    //
    // 10/27/16 I decided to not treat this as lat/lon information but simply
    // develop a mathematical transform that will be correct for the data as given
    // _so long as_ the data are monotonic and uniform. jhrg

    vector<double> geo_transform(6);
    geo_transform[0] = x_values[0];
    geo_transform[1] = res_x;
    geo_transform[2] = 0; // Assumed because the x/y maps are vectors
    geo_transform[3] = y_values[0];
    geo_transform[4] = 0;
    geo_transform[5] = res_y;

    return geo_transform;
}

/**
 * @brief
 *
 * @todo Improve (don't copy return values, protect sample values, plug leaks).
 *
 * @param x
 * @param y
 * @param sample_x
 * @param sample_y
 * @return
 */
vector<GDAL_GCP> get_gcp_data(Array *x, Array *y, int sample_x, int sample_y) {
    SizeBox size = get_size_box(x, y);

    y->read();
    vector<double> y_values(size.y_size);
    extract_double_array(y, y_values);

    x->read();
    vector<double> x_values(size.x_size);
    extract_double_array(x, x_values);

    // Build the GCP list.

    // Determine how many control points to use. Subset by a factor of M
    // but never use less than 10% of of the x and y axis values (each)
    // FIXME Just use given values for now, which will default to 1.

    // Build the GCP list, sampling as per sample_x and sample_y
    unsigned long n_gcps = (size.x_size / sample_x) * (size.y_size / sample_y);

    vector<GDAL_GCP> gcp_list(n_gcps);
    GDALInitGCPs(n_gcps, gcp_list.data()); // allocates the 'list'; free with Deinit

    unsigned long count = 0;
    for (int i = 0; i < size.x_size; i += sample_x) {
        // The test for count < n_gcps corrects for discrepancies between integer
        // division and the simple increment used by the loops. 3/29/17 jhrg
        for (int j = 0; count < n_gcps && j < size.y_size; j += sample_y) {
            gcp_list[count].dfGCPLine = j;
            gcp_list[count].dfGCPPixel = i;
            gcp_list[count].dfGCPX = x_values[i];
            gcp_list[count].dfGCPY = y_values[j];

            ++count;
        }
    }

    return gcp_list;
}

GDALDataType get_array_type(const Array *a) {
    switch (const_cast<Array *>(a)->var()->type()) {
        case dods_byte_c:
            return GDT_Byte;

        case dods_uint16_c:
            return GDT_UInt16;

        case dods_int16_c:
            return GDT_Int16;

        case dods_uint32_c:
            return GDT_UInt32;

        case dods_int32_c:
            return GDT_Int32;

        case dods_float32_c:
            return GDT_Float32;

        case dods_float64_c:
            return GDT_Float64;

        // DAP4 support
        case dods_uint8_c:
        case dods_int8_c:
            return GDT_Byte;

        case dods_uint64_c:
        case dods_int64_c:
        default: {
            string msg = "Cannot perform geo-spatial operations on "
                         + const_cast<Array *>(a)->var()->type_name() + " data.";
            BESDEBUG(DEBUG_KEY, "ERROR get_array_type(): " << msg << endl);
            throw BESError(msg,BES_SYNTAX_USER_ERROR,__FILE__,__LINE__);
        }
    }
}

/**
 * @brief Used to transfer data values from a gdal dataset to a dap Array
 * @param band Read from this raster and
 * @param y Rows
 * @param x Cols
 * @param a Set the values in this array
 * @return Return a pointer to parameter 'a'
 */
template<typename T>
static Array *transfer_values_helper(GDALRasterBand *band, const unsigned long x, const unsigned long y, Array *a) {
    // get the data
    vector<T> buf(x * y);
    CPLErr error = band->RasterIO(GF_Read, 0, 0, x, y, buf.data(), x, y, get_array_type(a), 0, 0);

    if (error != CPLE_None) {
        string msg = string("Could not extract data for array.") + CPLGetLastErrorMsg();
        BESDEBUG(DEBUG_KEY, "ERROR transfer_values_helper(): " << msg << endl);
        throw BESError(msg,BES_SYNTAX_USER_ERROR,__FILE__,__LINE__);
    }
    a->set_value(buf, buf.size());

    return a;
}

/**
 * @brief Extract data from a gdal dataset and store it in a dap Array
 *
 * @param source The gdal dataset; data source
 * @param dest The dap Array; destnation
 * @return
 */
Array *build_array_from_gdal_dataset(GDALDataset *source, const Array *dest) {
    // Get the GDALDataset size
    GDALRasterBand *band = source->GetRasterBand(1);
    unsigned long x = band->GetXSize();
    unsigned long y = band->GetYSize();

    // Build a new DAP Array; use the dest Array's element type
    Array *result = new Array("result", const_cast<Array *>(dest)->var()->ptr_duplicate());
    result->append_dim(y);
    result->append_dim(x);

    // get the data
    switch (result->var()->type()) {
        case dods_byte_c:
            return transfer_values_helper<dods_byte>(source->GetRasterBand(1), x, y, result);
            break;
        case dods_uint16_c:
            return transfer_values_helper<dods_uint16>(source->GetRasterBand(1), x, y, result);
            break;
        case dods_int16_c:
            return transfer_values_helper<dods_int16>(source->GetRasterBand(1), x, y, result);
            break;
        case dods_uint32_c:
            return transfer_values_helper<dods_uint32>(source->GetRasterBand(1), x, y, result);
            break;
        case dods_int32_c:
            return transfer_values_helper<dods_int32>(source->GetRasterBand(1), x, y, result);
            break;
        case dods_float32_c:
            return transfer_values_helper<dods_float32>(source->GetRasterBand(1), x, y, result);
            break;
        case dods_float64_c:
            return transfer_values_helper<dods_float64>(source->GetRasterBand(1), x, y, result);
            break;
        case dods_uint8_c:
            return transfer_values_helper<dods_byte>(source->GetRasterBand(1), x, y, result);
            break;
        case dods_int8_c:
            return transfer_values_helper<dods_int8>(source->GetRasterBand(1), x, y, result);
            break;
        case dods_uint64_c:
        case dods_int64_c:
        default:
            string msg = "The source array to a geo-function contained an unsupported numeric type.";
            BESDEBUG(DEBUG_KEY, "ERROR build_array_from_gdal_dataset(): " << msg << endl);
            throw BESError(msg,BES_SYNTAX_USER_ERROR,__FILE__,__LINE__);
    }
}

/**
 * @brief Extract data from a gdal dataset and store it in a dap Array
 *
 * @param source3D The gdal dataset; 3D data source
 * @param dest The dap Array; destnation
 * @return
 */
Array *build_array_from_gdal_dataset_3D(GDALDataset *source3D, const Array *dest) {
    // DAP array result
    int t_size = source3D->GetRasterCount();
    int x_size = source3D->GetRasterBand(1)->GetXSize();
    int y_size = source3D->GetRasterBand(1)->GetYSize();
    Array *result = new Array("result", const_cast<Array *>(dest)->var()->ptr_duplicate());
    result->append_dim(t_size);
    result->append_dim(y_size);
    result->append_dim(x_size);

    vector<dods_float32> res(t_size * x_size * y_size);
    for (int i = 0; i < t_size; i++) {
        // get the data
        GDALRasterBand *band = source3D->GetRasterBand(i + 1);
        if (!band)
            throw Error(string("Could not get the GDALRasterBand for the GDALDataset: ") + CPLGetLastErrorMsg());
        vector<double> gt(6);
        source3D->GetGeoTransform(gt.data());
        // Extract data from band
        vector<dods_float32> buf(x_size * y_size);
        CPLErr error = band->RasterIO(GF_Read, 0, 0, x_size, y_size, buf.data(), x_size, y_size, get_array_type(dest),
                                      0, 0);
        if (error != CPLE_None)
            throw Error(string("Could not extract data for translated GDAL Dataset.") + CPLGetLastErrorMsg());
        if (i == 0) {
            res = buf;
        }
        else {
            res.insert(res.end(), buf.begin(), buf.end());
        }
    }
    result->set_value(res, res.size());
    return result;
}


/**
 * @brief build lon and lat maps using a GDAL dataset
 *
 * Given a GDAL Dataset, use the geo-transform information along with
 * the dataset's extent (height and width in pixels) to build Maps/shared
 * dimensions for DAP2/4 Grid/Coverages. The two Array arguments must
 * be allocated by the caller and have an element type of dods_float32, but
 * their dimensionality should not be set.
 *
 * @note
 * Xgeo = GT(0) + Xpixel*GT(1) + Yline*GT(2)
 * Ygeo = GT(3) + Xpixel*GT(4) + Yline*GT(5)
 * For an inverted dataset, use min_y and res_y for GT(3) and GT(5)
 * In case of north up images, the GT(2) and GT(4) coefficients are zero
 *
 * @param dst Source for the DAP2 Grid maps (or DAP4 shared dimensions)
 * @param x_map value-result parameter for the longitude map (uses dods_float32
 * elements)
 * @param y_map value-result parameter for the latitude map
 * @param name_maps If true, name the x map "Latitude" and the y map "Longitude"
 * if false, do not name the maps.
 */
void build_maps_from_gdal_dataset(GDALDataset *dst, Array *x_map, Array *y_map, bool name_maps /*default false */) {
    // get the geo-transform data
    vector<double> gt(6);
    dst->GetGeoTransform(gt.data());

    // Get the GDALDataset size
    GDALRasterBand *band = dst->GetRasterBand(1);

    // Build Lon map
    unsigned long x = band->GetXSize(); // x_map_vals

    if (name_maps) {
        x_map->append_dim(x, "Longitude");
    }
    else {
        x_map->append_dim(x);
    }

    // for each value, use the geo-transform data to compute a value and store it.
    vector<dods_float32> x_map_vals(x);
    dods_float32 *cur_x = x_map_vals.data();
    dods_float32 *prev_x = cur_x;
    // x_map_vals[0] = gt[0];
    *cur_x++ = gt[0];
    for (unsigned long i = 1; i < x; ++i) {
        // x_map_vals[i] = gt[0] + i * gt[1];
        // x_map_vals[i] = x_map_vals[i-1] + gt[1];
        *cur_x++ = *prev_x++ + gt[1];
    }

    x_map->set_value(x_map_vals.data(), x); // copies values to new storage

    // Build the Lat map
    unsigned long y = band->GetYSize();

    if (name_maps) {
        y_map->append_dim(y, "Latitude");
    }
    else {
        y_map->append_dim(y);
    }

    // for each value, use the geo-transform data to compute a value and store it.
    vector<dods_float32> y_map_vals(y);
    dods_float32 *cur_y = y_map_vals.data();
    dods_float32 *prev_y = cur_y;
    // y_map_vals[0] = gt[3];
    *cur_y++ = gt[3];
    for (unsigned long i = 1; i < y; ++i) {
        // y_map_vals[i] = gt[3] + i * gt[5];
        // y_map_vals[i] = y_map_vals[i-1] + gt[5];
        *cur_y++ = *prev_y++ + gt[5];
    }

    y_map->set_value(y_map_vals.data(), y);
}

/**
 * @brief build time, lon and lat maps using a 3D GDAL dataset
 *
 * Given a 3D GDAL Dataset, use the geo-transform information along with
 * the dataset's extent (height and width in pixels) to build Maps/shared
 * dimensions for DAP2/4 Grid/Coverages. The three Array arguments must
 * be allocated by the caller and have an element type of dods_float32, but
 * their dimensionality should not be set.
 *
 * @note
 * Xgeo = GT(0) + Xpixel*GT(1) + Yline*GT(2)
 * Ygeo = GT(3) + Xpixel*GT(4) + Yline*GT(5)
 * For an inverted dataset, use min_y and res_y for GT(3) and GT(5)
 * In case of north up images, the GT(2) and GT(4) coefficients are zero
 *
 * @param dst Source for the DAP2 Grid maps (or DAP4 shared dimensions)
 * @param t_map value-result parameter for time map (get values from t)
 * @param x_map value-result parameter for the longitude map (uses dods_float32
 * elements)
 * @param y_map value-result parameter for the latitude map
 * @param name_maps If true, name t map "Time", the x map "Latitude" and the y map "Longitude"
 * if false, do not name the maps.
 */
void build_maps_from_gdal_dataset_3D(GDALDataset *dst, Array *t, Array *t_map, Array *x_map, Array *y_map,
                                     bool name_maps /*default false */) {
    // get the geo-transform data
    vector<double> gt(6);
    dst->GetGeoTransform(gt.data());

    // Get the GDALDataset size
    GDALRasterBand *band = dst->GetRasterBand(1);

    // Build Time map
    int t_size = t->length(); // was t->size(); jhrg 12/2/25

    if (name_maps) {
        t_map->append_dim(t_size, "Time");
    }
    else {
        t_map->append_dim(t_size);
    }

    vector<dods_float32> t_buf(t_size);
    t->value(t_buf.data());

    t_map->set_value(t_buf.data(), t_size);

    // Build Lon map
    unsigned long x = band->GetXSize(); // x_map_vals

    if (name_maps) {
        x_map->append_dim(x, "Longitude");
    }
    else {
        x_map->append_dim(x);
    }

    // for each value, use the geo-transform data to compute a value and store it.
    vector<dods_float32> x_map_vals(x);
    dods_float32 *cur_x = x_map_vals.data();
    dods_float32 *prev_x = cur_x;
    // x_map_vals[0] = gt[0];
    *cur_x++ = gt[0];
    for (unsigned long i = 1; i < x; ++i) {
        // x_map_vals[i] = gt[0] + i * gt[1];
        // x_map_vals[i] = x_map_vals[i-1] + gt[1];
        *cur_x++ = *prev_x++ + gt[1];
    }

    x_map->set_value(x_map_vals.data(), x); // copies values to new storage

    // Build the Lat map
    unsigned long y = band->GetYSize();

    if (name_maps) {
        y_map->append_dim(y, "Latitude");
    }
    else {
        y_map->append_dim(y);
    }

    // for each value, use the geo-transform data to compute a value and store it.
    vector<dods_float32> y_map_vals(y);
    dods_float32 *cur_y = y_map_vals.data();
    dods_float32 *prev_y = cur_y;
    // y_map_vals[0] = gt[3];
    *cur_y++ = gt[3];
    for (unsigned long i = 1; i < y; ++i) {
        // y_map_vals[i] = gt[3] + i * gt[5];
        // y_map_vals[i] = y_map_vals[i-1] + gt[5];
        *cur_y++ = *prev_y++ + gt[5];
    }

    y_map->set_value(y_map_vals.data(), y);
}

/**
 * @brief Get the Array's 'no data' value
 *
 * Heuristic search for the missing value flag used by these data.
 *
 * @param src The Array to get the 'no data' attribute from/
 * @return The value of the 'no data' attribute or NaN
 */
double get_missing_data_value(const Array *src) {
    Array *a = const_cast<Array *>(src);

    // Read this from the 'missing_value' or '_FillValue' attributes
    string mv_attr = a->get_attr_table().get_attr("missing_value");
    if (mv_attr.empty()) mv_attr = a->get_attr_table().get_attr("_FillValue");

    double missing_data = numeric_limits<double>::quiet_NaN();
    if (!mv_attr.empty()) {
        char *endptr;
        missing_data = strtod(mv_attr.c_str(), &endptr);
        if (missing_data == 0.0 && endptr == mv_attr.c_str())
            missing_data = numeric_limits<double>::quiet_NaN();
    }

    return missing_data;
}

/**
 * @brief Get the libdap Array::Dim_iter for the Array's X dimension
 *
 * @param src The Array
 * @return The libdap Array::Dim_iter
 */
Array::Dim_iter get_x_dim(const libdap::Array *src) {
    Array *a = const_cast<Array *>(src);
    int numDims = a->dimensions();
    if (numDims < 2) {
        stringstream ss;
        ss << "Ouch! Retrieving the 'x' dimension for the array ";
        a->print_decl(ss, "", false, true, true);
        ss << " FAILED Because it has less than 2 dimensions" << endl;
        BESDEBUG(DEBUG_KEY, ss.str());
        throw BESError(ss.str(),BES_SYNTAX_USER_ERROR,__FILE__,__LINE__);
    }
    Array::Dim_iter start = a->dim_begin();
    Array::Dim_iter xDim = start + numDims - 1;

    return xDim;
}

/**
 * @brief Get the libdap Array::Dim_iter for the Array's Y dimension
 *
 * @param src The Array
 * @return The libdap Array::Dim_iter
 */
Array::Dim_iter get_y_dim(const libdap::Array *src) {
    Array *a = const_cast<Array *>(src);
    int numDims = a->dimensions();
    if (numDims < 2) {
        stringstream ss;
        ss << "Ouch! Retrieving the 'y' dimension for the array ";
        a->print_decl(ss, "", false, true, true);
        ss << " FAILED Because it has less than 2 dimensions" << endl;
        BESDEBUG(DEBUG_KEY, ss.str());
        throw BESError(ss.str(),BES_SYNTAX_USER_ERROR,__FILE__,__LINE__);
    }
    Array::Dim_iter start = a->dim_begin();
    Array::Dim_iter yDim = start + numDims - 2;
    return yDim;
}


/**
 * @brief Is this a 2D Array?
 *
 * Look at this Array and determine if it is 2D or 'effectively' 2D.
 * The latter is true when the left-most dimensions all have a size
 * of one (1).
 *
 * @param src The Array
 * @return True if the Array is effectively/actually 2D, false otherwise.
 */
bool array_is_effectively_2D(const libdap::Array *src) {
    Array *a = const_cast<Array *>(src);
    int numDims = a->dimensions();
    if (numDims == 2) return true;
    if (numDims < 2) return false;
    // numDims more than 2. Last dim should be x
    Array::Dim_iter xDim = get_x_dim(a);
    for (Array::Dim_iter thisDim = a->dim_begin(); thisDim < xDim; thisDim++) {
        unsigned long size = a->dimension_size(thisDim, true);
        if (size > 1) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Read data from an Array and load it into a GDAL RasterBand
 *
 * This function reads data from the array into an already-allocated band.
 * The add_band_data() function is probably more efficient since it builds
 * a band that uses the data from the DAP Array (and thus without making a
 * copy).
 *
 * @param src Read data from this Array
 * @param band The RasterBand; modified so that it holds a copy of data from 'src'
 */
void read_band_data(const Array *src, GDALRasterBand *band) {
    Array *a = const_cast<Array *>(src);

    if (!array_is_effectively_2D(src)) {
        stringstream ss;
        ss << "Cannot perform geo-spatial operations on an Array (";
        ss << a->name() << ") with " << a->dimensions() << " dimensions.";
        ss << "Because the constrained shape of the array: ";
        a->print_decl(ss, "", false, true, true);
        ss << " is not a two-dimensional array." << endl;
        BESDEBUG(DEBUG_KEY, ss.str());
        throw BESError(ss.str(), BES_SYNTAX_USER_ERROR, __FILE__, __LINE__);
    }

    unsigned long x = a->dimension_size(get_x_dim(a), true);
    unsigned long y = a->dimension_size(get_y_dim(a), true);

    a->read(); // Should this code use intern_data()? jhrg 10/11/16

    // We may be able to use AddBand() to skip the I/O operation here
    // For now, we use read() to load the data values and get_buf() to
    // access a pointer to them.
    CPLErr error = band->RasterIO(GF_Write, 0, 0, x, y, a->get_buf(), x, y, get_array_type(a), 0, 0);

    if (error != CPLE_None) {
        string msg = "Could not load data for grid '" + a->name() + "' msg: '" + CPLGetLastErrorMsg() + "'";
        BESDEBUG(DEBUG_KEY, "ERROR read_band_data(): " << msg << endl);
        throw BESError(msg,BES_SYNTAX_USER_ERROR,__FILE__,__LINE__);
    }
}

/**
 * @brief Share the Array's internal buffer with GDAL
 *
 * This can avoid allocating temporary memory.
 *
 * @note This does not seem to work.
 *
 * @param src The Array
 * @param ds The GDALDataset; modified so that it has a new band
 */
void add_band_data(const Array *src, GDALDataset *ds) {
    Array *a = const_cast<Array *>(src);

     a->read();

    // The MEMory driver supports the DATAPOINTER option.
    char **options = NULL;
    ostringstream oss;
    oss << reinterpret_cast<unsigned long>(a->get_buf());
    options = CSLSetNameValue(options, "DATAPOINTER", oss.str().c_str());

    CPLErr error = ds->AddBand(get_array_type(a), options);

    CSLDestroy(options);

    if (error != CPLE_None) {
        string msg = "Could not add data for grid '" + a->name() + "': " + CPLGetLastErrorMsg();
        BESDEBUG(DEBUG_KEY, "ERROR add_band_data(): " << msg << endl);
        throw BESError(msg,BES_INTERNAL_ERROR,__FILE__,__LINE__);
    }
}

#define ADD_BAND 0

/**
 * @brief Build a GDAL Dataset object for this data/lon/lat combination
 *
 * @note Supported values for the srs parameter
 * "WGS84": same as "EPSG:4326" but has no dependence on EPSG data files.
 * "WGS72": same as "EPSG:4322" but has no dependence on EPSG data files.
 * "NAD27": same as "EPSG:4267" but has no dependence on EPSG data files.
 * "NAD83": same as "EPSG:4269" but has no dependence on EPSG data files.
 * "EPSG:n": same as doing an ImportFromEPSG(n).
 *
 * @param data
 * @param lon
 * @param lat
 * @param srs The SRS/CRS of the data array; defaults to WGS84 which
 * uses lat, lon axis order.
 * @return An auto_ptr<GDALDataset>
 */
unique_ptr<GDALDataset> build_src_dataset(Array *data, Array *x, Array *y, const string &srs) {
    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("MEM");
    if (!driver) {
        string msg = string("Could not get the Memory driver for GDAL: ") + CPLGetLastErrorMsg();
        BESDEBUG(DEBUG_KEY, "ERROR build_src_dataset(): " << msg << endl);
        throw BESError(msg,BES_INTERNAL_ERROR,__FILE__,__LINE__);
    }

    SizeBox array_size = get_size_box(x, y);

    // The MEM driver takes no creation options jhrg 10/6/16
    unique_ptr<GDALDataset> ds(driver->Create("result", array_size.x_size, array_size.y_size,
                                              1 /* nBands*/, get_array_type(data), NULL /* driver_options */));

#if ADD_BAND
    add_band_data(data, ds.get());
#endif

    // Get the one band for this dataset and load it with data
    GDALRasterBand *band = ds->GetRasterBand(1);
    if (!band) {
        string msg = "Could not get the GDAL RasterBand for Array '" + data->name() + "': " + CPLGetLastErrorMsg();
        BESDEBUG(DEBUG_KEY, "ERROR build_src_dataset():  " << msg << endl);
        throw BESError(msg,BES_INTERNAL_ERROR,__FILE__,__LINE__);
    }

    // Set the no data value here; I'm not sure what the affect of using NaN will be... jhrg 10/11/16
    double no_data = get_missing_data_value(data);
    band->SetNoDataValue(no_data);

#if !ADD_BAND
    read_band_data(data, band);
#endif

    vector<double> geo_transform = get_geotransform_data(x, y);
    ds->SetGeoTransform(geo_transform.data());

    OGRSpatialReference native_srs;
    if (CE_None != native_srs.SetWellKnownGeogCS(srs.c_str())) {
        string msg = "Could not set '" + srs + "' as the dataset native CRS.";
        BESDEBUG(DEBUG_KEY, "ERROR build_src_dataset(): " << msg << endl);
        throw BESError(msg,BES_SYNTAX_USER_ERROR,__FILE__,__LINE__);
    }
    // I'm not sure what to do about the Projected Coordinate system. jhrg 10/6/16
    // native_srs.SetUTM( 11, TRUE );

    // Connect the SRS/CRS to the GDAL Dataset
    char *pszSRS_WKT = NULL;
    native_srs.exportToWkt(&pszSRS_WKT);
    ds->SetProjection(pszSRS_WKT);
    CPLFree(pszSRS_WKT);

    return ds;
}

/**
 * @brief Scale a GDAL dataset
 *
 * @param src The source GDALDataset
 * @param size The destination size
 * @param interp The interpolation algorithm to use (default: nearest neighbor,
 * other options are bilinear, cubic, cubicspline, lanczos, average, mode)
 * @param crs The CRS to use for the result (default is to use the CRS of 'src')
 * @return An auto_ptr to the result (a new GDALDataset instance)
 */
unique_ptr<GDALDataset> scale_dataset(unique_ptr<GDALDataset> &src, const SizeBox &size, const string &crs /*""*/,
                                      const string &interp /*nearest*/) {
    char **argv = NULL;
    argv = CSLAddString(argv, "-of"); // output format
    argv = CSLAddString(argv, "MEM");

    argv = CSLAddString(argv, "-outsize"); // output size
    ostringstream oss;
    oss << size.x_size;
    argv = CSLAddString(argv, oss.str().c_str()); // size x
    oss.str("");
    oss << size.y_size;
    argv = CSLAddString(argv, oss.str().c_str()); // size y

    argv = CSLAddString(argv, "-b"); // band number
    argv = CSLAddString(argv, "1");

    argv = CSLAddString(argv, "-r"); // resampling
    argv = CSLAddString(argv, interp.c_str()); // {nearest(default),bilinear,cubic,cubicspline,lanczos,average,mode}

    if (!crs.empty()) {
        argv = CSLAddString(argv, "-a_srs"); // dst SRS (WKT or "EPSG:n")
        argv = CSLAddString(argv, crs.c_str());
    }

    if (BESISDEBUG(DEBUG_KEY)) {
        char **local = argv;
        while (*local) {
            BESDEBUG(DEBUG_KEY, "argv: " << *local++ << endl);
        }
    }

    GDALTranslateOptions *options = GDALTranslateOptionsNew(argv, NULL /*binary options*/);

    int usage_error = CE_None; // result
    GDALDatasetH dst_handle = GDALTranslate("warped_dst", src.get(), options, &usage_error);
    if (!dst_handle || usage_error != CE_None) {
        GDALClose(dst_handle);
        GDALTranslateOptionsFree(options);
        string msg = string("Error calling GDAL translate: ") + CPLGetLastErrorMsg();
        BESDEBUG(DEBUG_KEY, "ERROR scale_dataset(): " << msg << endl);
        throw BESError(msg, BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }

    unique_ptr<GDALDataset> dst(static_cast<GDALDataset *>(dst_handle));

    GDALTranslateOptionsFree(options);

    return dst;
}


unique_ptr<GDALDataset> scale_dataset_3D(unique_ptr<GDALDataset> &src, const SizeBox &size, const string &crs /*""*/,
                                         const string &interp /*nearest*/) {
    char **argv = NULL;
    argv = CSLAddString(argv, "-of"); // output format
    argv = CSLAddString(argv, "MEM");

    argv = CSLAddString(argv, "-outsize"); // output size
    ostringstream oss;
    oss << size.x_size;
    argv = CSLAddString(argv, oss.str().c_str()); // size x
    oss.str("");
    oss << size.y_size;
    argv = CSLAddString(argv, oss.str().c_str()); // size y

    // all bands in src
    int n_bands = src.get()->GetRasterCount();
    for (int i = 0; i < n_bands; i++) {
        oss.str("");
        oss << i + 1;
        argv = CSLAddString(argv, "-b");
        argv = CSLAddString(argv, oss.str().c_str());
    }

    argv = CSLAddString(argv, "-r"); // resampling
    argv = CSLAddString(argv, interp.c_str()); // {nearest(default),bilinear,cubic,cubicspline,lanczos,average,mode}

    if (!crs.empty()) {
        argv = CSLAddString(argv, "-a_srs"); // dst SRS (WKT or "EPSG:n")
        argv = CSLAddString(argv, crs.c_str());
    }

    if (BESISDEBUG(DEBUG_KEY)) {
        char **local = argv;
        while (*local) {
            BESDEBUG(DEBUG_KEY, "argv: " << *local++ << endl);
        }
    }

    GDALTranslateOptions *options = GDALTranslateOptionsNew(argv, NULL /*binary options*/);
    int usage_error = CE_None; // result
    GDALDatasetH dst_handle = GDALTranslate("warped_dst", src.get(), options, &usage_error);
    if (!dst_handle || usage_error != CE_None) {
        GDALClose(dst_handle);
        GDALTranslateOptionsFree(options);
        string msg = string("Error calling GDAL translate: ") + CPLGetLastErrorMsg();
        BESDEBUG(DEBUG_KEY, "ERROR scale_dataset(): " << msg << endl);
        throw BESError(msg, BES_INTERNAL_ERROR, __FILE__, __LINE__);
    }

    unique_ptr<GDALDataset> dst(static_cast<GDALDataset *>(dst_handle));

    GDALTranslateOptionsFree(options);

    return dst;
}

/**
 * @brief Scale a Grid; this version takes the data, lon and lat Arrays as separate arguments
 *
 * @param data
 * @param x
 * @param y
 * @param size
 * @param crs
 * @param interp
 * @return The scaled Grid where the first map holds the longitude data and second
 * holds the latitude data.
 */
Grid *scale_dap_array(const Array *data, const Array *x, const Array *y, const SizeBox &size,
                      const string &crs, const string &interp) {
    // Build GDALDataset for Grid g with lon and lat maps as given
    auto *d = const_cast<Array *>(data);

    unique_ptr<GDALDataset> src = build_src_dataset(d, const_cast<Array *>(x), const_cast<Array *>(y));

    // scale to the new size, using optional CRS and interpolation params
    unique_ptr<GDALDataset> dst = scale_dataset(src, size, crs, interp);

    // Build a result Grid: extract the data, build the maps and assemble
    unique_ptr<Array> built_data(build_array_from_gdal_dataset(dst.get(), d));

    //unique_ptr<Array> built_lat(new Array(y->name(), new Float32(y->name())));
    auto built_lat = make_unique<Array>(y->name(), new Float32(y->name()));
    //unique_ptr<Array> built_lon(new Array(x->name(), new Float32(x->name())));
    auto built_lon = make_unique<Array>(x->name(), new Float32(x->name()));

    build_maps_from_gdal_dataset(dst.get(), built_lon.get(), built_lat.get());

    //unique_ptr<Grid> result(new Grid(d->name()));
    auto result = make_unique<Grid>(d->name());
    result->set_array(built_data.release());
    result->add_map(built_lat.release(), false);
    result->add_map(built_lon.release(), false);

    return result.release();
}

/**
 * @brief Scale a DAP2 Grid given the Grid, dest lat/lon subset, dest size and CRS.
 *
 * @param g The Grid that holds the source data
 * @param size The size in pixels for the result
 * @param crs Transform the result to the given CRS; default is the src CRS
 * @param interp Use this interpolation scheme; default is NearestNeighboor. This
 * uses the GDAL constants
 * @return The new Grid variable
 */
Grid *scale_dap_grid(const Grid *g, const SizeBox &size, const string &crs, const string &interp) {
    string func(__func__);
    func += "() - ";

    if (!g) {
        throw BESError(func + "The Grid object is null.",BES_INTERNAL_ERROR,__FILE__,__LINE__);
    }
    // Build GDALDataset for Grid g with lon and lat maps as given
    Array *data = dynamic_cast<Array *>(const_cast<Grid *>(g)->array_var());
    if (!data) {
        throw BESError(func + "Unable to obtain data array from Grid '" + g->name() + "'",BES_INTERNAL_ERROR,__FILE__,
                       __LINE__);
    }
    // return iteration
    Grid::Map_riter ritr = const_cast<Grid *>(g)->map_rbegin();
    Array *x = dynamic_cast<Array *>(*ritr);
    Array *y = dynamic_cast<Array *>(*++ritr);

    if (!x || !y) {
        throw BESError(func + "Unable to obtain 2 Map arrays from Grid '" + g->name() + "'",BES_INTERNAL_ERROR,__FILE__,
                       __LINE__);
    }

    return scale_dap_array(data, x, y, size, crs, interp);
}

#define ADD_BAND 0

/**
 * @brief Build a GDAL Dataset object for this data/time/lon/lat combination
 *
 * @note Supported values for the srs parameter
 * "WGS84": same as "EPSG:4326" but has no dependence on EPSG data files.
 * "WGS72": same as "EPSG:4322" but has no dependence on EPSG data files.
 * "NAD27": same as "EPSG:4267" but has no dependence on EPSG data files.
 * "NAD83": same as "EPSG:4269" but has no dependence on EPSG data files.
 * "EPSG:n": same as doing an ImportFromEPSG(n).
 *
 * @param data
 * @param t
 * @param x
 * @param y
 * @param srs The SRS/CRS of the data array; defaults to WGS84 which
 * uses lat, lon axis order.
 * @return An auto_ptr<GDALDataset> with number of bands equal to size of time array
 */
unique_ptr<GDALDataset> build_src_dataset_3D(Array *data, Array *t, Array *x, Array *y, const string &srs) {
    const Array *d = data;

    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("MEM");
    if (!driver) {
        string msg = string("Could not get the Memory driver for GDAL: ") + CPLGetLastErrorMsg();
        BESDEBUG(DEBUG_KEY, "ERROR build_src_dataset(): " << msg << endl);
        throw BESError(msg,BES_INTERNAL_ERROR,__FILE__,__LINE__);
    }

    // Below Array::size() was used but that method was removed and replaced by length().
    // jhrg 12/2/25
    SizeBox array_size = get_size_box(x, y);
    int nBands = t->length();
    BESDEBUG(DEBUG_KEY, "nBands = " << nBands << endl);
    int nBytes = data->prototype()->width();
    const int data_size = x->length() * y->length();
    unsigned int dsize = data_size * nBytes;

    unique_ptr<GDALDataset> ds(driver->Create("result", array_size.x_size, array_size.y_size, nBands, get_array_type(d),
                                              NULL /* driver_options */));
    data->read();
    // start band loop
    for (int i = 1; i <= nBands; i++) {
        GDALRasterBand *band = ds->GetRasterBand(i);
        if (!band) {
            string msg = "Could not get the GDAL RasterBand for Array '" + data->name() + "': " + CPLGetLastErrorMsg();
            BESDEBUG(DEBUG_KEY, "ERROR build_src_dataset():  " << msg << endl);
            throw BESError(msg,BES_INTERNAL_ERROR,__FILE__,__LINE__);
        }

        double no_data = get_missing_data_value(data);
        band->SetNoDataValue(no_data);

#if !ADD_BAND
        CPLErr error = band->RasterIO(GF_Write, 0, 0, x->length(), y->length(), data->get_buf() + dsize * (i - 1),
                                      x->length(), y->length(), get_array_type(data), 0, 0);
        if (error != CPLE_None)
            throw Error("Could not write data for band: " + long_to_string(i) + ": " + string(CPLGetLastErrorMsg()));
#endif
    } // end band loop
    vector<double> geo_transform = get_geotransform_data(x, y);
    ds->SetGeoTransform(geo_transform.data());

    OGRSpatialReference native_srs;
    if (CE_None != native_srs.SetWellKnownGeogCS(srs.c_str())) {
        string msg = "Could not set '" + srs + "' as the dataset native CRS.";
        BESDEBUG(DEBUG_KEY, "ERROR build_src_dataset(): " << msg << endl);
        throw BESError(msg,BES_SYNTAX_USER_ERROR,__FILE__,__LINE__);
    }

    // Connect the SRS/CRS to the GDAL Dataset
    char *pszSRS_WKT = NULL;
    native_srs.exportToWkt(&pszSRS_WKT);
    ds->SetProjection(pszSRS_WKT);
    CPLFree(pszSRS_WKT);

    return ds;
}

/**
 * @brief Scale a Grid; this version takes the data, lon and lat Arrays as separate arguments
 *
 * @param data
 * @param time
 * @param lon
 * @param lat
 * @param size
 * @param crs
 * @param interp
 * @return The scaled Grid where the first map holds time, the second holds longitude data and third
 * holds the latitude data.
 */
Grid *scale_dap_array_3D(const Array *data, const Array *time, const Array *lon, const Array *lat, const SizeBox &size,
                         const string &crs, const string &interp) {
    // Build GDALDataset for Grid g with lon and lat maps as given
    Array *d = const_cast<Array *>(data);
    Array *t = const_cast<Array *>(time);
    Array *x = const_cast<Array *>(lon);
    Array *y = const_cast<Array *>(lat);

    // get GDALDataset with bands
    unique_ptr<GDALDataset> src = build_src_dataset_3D(d, const_cast<Array *>(t), const_cast<Array *>(x),
                                                       const_cast<Array *>(y));

    // scale all bands to the new size, using optional CRS and interpolation params
    unique_ptr<GDALDataset> dst = scale_dataset_3D(src, size, crs, interp);

    // Build a result Grid: extract the data, build the maps and assemble
    unique_ptr<Array> built_data(build_array_from_gdal_dataset_3D(dst.get(), d));
    //unique_ptr<Array> built_time(new Array(t->name(), new Float32(t->name())));
    auto built_time = make_unique<Array>(t->name(), new Float32(t->name()));
    //unique_ptr<Array> built_lat(new Array(y->name(), new Float32(y->name())));
    auto built_lat = make_unique<Array>(y->name(), new Float32(y->name()));
    //unique_ptr<Array> built_lon(new Array(x->name(), new Float32(x->name())));
    auto built_lon = make_unique<Array>(x->name(), new Float32(x->name()));


    // Build maps for grid
    build_maps_from_gdal_dataset_3D(dst.get(), t, built_time.get(), built_lon.get(), built_lat.get());

    // get result Grid
    unique_ptr<Grid> result(new Grid(d->name()));
    result->set_array(built_data.release());
    result->add_map(built_time.release(), false);
    result->add_map(built_lat.release(), false);
    result->add_map(built_lon.release(), false);

    return result.release();
}
}
