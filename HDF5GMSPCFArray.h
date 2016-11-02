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
/// \file HDF5GMSPCFArray.h
/// \brief This class specifies the retrieval of data values for special  HDF5 products
/// Currently this only applies to ACOS level 2 and OCO2 level 1B data.
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2016 The HDF Group
///
/// All rights reserved.

#ifndef _HDF5GMSPCFARRAY_H
#define _HDF5GMSPCFARRAY_H

// STL includes
#include <string>
#include <vector>

// DODS includes
#include "HDF5CF.h"
//#include <Array.h>
#include "HDF5BaseArray.h"

using namespace libdap;

class HDF5GMSPCFArray:public HDF5BaseArray {
    public:
        HDF5GMSPCFArray(int h5_rank, const string & h5_filename, const hid_t h5_fileid, H5DataType h5_dtype, const string &varfullpath, H5DataType h5_otype, int h5_sdbit, int h5_numofdbits, const string & n="",  BaseType * v = 0):
        HDF5BaseArray(n,v),
        rank(h5_rank),
        filename(h5_filename),
        fileid(h5_fileid),
        dtype(h5_dtype),
        varname(varfullpath),
        otype(h5_otype),
        sdbit(h5_sdbit),
        numofdbits(h5_numofdbits) 
        
{
}
        
        virtual ~ HDF5GMSPCFArray() {
        }
        virtual BaseType *ptr_duplicate();
        virtual bool read();
        //int format_constraint (int *cor, int *step, int *edg);
        virtual void read_data_NOT_from_mem_cache(bool add_cache,void*buf);


    private:
        int rank;
        string filename;
        hid_t fileid;
        H5DataType dtype;
        string varname;
        H5DataType otype;
        int sdbit;
        int numofdbits;
};

#endif                          // _HDF5GMSPCFARRAY_H

