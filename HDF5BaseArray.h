// This file is part of hdf5_handler an HDF5 file handler for the OPeNDAP
// data server.

// Author: Muqun Yang <myang6@hdfgroup.org> 

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

#ifndef _HDF5BASEARRAY_H
#define _HDF5BASEARRAY_H

// STL includes
#include <string>
#include <vector>

// DODS includes
#include <Array.h>
#include <HDF5CFUtil.h>
#include <D4Group.h>

///////////////////////////////////////////////////////////////////////////////
/// \file HDF5BaseArray.h
/// \brief A helper class that aims to reduce code redundence for different special CF derived array class
/// For example, format_constraint has been called by different CF derived array class,
/// and write_nature_number_buffer has also be used by missing variables on both 
/// HDF-EOS5 and generic HDF5 products. 
/// 
/// This class converts HDF5 array type into DAP array.
/// 
/// \author Kent Yang       (myang6@hdfgroup.org)
/// Copyright (c) 2011-2016 The HDF Group
/// 
/// All rights reserved.
///
/// 
////////////////////////////////////////////////////////////////////////////////


class HDF5BaseArray: public libdap::Array {
public:
    HDF5BaseArray(const std::string & n = "", libdap::BaseType * v = 0) :
        libdap::Array(n, v)
    {
    }

    virtual ~ HDF5BaseArray()
    {
    }

protected:
#if 0
    //virtual BaseType *ptr_duplicate();
    //virtual bool read();
#endif
    int format_constraint(int *cor, int *step, int *edg);
    void write_nature_number_buffer(int rank, int tnumelm);
    void read_data_from_mem_cache(H5DataType h5type, const std::vector<size_t> &h5_dimsizes, void*buf,const bool is_dap4);
    virtual void read_data_NOT_from_mem_cache(bool add_cache, void*buf) = 0;

    size_t INDEX_nD_TO_1D(const std::vector<size_t> &dims, const std::vector<size_t> &pos) const;

    template<typename T> int subset(    void* input,
                                        int rank,
                                        const std::vector<size_t> & dim,
                                        int start[],
                                        int stride[],
                                        int edge[],
                                        std::vector<T> *poutput,
                                        std::vector<size_t>& pos,
                                        int index);
    
    std::string check_str_sect_in_list(const std::vector<string> &, const std::string &, char) const;
    bool check_var_cache_files(const std::vector<string>&, const std::string &, const std::string &) const;
    void handle_data_with_mem_cache(H5DataType, size_t t_elems, const short cache_case, const std::string & key,const bool is_dap4);

public:
    libdap::BaseType *h5cfdims_transform_to_dap4(libdap::D4Group *grp);

};


#endif                          // _HDF5BASEARRAY_H

