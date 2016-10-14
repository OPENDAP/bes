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
#include <limits>
#include <sstream>
#include <cassert>

#include <gdal.h>
#include <gdal_priv.h>
#include <gdal_utils.h>
#include <ogr_spatialref.h>
#include <gdalwarper.h>

#include <Str.h>
#include <Float32.h>
#include <Array.h>
#include <Grid.h>

#include <util.h>
#include <Error.h>
#include <debug.h>

#include "ScaleGrid.h"

using namespace std;
using namespace libdap;

namespace functions {

#if 0
// This code is used to build GCPs and various other info when a 'coverage'
// has two-dimensional lat and lon maps. I'm not using it now, but maybe in
// the future we will be... jhrg 10/5/16
void SetGeoBBoxAndGCPs(int nXSize, int nYSize)
{
    DBG(cerr << "SetGeoBBoxAndGCPs() - BEGIN" << endl);

    // reuse the Dim_iter for both lat and lon arrays
    Array::Dim_iter i = m_lat->dim_begin();
    int nLatXSize = m_lat->dimension_size(i, true);
    int nLatYSize = m_lat->dimension_size(i + 1, true);
    i = m_lon->dim_begin();
    int nLonXSize = m_lon->dimension_size(i, true);
    int nLonYSize = m_lon->dimension_size(i + 1, true);

    DBG(cerr << "SetGeoBBoxAndGCPs() - nXSize: "<< nXSize << "  nLatXSize: " << nLatXSize << endl );
    DBG(cerr << "SetGeoBBoxAndGCPs() - nYSize: "<< nYSize << "  nLatYSize: " << nLatYSize << endl );

    if (nXSize != nLatXSize || nLatXSize != nLonXSize || nYSize != nLatYSize || nLatYSize != nLonYSize)
        throw Error("SetGeoBBoxAndGCPs() - The size of latitude/longitude and the data field does not match.");

    m_lat->read();
    m_lon->read();
    double *dataLat = extract_double_array(m_lat);
    double *dataLon = extract_double_array(m_lon);

    DBG(cerr << "SetGeoBBoxAndGCPs() - Past lat/lon data read" << endl);

    try {
        mdSrcGeoMinX = 360;
        mdSrcGeoMaxX = -360;
        mdSrcGeoMinY = 90;
        mdSrcGeoMaxY = -90;

        // Sample every other row and column; This reduces the number of
        // Grid Control points (GCPs) which speeds the regridding process
        // Could we go smaller with no loss in quality? jhrg 10/5/16
        int xSpace = 2;
        int ySpace = 2;

        int nGCPs = 0;
        GDAL_GCP gdalCGP;

        for (int iLine = 0; iLine < nYSize - ySpace; iLine += ySpace) {
            for (int iPixel = 0; iPixel < nXSize - xSpace; iPixel += xSpace) {
                double x = *(dataLon + (iLine * nYSize) + iPixel);
                double y = *(dataLat + (iLine * nYSize) + iPixel);

                if (isValidLongitude(x) && isValidLatitude(y)) {
                    char pChr[64];
                    snprintf(pChr, 64, "%d", ++nGCPs);
                    GDALInitGCPs(1, &gdalCGP);
                    gdalCGP.pszId = strdup(pChr);
                    gdalCGP.pszInfo = strdup("");
                    gdalCGP.dfGCPLine = iLine;
                    gdalCGP.dfGCPPixel = iPixel;
                    gdalCGP.dfGCPX = x;
                    gdalCGP.dfGCPY = y;

                    DBG2(cerr << "iLine, iPixel: " << iLine << ", " << iPixel << " --> x,y: " << x << ", " << y << endl);

                    gdalCGP.dfGCPZ = 0;
                    m_gdalGCPs.push_back(gdalCGP);

                    mdSrcGeoMinX = MIN(mdSrcGeoMinX, gdalCGP.dfGCPX);
                    mdSrcGeoMaxX = MAX(mdSrcGeoMaxX, gdalCGP.dfGCPX);
                    mdSrcGeoMinY = MIN(mdSrcGeoMinY, gdalCGP.dfGCPY);
                    mdSrcGeoMaxY = MAX(mdSrcGeoMaxY, gdalCGP.dfGCPY);
                }
            }
        }
    }
    catch (...) {
        delete[] dataLat;
        delete[] dataLon;
        throw;
    }

    delete[] dataLat;
    delete[] dataLon;

    DBG(cerr << "SetGeoBBoxAndGCPs() - END" << endl);

    // For the swath2grid code, I used this as well, in the caller.
    // This provides info for the dest GDALDataset, where the swath
    // (a referenceable grid) is now a rectified grid. For the case
    // I'm coding now, the input is a rectified grid. jhrg 10/5/16
    double resX, resY;
    if (mdSrcGeoMinX > mdSrcGeoMaxX && mdSrcGeoMinX > 0 && mdSrcGeoMaxX < 0)
        resX = (360 + mdSrcGeoMaxX - mdSrcGeoMinX) / (src_x_size - 1);
    else
        resX = (mdSrcGeoMaxX - mdSrcGeoMinX) / (src_x_size - 1);

    resY = (mdSrcGeoMaxY - mdSrcGeoMinY) / (nYSize - 1);

    double res = MIN(resX, resY);

    if (mdSrcGeoMinX > mdSrcGeoMaxX && mdSrcGeoMinX > 0 && mdSrcGeoMaxX < 0)
        mi_RectifiedImageXSize = (int) ((360 + mdSrcGeoMaxX - mdSrcGeoMinX) / res) + 1;
    else
        mi_RectifiedImageXSize = (int) ((mdSrcGeoMaxX - mdSrcGeoMinX) / res) + 1;

    mi_RectifiedImageYSize = (int) fabs((mdSrcGeoMaxY - mdSrcGeoMinY) / res) + 1;

    DBG(cerr << "SetGeoTransform() - Source image size: " << src_x_size << ", " << nYSize << endl);
    DBG(cerr << "SetGeoTransform() - Rectified image size: " << mi_RectifiedImageXSize << ", " << mi_RectifiedImageYSize << endl);

    md_Geotransform[0] = mdSrcGeoMinX;
    md_Geotransform[1] = res;
    md_Geotransform[2] = 0;
    md_Geotransform[3] = mdSrcGeoMaxY;
    md_Geotransform[4] = 0;
    md_Geotransform[5] = -res;
}
#endif

inline static int is_valid_lat(const double lat)
{
    return (lat >= -90 && lat <= 90);
}

inline static int is_valid_lon(const double lon)
{
    return (lon >= -180 && lon <= 180);
}

SizeBox get_size_box(Array *lat, Array *lon)
{
	// Latitude is Y
    int src_y_size = lat->dimension_size(lat->dim_begin(), true);
    // Longitude is X
    int src_x_size = lon->dimension_size(lon->dim_begin(), true);

    return SizeBox(src_x_size, src_y_size);
}

/**
 * @brief Extract the geo-transform coordinates from a DP2 Grid
 *
 * @note Side effect: Data are read into the lat and lon Arrays
 *
 * @param lat
 * @param lon
 * @return A vector<double> of length 6 with the GDAL geo-transform parameters
 */
vector<double> get_geotransform_data(Array *lat, Array *lon)
{
    SizeBox size = get_size_box(lat, lon);

    lat->read();
	vector<double> lat_values(size.y_size);
	extract_double_array(lat, lat_values);

	lon->read();
	vector<double> lon_values(size.x_size);
	extract_double_array(lon, lon_values);

	double min_x = 360;
	double max_x = -360;
	double min_y = 90;
	double max_y = -90;

	for (int x = 0; x < size.x_size; ++x) {
		double lon = lon_values[x];
		if (is_valid_lon(lon)) {
			min_x = min(min_x, lon);
			max_x = max(max_x, lon);
		}
	}

	for (int y = 0; y < size.y_size; ++y) {
		double lat = lat_values[y];
		if (is_valid_lat(y)) {
			min_y = min(min_y, lat);
			max_y = max(max_y, lat);
		}
	}

	// Use size-1 in the denominator because an axis that spans N values
	// (on a closed interval) will have N+1 values (e.g., 3,2,1,0,1,2,3)
    double res_x, res_y;
    if (min_x > max_x && min_x > 0 && max_x < 0)
        res_x = (360 + max_x - min_x) / (size.x_size - 1);
    else
        res_x = (max_x - min_x) / (size.x_size - 1);

    res_y = (max_y - min_y) / (size.y_size - 1);

    // Removed jhrg 10/7/16 double res = min(res_x, res_y);

    // Xgeo = GT(0) + Xpixel*GT(1) + Yline*GT(2)
    // Ygeo = GT(3) + Xpixel*GT(4) + Yline*GT(5)

    // In case of north up images, the GT(2) and GT(4) coefficients are zero,
    // and the GT(1) is pixel width, and GT(5) is pixel height. The (GT(0),GT(3))
    // position is the top left corner of the top left pixel of the raster.
    //
    // Note that for an inverted dataset, use min_y and res_y for GT(3) and GT(5)

    vector<double> geo_transform(6);
    geo_transform[0] = min_x;
    geo_transform[1] = res_x;
    geo_transform[2] = 0;
    geo_transform[3] = max_y;   // Assume max lat is at the top
    geo_transform[4] = 0;
    geo_transform[5] = -res_y;  // moving down (lower lats) corresponds to ++index

    return geo_transform;
}

GDALDataType get_array_type(const Array *a)
{
	switch (const_cast<Array*>(a)->var()->type()) {
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
	default:
		throw Error("Cannot perform geo-spatial operations on "
				+ const_cast<Array*>(a)->var()->type_name() + " data.");
	}
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
void read_band_data(const Array *src, GDALRasterBand* band)
{
    Array *a = const_cast<Array*>(src);
    if (a->dimensions() != 2)
        throw Error("Cannot perform geo-spatial operations on an Array ("
            + a->name() + ") with " + long_to_string(a->dimensions()) + " dimensions.");

    // Assume row x col (i.e., [y][x]) order of dimensions
    unsigned long y = a->dimension_size(a->dim_begin(), true);
    unsigned long x =  a->dimension_size(a->dim_begin() + 1, true);

	a->read();  // Should this code use intern_data()? jhrg 10/11/16

    // We may be able to use AddBand() to skip the I/O operation here
	// For now, we use read() to load the data values and get_buf() to
	// access a pointer to them.
	CPLErr error = band->RasterIO(GF_Write, 0, 0, x, y, a->get_buf(),
	    x, y, get_array_type(a), 0, 0);

	if (error != CPLE_None)
		throw Error("Could not load data for grid '" + a->name() + "': " + CPLGetLastErrorMsg());
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
void add_band_data(const Array *src, GDALDataset* ds)
{
    Array *a = const_cast<Array*>(src);

    assert(a->dimensions() == 2);

    a->read();

    // The MEMory driver supports the DATAPOINTER option.
    char **options = NULL;
    ostringstream oss;
    oss << reinterpret_cast<unsigned long>(a->get_buf());
    options = CSLSetNameValue(options, "DATAPOINTER", oss.str().c_str());

    CPLErr error = ds->AddBand(get_array_type(a), options);

    CSLDestroy(options);

    if (error != CPLE_None)
        throw Error("Could not add data for grid '" + a->name() + "': " + CPLGetLastErrorMsg());
}

/**
 * @brief Used to transfer data values from a gdal dataset to a dap Array
 * @param band Read from this raster and
 * @param y Rows
 * @param x Cols
 * @param a Set the values in this array
 * @return Return a pointer to parameter 'a'
 */
template <typename T>
static Array *transfer_values_helper(GDALRasterBand *band, const unsigned long y, const unsigned long x, Array *a)
{
    // get the data
    vector<T> buf(y * x);
    CPLErr error = band->RasterIO(GF_Read, 0, 0, x, y, &buf[0], x, y, get_array_type(a), 0, 0);

    if (error != CPLE_None)
        throw Error(string("Could not extract data for array.") + CPLGetLastErrorMsg());

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
Array *build_array_from_gdal_dataset(auto_ptr<GDALDataset> source, const Array *dest)
{
    // Get the GDALDataset size
    GDALRasterBand *band = source->GetRasterBand(1);
    unsigned long y = band->GetYSize();
    unsigned long x = band->GetXSize();

    // Build a new DAP Array; use the dest Array's element type
    Array *result = new Array("result", const_cast<Array*>(dest)->var()->ptr_duplicate());
    result->append_dim(y);
    result->append_dim(x);

    // get the data
    switch (result->var()->type()) {
    case dods_byte_c:
        return transfer_values_helper<dods_byte>(source->GetRasterBand(1), y, x, result);
        break;
    case dods_uint16_c:
        return transfer_values_helper<dods_uint16>(source->GetRasterBand(1), y, x, result);
        break;
    case dods_int16_c:
        return transfer_values_helper<dods_int16>(source->GetRasterBand(1), y, x, result);
        break;
    case dods_uint32_c:
        return transfer_values_helper<dods_uint32>(source->GetRasterBand(1), y, x, result);
        break;
    case dods_int32_c:
        return transfer_values_helper<dods_int32>(source->GetRasterBand(1), y, x, result);
        break;
    case dods_float32_c:
        return transfer_values_helper<dods_float32>(source->GetRasterBand(1), y, x, result);
        break;
    case dods_float64_c:
        return transfer_values_helper<dods_float64>(source->GetRasterBand(1), y, x, result);
        break;
    case dods_uint8_c:
        return transfer_values_helper<dods_byte>(source->GetRasterBand(1), y, x, result);
        break;
    case dods_int8_c:
        return transfer_values_helper<dods_int8>(source->GetRasterBand(1), y, x, result);
        break;
    case dods_uint64_c:
    case dods_int64_c:
    default:
        throw InternalErr(__FILE__, __LINE__,
                "The source array to a geo-function contained an unsupported numeric type.");
    }
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
 * @param lon_map value-result parameter for the longitude map (uses dods_float32
 * elements)
 * @param lat_map value-result parameter for the latitude map
 */
void build_maps_from_gdal_dataset(GDALDataset *dst, Array *lon_map, Array *lat_map)
{
    // get the geo-transform data
    vector<double> gt(6);
    dst->GetGeoTransform(&gt[0]);

    // Get the GDALDataset size
    GDALRasterBand *band = dst->GetRasterBand(1);

    // Build Lon map
    unsigned long x = band->GetXSize(); // lon

    lon_map->append_dim(x, "Longitude");

    // for each value, use the geo-transform data to compute a value and store it.
    vector<dods_float32> lon(x);
    dods_float32 *cur_lon = &lon[0];
    dods_float32 *prev_lon = cur_lon;
    // lon[0] = gt[0];
    *cur_lon++ = gt[0];
    for (unsigned long i = 1; i < x; ++i) {
        // lon[i] = gt[0] + i * gt[1];
        // lon[i] = lon[i-1] + gt[1];
        *cur_lon++ = *prev_lon++ + gt[1];
    }

    lon_map->set_value(&lon[0], x); // copies values to new storage

    // Build the Lat map
    unsigned long y = band->GetYSize();

    lat_map->append_dim(y, "Latitude");

    // for each value, use the geo-transform data to compute a value and store it.
    vector<dods_float32> lat(y);
    dods_float32 *cur_lat = &lat[0];
    dods_float32 *prev_lat = cur_lat;
    // lat[0] = gt[3];
    *cur_lat++ = gt[3];
    for (unsigned long i = 1; i < y; ++i) {
        // lat[i] = gt[3] + i * gt[5];
        // lat[i] = lat[i-1] + gt[5];
        *cur_lat++ = *prev_lat++ + gt[5];
    }

    lat_map->set_value(&lat[0], y);
}

/**
 * @brief Get the Array's 'no data' value
 *
 * Heuristic search for the missing value flag used by these data.
 *
 * @param src The Array to get the 'no data' attribute from/
 * @return The value of the 'no data' attribute or NaN
 */
double get_missing_data_value(const Array *src)
{
    Array *a = const_cast<Array*>(src);

    // Read this from the 'missing_value' or '_FillValue' attributes
    string mv_attr = a->get_attr_table().get_attr("missing_value");
    if (mv_attr.empty()) mv_attr = a->get_attr_table().get_attr("_FillValue");

    double missing_data = numeric_limits<double>::quiet_NaN();
    if (!mv_attr.empty()) {
        char *endptr;
        missing_data = strtod(mv_attr.c_str(), &endptr);
#ifndef NDEBUG
        if (missing_data == 0.0 && endptr == mv_attr.c_str())
            cerr << "Error converting no data attribute to double: " << strerror(errno) << endl;
#endif
    }

    return missing_data;
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
 * @param srs The SRS/CRS of the data array; defaults to WGS84
 * @return An auto_ptr<GDALDataset>
 */
auto_ptr<GDALDataset> build_src_dataset(Array *data, Array *lon, Array *lat, const string &srs)
{
    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("MEM");
    if(!driver)
        throw Error(string("Could not get the Memory driver for GDAL: ") + CPLGetLastErrorMsg());

    SizeBox array_size = get_size_box(lat, lon);

    // The MEM driver takes no creation options jhrg 10/6/16
    auto_ptr<GDALDataset> ds(driver->Create("result", array_size.x_size, array_size.y_size,
    		1 /* nBands*/, get_array_type(data), NULL /* driver_options */));

#if ADD_BAND
    add_band_data(data, ds.get());
#endif

    // Get the one band for this dataset and load it with data
	GDALRasterBand *band = ds->GetRasterBand(1);
	if (!band)
		throw Error("Could not get the GDAL RasterBand for Array '" + data->name() + "': " + CPLGetLastErrorMsg());

	// Set the no data value here; I'm not sure what the affect of using NaN will be... jhrg 10/11/16
	double no_data = get_missing_data_value(data);
	band->SetNoDataValue(no_data);

#if !ADD_BAND
	read_band_data(data, band);
#endif

	vector<double> geo_transform = get_geotransform_data(lat, lon);
    ds->SetGeoTransform(&geo_transform[0]);

    OGRSpatialReference native_srs;
    if (CE_None != native_srs.SetWellKnownGeogCS(srs.c_str()))
    	throw Error("Could not set '" + srs + "' as the dataset native CRS.");

    // I'm not sure what to do about the Projected Coordinate system. jhrg 10/6/16
    // native_srs.SetUTM( 11, TRUE );

    // Connect the SRS/CRS to the GDAL Dataset
    char *pszSRS_WKT = NULL;
    native_srs.exportToWkt( &pszSRS_WKT );
    ds->SetProjection( pszSRS_WKT );
    CPLFree( pszSRS_WKT );

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
auto_ptr<GDALDataset> scale_dataset(auto_ptr<GDALDataset> src, const SizeBox &size,
    const string &interp /*nearest*/, const string &crs /*""*/)
{
    char **argv = NULL;
    argv = CSLAddString(argv, "-of");       // output format
    argv = CSLAddString(argv, "MEM");

    argv = CSLAddString(argv, "-outsize");  // output size
    ostringstream oss;
    oss << size.x_size;
    argv = CSLAddString(argv, oss.str().c_str());    // size x
    oss.str(""); oss << size.y_size;
    argv = CSLAddString(argv, oss.str().c_str());    // size y

    argv = CSLAddString(argv, "-b");    // band number
    argv = CSLAddString(argv, "1");

    argv = CSLAddString(argv, "-r");    // resampling
    argv = CSLAddString(argv, interp.c_str());  // {nearest(default),bilinear,cubic,cubicspline,lanczos,average,mode}

    if (!crs.empty()) {
        argv = CSLAddString(argv, "-a_srs");   // dst SRS (WKT or "EPSG:n")
        argv = CSLAddString(argv, crs.c_str());
    }

    GDALTranslateOptions *options = GDALTranslateOptionsNew(argv, NULL /*binary options*/);

    int usage_error = CE_None;   // result
    GDALDatasetH dst_handle = GDALTranslate("warped_dst", src.get(), options, &usage_error);
    if (!dst_handle || usage_error != CE_None) {
        GDALClose(dst_handle);
        GDALTranslateOptionsFree(options);
        throw Error(string("Error calling GDAL translate: ") + CPLGetLastErrorMsg());
    }

    auto_ptr<GDALDataset> dst(static_cast<GDALDataset*>(dst_handle));

    GDALTranslateOptionsFree(options);

    return dst;
}

/**
 * @brief Scale a Grid; this version takes the data, lon and lat Arrays as separate arguments
 *
 * @param data
 * @param lon
 * @param lat
 * @param size
 * @param crs
 * @param interp
 * @return The scaled Grid where the first map holds the longitude data and second
 * holds the latitude data.
 */
Grid *scale_dap_array(const Array *data, const Array *lon, const Array *lat, const SizeBox &size,
    const string &crs, const string &interp)
{
    // Build GDALDataset for Grid g with lon and lat maps as given
    Array *d = const_cast<Array*>(data);

    auto_ptr<GDALDataset> src = build_src_dataset(d, const_cast<Array*>(lon), const_cast<Array*>(lat));

    // scale to the new size, using optional CRS and interpolation params
    auto_ptr<GDALDataset> dst = scale_dataset(src, size, crs, interp);

    // Build a result Grid: extract the data, build the maps and assemble
    auto_ptr<Array> built_data(build_array_from_gdal_dataset(dst, d));

    auto_ptr<Array> built_lon(new Array(lon->name(), new Float32(lon->name())));
    auto_ptr<Array> built_lat(new Array(lat->name(), new Float32(lat->name())));

    build_maps_from_gdal_dataset(dst.get(), built_lon.get(), built_lat.get());

    auto_ptr<Grid> result(new Grid(d->name()));
    result->set_array(built_data.release());
    result->add_map(built_lon.release(), false);
    result->add_map(built_lat.release(), false);

    return result.release();
}

/**
 * @brief Scale a DAP2 Grid given the Grid, dest lat/lon subset, dest size and CRS.
 *
 * @param src The Grid that holds the source data
 * @param size The size in pixels for the result
 * @param crs Transform the result to the given CRS; default is the src CRS
 * @param interp Use this interpolation scheme; default is NearestNeighboor. This
 * uses the GDAL constants
 * @return The new Grid variable
 */
Grid *scale_dap_grid(const Grid *g, const SizeBox &size, const string &crs, const string &interp)
{
    // Build GDALDataset for Grid g with lon and lat maps as given
    Array *data = static_cast<Array*>(const_cast<Grid*>(g)->array_var());
    Array *lon = static_cast<Array*>(*const_cast<Grid*>(g)->map_begin());
    Array *lat = static_cast<Array*>(*(const_cast<Grid*>(g)->map_begin() + 1));

    return scale_dap_array(data, lon, lat, size, crs, interp);
#if 0
    auto_ptr<GDALDataset> src = build_src_dataset(data, const_cast<Array*>(lon), const_cast<Array*>(lat));

    // scale to the new size, using optional CRS and interpolation params
    auto_ptr<GDALDataset> dst = scale_dataset(src, size, crs, interp);

    // Build a result Grid: extract the data, build the maps and assemble
    auto_ptr<Array> built_data(build_array_from_gdal_dataset(dst, data));

    auto_ptr<Array> built_lon(new Array("built_lon", new Float32("built_lon")));
    auto_ptr<Array> built_lat(new Array("built_lat", new Float32("built_lat")));

    build_maps_from_gdal_dataset(dst.get(), built_lon.get(), built_lat.get());

    auto_ptr<Grid> result(new Grid("scaled_" + g->name()));
    result->set_array(built_data.release());
    result->add_map(built_lon.release(), false);
    result->add_map(built_lat.release(), false);

    return result.release();
#endif
}

}
 // namespace libdap
