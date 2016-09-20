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

////////////////////////////////////////////////////////////////////////////////
/// \file HDF5CFArray.h
/// \brief This class includes the methods to read data array into DAP buffer from an HDF5 dataset for the CF option.
///
/// In the future, this may be merged with the dddefault option.
/// \author Muqun Yang <myang6@hdfgroup.org>
///
////////////////////////////////////////////////////////////////////////////////

#ifndef _HDF5CFARRAY_H
#define _HDF5CFARRAY_H

// STL includes
#include <string>
#include <vector>

// DODS includes
#include "HDF5CF.h"
#include <Array.h>

using namespace libdap;

class HDF5CFArray:public Array {
    public:
        HDF5CFArray(int h5_rank, 
                    const hid_t h5_file_id,
                    const string & h5_filename, 
                    H5DataType h5_dtype, 
                    const string &varfullpath, 
                    const size_t h5_total_elems,
                    const CVType h5_cvtype,
                    const string & n="",  
                    BaseType * v = 0):
                    Array(n,v),
                    rank(h5_rank),
                    fileid(h5_file_id),
                    filename(h5_filename),
                    dtype(h5_dtype),
                    total_elems(h5_total_elems),
                    cvtype(h5_cvtype),
                    varname(varfullpath) 
        {
        }
        
    virtual ~ HDF5CFArray() {
    }
    virtual BaseType *ptr_duplicate();
    virtual bool read();
    void read_data_from_mem_cache(vector<char>&buf);
    void read_data_from_file(bool add_cache,vector<char>&buf);
    int format_constraint (int *cor, int *step, int *edg);

  private:
        int rank;
        hid_t fileid;
        string filename;
        H5DataType dtype;
        string varname;
        size_t total_elems;
        CVType cvtype;
};

#endif                          // _HDF5CFARRAY_H

