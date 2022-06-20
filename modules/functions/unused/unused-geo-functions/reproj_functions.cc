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

#include <climits>

#include <iostream>
#include <vector>

#define DODS_DEBUG

#include <gdal.h>
#include <gdal_priv.h>
#include <ogr_spatialref.h>
#include <gdalwarper.h>

#include <libdap/Str.h>
#include <libdap/Array.h>
#include <libdap/Grid.h>

#include <libdap/util.h>
#include <libdap/Error.h>
#include <libdap/debug.h>

#include "reproj_functions.h"
#include "DAP_Dataset.h"

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
vector<double> get_geotransform_data(Array *lat, Array *lon, const SizeBox &size)
{
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

	// TODO Should these by using size - 1 in the denominator? jhrg 10/7/16
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

GDALDataType get_grid_type(const Grid *g)
{
	switch (const_cast<Grid*>(g)->get_array()->type()) {
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
				+ const_cast<Grid*>(g)->get_array()->type_name() + " data.");
	}
}

/**
 * @brief Read data from a Grid and load it into a GDAL raster band
 * @param g
 * @param g_size
 * @param band
 */
void read_band_data(const Grid* g, const SizeBox &g_size, GDALRasterBand* band)
{
	// FIXME Use the Grid Array native type
	vector<double> values(g_size.y_size * g_size.x_size); // FIXME Assumes grid is 2D
	Array *a = const_cast<Grid*>(g)->get_array();
	a->read();
	extract_double_array(a, values);

	// TODO Look at using WriteBlock() because it might be faster. jhrg 10/6/16
	CPLErr error = band->RasterIO(GF_Write, 0, 0, g_size.x_size, g_size.y_size,
			values.data(), g_size.x_size, g_size.y_size, GDT_Float64, 0, 0);
	if (error != CPLE_None)
		throw Error("Could not load data for grid '" + g->name() + "': " + CPLGetLastErrorMsg());
}

/**
 * @brief build GDALDataset from a DAP2 Grid
 *
 * @note The GDAL Dataset object is made with a single band
 * Geo data are set
 * @param g Get data and attributes from this Grid
 * @return The GDAL Dataset
 */
unique_ptr<GDALDataset> build_src_dataset(Grid *g)
{
	// FIXME Must determine which axes are lat and lon. jhrg 10/6/16
    Array *lat = static_cast<Array*>(*g->get_map_iter(0));
    Array *lon = static_cast<Array*>(*g->get_map_iter(1));

    SizeBox g_size = get_size_box(lat, lon);
    GDALDataType gdal_type = get_grid_type(g);

    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("MEM");
    if(!driver)
        throw Error(string("Could not get the Memory driver for GDAL: ") + CPLGetLastErrorMsg());

    // The MEM driver takes no creation options (I think) jhrg 10/6/16
    unique_ptr<GDALDataset> ds(driver->Create("result", g_size.x_size, g_size.y_size,
    		1 /* nBands*/, gdal_type, NULL /* driver_options */));

    // Get the one band for this dataset and load it with data
	GDALRasterBand *band = ds->GetRasterBand(1);
	if (!band)
		throw Error("Could not get the GDAL Rasterband for grid '" + g->name() + "': " + CPLGetLastErrorMsg());

	read_band_data(g, g_size, band);

	vector<double> geo_transform = get_geotransform_data(lat, lon, g_size);
    ds->SetGeoTransform(geo_transform.data());

    // Supported values:
    // "WGS84": same as "EPSG:4326" but has no dependence on EPSG data files.
    // "WGS72": same as "EPSG:4322" but has no dependence on EPSG data files.
    // "NAD27": same as "EPSG:4267" but has no dependence on EPSG data files.
    // "NAD83": same as "EPSG:4269" but has no dependence on EPSG data files.
    // "EPSG:n": same as doing an ImportFromEPSG(n).
    OGRSpatialReference native_srs;
    if (CE_None != native_srs.SetWellKnownGeogCS("WGS84"))
    	throw Error("Could not set the dataset native CRS.");

    // FIXME I'm not sure what to do about the Projected Coordinate system. jhrg 10/6/16
    // native_srs.SetUTM( 11, TRUE );

    // TODO Is this needed? Does it conflict with SetGeoTransform above. jhrg 10/6/16
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
	psWarpOptions->pTransformerArg = GDALCreateGenImgProjTransformer(src_ds,
			GDALGetProjectionRef(src_ds), dst_ds, GDALGetProjectionRef(dst_ds),
			FALSE, 0.0, 1);
	psWarpOptions->pfnTransformer = GDALGenImgProjTransform;

	// Initialize and execute the warp operation.
	GDALWarpOperation oOperation;
	oOperation.Initialize(psWarpOptions);

	// TODO There's a method for multi-threads and also a MT warp options option. jhrg 10/6/16
	oOperation.ChunkAndWarpImage(0, 0, GDALGetRasterXSize(dst_ds), GDALGetRasterYSize(dst_ds));

	GDALDestroyGenImgProjTransformer(psWarpOptions->pTransformerArg);
	GDALDestroyWarpOptions(psWarpOptions);
}

unique_ptr<GDALDataset> build_dst_dataset(const Grid *g)
{
    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("MEM");
    if(!driver)
        throw Error(string("Could not get the Memory driver for GDAL: ") + CPLGetLastErrorMsg());

    GDALDataType gdal_type = get_grid_type(g);

    // The MEM driver takes no creation options (I think) jhrg 10/6/16
    unique_ptr<GDALDataset> ds(driver->Create("result", 10, 10,
            1 /* nBands*/, gdal_type, NULL /* driver_options */));

    return ds;
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
Grid *scale_dap_grid(Grid *src, GeoBox &bbox, SizeBox &size, const string &crs, const int interp)
{
	// Build GDALDataset for src
	unique_ptr<GDALDataset> src_ds = build_src_dataset(src);

	// Build GDALDaatset for result - Assume it's a double for now (jhrg 10/6/16)
    unique_ptr<GDALDataset> dst_ds = build_dst_dataset(src);

	// Call the GDAL warp code
    warp_raster(src_ds.get(), dst_ds.get());

	// Build the result Grid using the result GDALDataset    GDALRasterBand *poBand = satDataSet->GetRasterBand(1);
	//unique_ptr<Grid> dest(new Grid("result"));

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
    ds->SetGeoTransform(geo_transform.data());

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

/**
 * @todo The lat and lon arrays are passed in, but there's an assumption that the
 * source data array and the two lat and lon arrays are the same shape. But the
 * code does not actually test that.
 *
 * @todo Enable multiple bands paired with just the two lat/lon arrays? Not sure
 * if that is a good idea...
 */
void function_swath2array(int argc, BaseType * argv[], DDS &, BaseType **btpp)
{
#if 1
    DBG(cerr << "function_swath2array() - BEGIN" << endl);

    // Use the same documentation for both swath2array and swath2grid
    string info =
        string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
            + "<function name=\"swath2array\" version=\"1.0\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid\">\n"
            + "</function>\n";

    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(info);
        *btpp = response;
        DBG(cerr << "function_swath2array() - END (no args)" << endl);
        return;
    }

    // TODO Add optional fourth arg that lets the caller say which datum to use;
    // default to WGS84
    if (argc != 3)
        throw Error(
            "The function swath2array() requires three arguments. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    Array *src = dynamic_cast<Array*>(argv[0]);
    if (!src)
        throw Error(
            "The first argument to swath2array() must be a data array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    Array *lat = dynamic_cast<Array*>(argv[1]);
    if (!lat)
        throw Error(
            "The second argument to swath2array() must be a latitude array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    Array *lon = dynamic_cast<Array*>(argv[2]);
    if (!lon)
        throw Error(
            "The third argument to swath2array() must be a longitude array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    // The args passed into the function using argv[] are deleted after the call.

    DAP_Dataset ds(src, lat, lon);

    try {
        ds.InitializeDataset(0);

        *btpp = ds.GetDAPArray();
    }
    catch (Error &e) {
        DBG(cerr << "function_swath2array() - Encountered libdap::Error   msg: '" << e.get_error_message() << "'" << endl);
        throw e;
    }
    catch (...) {
        DBG(cerr << "function_swath2array() - Encountered unknown exception." << endl);
        throw;
    }

    DBG(cerr << "function_swath2array() - END" << endl);

    return;
#endif
}

/**
 * @todo The lat and lon arrays are passed in, but there's an assumption that the
 * source data array and the two lat and lon arrays are the same shape. But the
 * code does not actually test that.
 *
 * @todo Enable multiple bands paired with just the two lat/lon arrays? Not sure
 * if that is a good idea...
 */
void function_swath2grid(int argc, BaseType * argv[], DDS &, BaseType **btpp)
{
#if 1
	DBG(cerr << "function_swath2grid() - BEGIN" << endl);

    string info =
        string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
            + "<function name=\"swath2grid\" version=\"1.0\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid\">\n"
            + "</function>\n";

    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(info);
        *btpp = response;
        return;
    }

    // TODO Add optional fourth arg that lets the caller say which datum to use;
    // default to WGS84
    if (argc != 3)
        throw Error(
            "The function swath2grid() requires three arguments. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    Array *src = dynamic_cast<Array*>(argv[0]);
    if (!src)
        throw Error(
            "The first argument to swath2grid() must be a data array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    Array *lat = dynamic_cast<Array*>(argv[1]);
    if (!lat)
        throw Error(
            "The second argument to swath2grid() must be a latitude array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    Array *lon = dynamic_cast<Array*>(argv[2]);
    if (!lon)
        throw Error(
            "The third argument to swath2grid() must be a longitude array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");

    // The args passed into the function using argv[] are deleted after the call.

    DAP_Dataset ds(src, lat, lon);

    try {
        DBG(cerr << "function_swath2grid() - Calling DAP_Dataset::InitialDataset()" << endl);
        ds.InitializeDataset(0);

        DBG(cerr << "function_swath2grid() - Calling DAP_Dataset::GetDAPGrid()" << endl);
        *btpp = ds.GetDAPGrid();

    }
    catch (libdap::Error &e) {
        DBG(cerr << "function_swath2grid() - Caught libdap::Error  msg:" << e.get_error_message() << endl);
        throw e;
    }
    catch (...) {
        DBG(cerr << "function_swath2grid() - Caught unknown exception." << endl);
        throw;
    }

    DBG(cerr << "function_swath2grid() - END" << endl);

    return;
#endif
}

#if 0
/**
 * @todo The lat and lon arrays are passed in, but there's an assumption that the
 * source data array and the two lat and lon arrays are the same shape. But the
 * code does not actually test that.
 *
 * @todo Enable multiple bands paired with just the two lat/lon arrays? Not sure
 * if that is a good idea...
 */
void function_changeCRS(int argc, BaseType * argv[], DDS &, BaseType **btpp)
{
    DBG(cerr << "function_changeCRS() - BEGIN" << endl);

    string functionName = "crs";

    string info = string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
    + "<function name=\""+functionName+"\" version=\"1.0\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#crs\">\n"
    + "</function>\n";

    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(info);
        *btpp = response;
        return;
    }

    if (argc != 5)
    throw Error("The function "+functionName+"() requires five arguments. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#"+functionName);

    Array *src = dynamic_cast<Array*>(argv[0]);
    if (!src)
    throw Error("The first argument to "+functionName+"() must be a data array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#"+functionName);

    Array *lat = dynamic_cast<Array*>(argv[1]);
    if (!lat)
    throw Error("The second argument to "+functionName+"() must be a latitude array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#"+functionName);

    Array *lon = dynamic_cast<Array*>(argv[2]);
    if (!lon)
    throw Error("The third argument to "+functionName+"() must be a longitude array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#"+functionName);

    Str *nativeCrsName = dynamic_cast<Str*>(argv[3]);
    if (!src)
    throw Error("The fourth argument to "+functionName+"() must be a string identifying the native CRS of the data array. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#"+functionName);

    Str *targetCrsName = dynamic_cast<Str*>(argv[4]);
    if (!src)
    throw Error("The fifth argument to "+functionName+"() must be a string identifying the target CRS. See http://docs.opendap.org/index.php/Server_Side_Processing_Functions#"+functionName);

    // The args passed into the function using argv[] are deleted after the call.

    DAP_Dataset ds(src, lat, lon);

    try {
        ds.InitializeDataset(0);

        *btpp = ds.GetDAPGrid();
    }
    catch(Error &e) {
        DBG(cerr << "caught Error: " << e.get_error_message() << endl);
        throw e;
    }
    catch(...) {
        DBG(cerr << "caught unknown exception" << endl);
        throw;
    }

    DBG(cerr << "function_changeCRS() - END" << endl);

    return;
}
#endif

}
 // namespace libdap
