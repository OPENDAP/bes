/******************************************************************************
 * $Id: BoundingBox.cpp 2011-07-19 16:24:00Z $
 *
 * Project:  The Open Geospatial Consortium (OGC) Web Coverage Service (WCS)
 * 			 for Earth Observation: Open Source Reference Implementation
 * Purpose:  BoundingBox class implementation, enable transform bounding box
 * 			 between different CRS
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

#include <vector>
#include <iostream>

#include "BoundingBox.h"
#include "wcs_error.h"

using namespace std;

My2DPoint::~My2DPoint()
{

}

BoundingBox::BoundingBox()
{

}

BoundingBox::~BoundingBox()
{

}

BoundingBox BoundingBox::TransformWorkExtend(OGRSpatialReference &dstCRS, int &IsOK)
{
	if (mo_CRS.IsSame(&dstCRS))
	{
		IsOK = TRUE;
		return *this;
	}

	OGRCoordinateTransformation *poCT = OGRCreateCoordinateTransformation(&mo_CRS, &dstCRS);
	if (poCT == NULL)
	{
		IsOK = FALSE;
		return *this;
	}

	double xdes = (mo_UpperRightPT.mi_X - mo_LowerLeftPT.mi_X) / 100;

	if (mo_CRS.IsGeographic()
			&& mo_UpperRightPT.mi_X < mo_LowerLeftPT.mi_X
			&& mo_LowerLeftPT.mi_X > 0 && mo_UpperRightPT.mi_X < 0)
	{
		xdes = (360 + mo_UpperRightPT.mi_X - mo_LowerLeftPT.mi_X) / 100;
	}

	vector<double> x;
	vector<double> y;
	//up and down edge
	for (double stepX = mo_LowerLeftPT.mi_X; stepX < mo_UpperRightPT.mi_X; stepX
			+= xdes)
	{
		x.push_back(stepX);
		y.push_back(mo_UpperRightPT.mi_Y);
		x.push_back(stepX);
		y.push_back(mo_LowerLeftPT.mi_Y);
	}
	x.push_back(mo_UpperRightPT.mi_X);
	y.push_back(mo_UpperRightPT.mi_Y);
	x.push_back(mo_UpperRightPT.mi_X);
	y.push_back(mo_LowerLeftPT.mi_Y);

	double yMin = numeric_limits<double>::max();
	double yMax = numeric_limits<double>::min();

	int k = 0;
	vector<int> bSuccess;
	vector<double> tmpX;
	vector<double> tmpY;

	for (unsigned int i = 0; i < x.size(); i++)
	{
		tmpX.push_back(x[i]);
		tmpY.push_back(y[i]);
		bSuccess.push_back(0);
	}

	poCT->TransformEx(x.size(), &tmpX[0], &tmpY[0], NULL, &bSuccess[0]);

	for (unsigned int n = 0; n < x.size(); n++)
	{
		if (bSuccess[n])
		{
			++k;
			yMin = MIN(yMin,tmpY[n]);
			yMax = MAX(yMax,tmpY[n]);
		}
	}

	if (k < 80)
	{
		IsOK = FALSE;
		OCTDestroyCoordinateTransformation(poCT);
		return *this;
	}

	//find xmin on left edge and xmax on right edge
	double xMin;
	double xMax;

	double tmpPTX[2];
	double tmpPTY[2];

	int isSucc[2];

	tmpPTX[0] = mo_LowerLeftPT.mi_X;
	tmpPTX[1] = mo_LowerLeftPT.mi_X;
	tmpPTY[0] = mo_LowerLeftPT.mi_Y;
	tmpPTY[1] = mo_UpperRightPT.mi_Y;

	poCT->TransformEx(2, tmpPTX, tmpPTY, NULL, isSucc);
	if (isSucc[0] && isSucc[1])
	{
		xMin = MIN(tmpPTX[0],tmpPTX[1]);
	}
	else
	{
		OCTDestroyCoordinateTransformation(poCT);
		IsOK = FALSE;
		return *this;
	}

	tmpPTX[0] = mo_UpperRightPT.mi_X;
	tmpPTX[1] = mo_UpperRightPT.mi_X;
	tmpPTY[0] = mo_UpperRightPT.mi_Y;
	tmpPTY[1] = mo_LowerLeftPT.mi_Y;

	poCT->TransformEx(2, tmpPTX, tmpPTY, NULL, isSucc);
	if (isSucc[0] && isSucc[1])
	{
		xMax = MAX(tmpPTX[0],tmpPTX[1]);
	}
	else
	{
		OCTDestroyCoordinateTransformation(poCT);
		IsOK = FALSE;
		return *this;
	}

	BoundingBox bbox(My2DPoint(xMin, yMin), My2DPoint(xMax, yMax), dstCRS);

	if (dstCRS.IsGeographic())
	{
		if (xMin > 180)
			xMin = xMin - 360;
		if (xMax > 180)
			xMax = xMax - 360;

		if (xMin >= 180.)
			xMin = 180.;
		if (xMin <= -180.)
			xMin = -180.;
		if (xMax >= 180.)
			xMax = 180.;
		if (xMax <= -180.)
			xMax = -180.;

		if (yMin < 0.)
			yMin = 0.;
		if (yMax > 90.)
			yMax = 90.;
	}
	OCTDestroyCoordinateTransformation(poCT);
	IsOK = TRUE;
	return BoundingBox(My2DPoint(xMin, yMin), My2DPoint(xMax, yMax), dstCRS);
}

BoundingBox BoundingBox::Transform(const double GeoTransform[])
{
	My2DPoint llPt;
	My2DPoint urPt;
	llPt.mi_X = GeoTransform[0] + GeoTransform[1] * mo_LowerLeftPT.mi_X + GeoTransform[2] * mo_LowerLeftPT.mi_Y;
	llPt.mi_Y = GeoTransform[3] + GeoTransform[4] * mo_LowerLeftPT.mi_X + GeoTransform[5] * mo_LowerLeftPT.mi_Y;
	urPt.mi_X = GeoTransform[0] + GeoTransform[1] * mo_UpperRightPT.mi_X + GeoTransform[2] * mo_UpperRightPT.mi_Y;
	urPt.mi_Y = GeoTransform[3] + GeoTransform[4] * mo_UpperRightPT.mi_X + GeoTransform[5] * mo_UpperRightPT.mi_Y;

	return BoundingBox(llPt,urPt,mo_CRS);
}

/**
 * Transform Coordinates of lowercorner_left and upcorner_right
 * lowLeft, upRight.
 * the return value has considered the case of image area crossing 180/-180 longitude line,
 * so lowLeft.x maybe bigger than upRight.x
 */
CPLErr CPL_STDCALL bBox_transFormmate(OGRSpatialReference& oSrcCRS,
		OGRSpatialReference& oDesCRS, My2DPoint& lowLeft, My2DPoint& upRight)
{
	if (oSrcCRS.IsSame(&oDesCRS))
		return CE_None;

	OGRCoordinateTransformation *poCT = OGRCreateCoordinateTransformation(&oSrcCRS, &oDesCRS);
	if (poCT == NULL)
	{
		SetWCS_ErrorLocator("bBox_transFormmate()");
		WCS_Error(CE_Failure, OGC_WCS_NoApplicableCode, "Failed to Create \"OGRCoordinateTransformation\"");
		return CE_Failure;
	}

	double xdes = (upRight.mi_X - lowLeft.mi_X) / 100;
	if (oSrcCRS.IsGeographic() && upRight.mi_X < lowLeft.mi_X && lowLeft.mi_X > 0 && upRight.mi_X < 0)
	{
		xdes = (360 + upRight.mi_X - lowLeft.mi_X) / 100;
	}

	vector<double> x, y;
	//up and down edge
	for (double stepX = lowLeft.mi_X; stepX < upRight.mi_X; stepX += xdes)
	{
		x.push_back(stepX);
		y.push_back(upRight.mi_Y);
		x.push_back(stepX);
		y.push_back(lowLeft.mi_Y);
	}
	x.push_back(upRight.mi_X);
	y.push_back(upRight.mi_Y);
	x.push_back(upRight.mi_X);
	y.push_back(lowLeft.mi_Y);

	double yMin = numeric_limits<double>::max();
	double yMax = -numeric_limits<double>::max();

	int k = 0;
	vector<int> bSuccess;
	vector<double> tmpX;
	vector<double> tmpY;

	for (unsigned int i = 0; i < x.size(); i++)
	{
		tmpX.push_back(x[i]);
		tmpY.push_back(y[i]);
		bSuccess.push_back(0);
	}

	poCT->TransformEx(x.size(), &tmpX[0], &tmpY[0], NULL, &bSuccess[0]);

	for (unsigned int n = 0; n < x.size(); n++)
	{
		if (bSuccess[n])
		{
			++k;
			yMin = MIN(yMin,tmpY[n]);
			yMax = MAX(yMax,tmpY[n]);
		}
	}

	if (k < 80)
	{
		OCTDestroyCoordinateTransformation(poCT);
		SetWCS_ErrorLocator("bBox_transFormmate()");
		WCS_Error(CE_Failure, OGC_WCS_NoApplicableCode, "Failed to Transform Coordinates");
		return CE_Failure;
	}

	double xMin;
	double xMax;

	//find xmin on left edge and xmax on right edge
	double tmpPTX[2];
	double tmpPTY[2];
	int isOK[2];

	tmpPTX[0] = lowLeft.mi_X;
	tmpPTX[1] = lowLeft.mi_X;
	tmpPTY[0] = upRight.mi_Y;
	tmpPTY[1] = lowLeft.mi_Y;

	poCT->TransformEx(2, tmpPTX, tmpPTY, NULL, isOK);
	if (isOK[0] && isOK[1])
	{
		xMin = MIN(tmpPTX[0],tmpPTX[1]);
	}
	else
	{
		OCTDestroyCoordinateTransformation(poCT);
		SetWCS_ErrorLocator("bBox_transFormmate()");
		WCS_Error(CE_Failure, OGC_WCS_NoApplicableCode, "Failed to Transform Coordinates");
		return CE_Failure;
	}

	tmpPTX[0] = upRight.mi_X;
	tmpPTX[1] = upRight.mi_X;
	tmpPTY[0] = upRight.mi_Y;
	tmpPTY[1] = lowLeft.mi_Y;

	poCT->TransformEx(2, tmpPTX, tmpPTY, NULL, isOK);
	if (isOK[0] && isOK[1])
	{
		xMax = MAX(tmpPTX[0],tmpPTX[1]);
	}
	else
	{
		SetWCS_ErrorLocator("bBox_transFormmate()");
		OCTDestroyCoordinateTransformation(poCT);
		WCS_Error(CE_Failure, OGC_WCS_NoApplicableCode, "Failed to Transform Coordinates");
		return CE_Failure;
	}
	
	lowLeft.mi_X = xMin;
	lowLeft.mi_Y = yMin;
	upRight.mi_X = xMax;
	upRight.mi_Y = yMax;

	if (oDesCRS.IsGeographic())
	{
		if (xMin > 180)
			lowLeft.mi_X = xMin - 360;
		if (xMax > 180)
			upRight.mi_X = xMax - 360;

		if (lowLeft.mi_X >= 180.)
			lowLeft.mi_X = 180.;
		if (lowLeft.mi_X <= -180.)
			lowLeft.mi_X = -180.;
		if (upRight.mi_X >= 180.)
			upRight.mi_X = 180.;
		if (upRight.mi_X <= -180.)
			upRight.mi_X = -180.;

		if (lowLeft.mi_Y <= -90.)
			lowLeft.mi_Y = -90.;
		if (upRight.mi_Y >= 90.)
			upRight.mi_Y = 90.;
	}
	OCTDestroyCoordinateTransformation(poCT);

	return CE_None;
}
