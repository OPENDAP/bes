
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

#ifndef DAP_DATASET_H_
#define DAP_DATASET_H_

#include <string>
#include "AbstractDataset.h"
#include "wcsUtil.h"

namespace libdap {

class Array;
class Grid;

/************************************************************************/
/* ==================================================================== */
/*                           DAP_Dataset                            */
/* ==================================================================== */
/************************************************************************/

//! DAP_Dataset is a subclass of AbstractDataset, used to process NOAA GOES data.
/**
 * \class DAP_Dataset "DAP_Dataset.h"
 *
 * GOES satellites provide the kind of continuous monitoring necessary for
 * intensive data analysis. They circle the Earth in a geosynchronous orbit,
 * which means they orbit the equatorial plane of the Earth at a speed
 * matching the Earth's rotation. This allows them to hover continuously
 * over one position on the surface. The geosynchronous plane is about
 * 35,800 km (22,300 miles) above the Earth, high enough to allow the
 * satellites a full-disc view of the Earth. Because they stay above a
 * fixed spot on the surface, they provide a constant vigil for the atmospheric
 * "triggers" for severe weather conditions such as tornadoes, flash floods,
 * hail storms, and hurricanes. When these conditions develop the GOES
 * satellites are able to monitor storm development and track their movements.
 *
 * GOES satellite imagery is also used to estimate rainfall during
 * the thunderstorms and hurricanes for flash flood warnings, as well
 * as estimates snowfall accumulations and overall extent of snow cover.
 * Such data help meteorologists issue winter storm warnings and spring
 * snow melt advisories. Satellite sensors also detect ice fields and map
 * the movements of sea and lake ice.
 *
 * For more inforamtion about NOAA GOES data, please access
 * <a href=http://www.oso.noaa.gov/GOES/>http://www.oso.noaa.gov/GOES/</a>
 *
 * DAP_Dataset is a subclass of AbstractDataset, which is used to
 * process GOES Imager and Sounder products.
 */

class DAP_Dataset : public AbstractDataset {
protected:
    string m_ncLatDataSetName;
    string m_ncLonDataSetName;
    string m_ncCoverageIDName;

    // Instead of using names and opening files with GDAL,
    // store pointers to Arrays read by the underlying DAP
    // server constraint evaluator.
    Array *m_src;
    Array *m_lat;
    Array *m_lon;

    int mi_RectifiedImageXSize;
    int mi_RectifiedImageYSize;
    int mi_SrcImageXSize;
    int mi_SrcImageYSize;

    double mb_LatLonBBox[4];
    double mdSrcGeoMinX;
    double mdSrcGeoMinY;
    double mdSrcGeoMaxX;
    double mdSrcGeoMaxY;

    // This is not set until GetDAPArray() is called.
    double m_geo_transform_coef[6];

    vector<GDAL_GCP> m_gdalGCPs;

public:
    CPLErr SetGCPGeoRef4VRTDataset(GDALDataset*);
    void SetGeoBBoxAndGCPs(int xSize, int ySize);
    CPLErr RectifyGOESDataSet();
    CPLErr setResampleStandard(GDALDataset* hSrcDS, int& xRSValue, int& yRSValue);

    int isValidLatitude(const double &lat)
    {
        return (lat >= -90 && lat <= 90);
    }
    int isValidLongitude(const double &lon)
    {
        return (lon >= -180 && lon <= 180);
    }

    virtual CPLErr SetGeoTransform();
#if 0
    virtual CPLErr SetMetaDataList(GDALDataset* hSrcDS); //TODO Remove
#endif
    virtual CPLErr SetNativeCRS();
    virtual CPLErr SetGDALDataset(const int isSimple = 0);
    virtual CPLErr InitializeDataset(const int isSimple = 0);
    virtual CPLErr GetGeoMinMax(double geoMinMax[]);

public:
    DAP_Dataset();
    DAP_Dataset(const string& id, vector<int> &rBandList);

    // Added jhrg 11/23/12
    DAP_Dataset(Array *src, Array *lat, Array *lon);
    Array *GetDAPArray();
    Grid *GetDAPGrid();

    virtual ~DAP_Dataset();
};

} // namespace libdap
#endif /* DAP_DATASET_H_ */
