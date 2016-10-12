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
#include <ogr_spatialref.h>
#include <gdalwarper.h>

#include <Str.h>
#include <Array.h>
#include <Grid.h>

#include <util.h>
#include <Error.h>
#include <debug.h>

#include "scale_util.h"

using namespace std;

namespace libdap {

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
 * @param src
 * @param size
 * @param band The RasterBand is modified
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

#if 1
/**
 * @brief Share the Array's internal buffer with GDAL
 *
 * This will not work; we need a way to access the internal buffer.
 * As a test, to see if this code will work (there is some doubt about
 * the DATAPOINTER option working on 64 bit machines), hack a version
 * that allocates a buffer here and uses AddBand with that. This test
 * leaks memory.
 *
 * @param src
 * @param ds
 */
void add_band_data(const Array *src, const SizeBox &size, GDALDataset* ds)
{
    Array *a = const_cast<Array*>(src);

    assert(a->dimensions() == 2);

    a->read();

    // The MEMory driver supports the DATAPOINTER option.
    char **options = NULL;
    ostringstream oss;
    oss << a->get_buf();
    options = CSLSetNameValue(options, "DATAPOINTER", oss.str().c_str());

    CPLErr error = ds->AddBand(get_array_type(a), options);

    CSLDestroy( options );

    if (error != CPLE_None)
        throw Error("Could not add data for grid '" + a->name() + "': " + CPLGetLastErrorMsg());
}
#endif

/**
 * @brief Get the Array's 'no data' value
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
    //GDALDataType gdal_type = get_array_type(data);

    // The MEM driver takes no creation options jhrg 10/6/16
    auto_ptr<GDALDataset> ds(driver->Create("result", array_size.x_size, array_size.y_size,
    		1 /* nBands*/, get_array_type(data), NULL /* driver_options */));

    // Get the one band for this dataset and load it with data
	GDALRasterBand *band = ds->GetRasterBand(1);
	if (!band)
		throw Error("Could not get the GDAL RasterBand for Array '" + data->name() + "': " + CPLGetLastErrorMsg());

	// Set the no data value here; I'm not sure what the affect of using NaN will be... jhrg 10/11/16
	double no_data = get_missing_data_value(data);
	band->SetNoDataValue(no_data);

	read_band_data(data, band);

	vector<double> geo_transform = get_geotransform_data(lat, lon);
    ds->SetGeoTransform(&geo_transform[0]);

    OGRSpatialReference native_srs;
    if (CE_None != native_srs.SetWellKnownGeogCS(srs.c_str()))
    	throw Error("Could not set '" + srs + "' as the dataset native CRS.");

    // TODO I'm not sure what to do about the Projected Coordinate system. jhrg 10/6/16
    // native_srs.SetUTM( 11, TRUE );

    // Connect the SRS/CRS to the GDAL Dataset
    char *pszSRS_WKT = NULL;
    native_srs.exportToWkt( &pszSRS_WKT );
    ds->SetProjection( pszSRS_WKT );
    CPLFree( pszSRS_WKT );

    return ds;
}

/**
 * @brief Build a GDAL Dataset for the given size and type
 * @param a
 * @param size
 * @return
 */
auto_ptr<GDALDataset> build_dst_dataset(SizeBox &size, GDALDataType gdal_type, const string &srs)
{
    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("MEM");
    if(!driver)
        throw Error(string("Could not get the Memory driver for GDAL: ") + CPLGetLastErrorMsg());

    // The MEM driver takes no creation options (I think) jhrg 10/6/16
    auto_ptr<GDALDataset> ds(driver->Create("result", size.x_size, size.y_size,
            1 /* nBands*/, gdal_type, NULL /* driver_options */));

    OGRSpatialReference native_srs;
    if (CE_None != native_srs.SetWellKnownGeogCS(srs.c_str()))
        throw Error("Could not set '" + srs + "' as the dataset native CRS.");

    // TODO I'm not sure what to do about the Projected Coordinate system. jhrg 10/6/16
    // native_srs.SetUTM( 11, TRUE );

    // Connect the SRS/CRS to the GDAL Dataset
    char *pszSRS_WKT = NULL;
    native_srs.exportToWkt( &pszSRS_WKT );
    ds->SetProjection( pszSRS_WKT );
    CPLFree( pszSRS_WKT );

    return ds;
}

/**
 * @brief Warp the source to dest using their SRS and sizes
 *
 * @todo Read the warp API to learn about memory options for optimization
 * and interpolation options
 *
 * @param hSrcDS
 * @param hDstDS
 */
void warp_raster(GDALDataset *src_ds, GDALDataset *dst_ds)
{
    // Setup warp options.
    GDALWarpOptions* psWarpOptions = GDALCreateWarpOptions();
    psWarpOptions->hSrcDS = src_ds;
    psWarpOptions->hDstDS = dst_ds;
    psWarpOptions->nBandCount = 1;
    psWarpOptions->panSrcBands = (int*) (CPLMalloc(sizeof(int) * psWarpOptions->nBandCount));
    psWarpOptions->panSrcBands[0] = 1;
    psWarpOptions->panDstBands = (int*) (CPLMalloc(sizeof(int) * psWarpOptions->nBandCount));
    psWarpOptions->panDstBands[0] = 1;

    // Establish reprojection transformer.
    const char *src_wkt = GDALGetProjectionRef(src_ds);
    char *dst_wkt = NULL;

    OGRSpatialReference oSRS;
    //oSRS.SetUTM( 11, TRUE );
    oSRS.SetWellKnownGeogCS( "WGS84" );
    oSRS.exportToWkt( &dst_wkt );

    psWarpOptions->pTransformerArg = GDALCreateGenImgProjTransformer(src_ds, src_wkt, NULL, dst_wkt,
            false /* Use GCP OK*/, 0.0 /*allowed error threshold*/, 1 /*GCP polynomial order*/);

    if (psWarpOptions->pTransformerArg == NULL)
        throw Error(string("Could not build GDAL Transformer Argument: ") + CPLGetLastErrorMsg());

    vector<double> adfDstGeoTransform(6);
    int nPixels=0, nLines=0;
    CPLErr eErr;
    eErr = GDALSuggestedWarpOutput( src_ds,
                                    GDALGenImgProjTransform, psWarpOptions->pTransformerArg,
                                    &adfDstGeoTransform[0], &nPixels, &nLines );
    if (eErr != CE_None)
        throw Error(string("Could not get suggest warp output: ") + CPLGetLastErrorMsg());

    GDALDestroyGenImgProjTransformer( psWarpOptions->pTransformerArg );

    cerr << "nPixels: " << nPixels << endl;
    cerr << "nLines: " << nLines << endl;
    cerr << "adfDstGeoTransform: ";
    copy(adfDstGeoTransform.begin(), adfDstGeoTransform.end(), ostream_iterator<double>(cerr, " "));
    cerr << endl;

#if 0
    psWarpOptions->pfnTransformer = GDALGenImgProjTransform;

    // Initialize and execute the warp operation.
    GDALWarpOperation oOperation;
    oOperation.Initialize(psWarpOptions);

    // TODO There's a method for multi-threads and also a MT warp options option. jhrg 10/6/16
    oOperation.ChunkAndWarpImage(0, 0, GDALGetRasterXSize(dst_ds), GDALGetRasterYSize(dst_ds));

    GDALDestroyGenImgProjTransformer(psWarpOptions->pTransformerArg);
    GDALDestroyWarpOptions(psWarpOptions);
#endif
}


/**
 * @brief Scale a DAP2 Grid given the Grid, dest lat/lon subset, dest size and CRS.
 *
 * @param src The Grid that holds the source data
 * @param bbox The lat/lon box corner points
 * @param size The size in pixels for the result
 * @param crs Transform the result to the given CRS; default is the src CRS
 * @param interp Use this interpolation scheme; default is NearestNeighboor. This
 * uses the GDAL constants
 * @return The new Grid variable
 */
Grid *scale_dap_grid(Grid *src, SizeBox &size, const string &dest_crs, const int interp)
{
    // Pull out lat, lon and array

#if 0
	// Build GDALDataset for src
	auto_ptr<GDALDataset> src_ds = build_src_dataset(src);

	// Build GDALDaatset for result - Assume it's a double for now (jhrg 10/6/16)
    GDALDataType gdal_type = get_array_type(src);
    auto_ptr<GDALDataset> dst_ds = build_dst_dataset(size, gdal_type);

	// Call the GDAL warp code
    warp_raster(src_ds.get(), dst_ds.get());

	// Build the result Grid using the result GDALDataset    GDALRasterBand *poBand = satDataSet->GetRasterBand(1);
	//auto_ptr<Grid> dest(new Grid("result"));
#endif
#if 0
     // Supported values:
    // "WGS84": same as "EPSG:4326" but has no dependence on EPSG data files.
    // "WGS72": same as "EPSG:4322" but has no dependence on EPSG data files.
    // "NAD27": same as "EPSG:4267" but has no dependence on EPSG data files.
    // "NAD83": same as "EPSG:4269" but has no dependence on EPSG data files.
    // "EPSG:n": same as doing an ImportFromEPSG(n).
    OGRSpatialReference native_crs;
    if (CE_None != native_crs.SetWellKnownGeogCS("WGS84"))
    	throw Error("Could not set the dataset native CRS.");

    vector<double> geo_transform = get_geotransform_data(lat, lon, g_size);
    ds->SetGeoTransform(&geo_transform[0]);

   // Read this from the 'missing_value' or '_FillValue' attributes
     string mv_attr = g->get_attr_table().get_attr("missing_value");
     if (mv_attr.empty())
     	mv_attr = g->get_attr_table().get_attr("_FillValue");

     float missing_value = 0.0;
     if (!mv_attr.empty())
     	missing_value = atof(mv_attr.c_str());

 	// Hack code from CPLErr DAP_Dataset::SetGDALDataset(const int isSimple). jhrg 10/5/16
     poBand->SetNoDataValue(md_MissingValue);

    DBG(cerr << "SetGDALDataset() - Reading data" << endl);
    m_src->read();
    double *data = extract_double_array(m_src);
    DBG(cerr << "SetGDALDataset() - Data read." << endl);


    DBG(cerr << "SetGDALDataset() - Calling RasterIO(GF_Write,0,0,"<<
            mi_RectifiedImageXSize<<","<< mi_RectifiedImageYSize<< ",data,"<<
            mi_SrcImageXSize<< ","<< mi_SrcImageYSize << ","<< eBandType << ",0,0)" << endl);


    if (CE_None != poBand->RasterIO(GF_Write, 0, 0, mi_RectifiedImageXSize, mi_RectifiedImageYSize, data,
                    mi_SrcImageXSize, mi_SrcImageYSize, eBandType, 0, 0)) {
        GDALClose((GDALDatasetH) satDataSet);
        throw Error("Failed to set satellite data band to '"+driverName+"' DataSet (" + string(CPLGetLastErrorMsg()) + ").");
    }
    delete[] data;
    DBG(cerr << "SetGDALDataset() -  RasterIO call completed" << endl);


    //set GCPs for this VRTDataset
    if (CE_None != SetGCPGeoRef4VRTDataset(satDataSet)) {
        GDALClose((GDALDatasetH) satDataSet);
        throw Error("Could not georeference the virtual dataset (" + string(CPLGetLastErrorMsg()) + ").");
    }

    DBG(cerr << "SetGDALDataset() - satDataSet: " << satDataSet << endl);

    maptr_DS.reset(satDataSet);


    CPLErr err_code = CE_None;
    if (!isSimple)
        err_code = RectifyGOESDataSet();

    DBG(cerr << "SetGDALDataset() - END" << endl);

    return err_code;
#endif
}

}
 // namespace libdap
