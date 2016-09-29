
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include <cstdlib>

#define DODS_DEBUG

#include "DAP_Dataset.h"

#include "Array.h"
#include "Grid.h"
#include "Float64.h"

#include "Error.h"
#if 0
#include "ConstraintEvaluator.h"
#include "Sequence.h"
#include "Structure.h"
#include "ServerFunction.h"
#endif

#include "util.h"
#include "debug.h"

using namespace libdap;
using namespace std;

#if 0
#define GOES_TIME_DEBUG FALSE
#endif

namespace libdap {

std::string datasetInfo( GDALDataset  *poDataset){

    stringstream ss;

    double        adfGeoTransform[6];

    ss << "########## GDALDataset ##########"<< endl;

    ss << "Description: " << poDataset->GetDescription() << endl;

    ss << "Driver: '"<<  poDataset->GetDriver()->GetDescription() << "' "<<  poDataset->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME )<< endl;

    ss << "Size is: [" << poDataset->GetRasterXSize() << "][" << poDataset->GetRasterYSize() << "][" << poDataset->GetRasterCount() << "]"<< endl;

    if( poDataset->GetProjectionRef()  != NULL )
        ss << "Projection Ref: '" << poDataset->GetProjectionRef() << "'" << endl;

    ss << "GCPProjection: '" << poDataset->GetGCPProjection() << "'" << endl;

    if( poDataset->GetGeoTransform( adfGeoTransform ) == CE_None )
    {
        ss << "GeoTransform[6]: " << endl;
        ss << "    Origin(0,3)    = (" << adfGeoTransform[0]<< "," <<  adfGeoTransform[3] << ")" << endl;
        ss << "    PixelSize(1,5) = (" << adfGeoTransform[1] << "," <<  adfGeoTransform[5] << ")" << endl;
        ss << "    Mysto(2,4)     = (" << adfGeoTransform[2] << "," <<  adfGeoTransform[4] << ")" << endl;


    }

    return ss.str();
}

DAP_Dataset::DAP_Dataset()
{
}

/************************************************************************/
/*                           ~DAP_Dataset()                             */
/************************************************************************/

/**
 * \brief Destroy an open DAP_Dataset object.
 *
 * This is the accepted method of closing a DAP_Dataset dataset and
 * deallocating all resources associated with it.
 */

DAP_Dataset::~DAP_Dataset()
{
}

/************************************************************************/
/*                           DAP_Dataset()                              */
/************************************************************************/

/**
 * \brief Create an DAP_Dataset object.
 *
 * This is the accepted method of creating a DAP_Dataset object and
 * allocating all resources associated with it.
 *
 * @param id The coverage identifier.
 *
 * @param rBandList The field list selected for this coverage. For TRMM
 * daily data, the user could specify multiple days range in request.
 * Each day is seemed as one field.
 *
 * @return A DAP_Dataset object.
 */

DAP_Dataset::DAP_Dataset(const string& id, vector<int> &rBandList) :
        AbstractDataset(id, rBandList)
{
    md_MissingValue = 0;
    mb_GeoTransformSet = FALSE;
}

/**
 * @brief Initialize a DAP Dataset using Array objects already read.
 *
 *
 */

DAP_Dataset::DAP_Dataset(Array *src, Array *lat, Array *lon) :
        AbstractDataset(), m_src(src), m_lat(lat), m_lon(lon)
{
#if 1
    // TODO Remove these?
    DBG(cerr << "Registering GDAL drivers" << endl);
    GDALAllRegister();
    OGRRegisterAll();
#endif

    CPLSetErrorHandler(CPLQuietErrorHandler);

    // Read this from the 'missing_value' or '_FillValue' attributes
    string missing_value = m_src->get_attr_table().get_attr("missing_value");
    if (missing_value.empty())
        missing_value = m_src->get_attr_table().get_attr("_FillValue");

    if (!missing_value.empty())
        md_MissingValue = atof(missing_value.c_str());
    else
        md_MissingValue = 0;

    mb_GeoTransformSet = FALSE;
}

/************************************************************************/
/*                           InitialDataset()                           */
/************************************************************************/

/**
 * \brief Initialize the GOES dataset with NetCDF format.

 * This method is the implementation for initializing a GOES dataset with NetCDF format.
 * Within this method, SetNativeCRS(), SetGeoTransform() and SetGDALDataset()
 * will be called to initialize an GOES dataset.
 *
 * @note To use this, call this method and then access the GDALDataset that
 * contains the reprojected array using maptr_DS.get().
 *
 * @param isSimple the WCS request type.  When user executing a DescribeCoverage
 * request, isSimple is set to 1, and for GetCoverage, is set to 0.
 *
 * @return CE_None on success or CE_Failure on failure.
 */

CPLErr DAP_Dataset::InitializeDataset(const int isSimple)
{
    DBG(cerr << "InitialDataset() - BEGIN" << endl);

    // Might break that operation out so the remap is a separate call
    if (CE_None != SetNativeCRS() || CE_None != SetGeoTransform())
        throw Error("DAP_Dataset::InitialDataset() - Could not set the dataset native CRS or the GeoTransform.");

    DBG(cerr << "InitialDataset() - Calling SetGDALDataset()" << endl);

    if (CE_None != SetGDALDataset(isSimple)) {
        GDALClose(maptr_DS.release());
        throw Error("DAP_Dataset::InitialDataset() - Could not re-project the dataset.");
    }
    DBG(cerr << "InitialDataset() - END" << endl);

    return CE_None;
}

/************************************************************************/
/*                            GetDAPArray()                            */
/************************************************************************/

/**
 * @brief Build a DAP Array from the GDALDataset
 */
Array *DAP_Dataset::GetDAPArray()
{
    DBG(cerr << "GetDAPArray() - BEGIN" << endl);
    DBG(cerr << "GetDAPArray() - maptr_DS: " << maptr_DS.get() << endl);
    DBG(cerr << "GetDAPArray() - raster band count: " << maptr_DS->GetRasterCount() << endl);

    // There should be just one band
    if (maptr_DS->GetRasterCount() != 1)
        throw Error("In function swath2grid(), expected a single raster band.");

    // Get the x and y dimensions of the raster band
    int x = maptr_DS->GetRasterXSize();
    int y = maptr_DS->GetRasterYSize();
    GDALRasterBand *rb = maptr_DS->GetRasterBand(1);
    if (!rb)
        throw Error("DAP_Dataset::GetDAPArray() - In function swath2grid(), could not access the raster data.");

    // Since the DAP_Dataset code works with all data values as doubles,
    // Assume the raster band has GDAL type GDT_Float64, but test anyway
    if (GDT_Float64 != rb->GetRasterDataType())
        throw Error("In function swath2grid(), expected raster data to be of type double.");

    DBG(cerr << "GetDAPArray() - Destination array will have dimensions: " << x << ", " << y << endl);

    Array *a = new Array(m_src->name(), new Float64(m_src->name()));

    // Make the result array have two dimensions
    Array::Dim_iter i = m_src->dim_begin();

    a->append_dim(x, m_src->dimension_name(i));
    ++i;

    if (i == m_src->dim_end())
        throw Error("DAP_Dataset::GetDAPArray() - In function swath2grid(), expected source array to have two dimensions (2).");

    a->append_dim(y, m_src->dimension_name(i));

    // Poke in the data values
    /* RasterIO (   GDALRWFlag  eRWFlag,
    int     nXOff,
    int     nYOff,
    int     nXSize,
    int     nYSize,
    void *  pData,
    int     nBufXSize,
    int     nBufYSize,
    GDALDataType    eBufType,
    int     nPixelSpace,
    int     nLineSpace
    ) */
    vector<double> data(x * y);
    rb->RasterIO(GF_Read, 0, 0, x, y, &data[0], x, y, GDT_Float64, 0, 0);

    // NB: set_value() copies into new storage
    a->set_value(data, data.size());

    // Now poke in some attributes
    // TODO Make these CF attributes
    string projection_info = maptr_DS->GetProjectionRef();
    string gcp_projection_info = maptr_DS->GetGCPProjection();

    // This sets the instance variable that holds the geotransform coefs. These
    // are needed by the GetDAPGrid() method.
    if (CE_None != maptr_DS->GetGeoTransform (m_geo_transform_coef))
        throw Error("DAP_Dataset::GetDAPArray() - In function swath2grid(), could not access the geo transform data.");

    DBG(cerr << "GetDAPArray() - projection_info: " << projection_info << endl);
    DBG(cerr << "GetDAPArray() - gcp_projection_info: " << gcp_projection_info << endl);
    DBG(cerr << "GetDAPArray() - geo_transform coefs: " << double_to_string(m_geo_transform_coef[0]) << endl);

    AttrTable &attr = a->get_attr_table();
    attr.append_attr("projection", "String", projection_info);
    attr.append_attr("gcp_projection", "String", gcp_projection_info);
    for (unsigned int i = 0; i < sizeof(m_geo_transform_coef); ++i) {
        attr.append_attr("geo_transform_coefs", "String", double_to_string(m_geo_transform_coef[i]));
    }
    DBG(cerr << "GetDAPArray() - END" << endl);

    return a;
}

/************************************************************************/
/*                            GetDAPGrid()                              */
/************************************************************************/

/**
 * @brief Build a DAP Grid from the GDALDataset
 */
Grid *DAP_Dataset::GetDAPGrid()
{
    DBG(cerr << "GetDAPGrid() - BEGIN" << endl);

    Array *a = GetDAPArray();
    Array::Dim_iter i = a->dim_begin();
    int lon_size = a->dimension_size(i);
    int lat_size = a->dimension_size(++i);

    Grid *g = new Grid(a->name());
    g->add_var_nocopy(a, array);

    // Add maps; assume lon, lat; only two dimensions
    Array *lon = new Array("longitude", new Float64("longitude"));
    lon->append_dim(lon_size);

    vector<double> data(max(lon_size, lat_size)); // (re)use this for both lon and lat

    // Compute values
    // u = a*x + b*y
    // v = c*x + d*y
    // u,v --> x,y --> lon,lat
    // The constants a, b, c, d are given by the 1, 2, 4, and 5 entries in the geotransform array.

    if (m_geo_transform_coef[2] != 0)
    	throw Error("The transformed data's Geographic projection should not be rotated.");
    for (int j = 0; j < lon_size; ++j) {
        data[j] = m_geo_transform_coef[1] * j + m_geo_transform_coef[0];
    }

    // load (copy) values
    lon->set_value(&data[0], lon_size);
    // Set the map
    g->add_var_nocopy(lon, maps);

    // Now do the latitude map
    Array *lat = new Array("latitude", new Float64("latitude"));
    lat->append_dim(lat_size);

    if (m_geo_transform_coef[4] != 0)
    	throw Error("The transformed data's Geographic projection should not be rotated.");
    for (int k = 0; k < lat_size; ++k) {
        data[k] = m_geo_transform_coef[5] * k + m_geo_transform_coef[3];
    }

    lat->set_value(&data[0], lat_size);
    g->add_var_nocopy(lat, maps);

    DBG(cerr << "GetDAPGrid() - END" << endl);

    return g;
}

/************************************************************************/
/*                            SetNativeCRS()                            */
/************************************************************************/

/**
 * \brief Set the Native CRS for a GOES dataset.
 *
 * The method will set the CRS for a GOES dataset as an native CRS.
 *
 * Since the original GOES data adopt satellite CRS to recored its value,
 * like MODIS swath data, each data point has its corresponding latitude
 * and longitude value, those coordinates could be fetched in another two fields.
 *
 * The native CRS for GOES Imager and Sounder data is assigned to EPSG:4326 if
 * both the latitude and longitude are existed.
 *
 * @return CE_None on success or CE_Failure on failure.
 */

CPLErr DAP_Dataset::SetNativeCRS()
{
    DBG(cerr << "SetNativeCRS() - BEGIN" << endl);

    mo_NativeCRS.SetWellKnownGeogCS("WGS84");

    DBG(cerr << "SetNativeCRS() - END" << endl);

    return CE_None;
}

/************************************************************************/
/*                           SetGeoTransform()                          */
/************************************************************************/

/**
 * \brief Set the affine GeoTransform matrix for a GOES data.
 *
 * The method will set a GeoTransform matrix for a GOES data
 * by parsing the coordinates values existed in longitude and latitude field.
 *
 * The CRS for the bounding box is EPSG:4326.
 *
 * @return CE_None on success or CE_Failure on failure.
 */

CPLErr DAP_Dataset::SetGeoTransform()
{
    DBG(cerr << "SetGeoTransform() - BEGIN" << endl);

    // TODO Look at this; is this correct
    // Assume the array is two dimensional
    Array::Dim_iter i = m_src->dim_begin();
#if 0
    // ORIGINAL code; maybe wrong
    int nXSize = m_src->dimension_size(i, true);
    int nYSize = m_src->dimension_size(i + 1, true);
#endif
    // Data are in row-major order, so the first dim is the Y-axis value
    int nXSize = m_src->dimension_size(i, true);
    int nYSize = m_src->dimension_size(i + 1, true);

    mi_SrcImageXSize = nXSize;
    mi_SrcImageYSize = nYSize;

    SetGeoBBoxAndGCPs(nXSize, nYSize);

    double resX, resY;
    if (mdSrcGeoMinX > mdSrcGeoMaxX && mdSrcGeoMinX > 0 && mdSrcGeoMaxX < 0)
        resX = (360 + mdSrcGeoMaxX - mdSrcGeoMinX) / (nXSize - 1);
    else
        resX = (mdSrcGeoMaxX - mdSrcGeoMinX) / (nXSize - 1);

    resY = (mdSrcGeoMaxY - mdSrcGeoMinY) / (nYSize - 1);

    double res = MIN(resX, resY);

    if (mdSrcGeoMinX > mdSrcGeoMaxX && mdSrcGeoMinX > 0 && mdSrcGeoMaxX < 0)
        mi_RectifiedImageXSize = (int) ((360 + mdSrcGeoMaxX - mdSrcGeoMinX) / res) + 1;
    else
        mi_RectifiedImageXSize = (int) ((mdSrcGeoMaxX - mdSrcGeoMinX) / res) + 1;

    mi_RectifiedImageYSize = (int) fabs((mdSrcGeoMaxY - mdSrcGeoMinY) / res) + 1;

    DBG(cerr << "SetGeoTransform() - Source image size: " << nXSize << ", " << nYSize << endl);
    DBG(cerr << "SetGeoTransform() - Rectified image size: " << mi_RectifiedImageXSize << ", " << mi_RectifiedImageYSize << endl);

    md_Geotransform[0] = mdSrcGeoMinX;
    md_Geotransform[1] = res;
    md_Geotransform[2] = 0;
    md_Geotransform[3] = mdSrcGeoMaxY;
    md_Geotransform[4] = 0;
    md_Geotransform[5] = -res;
    mb_GeoTransformSet = TRUE;

    DBG(cerr << "SetGeoTransform() - END" << endl);

    return CE_None;
}

/************************************************************************/
/*                         SetGeoBBoxAndGCPs()                          */
/************************************************************************/

/**
 * \brief Set the native geographical bounding box and GCP array for a GOES data.
 *
 * The method will set the native geographical bounding box
 * by comparing the coordinates values existed in longitude and latitude field.
 *
 * @param poVDS The GDAL dataset returned by calling GDALOpen() method.
 *
 * @return CE_None on success or CE_Failure on failure.
 */

void DAP_Dataset::SetGeoBBoxAndGCPs(int nXSize, int nYSize)
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
        throw Error("SetGeoBBoxAndGCPs() - The size of latitude/longitude and data field does not match.");

#if 0
    /*
     *	Re-sample Standards:
     *	Height | Width
     *	(0, 500)		every other one pixel
     *	[500, 1000)		every other two pixels
     *	[1000,1500)		every other three pixels
     *	[1500,2000)		every other four pixels
     *	... ...
     */

    int xSpace = 1;
    int ySpace = 1;
    //setResampleStandard(poVDS, xSpace, ySpace);

    // TODO understand how GMU picked this value.
    // xSpace and ySpace are the stride values for sampling in
    // the x and y dimensions.
    const int RESAMPLE_STANDARD = 500;

    xSpace = int(nXSize / RESAMPLE_STANDARD) + 2;
    ySpace = int(nYSize / RESAMPLE_STANDARD) + 2;
#endif

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

        // Sample every other row and column
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
}

/************************************************************************/
/*                           SetGDALDataset()                           */
/************************************************************************/

/**
 * \brief Make a 'memory' dataset with one band
 *
 * @return CE_None on success or CE_Failure on failure.
 */

CPLErr DAP_Dataset::SetGDALDataset(const int isSimple)
{
    DBG(cerr << "SetGDALDataset() - BEGIN" << endl);

    // NB: mi_RectifiedImageXSize & Y are set in SetGeoTransform()
    GDALDataType eBandType = GDT_Float64;
    // VRT, which was used in the original sample code, is not supported in this context, so I used MEM
    string driverName = "MEM";
    GDALDriverH poDriver = GDALGetDriverByName(driverName.c_str());
    if (!poDriver) {
        throw Error("Failed to get '"+driverName+"' driver (" + string(CPLGetLastErrorMsg()) + ").");
    }
    DBG(cerr << "SetGDALDataset() - Acquired '"+driverName+"' driver" << endl);


    GDALDataset* satDataSet = (GDALDataset*) GDALCreate(poDriver, "", mi_RectifiedImageXSize, mi_RectifiedImageYSize,
            1, eBandType, NULL);
    if (!satDataSet) {
        GDALClose(poDriver);
        throw Error("Failed to create MEM dataSet (" + string(CPLGetLastErrorMsg()) + ").");
    }
    DBG(cerr << "SetGDALDataset() - Created GDALDataset" << endl);

    GDALRasterBand *poBand = satDataSet->GetRasterBand(1);
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
}

/************************************************************************/
/*                       SetGCPGeoRef4VRTDataset()                      */
/************************************************************************/

/**
 * \brief Set the GCP array for the VRT dataset.
 *
 * This method is used to set the GCP array to created VRT dataset based on GDAL
 * method SetGCPs().
 *
 * @param poVDS The VRT dataset.
 *
 * @return CE_None on success or CE_Failure on failure.
 */

CPLErr DAP_Dataset::SetGCPGeoRef4VRTDataset(GDALDataset* poVDS)
{
    char* psTargetSRS;
    mo_NativeCRS.exportToWkt(&psTargetSRS);

#if (__GNUC__ >=4 && __GNUC_MINOR__ > 1)
    if (CE_None != poVDS->SetGCPs(m_gdalGCPs.size(), (GDAL_GCP*) (m_gdalGCPs.data()), psTargetSRS)) {
        OGRFree(psTargetSRS);
        throw Error("Failed to set GCPs.");
    }
#else
    {
        if(CE_None!=poVDS->SetGCPs(m_gdalGCPs.size(), (GDAL_GCP*)&m_gdalGCPs[0], psTargetSRS))
        {
            OGRFree( psTargetSRS );
            throw Error("Failed to set GCPs.");
        }
    }
#endif

    OGRFree(psTargetSRS);

    return CE_None;
}
#if 0
/************************************************************************/
/*                        SetMetaDataList()                             */
/************************************************************************/

/**
 * \brief Set the metadata list for this coverage.
 *
 * The method will set the metadata list for the coverage based on its
 * corresponding GDALDataset object.
 *
 * @param hSrc the GDALDataset object corresponding to coverage.
 *
 * @return CE_None on success or CE_Failure on failure.
 */

CPLErr DAP_Dataset::SetMetaDataList(GDALDataset* hSrcDS)
{
    // TODO Remove
#if 0
    mv_MetaDataList.push_back("Product_Description=The data was created by GMU WCS from NOAA GOES satellite data.");
    mv_MetaDataList.push_back("unit=GVAR");
    mv_MetaDataList.push_back("FillValue=0");
    ms_FieldQuantityDef = "GVAR";
    ms_AllowRanges = "0 65535";
    ms_CoveragePlatform = "GOES-11";
    ms_CoverageInstrument = "GOES-11";
    ms_CoverageSensor = "Imager";
#endif

    return CE_None;
}
#endif
/************************************************************************/
/*                          GetGeoMinMax()                              */
/************************************************************************/

/**
 * \brief Get the min/max coordinates of laitutude and longitude.
 *
 * The method will fetch the min/max coordinates of laitutude and longitude.
 *
 * @param geoMinMax an existing four double buffer into which the
 * native geographical bounding box values will be placed.
 *
 * @return CE_None on success or CE_Failure on failure.
 */

CPLErr DAP_Dataset::GetGeoMinMax(double geoMinMax[])
{
    if (!mb_GeoTransformSet)
        return CE_Failure;

    geoMinMax[0] = mdSrcGeoMinX;
    geoMinMax[2] = mdSrcGeoMinY;
    geoMinMax[1] = mdSrcGeoMaxX;
    geoMinMax[3] = mdSrcGeoMaxY;

    return CE_None;
}

/************************************************************************/
/*                          RectifyGOESDataSet()                        */
/************************************************************************/

/**
 * \brief Convert the GOES dataset from satellite CRS project to grid CRS.
 *
 * The method will convert the GOES dataset from satellite CRS project to
 * grid CRS based on GDAL API GDALReprojectImage;
 *
 * @return CE_None on success or CE_Failure on failure.
 */

CPLErr DAP_Dataset::RectifyGOESDataSet()
{
    DBG(cerr << "RectifyGOESDataSet() - BEGIN" << endl);

    char *pszDstWKT;
    mo_NativeCRS.exportToWkt(&pszDstWKT);
    DBG(cerr << "RectifyGOESDataSet() - pszDstWKT: '" << pszDstWKT << "'" << endl);

    string driverName="VRT";
    GDALDriverH poDriver = GDALGetDriverByName(driverName.c_str()); // VRT
    GDALDataset* rectDataSet = (GDALDataset*) GDALCreate(poDriver, "", mi_RectifiedImageXSize, mi_RectifiedImageYSize,
            maptr_DS->GetRasterCount(), maptr_DS->GetRasterBand(1)->GetRasterDataType(), NULL);
    if (NULL == rectDataSet) {
        GDALClose(poDriver);
        OGRFree(pszDstWKT);
        throw Error("DAP_Dataset::RectifyGOESDataSet(): Failed to create \""+driverName+"\" dataSet.");
    }
    DBG(cerr << "RectifyGOESDataSet() - Got '"<< driverName << "' driver."<< endl);


    rectDataSet->SetProjection(pszDstWKT);
    rectDataSet->SetGeoTransform(md_Geotransform);

    DBG(cerr << "RectifyGOESDataSet() - rectDataSet:\n" << datasetInfo(rectDataSet) << endl);
    DBG(cerr << "RectifyGOESDataSet() - satDataSet:\n" << datasetInfo(maptr_DS.get()) << endl);

    // FIXME Magic value of 0.125
    if (CE_None != GDALReprojectImage(maptr_DS.get(), NULL, rectDataSet, pszDstWKT,
            GRA_NearestNeighbour, 0, 0.125, GDALTermProgress, NULL, NULL)) {
        GDALClose(rectDataSet);
        GDALClose(poDriver); // I had to comment this out to stop some seg fault think.. Weird...
        OGRFree(pszDstWKT);
        throw Error("DAP_Dataset::RectifyGOESDataSet() - Failed to re-project satellite data from GCP CRS to geographical CRS.");
    }

    OGRFree(pszDstWKT);
    GDALClose(maptr_DS.release());

    maptr_DS.reset(rectDataSet);

    DBG(cerr << "RectifyGOESDataSet() - END" << endl);

    return CE_None;
}



} // namespace libdap
