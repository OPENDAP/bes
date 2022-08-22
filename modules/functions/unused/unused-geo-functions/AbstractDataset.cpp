/******************************************************************************
 * $Id: AbstractDataset.cpp 2011-07-19 16:24:00Z $
 *
 * Project:  The Open Geospatial Consortium (OGC) Web Coverage Service (WCS)
 * 			 for Earth Observation: Open Source Reference Implementation
 * Purpose:  AbstractDataset implementation for providing an abstract data
 * 			 model for multiple data source.
 * Author:   Yuanzheng Shao, yshao3@gmu.edu
 *
 ******************************************************************************
 * Copyright (c) 2011, Liping Di <ldi@gmu.edu>, Yuanzheng Shao <yshao3@gmu.edu>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include <iostream>

#include "AbstractDataset.h"

using namespace std;

/************************************************************************/
/* ==================================================================== */
/*                           AbstractDataset                            */
/* ==================================================================== */
/************************************************************************/

/**
 * \class AbstractDataset "AbstractDataset.h"
 *
 * An abstract dataset encapsulating one or more raster bands, which is
 * based on GDALDataset, and adds the support to metadata model: ISO 19115
 * and 1SO 19115 (2). A series of Fetch functions are provided for the
 * implementation of aWeb Coverage Service (WCS) endpoint.
 *
 * Use WCSTCreateDataset() to create a AbstractDataset for a named coverage
 * identifier and band list.
 */

/************************************************************************/
/*                            AbstractDataset()                         */
/************************************************************************/
AbstractDataset::AbstractDataset()
{
}

/************************************************************************/
/*                            AbstractDataset()                         */
/************************************************************************/

/**
 * \brief Constructor of an AbstractDataset.
 *
 * This is the accepted method of opening an abstract dataset and allocating
 * all resources associated with it.
 */

AbstractDataset::AbstractDataset(const string& id, vector<int> &rBandList) :
	ms_CoverageID(id), mv_BandList(rBandList)
{
}

/************************************************************************/
/*                            ~AbstractDataset()                        */
/************************************************************************/

/**
 * \brief Destroy an open AbstractDataset object.
 *
 * This is the accepted method of closing a AbstractDataset dataset and
 * deallocating all resources associated with it.
 */

AbstractDataset::~AbstractDataset()
{
	if (maptr_DS.get())
		GDALClose(maptr_DS.release());
}

/************************************************************************/
/*                            InitialDataset()                          */
/************************************************************************/

/**
 * \brief Initialize the data set.
 *
 * This is the virtual function for initializing AbstarctDataset. The
 * subclasses of AbstarctDataset will call SetNativeCRS(), SetGeoTransform()
 * and SetGDALDataset() to initialize the associated fields.
 *
 * @param isSimple The WCS request type.  When user executing a DescribeCoverage
 * request, isSimple is set to 1, and for GetCoverage, is set to 0.
 *
 * @return CE_None on success or CE_Failure on failure.
 */

CPLErr AbstractDataset::InitializeDataset(const int isSimple)
{
	return CE_Failure;
}

/************************************************************************/
/*                            GetGDALDataset()                          */
/************************************************************************/

/**
 * \brief Get the GDALDataset object from AbstarctDataset.
 *
 * An AbstarctDataset encapsulating one GDALDataset. GetGDALDataset()
 * returns a GDALDatset object, which is used to fetch its information
 * through GDAL APIs.
 *
 * @return GDALDatset object.
 */

GDALDataset* AbstractDataset::GetGDALDataset()
{
	return maptr_DS.get();
}

/************************************************************************/
/*                            SetGDALDataset()                          */
/************************************************************************/

/**
 * \brief Set the GDALDataset object to AbstarctDataset.
 *
 * This is the virtual function for setting the abstract dataset by
 * calling GDAL functions.
 *
 * @param isSimple the WCS request type.  When user executing a DescribeCoverage
 * request, isSimple is set to 1, and for GetCoverage, is set to 0.
 *
 * @return CE_None on success or CE_Failure on failure.
 */

CPLErr AbstractDataset::SetGDALDataset(const int isSimple)
{
	return CE_Failure;
}

/************************************************************************/
/*                            GetNativeCRS()                            */
/************************************************************************/

/**
 * \brief Get the Native CRS of an AbstarctDataset.
 *
 * The method will return the CRS obejct, which is an OGRSpatialReference
 * object.
 *
 * @return an OGRSpatialReference object corresponding the native CRS of
 * coverage.
 */

const OGRSpatialReference& AbstractDataset::GetNativeCRS()
{
	return mo_NativeCRS;
}

/************************************************************************/
/*                            SetNativeCRS()                            */
/************************************************************************/

/**
 * \brief Set the Native CRS for an AbstarctDataset.
 *
 * The method will set the CRS for an AbstractDataset as an native CRS. For
 * some served coverage, GDAL could not tell its native CRS, this method
 * should be called to set its native CRS.
 *
 * @return CE_None on success or CE_Failure on failure.
 */

CPLErr AbstractDataset::SetNativeCRS()
{
    char* wktStr = (char*) maptr_DS->GetProjectionRef();
    cerr << "AbstractDataset::SetNativeCRS() - wktStr: " << wktStr << endl;
    return SetNativeCRS(wktStr);
}

CPLErr AbstractDataset::SetNativeCRS(string wktStr)
{
    string crsNameWktStr(wktStr);
    char s[wktStr.size()+1];
    strcpy(s,wktStr.c_str());
    char *crsName =  s;

    if (!wktStr.size() && OGRERR_NONE == mo_NativeCRS.importFromWkt(&crsName))
        return CE_None;

    return CE_Failure;
}

/************************************************************************/
/*                            SetGeoTransform()                         */
/************************************************************************/

/**
 * \brief Set the affine GeoTransform matrix for a coverage.
 *
 * The method will set a GeoTransform matrix for a coverage. The method
 * GetGeoTransform of GDAL library will be called to get the matrix.
 *
 * @return CE_None on success or CE_Failure on failure.
 */

CPLErr AbstractDataset::SetGeoTransform()
{
	if (CE_None != maptr_DS->GetGeoTransform(md_Geotransform))
		return CE_Failure;

	//Is the returned matrix correct? check the resolution values;
	if(md_Geotransform[2] == 0 && md_Geotransform[5] == 0)
		return CE_Failure;

	mb_GeoTransformSet = TRUE;

	return CE_None;
}

/************************************************************************/
/*                            GetGeoTransform()                         */
/************************************************************************/

/**
 * \brief Fetch the affine transformation coefficients.
 *
 * Fetches the coefficients for transforming between pixel/line (P,L) raster
 * space, and projection coordinates (Xp,Yp) space.
 *
 * \code
 *   Xp = padfTransform[0] + P*padfTransform[1] + L*padfTransform[2];
 *   Yp = padfTransform[3] + P*padfTransform[4] + L*padfTransform[5];
 * \endcode
 *
 * In a north up image, padfTransform[1] is the pixel width, and
 * padfTransform[5] is the pixel height.  The upper left corner of the
 * upper left pixel is at position (padfTransform[0],padfTransform[3]).
 *
 * The default transform is (0,1,0,0,0,1) and should be returned even when
 * a CE_Failure error is returned, such as for formats that don't support
 * transformation to projection coordinates.
 *
 * NOTE: GetGeoTransform() isn't expressive enough to handle the variety of
 * OGC Grid Coverages pixel/line to projection transformation schemes.
 * Eventually this method will be depreciated in favour of a more general
 * scheme.
 *
 * @param geoTrans an existing six double buffer into which the
 * transformation will be placed.
 *
 * @return TRUE on success or FALSE on failure.
 */

int AbstractDataset::GetGeoTransform(double geoTrans[])
{
	if (!mb_GeoTransformSet)//Geo-Transform not setup in each data driver, then set default.
	{
		geoTrans[0]=0.0;
		geoTrans[0]=1.0;
		geoTrans[0]=0.0;
		geoTrans[0]=0.0;
		geoTrans[0]=0.0;
		geoTrans[0]=1.0;
		return FALSE;
	}

	memcpy(geoTrans, md_Geotransform, sizeof(double) * 6);

	return TRUE;
}

/************************************************************************/
/*                            GetCoverageID()                           */
/************************************************************************/

/**
 * \brief Fetch the identifier of a coverage.
 *
 * The method will return the coverage identifier related to the abstarct
 * dataset. As to TRMM data, the coverage identifier likes:
 * TRMM:/Volumes/RAIDL1/GeoData/TRMM/TRMM_3B42_daily.2000.hdf:Daily
 *
 * As to MODIS data, the coverage identifier likes:
 * HDF4_EOS:EOS_GRID:"/Volumes/RAIDL1/GeoData/MOD15A2/2007/MYD15A2.A2007241.h12v09.005.2007256053902.hdf":MOD_Grid_MOD15A2:Lai_1km
 *
 * @return the string of coverage identifier.
 */

string AbstractDataset::GetCoverageID()
{
	return ms_CoverageID;
}

/************************************************************************/
/*                            GetResourceFileName()                     */
/************************************************************************/

/**
 * \brief Fetch the resource file name of a coverage.
 *
 * The method will return the full path corresponding the file contained
 * coverage.
 *
 * @return the string of resource file path.
 */

string AbstractDataset::GetResourceFileName()
{
	return ms_SrcFilename;
}

/************************************************************************/
/*                            GetDatasetName()                          */
/************************************************************************/

/**
 * \brief Fetch the dataset name of a coverage.
 *
 * The method will return the data set name corresponding the file contained
 * coverage. For example, MOD09GQ data has the coverage identifier as following;
 * HDF4_EOS:EOS_GRID:"/Volumes/RAIDL1/GeoData/MODISData/MOD09GQ/MOD09GQ.A2010001.h12v05.005.2010007003100.hdf":MODIS_Grid_2D:sur_refl_b01_1
 *
 * sur_refl_b01_1 is seemed as the dataset name.
 *
 * @return the string of dataset name.
 */

string AbstractDataset::GetDatasetName()
{
	return ms_DatasetName;
}

/************************************************************************/
/*                            GetDataTypeName()                         */
/************************************************************************/

/**
 * \brief Fetch the dataset type name of a coverage.
 *
 * The method will return the data set name corresponding the file contained
 * coverage. For example, MOD09GQ data has the coverage identifier as following;
 * HDF4_EOS:EOS_GRID:"/Volumes/RAIDL1/GeoData/MODISData/MOD09GQ/MOD09GQ.A2010001.h12v05.005.2010007003100.hdf":MODIS_Grid_2D:sur_refl_b01_1
 *
 * MODIS_Grid_2D is seemed as the dataset type name.
 *
 * @return the string of dataset type name.
 */

string AbstractDataset::GetDataTypeName()
{
	return ms_DataTypeName;
}

/************************************************************************/
/*                            GetDataTypeName()                         */
/************************************************************************/

/**
 * \brief Fetch the dataset description of a coverage.
 *
 * The method will build a description for the coverage. The coverage
 * extent, dataset name, dataset type name will be contained in the
 * description,
 *
 * @return the string of dataset description.
 */

string AbstractDataset::GetDatasetDescription()
{
	//[15x2030x1354] Band JPEG2000 (16-bit unsigned integer)
	string rtnBuf;
	int aiDimSizes[3];
	int nBandCount = maptr_DS->GetRasterCount();
	string pszString;
	if (nBandCount > 1)
	{
		aiDimSizes[0] = nBandCount;
		aiDimSizes[1] = GetImageYSize();
		aiDimSizes[2] = GetImageXSize();
		pszString = SPrintArray(GDT_UInt32, aiDimSizes, 3, "x");
	}
	else
	{
		aiDimSizes[0] = GetImageYSize();
		aiDimSizes[1] = GetImageXSize();
		pszString = SPrintArray(GDT_UInt32, aiDimSizes, 2, "x");
	}

	rtnBuf = "[" + pszString + "] " + ms_DatasetName + " " + ms_DataTypeName + " (" +
			GDALGetDataTypeName(maptr_DS->GetRasterBand(1)->GetRasterDataType()) + ")";

	return rtnBuf;
}

/************************************************************************/
/*                            GetNativeFormat()                         */
/************************************************************************/

/**
 * \brief Fetch the native format of a coverage.
 *
 * The method will return the native format of a coverage. GDAL API
 * GDALGetDriverShortName() will be called to generate the format.
 *
 * @return the string of dataset native format, in forms of GDAL Formats
 * Code. See http://gdal.org/formats_list.html for details.
 */

string AbstractDataset::GetNativeFormat()
{
	return ms_NativeFormat;
}

/************************************************************************/
/*                            IsbGeoTransformSet()                      */
/************************************************************************/

/**
 * \brief Determine whether set the affine geoTransform for a coverage.
 *
 * The method will return the status of the affine GeoTransform matrix.
 *
 * @return TRUE if set already or FALSE on failure..
 */

int AbstractDataset::IsbGeoTransformSet()
{
	return mb_GeoTransformSet;
}

/************************************************************************/
/*                            GetNativeBBox()                           */
/************************************************************************/

/**
 * \brief Fetch the bounding box of a coverage under the native CRS.
 *
 * The method will fetch the bounding box of the coverage under native CRS.
 * The sequence of values stored in array is: xmin, xmax, ymin, ymax
 *
 * @param bBox an existing four double buffer into which the
 * native bounding box values will be placed.
 */

void AbstractDataset::GetNativeBBox(double bBox[])
{
	if (mb_GeoTransformSet)
	{
		bBox[0] = md_Geotransform[0];
		bBox[1] = bBox[0] + GetImageXSize() * md_Geotransform[1];
		bBox[3] = md_Geotransform[3];
		bBox[2] = bBox[3] + GetImageYSize() * md_Geotransform[5];
	}
	else
	{
		bBox[0] = 0;
		bBox[1] = maptr_DS->GetRasterXSize() - 1;
		bBox[2] = 0;
		bBox[3] = maptr_DS->GetRasterYSize() - 1;
	}
}

/************************************************************************/
/*                            GetGeoMinMax()                            */
/************************************************************************/

/**
 * \brief Fetch the bounding box of a coverage under EPSG:4326.
 *
 * The method will fetch the bounding box of the coverage under EPSG:4326
 * CRS. The sequence of values stored in array is: xmin, xmax, ymin, ymax
 *
 * @param bBox an existing four double buffer into which the geographic
 * bounding box values will be placed.
 *
 * @return CE_None on success or CE_Failure on failure.
 */

CPLErr AbstractDataset::GetGeoMinMax(double geoMinMax[])
{
	if (!mb_GeoTransformSet)
	{
		SetWCS_ErrorLocator("AbstractDataset::getGeoMinMax()");
		WCS_Error(CE_Failure, OGC_WCS_NoApplicableCode, "Failed to get Geo-BoundingBox coordinates");
		return CE_Failure;
	}

	geoMinMax[0] = md_Geotransform[0];
	geoMinMax[1] = geoMinMax[0] + GetImageXSize() * md_Geotransform[1];
	geoMinMax[3] = md_Geotransform[3];
	geoMinMax[2] = geoMinMax[3] + GetImageYSize() * md_Geotransform[5];


	if (mo_NativeCRS.IsGeographic() || Find_Compare_SubStr(ms_DataTypeName, "MODIS"))//for modis data
	{
		geoMinMax[0] = (geoMinMax[0] >= -180.0) ? geoMinMax[0] : -180.0;
		geoMinMax[1] = (geoMinMax[1] <=  180.0) ? geoMinMax[1] :  180.0;
		geoMinMax[2] = (geoMinMax[2] >=  -90.0) ? geoMinMax[2] :  -90.0;
		geoMinMax[3] = (geoMinMax[3] <=   90.0) ? geoMinMax[3] :   90.0;
		return CE_None;
	}

	OGRSpatialReference oGeoSRS;
	oGeoSRS.CopyGeogCSFrom(&mo_NativeCRS);

	My2DPoint llPt(geoMinMax[0], geoMinMax[2]);
	My2DPoint urPt(geoMinMax[1], geoMinMax[3]);
	if (CE_None != bBox_transFormmate(mo_NativeCRS, oGeoSRS, llPt, urPt))
	{
		SetWCS_ErrorLocator("AbstractDataset::getGeoMinMax()");
		WCS_Error(CE_Failure, OGC_WCS_NoApplicableCode, "Failed to transform bounding box coordinates to geographic coordinates.");
		return CE_Failure;
	}

	geoMinMax[0] = llPt.mi_X;
	geoMinMax[1] = urPt.mi_X;
	geoMinMax[2] = llPt.mi_Y;
	geoMinMax[3] = urPt.mi_Y;

	return CE_None;
}


/************************************************************************/
/*                            GetImageXSize()                           */
/************************************************************************/

/**
 * \brief Fetch coverage width in pixels.
 *
 * The method will return the width of coverage in pixels. GDAL API
 * GetRasterXSize() will be called to generate the width value.
 *
 * @return the width in pixels of raster bands in this coverage.
 */

int AbstractDataset::GetImageXSize()
{
	return maptr_DS->GetRasterXSize();
}

/************************************************************************/
/*                            GetImageYSize()                           */
/************************************************************************/

/**
 * \brief Fetch coverage height in pixels.
 *
 * The method will return the height of coverage in pixels. GDAL API
 * GetRasterYSize() will be called to generate the height value.
 *
 * @return the height in pixels of raster bands in this coverage.
 */
int AbstractDataset::GetImageYSize()
{
	return maptr_DS->GetRasterYSize();
}

/************************************************************************/
/*                        GetCoverageBeginTime()                        */
/************************************************************************/

/**
 * \brief Fetch the begin date/time of a coverage.
 *
 * The method will return the begin date/time of a coverage. For MODIS data,
 * each granule will cover a range of time; for TRMM data, the daily data
 * will cover a whole day, and monthly data will cover a whole month.
 *
 * @return the string of begin date/time corresponding to the coverage.
 */

string AbstractDataset::GetCoverageBeginTime()
{
	return ms_CoverageBeginTime;
}

/************************************************************************/
/*                        GetCoverageBeginTime()                        */
/************************************************************************/

/**
 * \brief Fetch the end date/time of a coverage.
 *
 * The method will return the end date/time of a coverage. For MODIS data,
 * each granule will cover a range of time; for TRMM data, the daily data
 * will cover a whole day, and monthly data will cover a whole month.
 *
 * @return the string of end date/time corresponding to the coverage.
 */

string AbstractDataset::GetCoverageEndTime()
{
	return ms_CoverageEndTime;
}

/************************************************************************/
/*                        getCoverageSubType()                          */
/************************************************************************/

/**
 * \brief Fetch the coverage type.
 *
 * The method will return the type of the coverage, such as
 * "ReferenceableDataset" and "RectifiedDataset".
 *
 * @return the string of coverage type.
 */

string AbstractDataset::GetCoverageSubType()
{
	return ms_CoverageSubType;
}

/************************************************************************/
/*                        getFieldQuantityDef()                         */
/************************************************************************/

/**
 * \brief Fetch the field units of a coverage.
 *
 * The method will return the field units of coverage. For example to
 * MOD09GQ(collection name) sur_refl_b01_1(dataset name) data, units equals
 * to reflectance.
 *
 * @return the string of coverage field units.
 */

string AbstractDataset::GetFieldQuantityDef()
{
	return ms_FieldQuantityDef;
}

/************************************************************************/
/*                        GetAllowValues()                              */
/************************************************************************/

/**
 * \brief Fetch the allow values of a coverage.
 *
 * The method will return the valid range of a coverage. For example to
 * MOD09GQ(collection name) sur_refl_b01_1(dataset name) data, valid_range
 * equals to (-100, 16000).
 *
 * @return the string of valid range of a coverage, in the forms of "min, max".
 */

string AbstractDataset::GetAllowValues()
{
	return ms_AllowRanges;
}

/************************************************************************/
/*                        GetISO19115Metadata()                         */
/************************************************************************/

/**
 * \brief Fetch the coverage metadata which is compliant to ISO 19115:2003
 * - GeoGraphic information -- Metadata; and ISO 19115(2):2009 - GeoGraphic
 * information -- Metadata -- Part 2.
 *
 * The method will return the metadata of a coverage based on ISO 19115
 * and ISO 19115(2).
 *
 * ISO 19115:2003 defines the schema required for
 * describing geographic information and services. It provides information
 * about the identification, the extent, the quality, the spatial and temporal
 * schema, spatial reference, and distribution of digital geographic data.
 *
 * ISO 19115-2:2009 extends the existing geographic metadata standard by
 * defining the schema required for describing imagery and gridded data.
 * It provides information about the properties of the measuring equipment
 * used to acquire the data, the geometry of the measuring process employed
 * by the equipment, and the production process used to digitize the raw data.
 * This extension deals with metadata needed to describe the derivation of
 * geographic information from raw data, including the properties of the
 * measuring system, and the numerical methods and computational procedures
 * used in the derivation. The metadata required to address coverage data in
 * general is addressed sufficiently in the general part of ISO 19115.
 *
 * @return the string of metadata of a coverage.
 */

string AbstractDataset::GetISO19115Metadata()
{
	return ms_ISO19115Metadata;
}

/************************************************************************/
/*                        GetCoverageArchiveTime()                      */
/************************************************************************/

/**
 * \brief Fetch the archive date/time of this dataset.
 *
 * The method will return the archive date/time of dataset (granule).
 *
 * @return The string of archive date/time.
 */

string AbstractDataset::GetCoverageArchiveTime()
{
	return ms_CoverageArchiveTime;
}

/************************************************************************/
/*                        GetCoveragePlatform()                         */
/************************************************************************/

/**
 * \brief Fetch the platform name of this dataset.
 *
 * The method will return the platform name of dataset (granule).
 *
 * @return The string of platform name.
 */

string AbstractDataset::GetCoveragePlatform()
{
	return ms_CoveragePlatform;
}

/************************************************************************/
/*                        GetCoverageInstrument()                       */
/************************************************************************/

/**
 * \brief Fetch the instrument name of this dataset.
 *
 * The method will return the instrument name of dataset (granule).
 *
 * @return The string of instrument name.
 */

string AbstractDataset::GetCoverageInstrument()
{
	return ms_CoverageInstrument;
}

/************************************************************************/
/*                        GetCoverageInstrument()                       */
/************************************************************************/

/**
 * \brief Fetch the sensor name of this dataset.
 *
 * The method will return the sensor name of dataset (granule).
 *
 * @return The string of sensor name.
 */

string AbstractDataset::GetCoverageSensor()
{
	return ms_CoverageSensor;
}

/************************************************************************/
/*                        GetImageBandCount()                           */
/************************************************************************/

/**
 * \brief Fetch the number of raster bands on this dataset.
 *
 * The method will return the number of raster bands on this dataset. GDAL
 * API GetRasterCount() will be called to get the count number.
 *
 * @return the number of raster bands on this dataset.
 */

int AbstractDataset::GetImageBandCount()
{
	return maptr_DS->GetRasterCount();
}

/************************************************************************/
/*                        getMissingValue()                             */
/************************************************************************/

/**
 * \brief Fetch the filled value (missing value) of a coverage.
 *
 * The method will return the filled value of a coverage.
 *
 * @return the value of filled value of a coverage.
 */

const double& AbstractDataset::GetMissingValue()
{
	return md_MissingValue;
}

/************************************************************************/
/*                        GetProjectionRef()                            */
/************************************************************************/

/**
 * \brief Fetch the native projection reference of a coverage.
 *
 * The method will return the the native projection reference of a coverage.
 *
 * @return the string of the native projection reference of a coverage.
 */

string AbstractDataset::GetProjectionRef()
{
	char* pszProjection = NULL;
	mo_NativeCRS.exportToWkt(&pszProjection);
	string tmpStr = pszProjection;
	CPLFree(pszProjection);

	return tmpStr;
}

/************************************************************************/
/*                        SetMetaDataList()                             */
/************************************************************************/

/**
 * \brief Set the metadata list for this coverage based on dataset object.
 *
 * The method will set the metadata list for the coverage based on its
 * corresponding GDALDataset object. Will be implemented in the subclasses
 * of AbstractDataset.
 *
 * @param hSrc the GDALDataset object corresponding to coverage.
 *
 * @return CE_Failure.
 */

CPLErr AbstractDataset::SetMetaDataList(GDALDataset* hSrc)
{
	return CE_Failure;
}

/************************************************************************/
/*                        GetMetaDataList()                             */
/************************************************************************/

/**
 * \brief Fetch the metadata list for this coverage.
 *
 * The method will return the metadata list for the coverage.
 *
 * @return the list of metadate.
 */

vector<string> AbstractDataset::GetMetaDataList()
{
	return mv_MetaDataList;
}

/************************************************************************/
/*                            GetBandList()                             */
/************************************************************************/

/**
 * \brief Fetch the contained band list of request coverage.
 *
 * The method will return the contained band list of request coverage.
 *
 * @return the band array.
 */

vector<int> AbstractDataset::GetBandList()
{
	return mv_BandList;
}

/************************************************************************/
/*                        getNativeCRS_URN()                            */
/************************************************************************/

/**
 * \brief Fetch the native CRS code of coverage.
 *
 * The method will return the native CRS code of coverage.
 *
 * @return the string of the URN for native CRS of coverage.
 */

string AbstractDataset::GetNativeCRS_URN()
{
	string urn;

	if (mo_NativeCRS.IsProjected())
	{
		const char* nativeAuthorityName =
				mo_NativeCRS.GetAuthorityName("PROJCS");
		const char* nativeAuthorityCode =
				mo_NativeCRS.GetAuthorityCode("PROJCS");

		urn = "urn:ogc:def:crs:";

		if (nativeAuthorityName && (EQUAL(nativeAuthorityName,"EPSG")
				|| EQUAL(nativeAuthorityName,"OGP")
				|| EQUAL(nativeAuthorityName,"OGC")))
			urn += (string) nativeAuthorityName + (string) ":6.3:";
		else
			urn += "CSISS:0.0:";

		if (nativeAuthorityCode)
			urn += (string) nativeAuthorityCode;
		else
			urn += "80000000";
	}
	else if (mo_NativeCRS.IsGeographic())
	{
		const char* geoAuthorityName = mo_NativeCRS.GetAuthorityName("GEOGCS");
		const char* geoAuthorityCode = mo_NativeCRS.GetAuthorityCode("GEOGCS");

		urn = "urn:ogc:def:crs:";
		if (geoAuthorityName && (EQUAL(geoAuthorityName,"EPSG")
				|| EQUAL(geoAuthorityName,"OGP")
				|| EQUAL(geoAuthorityName,"OGC")))
			urn += (string) geoAuthorityName + (string) ":6.3:";
		else
			urn += "CSISS:0.0:";

		if (geoAuthorityCode)
			urn += (string) geoAuthorityCode;
		else
			urn += "70000000";
	}
	else
		urn = "urn:ogc:def:crs:OGC:0.0:imageCRS";

	return urn;
}

/************************************************************************/
/*                        GetGeoCRS_URN()                               */
/************************************************************************/

/**
 * \brief Fetch the Geographic CRS code of coverage.
 *
 * The method will return the Geographic CRS code of coverage.
 *
 * @return the URN for Geographic CRS of coverage.
 */

string AbstractDataset::GetGeoCRS_URN()
{
	string urn;

	if (mo_NativeCRS.IsProjected() || mo_NativeCRS.IsGeographic())
	{
		const char* geoAuthorityName = mo_NativeCRS.GetAuthorityName("GEOGCS");
		const char* geoAuthorityCode = mo_NativeCRS.GetAuthorityCode("GEOGCS");

		urn = "urn:ogc:def:crs:";
		if (geoAuthorityName && (EQUAL(geoAuthorityName,"EPSG")
				|| EQUAL(geoAuthorityName,"OGP")
				|| EQUAL(geoAuthorityName,"OGC")))
			urn += (string) geoAuthorityName + (string) ":6.3:";
		else
			urn += "CSISS:0.0:";

		if (geoAuthorityCode)
			urn += (string) geoAuthorityCode;
		else
			urn += "70000000";

	}
	else
		urn = "urn:ogc:def:crs:OGC:0.0:imageCRS";

	return urn;
}

/************************************************************************/
/*                        IsCrossingIDL()                               */
/************************************************************************/

/**
 * \brief Determine whether the extend of coverage cross the International
 * Date Line (IDL).
 *
 * The method will return the status of whether the extend of coverage
 * cross the International Date Line (IDL).
 *
 * @return the TRUE of the coverage extent cross the IDL, otherwise FALSE.
 */

int AbstractDataset::IsCrossingIDL()
{
	double bboxArray[4];
	GetNativeBBox(bboxArray);

	OGRSpatialReference latlonSRS;
	latlonSRS.SetWellKnownGeogCS("WGS84");
	My2DPoint llPtex(bboxArray[0], bboxArray[2]);
	My2DPoint urPtex(bboxArray[1], bboxArray[3]);
	bBox_transFormmate(mo_NativeCRS, latlonSRS, llPtex, urPtex);

	int bCrossCenter = false;
	if(urPtex.mi_X < 0 && llPtex.mi_X > 0)
		bCrossCenter = true;

	return bCrossCenter;
}

/************************************************************************/
/*                        GetSuggestedWarpResolution()                  */
/************************************************************************/

/**
 * \brief Get the suggested warp option, method 1.
 *
 * @return CE_Failure if an error occurs, otherwise CE_None.
 */

CPLErr AbstractDataset::GetSuggestedWarpResolution(	OGRSpatialReference& dstCRS,
													double adfDstGeoTransform[],
													int &nPixels,
													int &nLines)
{
	if (!dstCRS.IsProjected() && !dstCRS.IsGeographic())
	{
		adfDstGeoTransform[0] = 0;
		adfDstGeoTransform[1] = 1;
		adfDstGeoTransform[2] = 0;
		adfDstGeoTransform[3] = 0;
		adfDstGeoTransform[4] = 0;
		adfDstGeoTransform[5] = 1;

		nPixels = GetImageXSize();
		nLines = GetImageYSize();

	}
	else if (dstCRS.IsSame(&mo_NativeCRS))
	{
		memcpy(adfDstGeoTransform, md_Geotransform, sizeof(double) * 6);
		nPixels = GetImageXSize();
		nLines = GetImageYSize();
	}
	else
	{
		char *pszDstWKT;
		dstCRS.exportToWkt(&pszDstWKT);
		char *pszSrcWKT;
		mo_NativeCRS.exportToWkt(&pszSrcWKT);

		void *hTransformArg = GDALCreateGenImgProjTransformer(maptr_DS.get(),
				(const char*) pszSrcWKT, NULL, (const char*) pszDstWKT, TRUE, 1000.0, 0);
		OGRFree(pszDstWKT);
		OGRFree(pszSrcWKT);
		if (hTransformArg == NULL)
		{
			SetWCS_ErrorLocator("AbstractDataset::GetSuggestedWarpResolution()");
			WCS_Error(CE_Failure, OGC_WCS_NoApplicableCode, "Failed to Create GDAL GenImgProjTransformer.");

			return CE_Failure;
		}
		/* -------------------------------------------------------------------- */
		/*      Get approximate output definition.                              */
		/* -------------------------------------------------------------------- */
		if (GDALSuggestedWarpOutput(maptr_DS.get(), GDALGenImgProjTransform,
				hTransformArg, adfDstGeoTransform, &nPixels, &nLines) != CE_None)
		{
			SetWCS_ErrorLocator("AbstractDataset::GetSuggestedWarpResolution()");
			WCS_Error(CE_Failure, OGC_WCS_NoApplicableCode, "Failed to Computing Output Resolution.");

			return CE_Failure;
		}

		GDALDestroyGenImgProjTransformer(hTransformArg);
	}

	return CE_None;
}

/************************************************************************/
/*                        GetSuggestedWarpResolution2()                 */
/************************************************************************/

/**
 * \brief Get the suggested warp option, method 2.
 *
 * @return CE_Failure if an error occurs, otherwise CE_None.
 */

CPLErr AbstractDataset::GetSuggestedWarpResolution2(OGRSpatialReference& dstCRS,
													double adfDstGeoTransform[],
													int &nPixels,
													int &nLines)
{
	if (dstCRS.IsLocal())
	{
		adfDstGeoTransform[0] = 0;
		adfDstGeoTransform[1] = 1;
		adfDstGeoTransform[2] = 0;
		adfDstGeoTransform[3] = 0;
		adfDstGeoTransform[4] = 0;
		adfDstGeoTransform[5] = 1;

		nPixels = GetImageXSize();
		nLines = GetImageYSize();
	}
	else if (dstCRS.IsSame(&mo_NativeCRS))
	{
		memcpy(adfDstGeoTransform, md_Geotransform, sizeof(double) * 6);
		nPixels = GetImageXSize();
		nLines = GetImageYSize();
	}
	else
	{
		double bboxArray[4];
		GetNativeBBox(bboxArray);

		My2DPoint llPt(bboxArray[0], bboxArray[2]);
		My2DPoint urPt(bboxArray[1], bboxArray[3]);
		if (CE_None != bBox_transFormmate(mo_NativeCRS, dstCRS, llPt, urPt))
		{
			SetWCS_ErrorLocator("AbstractDataset::GetSuggestedWarpResolution2()");
			WCS_Error(CE_Failure, OGC_WCS_NoApplicableCode, "Failed to Transform bounding box Coordinate.");
			return CE_Failure;
		}

		double xRes, yRes;
		if(IsCrossingIDL())
		{
			yRes = (urPt.mi_Y - llPt.mi_Y) / GetImageYSize();
			xRes = fabs((llPt.mi_X - urPt.mi_X) / GetImageXSize());
			nLines = (urPt.mi_Y - llPt.mi_Y) / yRes + 0.5;
			nPixels = fabs((llPt.mi_X - urPt.mi_X) / xRes + 0.5);
		}else
		{
			xRes = (urPt.mi_X - llPt.mi_X) / GetImageXSize();
			yRes = (urPt.mi_Y - llPt.mi_Y) / GetImageYSize();
			nPixels = (urPt.mi_X - llPt.mi_X) / xRes + 0.5;
			nLines = (urPt.mi_Y - llPt.mi_Y) / yRes + 0.5;
		}

		xRes = MIN(xRes,yRes);
		yRes = MIN(xRes,yRes);

		adfDstGeoTransform[0] = llPt.mi_X;
		adfDstGeoTransform[1] = xRes;
		adfDstGeoTransform[2] = 0;
		adfDstGeoTransform[3] = urPt.mi_Y;
		adfDstGeoTransform[4] = 0;
		adfDstGeoTransform[5] = -yRes;
	}

	return CE_None;
}

/************************************************************************/
/*                           DatasetWarper()                            */
/************************************************************************/

/**
 * Wrap the dataset to a GDALDataset object based on input parameters.
 *
 * @return the warpped GDALDataset object.
 */
/************************************************************************/
/*                        DatasetWarper()                               */
/************************************************************************/

/**
 * \brief Wrap the dataset to a GDALDataset object based on input parameters.
 *
 * @return GDALDatset object.
 */

GDALDataset* AbstractDataset::DatasetWarper(int& IsRefDS,
											OGRSpatialReference& dstCRS,
											int& iDstRasterXsize,
											int& iDstRasterYsize,
											double pDstGeoTransform[],
											GDALResampleAlg eResampleAlg)
{
	OGRSpatialReference locCRS=dstCRS;
	if (dstCRS.IsLocal())
		locCRS=mo_NativeCRS;

	if((mo_NativeCRS.IsSame(&locCRS) &&
			iDstRasterXsize	== maptr_DS->GetRasterXSize() &&
			iDstRasterYsize == maptr_DS->GetRasterYSize())&&
			CPLIsEqual(md_Geotransform[0],pDstGeoTransform[0])&&
			CPLIsEqual(md_Geotransform[1],pDstGeoTransform[1])&&
			CPLIsEqual(md_Geotransform[3],pDstGeoTransform[3])&&
			CPLIsEqual(md_Geotransform[5],pDstGeoTransform[5]))
	{
		IsRefDS = TRUE;
		return maptr_DS.get();
	}

	char *sDstCRS_WKT;
	locCRS.exportToWkt(&sDstCRS_WKT);

	/* Create a memory data-set for re-projection */
	GDALDriverH poDriver = GDALGetDriverByName("MEM");

	int nBand = maptr_DS->GetRasterCount();
	GDALDataset* hMemDS = (GDALDataset*) GDALCreate(poDriver, "", iDstRasterXsize,
			iDstRasterYsize, nBand, GDALGetRasterDataType(maptr_DS->GetRasterBand(1)), NULL);
	if (NULL == hMemDS)
	{
		GDALClose(poDriver);
		OGRFree(sDstCRS_WKT);
		return NULL;
	}

	hMemDS->SetProjection(sDstCRS_WKT);
	hMemDS->SetGeoTransform(pDstGeoTransform);

	for (int i = 1; i <= nBand; i++)
	{
		hMemDS->GetRasterBand(i)->SetNoDataValue(md_MissingValue);
	}

	/* -------------------------------------------------------------------- */
	/*      Perform the re-projection.                                       */
	/* -------------------------------------------------------------------- */
	char *srcWKT;
	mo_NativeCRS.exportToWkt(&srcWKT);
	if (CE_None != GDALReprojectImage(maptr_DS.get(), srcWKT, hMemDS,
			sDstCRS_WKT, eResampleAlg, 0, 0.125, NULL, NULL, NULL))
	{
		GDALClose(poDriver);
		GDALClose(GDALDatasetH(hMemDS));
		OGRFree(sDstCRS_WKT);
		OGRFree(srcWKT);
		return NULL;
	}
	IsRefDS = FALSE;
	OGRFree(sDstCRS_WKT);
	OGRFree(srcWKT);

	return hMemDS;
}
