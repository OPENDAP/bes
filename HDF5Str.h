#ifndef _hdf5str_h
#define _hdf5str_h 1

#include <string>
#include <H5Ipublic.h>

#include <limits.h>

#ifndef STR_FLAG
#define STR_FLAG 1
#endif

#ifndef STR_NOFLAG
#define STR_NOFLAG 0
#endif

#include "Str.h"
#include "H5Git.h"

using namespace libdap;

////////////////////////////////////////////////////////////////////////////////
/// A class for handling string data type in HDF5.
///
/// This class that translates HDF5 string into DAP string.
/// 
/// @author Hyo-Kyung Lee   (hyoklee@hdfgroup.org)
/// @author Kent Yang       (ymuqun@hdfgroup.org)
/// @author James Gallagher (jgallagher@opendap.org)
///
/// Copyright (c) 2007 HDF Group
/// Copyright (c) 1999 National Center for Supercomputing Applications.
/// 
/// All rights reserved.
////////////////////////////////////////////////////////////////////////////////
class HDF5Str:public Str {
 private:
    hid_t dset_id;
    hid_t ty_id;
    int array_flag;

 public:

    /// Constructor
    HDF5Str(const string &n, const string &d);
    virtual ~ HDF5Str() { }

    /// Clone this instance.
    /// 
    /// Allocate a new instance and copy *this into it. This method must
    /// perform a deep copy.
    /// \return A newly allocated copy of this class    
    virtual BaseType *ptr_duplicate();

    /// Reads HDF5 string data into local buffer  
    virtual bool read();

    /// See return_type function defined in h5dds.cc.    
    friend string return_type(hid_t datatype);

    /// returns HDF5 dataset id.      
    hid_t get_did();

    /// returns HDF5 datatype id.
    hid_t get_tid();

    /// remembers HDF5 dataset id.  
    void set_did(hid_t dset);

    /// remembers HDF5 datatype id.
    void set_tid(hid_t type);

};

#endif
