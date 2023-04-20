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

#ifndef _HDF5MISSLLARRAY_H
#define _HDF5MISSLLARRAY_H

// STL includes
#include <string>
#include <vector>

#include <libdap/Array.h>
#include "h5dmr.h"


class HDF5MissLLArray:public libdap::Array {
    public:
        HDF5MissLLArray(bool var_is_lat,int h5_rank,const eos5_grid_info_t & eg_info, const std::string & n="",  libdap::BaseType * v = nullptr):
        libdap::Array(n,v),
        is_lat(var_is_lat),
        rank(h5_rank),
        g_info(eg_info)
        {
        }
        
    ~ HDF5MissLLArray() override = default;
    libdap::BaseType *ptr_duplicate() override;

    bool read() override;

    private:
        bool is_lat;
        int rank;
        eos5_grid_info_t g_info;
    bool read_data_geo();     
    bool read_data_non_geo();
    int64_t format_constraint (int64_t *offset, int64_t *step, int64_t *count);
    size_t INDEX_nD_TO_1D(const std::vector<size_t> &dims, const std::vector<size_t> &pos) const;
    template<typename T> int subset(    void* input,
                                        int rank,
                                        const std::vector<size_t> & dim,
                                        int64_t start[],
                                        int64_t stride[],
                                        int64_t edge[],
                                        std::vector<T> *poutput,
                                        std::vector<size_t>& pos,
                                        int index);

};

#endif                          // _HDF5MISSLLARRAY_H

