// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011-2023 The HDF Group, Inc. and OPeNDAP, Inc.
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
// You can contact The HDF Group, Inc. at 410 E University Ave,
// Suite 200, Champaign, IL 61820  

////////////////////////////////////////////////////////////////////////////////
/// \file HDF5GMCFFillIndexArray.h
/// \brief This class includes the methods to read data array into DAP buffer from an HDF5 dataset for the CF option.
/// \brief It is implemented to address the netCDF dimension without the dimensional scale. Since the HDF5 default fill value
/// \brief (0 for most platforms) fills in this dimension dataset, this will cause confusions for users.
/// \brief So the handler will fill in index number(0,1,2,...) just as the handling of missing coordinate variables.
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////

#ifndef _HDF5GMCFFILLINDEXARRAY_H
#define _HDF5GMCFFILLINDEXARRAY_H

// STL includes
#include <string>
#include <vector>

#include "HDF5CF.h"
#include "HDF5BaseArray.h"


class HDF5GMCFFillIndexArray: public HDF5BaseArray {
public:
    HDF5GMCFFillIndexArray(int h5_rank, H5DataType h5_dtype, bool h5_is_dap4, const string & n = "", libdap::BaseType * v = nullptr) :
        HDF5BaseArray(n, v), rank(h5_rank), dtype(h5_dtype),is_dap4(h5_is_dap4)
    {
    }

    ~ HDF5GMCFFillIndexArray() override = default;
    libdap::BaseType *ptr_duplicate() override;
    bool read() override;
    void read_data_NOT_from_mem_cache(bool add_cache, void* /*buf FIXME Not used ever? jhrg 7/5/17*/) override;

private:
    int rank;
    H5DataType dtype;
    bool is_dap4;
};

#endif                          // _HDF5GMCFFILLINDEXARRAY_H

