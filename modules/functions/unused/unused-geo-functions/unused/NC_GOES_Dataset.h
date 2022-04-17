/******************************************************************************
 * $Id: NC_GOES_Dataset.h 2011-07-19 16:24:00Z $
 *
 * Project:  The Open Geospatial Consortium (OGC) Web Coverage Service (WCS)
 * 			 for Earth Observation: Open Source Reference Implementation
 * Purpose:  NC_GOES_Dataset class definition
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

#ifndef NC_GOES_DATASET_H_
#define NC_GOES_DATASET_H_

#include <string>
#include "AbstractDataset.h"
#include "wcsUtil.h"

/************************************************************************/
/* ==================================================================== */
/*                           NC_GOES_Dataset                            */
/* ==================================================================== */
/************************************************************************/

//! NC_GOES_Dataset is a subclass of AbstractDataset, used to process NOAA GOES data.

/**
 * \class NC_GOES_Dataset "NC_GOES_Dataset.h"
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
 * NC_GOES_Dataset is a subclass of AbstractDataset, which is used to
 * process GOES Imager and Sounder products.
 */

class NC_GOES_Dataset : public AbstractDataset
{
protected:
	string	m_ncLatDataSetName;
	string	m_ncLonDataSetName;
	string	m_ncCoverageIDName;
	int 	mi_RectifiedImageXSize;
	int 	mi_RectifiedImageYSize;
	int		mi_GoesSrcImageXSize;
	int		mi_GoesSrcImageYSize;

	double	mb_LatLonBBox[4];
	double 	mdSrcGeoMinX;
	double 	mdSrcGeoMinY;
	double 	mdSrcGeoMaxX;
	double 	mdSrcGeoMaxY;

	vector<GDAL_GCP>   		m_gdalGCPs;

public:
	CPLErr SetGCPGeoRef4VRTDataset(GDALDataset* );
	CPLErr SetGeoBBoxAndGCPs(GDALDataset* hSrcDS);
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
	virtual CPLErr SetMetaDataList(GDALDataset* hSrcDS);
	virtual CPLErr SetNativeCRS();
	virtual CPLErr SetGDALDataset(const int isSimple=0);
	virtual CPLErr InitializeDataset(const int isSimple=0);
	virtual CPLErr GetGeoMinMax(double geoMinMax[]);

public:
	NC_GOES_Dataset();
	NC_GOES_Dataset(const string& id, vector<int>  &rBandList);
	virtual ~NC_GOES_Dataset();
};

#endif /* NC_GOES_DATASET_H_ */
