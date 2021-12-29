
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2016 OPeNDAP, Inc.
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

#ifndef geo_2d_array_h_
#define geo_2d_array_h_

#include <string>
#include <libdap/Array.h>

//#include "wcsUtil.h"

namespace libdap {

class Array;
class Grid;

//! GeoArray is a subclass of libdap::Array, it provides special operations for Geo-spatial data.
/**
 * \class GeoArray "GeoArray.h"
 *
 */

class Geo2DArray : public Array {
protected:
#if 0
	string m_ncLatDataSetName;
    string m_ncLonDataSetName;
    string m_ncCoverageIDName;
#endif
#if 0
    // Instead of using names and opening files with GDAL,
    // store pointers to Arrays read by the underlying DAP
    // server constraint evaluator.
    Array *d_src;
#endif
    Array *d_lat;
    Array *d_lon;

    int mi_RectifiedImageXSize;
    int mi_RectifiedImageYSize;
    int mi_SrcImageXSize;
    int mi_SrcImageYSize;

    double md_Geotransform[6];
    double md_GeoMinMax[4]; // Order: xmin, xmax, ymin, ymax
    double d_missing_value;

    bool mb_GeoTransformSet;
    bool mb_IsVirtualDS;

    OGRSpatialReference mo_NativeCRS;
    double mb_LatLonBBox[4];
    double mdSrcGeoMinX;
    double mdSrcGeoMinY;
    double mdSrcGeoMaxX;
    double mdSrcGeoMaxY;

    // This is not set until GetDAPArray() is called.
    double m_geo_transform_coef[6];

    vector<GDAL_GCP> m_gdalGCPs;

public:
    Geo2DArray(const string &n, BaseType *v, Array *lat, Array *lon, bool is_dap4 = false);
    Geo2DArray(const string &n, const string &d, BaseType *v, Array *lat, Array *lon, bool is_dap4 = false);
    Geo2DArray(const Geo2DArray &rhs);
    virtual ~Geo2DArray();

    Geo2DArray &operator=(const Geo2DArray &rhs);
    virtual BaseType *ptr_duplicate();

    void initialize();

    Array *GetDAPArray();
    Grid *GetDAPGrid();

    CPLErr set_gcp_georef(GDALDataset*);
    void set_geo_bbox_and_gcp(int xSize, int ySize);
    CPLErr rectify_dataset();
    CPLErr set_resample_standard(GDALDataset* hSrcDS, int& xRSValue, int& yRSValue);

    int is_valid_lat(const double &lat)
    {
        return (lat >= -90 && lat <= 90);
    }
    int is_valid_lon(const double &lon)
    {
        return (lon >= -180 && lon <= 180);
    }

    virtual CPLErr set_geo_transform();
#if 0
    virtual CPLErr SetMetaDataList(GDALDataset* hSrcDS); //TODO Remove
#endif
    virtual CPLErr set_native_crs();
    virtual CPLErr set_gdal_dataset(const int isSimple = 0);
    virtual CPLErr initialize_gdal_dataset(const int isSimple = 0);
    virtual CPLErr get_geo_min_max(double geoMinMax[]);

};

} // namespace libdap
#endif /* geo_2d_array_h_ */
