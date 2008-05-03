#ifndef _hdf5eosarray_h
#define _hdf5eosarray_h 1

#include <H5Ipublic.h>

#include "Array.h"
#include "H5Git.h"

using namespace libdap;

////////////////////////////////////////////////////////////////////////////////
/// A special class for handling array in NASA EOS HDF5 file.
///
/// This class converts NASA EOS HDF5 array type into DAP array.
/// 
/// @author Hyo-Kyung Lee   (hyoklee@hdfgroup.org)
/// @author Kent Yang       (ymuqun@hdfgroup.org)
/// @author James Gallagher (jgallagher@opendap.org)
///
/// @see HDF5TypeFactory H5EOS
///
/// Copyright (c) 2007 HDF Group
/// Copyright (c) 1999 National Center for Supercomputing Applications.
/// 
/// All rights reserved.
////////////////////////////////////////////////////////////////////////////////
class HDF5ArrayEOS:public Array {

  private:
    int d_num_dim;
    int d_num_elm;
    hid_t d_dset_id;
    hid_t d_ty_id;
    size_t d_memneed;

    int format_constraint(int *cor, int *step, int *edg);
    dods_float32 *get_dimension_data(dods_float32 * buf, int start,
                                     int stride, int stop, int count);

  public:
    /// Constructor
     HDF5ArrayEOS(const string & n = "", BaseType * v = 0);

     virtual ~ HDF5ArrayEOS();

    /// Clone this instance.
    /// 
    /// Allocate a new instance and copy *this into it. This method must perform a deep copy.
    /// \return A newly allocated copy of this class
    virtual BaseType *ptr_duplicate();

    /// Reads HDF5 NASA EOS array data into local buffer
    virtual bool read(const string & dataset);

    /// remembers memory size needed.
    void set_memneed(size_t need);

    /// remembers number of dimensions of this array.
    void set_numdim(int ndims);

    /// remembers number of elements in this array.  
    void set_numelm(int nelms);


    /// See return_type function defined in h5dds.cc.
    friend string return_type(hid_t datatype);
};

#endif
