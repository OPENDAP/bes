#ifndef _hdf5array_h
#define _hdf5array_h 1

#include <H5Ipublic.h>
#include <H5Rpublic.h>

#include "Array.h"
#include "H5Git.h"

using namespace libdap;

////////////////////////////////////////////////////////////////////////////////
/// A class for handling all types of array in HDF5.
///
/// This class converts HDF5 array type into DAP array.
/// 
/// @author Hyo-Kyung Lee   (hyoklee@hdfgroup.org)
/// @author Kent Yang       (ymuqun@hdfgroup.org)
/// @author James Gallagher (jgallagher@opendap.org)
///
/// @see HDF5TypeFactory
///
/// Copyright (c) 2007 HDF Group
/// Copyright (c) 1999 National Center for Supercomputing Applications.
/// 
/// All rights reserved.
////////////////////////////////////////////////////////////////////////////////
class HDF5Array:public Array {
  private:
    int d_num_dim;
    int d_num_elm;
    hid_t d_dset_id;
    hid_t d_ty_id;
    size_t d_memneed;


    int format_constraint(int *cor, int *step, int *edg);
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
     HDF5Array(const string & n = "", BaseType * v = 0);
     virtual ~ HDF5Array();

    /// Clone this instance.
    /// 
    /// Allocate a new instance and copy *this into it. This method must 
    /// perform a deep copy.
    /// \return A newly allocated copy of this class
    virtual BaseType *ptr_duplicate();

    /// Reads HDF5 array data into local buffer
    virtual bool read(const string & dataset);

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
