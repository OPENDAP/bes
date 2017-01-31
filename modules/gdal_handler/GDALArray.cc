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

#include <BESDebug.h>

#include "GDALTypes.h"

using namespace std;
using namespace libdap ;

// From gdal_dds.cc
void read_data_array(GDALArray *array, GDALRasterBandH hBand, GDALDataType eBufType);
void read_map_array(Array *map, GDALRasterBandH hBand, string filename);

/************************************************************************/
/* ==================================================================== */
/*                              GDALArray                               */
/* ==================================================================== */
/************************************************************************/

void GDALArray::m_duplicate(const GDALArray &a)
{
	filename = a.filename;
	hBand = a.hBand;
	eBufType = a.eBufType;
}

BaseType *
GDALArray::ptr_duplicate()
{
    return new GDALArray(*this);
}

GDALArray::GDALArray(const string &n, BaseType *v) : Array(n, v), filename(""), hBand(0), eBufType(GDT_Unknown)
{
    BESDEBUG("gdal", " Called GDALArray::GDALArray() 1" << endl);
}

GDALArray::GDALArray(const string &name, BaseType *proto, const string &filenameIn, GDALRasterBandH hBandIn, GDALDataType eBufTypeIn) :
		Array(name, proto), filename(filenameIn), hBand(hBandIn), eBufType(eBufTypeIn)
{
	BESDEBUG("gdal", " Called GDALArray::GDALArray() 2" << endl);
}

GDALArray::GDALArray(const GDALArray &src) : Array(src)
{
	m_duplicate(src);
}

GDALArray::~GDALArray()
{
}

bool
GDALArray::read()
{
	BESDEBUG("gdal", "Entering GDALArray::read()" << endl);

    if (read_p())
        return true;

    if (name() == "northing" || name() == "easting")
    	read_map_array(this, hBand, filename);
    else
    	read_data_array(this, hBand, eBufType);

    set_read_p(true);

    return true;
}
