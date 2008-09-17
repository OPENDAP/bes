#ifndef _HDF5Int16_h
#define _HDF5Int16_h 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <H5Ipublic.h>
#include "Int16.h"
#include "H5Git.h"

using namespace libdap;

/// A class for HDF5 signed 16 bit integer type.
/// 
/// This class provides a way to map HDF5 signed 16 bit integer to DAP Int16.
///
/// @author Hyo-Kyung Lee   (hyoklee@hdfgroup.org)
/// @author Kent Yang       (ymuqun@hdfgroup.org)
/// @author James Gallagher (jgallagher@opendap.org)
///
class HDF5Int16:public Int16 {
  private:
    hid_t dset_id;
    hid_t ty_id;

  public:

    /// Constructor
    HDF5Int16(const string &n, const string &d);
    virtual ~ HDF5Int16() { }

    /// Clone this instance.
    ///
    /// Allocate a new instance and copy *this into it. This method must
    /// perform a deep copy. 
    /// \return A newly allocated copy of this class  
    virtual BaseType *ptr_duplicate();

    /// Reads HDF5 16-bit integer data into local buffer
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
