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
/// \file HDF5GMCFMissLLArray.h
/// \brief This class specifies the retrieval of the missing lat/lon values for general HDF5 products
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2016 The HDF Group
///
/// All rights reserved.

#ifndef _HDF5GMCFMissLLARRAY_H
#define _HDF5GMCFMissLLARRAY_H

// STL includes
#include <string>
#include <vector>

// DODS includes
#include "HDF5CF.h"
//#include <libdap/Array.h>
#include "HDF5BaseArray.h"


class HDF5GMCFMissLLArray:public HDF5BaseArray {
    public:
        HDF5GMCFMissLLArray(int h5_rank, const string & h5_filename, const hid_t h5_fileid, H5DataType h5_dtype, const string &varfullpath, H5GCFProduct h5_product_type, CVType h5_cvartype, const string & n="",  libdap::BaseType * v = nullptr):
        HDF5BaseArray(n,v),
        rank(h5_rank),
        filename(h5_filename),
        fileid(h5_fileid),
        dtype(h5_dtype),
        varname(varfullpath),
        product_type(h5_product_type),
        cvartype(h5_cvartype)
    {
    }
        
    ~ HDF5GMCFMissLLArray() override = default;
    
    libdap::BaseType *ptr_duplicate() override;
    bool read() override;
    

    private:
        int rank;
        string filename;
        hid_t fileid;
        H5DataType dtype;
        string varname;
        H5GCFProduct product_type;
        CVType cvartype;    

    //template<class T> 
    template<typename T> 
    void obtain_ll_attr_value(hid_t file_id, hid_t s_root_id,const std::string& s_attr_name, T& attr_value,std::vector<char> & str_attr_value );
    void read_data_NOT_from_mem_cache(bool add_cache,void*buf) override;
    void obtain_aqu_obpg_l3_ll(int* offset,int* step,int nelms,bool add_cache, void*buf);

    void obtain_gpm_l3_ll(int* offset,int* step,int nelms,bool add_cache, void*buf);
    void obtain_gpm_l3_new_grid_info(hid_t fileid,vector<char>& grid_info_value1, vector<char>& grid_info_value2);
    void obtain_lat_lon_info(const vector<char>& grid_info_value1,
                             const vector<char>& grid_info_value2,int& latsize,int& lonsize,
                             float& lat_start,float& lon_start,float& lat_res,float& lon_res);
#if 0
    //void send_gpm_l3_ll_to_dap(const vector<char>& grid_info_value,int* offset,int* step,int nelms,bool add_cache, void*buf);
#endif
    void send_gpm_l3_ll_to_dap(const int latsize,const int lonsize,float lat_start,float lon_start,float lat_res, float lon_res, 
                               const int* offset, const int* step, const int nelms, const bool add_cache, void*buf);
};


#endif                          // _HDF5GMCFMissLLARRAY_H

