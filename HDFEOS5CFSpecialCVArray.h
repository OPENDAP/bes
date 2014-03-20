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
/// \file HDFEOS5CFSpecialCVArray.h
/// \brief This class specifies the retrieval of special coordinate variable  values for HDF-EOS5 products
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2013 The HDF Group
///
/// All rights reserved.

#ifndef _HDFEOS5CFSpecialCVARRAY_H
#define _HDFEOS5CFSpecialCVARRAY_H

// STL includes
#include <string>
#include <vector>

// DODS includes
#include "HDF5CF.h"
#include "HDF5BaseArray.h"

using namespace libdap;

class HDFEOS5CFSpecialCVArray:public HDF5BaseArray {
    public:
        HDFEOS5CFSpecialCVArray(int rank, /*const string & filename*/ const hid_t fileid, H5DataType dtype, int num_elm, const string &varfullpath, const string & n="",  BaseType * v = 0):
        HDF5BaseArray(n,v),
        rank(rank),
        //filename(filename),
        fileid(fileid),
        dtype(dtype),
        total_num_elm(num_elm),
        varname(varfullpath) {
    }

    virtual ~ HDFEOS5CFSpecialCVArray() {
    }
    virtual BaseType *ptr_duplicate();
    virtual bool read();

    private:
        int rank;
        //string filename;
        hid_t fileid;
        H5DataType dtype;
        int total_num_elm;
        string varname;
};

#endif // _HDFEOS5CFSpecialCVARRAY_H
