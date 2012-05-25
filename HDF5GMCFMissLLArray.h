// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011 The HDF Group, Inc. and OPeNDAP, Inc.
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  

/////////////////////////////////////////////////////////////////////////////
/// \file HDF5GMCFMissLLArray.h
/// \brief This class specifies the retrieval of the missing lat/lon values for general HDF5 products
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011 The HDF Group
///
/// All rights reserved.

#ifndef _HDF5GMCFMissLLARRAY_H
#define _HDF5GMCFMissLLARRAY_H

// STL includes
#include <string>
#include <vector>

// DODS includes
#include "HDF5CF.h"
#include <Array.h>

using namespace libdap;

class HDF5GMCFMissLLArray:public Array {
    public:
        HDF5GMCFMissLLArray(int rank, const string & filename, H5DataType dtype, const string &varfullpath, H5GCFProduct product_type, CVType cvartype, const string & n="",  BaseType * v = 0):
        Array(n,v),
        rank(rank),
        filename(filename),
        dtype(dtype),
        varname(varfullpath),
        product_type(product_type),
        cvartype(cvartype)
    {
    }
        
    virtual ~ HDF5GMCFMissLLArray() {
    }
    virtual BaseType *ptr_duplicate();
    virtual bool read();
    int format_constraint (int *cor, int *step, int *edg);
    template<class T> 
    void obtain_ll_attr_value(hid_t file_id, hid_t s_root_id,string s_attr_name, T& attr_value);

    private:
        int rank;
        string filename;
        H5DataType dtype;
        string varname;
        H5GCFProduct product_type;
        CVType cvartype;    
};

#endif                          // _HDF5GMCFMissLLARRAY_H

