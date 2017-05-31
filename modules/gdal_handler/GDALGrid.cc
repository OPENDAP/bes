// This file is part of the GDAL OPeNDAP Adapter

// Copyright (c) 2004 OPeNDAP, Inc.
// Author: Frank Warmerdam <warmerdam@pobox.com>
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
//#define DODS_DEBUG 1

#include <string>

#include <gdal.h>
#include <cpl_conv.h>

#include <BESDebug.h>

#include "GDALTypes.h"

using namespace std;
using namespace libdap;

// From gdal_dds.cc
void read_data_array(GDALArray *array, GDALRasterBandH hBand);
void read_map_array(Array *map, GDALRasterBandH hBand, GDALDatasetH hDS);

/************************************************************************/
/* ==================================================================== */
/*                               GDALGrid                               */
/* ==================================================================== */
/************************************************************************/

void GDALGrid::m_duplicate(const GDALGrid &g)
{
	filename = g.filename;
#if 0
	hBand = g.hBand;
	eBufType = g.eBufType;
#endif

}

// protected

BaseType *
GDALGrid::ptr_duplicate()
{
	return new GDALGrid(*this);
}

// public

GDALGrid::GDALGrid(const string &filenameIn, const string &name) :
        Grid(name), filename(filenameIn)
{
}

GDALGrid::GDALGrid(const GDALGrid &rhs) : Grid(rhs)
{
	m_duplicate(rhs);
}

GDALGrid &GDALGrid::operator=(const GDALGrid &rhs)
{
	if (this == &rhs) return *this;

	m_duplicate(rhs);

	return *this;
}

GDALGrid::~GDALGrid()
{
}

bool GDALGrid::read()
{
	BESDEBUG("gdal", "Entering GDALGrid::read()" << endl);

	if (read_p()) // nothing to do
		return true;

    GDALDatasetH hDS = GDALOpen(filename.c_str(), GA_ReadOnly);

    try {
        // Change: Instead of using the binary hBand (GDALRasterBandH), open the
        // file and then the Raster Band here. Assume each part of the Grid is in
        // the same file.

        GDALArray *array = static_cast<GDALArray*>(array_var());

        read_data_array(array, GDALGetRasterBand(hDS, array->get_gdal_band_num()));
        array->set_read_p(true);

        Map_iter miter = map_begin();
        array = static_cast<GDALArray*>((*miter));
        read_map_array(array, GDALGetRasterBand(hDS, array->get_gdal_band_num()), hDS);
        array->set_read_p(true);

        ++miter;
        array = static_cast<GDALArray*>(*miter);
        read_map_array(array, GDALGetRasterBand(hDS, array->get_gdal_band_num()), hDS);
        array->set_read_p(true);
    }
    catch (...) {
        GDALClose(hDS);
        throw;
    }

    GDALClose(hDS);

	return true;
}
