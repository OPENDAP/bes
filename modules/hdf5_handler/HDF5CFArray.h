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
/// \file HDF5CFArray.h
/// \brief This class includes the methods to read data array into DAP buffer from an HDF5 dataset for the CF option.
///
/// \author Keng Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////

#ifndef _HDF5CFARRAY_H
#define _HDF5CFARRAY_H

// STL includes
#include <string>
#include <vector>

// DODS includes
#include "HDF5CF.h"
#include "HDF5BaseArray.h"
#include "HDF5DiskCache.h"
#include <libdap/D4Group.h>
#include <libdap/D4Attributes.h>



class HDF5CFArray:public HDF5BaseArray {
    public:
        HDF5CFArray(int h5_rank, 
                    const hid_t h5_file_id,
                    const std::string & h5_filename, 
                    H5DataType h5_dtype, 
                    const std::vector<size_t>& h5_dimsizes,
                    const std::string &varfullpath, 
                    const size_t h5_total_elems,
                    const CVType h5_cvtype,
                    const bool h5_islatlon,
                    const float h5_comp_ratio,
                    const bool h5_is_dap4,
                    const std::string & n="",  
                    libdap::BaseType * v = nullptr):
                    HDF5BaseArray(n,v),
                    rank(h5_rank),
                    fileid(h5_file_id),
                    filename(h5_filename),
                    dtype(h5_dtype),
                    dimsizes(h5_dimsizes),
                    varname(varfullpath),
                    total_elems(h5_total_elems),
                    cvtype(h5_cvtype),
                    islatlon(h5_islatlon),
                    comp_ratio(h5_comp_ratio),
                    is_dap4(h5_is_dap4)
        {
        }
        
    ~ HDF5CFArray() override = default;
    
    libdap::BaseType *ptr_duplicate() override;
    bool read() override;
    void read_data_NOT_from_mem_cache(bool add_cache,void*buf) override;

    // Currently this routine is only used for 64-bit integer mapping to DAP4.
    libdap::BaseType *h5cfdims_transform_to_dap4_int64(libdap::D4Group *root);
#if 0
    //libdap::BaseType *h5cfdims_transform_to_dap4(libdap::D4Group *root);
    //void read_data_from_mem_cache(void*buf);
    //void read_data_from_file(bool add_cache,void*buf);
    //int format_constraint (int *cor, int *step, int *edg);
#endif

  private:
        int rank;
        hid_t fileid;
        std::string filename;
        H5DataType dtype;
        std::vector<size_t>dimsizes;
        std::string varname;
        size_t total_elems;
        CVType cvtype;
        bool islatlon;
        float comp_ratio;
        bool is_dap4;
        bool valid_disk_cache() const;
        bool valid_disk_cache_for_compressed_data(short dtype_size) const;
        bool obtain_cached_data(HDF5DiskCache*,const std::string&,int, std::vector<int64_t>&,std::vector<int64_t>&,size_t,short);
        void write_data_to_cache(hid_t dset_id, hid_t dspace_id,hid_t mspace_id,hid_t memtype, const std::string& cache_fpath,short dtype_size,const std::vector<char> &buf, int64_t nelms);
};

#endif                          // _HDF5CFARRAY_H

