/******************************************************************************
 * $Id: BoundingBox.h 2011-07-19 16:24:00Z $
 *
 * Project:  The Open Geospatial Consortium (OGC) Web Coverage Service (WCS)
 * 			 for Earth Observation: Open Source Reference Implementation
 * Purpose:  BoundingBox class definition
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

#ifndef BOUNDINGBOX_H_
#define BOUNDINGBOX_H_

#include <gdal.h>
#include <ogr_spatialref.h>
#include <limits>
#include <stdlib.h>

using namespace std;

/************************************************************************/
/* ==================================================================== */
/*                             My2DPoint                                */
/* ==================================================================== */
/************************************************************************/

/**
 * \class My2DPoint "BoundingBox.h"
 *
 * My2DPoint class is used to store the point coordinates.
 */

class My2DPoint
{
public:
	double mi_X;
	double mi_Y;

	virtual ~My2DPoint();

	My2DPoint()
	{
		mi_X = 0;
		mi_Y = 0;
	}

	My2DPoint(const double& xx, const double& yy) :
		mi_X(xx), mi_Y(yy)
	{
	}

	My2DPoint(const My2DPoint& p) :
		mi_X(p.mi_X), mi_Y(p.mi_Y)
	{
	}

	My2DPoint& operator =(const My2DPoint& p)
	{
		mi_X = p.mi_X;
		mi_Y = p.mi_Y;
		return *this;
	}
};

/************************************************************************/
/* ==================================================================== */
/*                             BoundingBox                              */
/* ==================================================================== */
/************************************************************************/

/**
 * \class BoundingBox "BoundingBox.h"
 *
 * BoundingBox class is used to transform bounding box between different
 * Coordinate Reference System.
 */

class BoundingBox
{
public:
	My2DPoint mo_LowerLeftPT;
	My2DPoint mo_UpperRightPT;
	OGRSpatialReference mo_CRS;

	BoundingBox(const My2DPoint& llpt, const My2DPoint& urpt, OGRSpatialReference& crs) :
		mo_LowerLeftPT(llpt), mo_UpperRightPT(urpt), mo_CRS(crs)
	{

	}

	BoundingBox(OGRSpatialReference& crs) :
		mo_LowerLeftPT(0,0), mo_UpperRightPT(0,0), mo_CRS(crs)
	{

	}
	BoundingBox& operator =(const BoundingBox& box)
	{
		mo_LowerLeftPT = box.mo_LowerLeftPT;
		mo_UpperRightPT = box.mo_UpperRightPT;
		mo_CRS = box.mo_CRS;
		return *this;
	}

	BoundingBox();
	virtual ~BoundingBox();

	BoundingBox Transform(OGRSpatialReference&, int&);
	BoundingBox TransformWorkExtend(OGRSpatialReference&, int&);
	BoundingBox Transform(const double*);
};

CPLErr CPL_DLL CPL_STDCALL bBox_transFormmate(	OGRSpatialReference&,
												OGRSpatialReference&,
												My2DPoint& lowLeft,
												My2DPoint& upRight);

#endif /* BOUNDINGBOX_H_ */
