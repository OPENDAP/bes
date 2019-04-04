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
/// \file HDFEOS5CFSpecialCVArray.h
/// \brief This class specifies the retrieval of special coordinate variable  values for HDF-EOS5 products
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2016 The HDF Group
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


class HDFEOS5CFSpecialCVArray:public HDF5BaseArray {
    public:
        HDFEOS5CFSpecialCVArray(int h5_rank, const std::string & h5_filename, const hid_t h5_fileid, H5DataType h5_dtype, int h5_num_elm, const std::string &varfullpath, const std::string & n="",  libdap::BaseType * v = 0):
        HDF5BaseArray(n,v),
        rank(h5_rank),
        filename(h5_filename),
        fileid(h5_fileid),
        dtype(h5_dtype),
        total_num_elm(h5_num_elm),
        varname(varfullpath) {
    }

    virtual ~ HDFEOS5CFSpecialCVArray() {
    }
    virtual libdap::BaseType *ptr_duplicate();
    virtual bool read();
    virtual void read_data_NOT_from_mem_cache(bool add_cache,void*buf);

    private:
        int rank;
        std::string filename;
        hid_t fileid;
        H5DataType dtype;
        int total_num_elm;
        std::string varname;
};

#endif // _HDFEOS5CFSpecialCVARRAY_H
