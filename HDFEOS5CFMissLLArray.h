// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011-2016 The HDF Group, Inc. and OPeNDAP, Inc.
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
/// Copyright (C) 2011-2016 The HDF Group
///
/// All rights reserved.

#ifndef _HDFEOS5CFMISSLLARRAY_H
#define _HDFEOS5CFMISSLLARRAY_H

// STL includes
#include <string>
#include <vector>

// DODS includes
#include "HDF5CF.h"
//#include <Array.h>
#include "HDF5BaseArray.h"
#include "HDF5DiskCache.h"


class HDFEOS5CFMissLLArray:public HDF5BaseArray {
    public:
        HDFEOS5CFMissLLArray(int h5_rank, const std::string & h5_filename, const hid_t h5_fileid,  const std::string &varfullpath, CVType h5_cvartype,float h5_point_lower, float h5_point_upper, float h5_point_left, float h5_point_right, EOS5GridPRType h5_eos5_pixelreg, EOS5GridOriginType h5_eos5_origin, EOS5GridPCType h5_eos5_projcode, const std::vector<double> h5_eos5_params,int h5_eos5_zone,int h5_eos5_sphere,int h5_xdimsize, int h5_ydimsize, const std::string & n="",  libdap::BaseType * v = 0):
        HDF5BaseArray(n,v),
        rank(h5_rank),
        filename(h5_filename),
        fileid(h5_fileid),
        varname(varfullpath),
        cvartype(h5_cvartype),
        point_lower(h5_point_lower),
        point_upper(h5_point_upper),
        point_left(h5_point_left),
        point_right(h5_point_right),
        eos5_pixelreg(h5_eos5_pixelreg),
        eos5_origin(h5_eos5_origin),
        eos5_projcode(h5_eos5_projcode),
        eos5_params(h5_eos5_params),
        eos5_zone(h5_eos5_zone),
        eos5_sphere(h5_eos5_sphere),
        xdimsize(h5_xdimsize),
        ydimsize(h5_ydimsize) {
        }
        
    virtual ~ HDFEOS5CFMissLLArray() {
    }
    virtual libdap::BaseType *ptr_duplicate();
    virtual bool read();
    virtual void read_data_NOT_from_mem_cache(bool add_cache,void*buf);
    void read_data_NOT_from_mem_cache_geo(bool add_cache,void*buf);
    std::string  obtain_ll_cache_name();
    //int format_constraint (int *cor, int *step, int *edg);

    private:
        int rank;
        std::string filename;
        hid_t  fileid;
        std::string varname;
        CVType cvartype;
        float point_lower;
        float point_upper;
        float point_left;
        float point_right; 
        EOS5GridPRType eos5_pixelreg; 
        EOS5GridOriginType eos5_origin;
        EOS5GridPCType eos5_projcode; 
        std::vector<double> eos5_params;
        int eos5_zone;
        int eos5_sphere;
        int xdimsize; 
        int ydimsize;
};

#endif                          // _HDFEOS5CFMISSLLARRAY_H

