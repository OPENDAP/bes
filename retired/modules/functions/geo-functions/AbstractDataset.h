/******************************************************************************
 * $Id: AbstractDataset.h 2011-07-19 16:24:00Z $
 *
 * Project:  The Open Geospatial Consortium (OGC) Web Coverage Service (WCS)
 * 			 for Earth Observation: Open Source Reference Implementation
 * Purpose:  AbstractDataset class definition
 * Author:   Yuanzheng Shao, yshao3@gmu.edu
 *
 ******************************************************************************
 * * Copyright (c) 2011, Liping Di <ldi@gmu.edu>, Yuanzheng Shao <yshao3@gmu.edu>
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

#ifndef ABSTRACTDATASET_H_
#define ABSTRACTDATASET_H_

#include <string>
#include <vector>
#include <memory>

#include <gdal.h>
#include <gdal_priv.h>
#include <gdalwarper.h>
#include <ogrsf_frmts.h>
#include <ogr_spatialref.h>
#include <cpl_conv.h>
#include <cpl_minixml.h>
#include <vrtdataset.h>

#include "wcsUtil.h"

/* ******************************************************************** */
/*                             AbstractDataset                          */
/* ******************************************************************** */

//! Abstract dataset model definition. Based on GDAL dataset model.
class AbstractDataset {

protected:
    std::unique_ptr<GDALDataset> maptr_DS;

    // Coverage Information Related
    std::string ms_CoverageID;
    std::string ms_CoverageBeginTime;
    std::string ms_CoverageEndTime;
    std::string ms_CoverageSubType;
    std::string ms_CoverageArchiveTime;
    std::string ms_CoveragePlatform;
    std::string ms_CoverageInstrument;
    std::string ms_CoverageSensor;
    std::string ms_SrcFilename;
    std::string ms_DatasetName;
    std::string ms_DataTypeName;
    std::string ms_NativeFormat;
    std::string ms_FieldQuantityDef;
    std::string ms_AllowRanges;
    std::string ms_ISO19115Metadata;

    std::vector<int> mv_BandList;
    std::vector<std::string> mv_MetaDataList;

    double md_Geotransform[6];
    double md_GeoMinMax[4]; // Order: xmin, xmax, ymin, ymax
    double md_MissingValue;

    int mb_GeoTransformSet;
    int mb_IsVirtualDS;

    OGRSpatialReference mo_NativeCRS;

protected:
    AbstractDataset();
    virtual CPLErr SetNativeCRS();
    virtual CPLErr SetNativeCRS(std::string mktStr);
    virtual CPLErr SetGeoTransform();
    virtual CPLErr SetGDALDataset(const int isSimple = 0);
    virtual CPLErr SetMetaDataList(GDALDataset*);

public:
    AbstractDataset(const std::string&, std::vector<int> &);
    virtual ~AbstractDataset();

    GDALDataset* GetGDALDataset();

    // Virtual Functions Definition
    virtual CPLErr InitializeDataset(const int isSimple = 0);

    // Fetch Function Related
    const OGRSpatialReference& GetNativeCRS();
    const double& GetMissingValue();
    int GetGeoTransform(double geoTrans[]);
    std::vector<std::string> GetMetaDataList();
    std::vector<int> GetBandList();
    void GetNativeBBox(double bBox[]);
    CPLErr GetGeoMinMax(double geoMinMax[]);

    int GetImageBandCount();
    int GetImageXSize();
    int GetImageYSize();
    std::string GetResourceFileName();
    std::string GetDatasetName();
    std::string GetDataTypeName();
    std::string GetNativeFormat();
    std::string GetCoverageID();
    std::string GetDatasetDescription();
    std::string GetNativeCRS_URN();
    std::string GetGeoCRS_URN();
    std::string GetProjectionRef();
    std::string GetCoverageBeginTime();
    std::string GetCoverageEndTime();
    std::string GetCoverageSubType();
    std::string GetFieldQuantityDef();
    std::string GetAllowValues();
    std::string GetISO19115Metadata();
    std::string GetCoverageArchiveTime();
    std::string GetCoveragePlatform();
    std::string GetCoverageInstrument();
    std::string GetCoverageSensor();

    // Fetch Variables Status Related
    int IsbGeoTransformSet();
    int IsCrossingIDL();

    CPLErr GetSuggestedWarpResolution(OGRSpatialReference& dstCRS, double adfDstGeoTransform[], int &nPixels,
                                      int &nLines);
    CPLErr GetSuggestedWarpResolution2(OGRSpatialReference& dstCRS, double adfDstGeoTransform[], int &nPixels,
                                       int &nLines);

    GDALDataset* DatasetWarper(int& IsRefDS, OGRSpatialReference& dstCRS, int& iDstRasterXsize, int& iDstRasterYsize,
                               double pDstGeoTransform[], GDALResampleAlg eResampleAlg = GRA_NearestNeighbour);
};

#endif /*ABSTRACTDATASET_H_*/
