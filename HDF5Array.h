// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org> and Muqun Yang
// <myang6@hdfgroup.org> 

// Copyright (c) 2009-2016 The HDF Group, Inc. and OPeNDAP, Inc.
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

#ifndef _hdf5array_h
#define _hdf5array_h 1

#include <H5Ipublic.h>
#include <H5Rpublic.h>

#include "Array.h"
#include "h5get.h"


///////////////////////////////////////////////////////////////////////////////
/// \file HDF5Array.h
/// \brief A class for handling all types of array in HDF5 for the default option.
///
/// This class converts HDF5 array type into DAP array.
/// 
/// \author Hyo-Kyung Lee   (hyoklee@hdfgroup.org)
/// \author Kent Yang       (myang6@hdfgroup.org)
/// \author James Gallagher (jgallagher@opendap.org)
///
////////////////////////////////////////////////////////////////////////////////
class HDF5Array:public libdap::Array {
  private:
    int d_num_dim;
    int d_num_elm;
    
//    hid_t d_dset_id;
//    hid_t d_ty_id;
    hsize_t d_memneed;
    string var_path;
    
    // Parse constraint expression and make HDF5 coordinate point location.
    // return number of elements to read. 
    int format_constraint(int *cor, int *step, int *edg);

    hid_t mkstr(int size, H5T_str_t pad);

    //bool m_array_of_structure(vector<char>&,bool,int,int,int*,int*,int*); // Used by read()
    bool m_array_of_structure(hid_t dsetid, std::vector<char>&values,bool has_values,int values_offset,int nelms,int* offset,int*count,int*step);
    bool m_array_in_structure();
    bool m_array_of_reference(hid_t dset_id,hid_t dtype_id);
    void m_intern_plain_array_data(char *convbuf,hid_t memtype);
    void m_array_of_atomic(hid_t, hid_t,int,int*,int*,int*);

    void do_array_read(hid_t dset_id,hid_t dtype_id,std::vector<char>&values,bool has_values,int values_offset,int nelms,int* offset,int* count, int* step);

    bool do_h5_array_type_read(hid_t dsetid, hid_t memb_id,std::vector<char>&values,bool has_values,int values_offset, int at_nelms,int* at_offset,int*at_count,int* at_step);

    inline int INDEX_nD_TO_1D (const std::vector < int > &dims,
                                const std::vector < int > &pos);
    bool obtain_next_pos(std::vector<int>& pos, std::vector<int>&start,std::vector<int>&end,std::vector<int>&step,int rank_change);

    template<typename T>  int subset(
            const T input[],
            int rank,
            std::vector<int> & dim,
            int start[],
            int stride[],
            int edge[],
            std::vector<T> *poutput,
            std::vector<int>& pos,
            int index);
    friend class HDF5Structure;
  public:

    /// Constructor
    HDF5Array(const std::string & n, const std::string &d, libdap::BaseType * v);
    virtual ~ HDF5Array();

    /// Clone this instance.
    /// 
    /// Allocate a new instance and copy *this into it. This method must 
    /// perform a deep copy.
    /// \return A newly allocated copy of this class
    virtual libdap::BaseType *ptr_duplicate();

    /// Reads HDF5 array data into local buffer
    virtual bool read();

    /// See return_type function defined in h5dds.cc.
    friend std::string return_type(hid_t datatype);

    /// remembers memory size needed.    
    void set_memneed(size_t need);

    /// remembers number of dimensions of this array.
    void set_numdim(int ndims);

    /// remembers number of elements in this array.  
    void set_numelm(int nelms);

    void set_varpath(const std::string vpath) { var_path = vpath;}
    libdap::BaseType *h5dims_transform_to_dap4(libdap::D4Group *root);
};

#endif
