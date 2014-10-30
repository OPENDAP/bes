// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011-2013 The HDF Group, Inc. and OPeNDAP, Inc.
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
//
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  

/////////////////////////////////////////////////////////////////////////////
/// \file HDFEOS5CFMissLLArray.h
/// \brief This class specifies the retrieval of the missing lat/lon values for HDF-EOS5 products
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2013 The HDF Group
///
/// All rights reserved.

#ifndef _HDFEOS5CFMISSLLARRAY_H
#define _HDFEOS5CFMISSLLARRAY_H

// STL includes
#include <string>
#include <vector>

// DODS includes
#include "HDF5CF.h"
#include <Array.h>

using namespace libdap;

class HDFEOS5CFMissLLArray:public Array {
    public:
        HDFEOS5CFMissLLArray(int rank, const string & filename, const hid_t fileid,  const string &varfullpath, CVType cvartype,float point_lower, float point_upper, float point_left, float point_right, EOS5GridPRType eos5_pixelreg, EOS5GridOriginType eos5_origin, EOS5GridPCType eos5_projcode, int xdimsize, int ydimsize, const string & n="",  BaseType * v = 0):
        Array(n,v),
        rank(rank),
        filename(filename),
        fileid(fileid),
        varname(varfullpath),
        cvartype(cvartype),
        point_lower(point_lower),
        point_upper(point_upper),
        point_left(point_left),
        point_right(point_right),
        eos5_pixelreg(eos5_pixelreg),
        eos5_origin(eos5_origin),
        eos5_projcode(eos5_projcode),
        xdimsize(xdimsize),
        ydimsize(ydimsize) {
        }
        
    virtual ~ HDFEOS5CFMissLLArray() {
    }
    virtual BaseType *ptr_duplicate();
    virtual bool read();
    int format_constraint (int *cor, int *step, int *edg);

    private:
        int rank;
        string filename;
        hid_t  fileid;
        string varname;
        CVType cvartype;
        float point_lower;
        float point_upper;
        float point_left;
        float point_right; 
        EOS5GridPRType eos5_pixelreg; 
        EOS5GridOriginType eos5_origin;
        EOS5GridPCType eos5_projcode; 
        int xdimsize; 
        int ydimsize;
};

#endif                          // _HDFEOS5CFMISSLLARRAY_H

