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

#ifndef _GDALTypes_h
#define _GDALTypes_h 1

#include <gdal.h>

#include <libdap/Array.h>
#include <libdap/Grid.h>

/************************************************************************/
/*                              GDALArray                               */
/************************************************************************/

class GDALArray: public libdap::Array {
    string filename;
    GDALDataType eBufType;
    int iBandNum;

    void m_duplicate(const GDALArray &a);

public:
    GDALArray(const string &n = "", BaseType *v = 0);
    GDALArray(const string &name, BaseType *proto, const string &filenameIn, GDALDataType eBufTypeIn, int iBandNumIn);
    GDALArray(const GDALArray &src);
    virtual ~GDALArray();

    virtual BaseType *ptr_duplicate();

    virtual int get_gdal_band_num() const { return iBandNum; }
    virtual GDALDataType get_gdal_buf_type() const { return eBufType; }

    virtual bool read();
};

/************************************************************************/
/*                               GDALGrid                               */
/************************************************************************/

class GDALGrid: public libdap::Grid {
    string filename;
    void m_duplicate(const GDALGrid &g);

public:
    GDALGrid(const GDALGrid &rhs);
    GDALGrid(const string &filenameIn, const string &name);

    virtual ~GDALGrid();

    GDALGrid &operator=(const GDALGrid &rhs);

    virtual BaseType *ptr_duplicate();

    virtual bool read();
};

#endif // ndef _GDALTypes_h


