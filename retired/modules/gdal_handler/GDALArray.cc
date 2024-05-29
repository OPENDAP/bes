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

#include <string>

#include <BESDebug.h>

#include "GDALTypes.h"
#include "gdal_utils.h"

using namespace std;
using namespace libdap;

/************************************************************************/
/* ==================================================================== */
/*                              GDALArray                               */
/* ==================================================================== */
/************************************************************************/

void GDALArray::m_duplicate(const GDALArray &a)
{
	filename = a.filename;
	eBufType = a.eBufType;
    iBandNum = a.iBandNum;
}

BaseType *
GDALArray::ptr_duplicate()
{
    return new GDALArray(*this);
}

GDALArray::GDALArray(const string &n, BaseType *v) : Array(n, v), filename(""), eBufType(GDT_Unknown), iBandNum(0)
{
    BESDEBUG("gdal", " Called GDALArray::GDALArray() 1" << endl);
}

GDALArray::GDALArray(const string &name, BaseType *proto, const string &filenameIn, GDALDataType eBufTypeIn,int iBandNumIn) :
        Array(name, proto), filename(filenameIn), eBufType(eBufTypeIn), iBandNum(iBandNumIn)
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

    if (read_p()) return true;

    GDALDatasetH hDS = GDALOpen(filename.c_str(), GA_ReadOnly);
    if (hDS == NULL)
        throw Error(string(CPLGetLastErrorMsg()));

    try {
        if (name() == "northing" || name() == "easting")
            read_map_array(this, GDALGetRasterBand(hDS, get_gdal_band_num()), hDS);
        else
            read_data_array(this, GDALGetRasterBand(hDS, get_gdal_band_num()));

        set_read_p(true);
    }
    catch (...) {
        GDALClose(hDS);
        throw;
    }

    GDALClose(hDS);

    return true;
}
