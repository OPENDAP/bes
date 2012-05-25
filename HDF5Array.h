// This file is part of hdf5_handler a HDF5 file handler for the OPeNDAP
// data server.

// Author: Hyo-Kyung Lee <hyoklee@hdfgroup.org> and Muqun Yang
// <myang6@hdfgroup.org> 

// Copyright (c) 2009 The HDF Group, Inc. and OPeNDAP, Inc.
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

#ifndef _hdf5array_h
#define _hdf5array_h 1

#include <H5Ipublic.h>
#include <H5Rpublic.h>

#include "Array.h"
#include "h5get.h"

using namespace libdap;

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
class HDF5Array:public Array {
  private:
    int d_num_dim;
    int d_num_elm;
    hid_t d_dset_id;
    hid_t d_ty_id;
    size_t d_memneed;
    
    // Parse constraint expression and make HDF5 coordinate point location.
    // return number of elements to read. 
    int format_constraint(int *cor, int *step, int *edg);

    // Handling constraint expression on array of structure requires the
    // following linearizion function since the HDF5 array of structure is
    // 1-D (linear) while constraint expression can be multi-dimensional.
    // Based on the \param start, \param stride, and \param count,
    // this function will pick the corresponding array indexes from HDF5 array
    // and the picked indexes will be stored under \param picks.
    int linearize_multi_dimensions(int *start, int *stride, int *count,
                                   int *picks);
    hid_t mkstr(int size, H5T_str_t pad);

    bool m_array_of_structure(); // Used by read()
    bool m_array_in_structure();
    void m_insert_simple_array(hid_t s1_tid, hsize_t *size2);
    bool m_array_of_reference();
    void m_intern_plain_array_data(char *convbuf);

  public:
    /// HDF5 data type class
     H5T_class_t d_type;

    /// Constructor
    HDF5Array(const string & n, const string &d, BaseType * v);
    virtual ~ HDF5Array();

    /// Clone this instance.
    /// 
    /// Allocate a new instance and copy *this into it. This method must 
    /// perform a deep copy.
    /// \return A newly allocated copy of this class
    virtual BaseType *ptr_duplicate();

    /// Reads HDF5 array data into local buffer
    virtual bool read();

    /// See return_type function defined in h5dds.cc.
    friend string return_type(hid_t datatype);

    /// returns HDF5 dataset id.
    hid_t get_did();
    /// returns HDF5 datatype id.
    hid_t get_tid();

    /// Reads HDF5 variable length string array data into local buffer
    bool read_vlen_string(hid_t d_dset_id, hid_t d_ty_id, int nelms,
                          int *offset, int *step, int *count);

    /// remembers HDF5 dataset id.
    void set_did(hid_t dset);

    /// remembers HDF5 datatype id.
    void set_tid(hid_t type);

    /// remembers memory size needed.    
    void set_memneed(size_t need);

    /// remembers number of dimensions of this array.
    void set_numdim(int ndims);

    /// remembers number of elements in this array.  
    void set_numelm(int nelms);
};

#endif
