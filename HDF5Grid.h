#ifndef _HDF5Grid_h
#define _HDF5Grid_h 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <H5Ipublic.h>

#include "Grid.h"
#include "H5Git.h"

using namespace libdap;

/// A HDF5Grid class.
/// 
/// This class provides a way to map HDF5 array in Grid format to DAP Grid.
///
/// @author Hyo-Kyung Lee (hyoklee@hdfgroup.org)
///
/// @see Grid HDF5TypeFactory
class HDF5Grid:public Grid {

  private:
    hid_t dset_id;
    hid_t ty_id;
  public:

    /// Constructor  
     HDF5Grid(const string & n = "");
     virtual ~ HDF5Grid();

    /// Clone this instance.
    /// 
    /// Allocate a new instance and copy *this into it. This method must perform a deep copy.
    /// \return A newly allocated copy of this class  
    virtual BaseType *ptr_duplicate();

    /// Reads data array and map arrays in from this Grid.
    virtual bool read(const string & dataset);

    /// See print_type function in h5das.cc
    friend string print_type(hid_t datatype);

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
