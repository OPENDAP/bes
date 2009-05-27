#ifndef _HDF5GridEOS_h
#define _HDF5GridEOS_h 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <H5Ipublic.h>

#include "Grid.h"
#include "H5Git.h"

using namespace libdap;

////////////////////////////////////////////////////////////////////////////////
/// A Grid subclass for handling HDF-EOS5 data.
/// 
/// This class synthesizes DAP Grids from NASA EOS HDF5 arrays that can be mapped to Grid.
///
/// @author Hyo-Kyung Lee (hyoklee@hdfgroup.org)
///
/// Copyright (c) The 2007-2009 HDF Group
////////////////////////////////////////////////////////////////////////////////
class HDF5GridEOS:public Grid {

private:
    hid_t dset_id;
    hid_t ty_id;

public:

    /// Constructor
    HDF5GridEOS(const string &n, const string &d);
    virtual ~ HDF5GridEOS();

    /// Clone this instance.
    /// 
    /// Allocate a new instance and copy *this into it. This method must perform a deep copy.
    /// \return A newly allocated copy of this class    
    virtual BaseType *ptr_duplicate();

    /// Reads data array and map arrays in from this EOS Grid.
    ///
    /// This function is different from normal Grid read function
    /// since maps data are resulted from parsing the metadata.
    virtual bool read();

    /// returns HDF5 dataset id.    
    hid_t get_did();

    /// returns HDF5 datatype id.
    hid_t get_tid();

    /// gets map data from H5EOS class that parsed the StructMetadata and held the computed
    /// map data array.
    dods_float32 *get_dimension_data(dods_float32 * buf, int start,
                                     int stride, int stop, int count);

    /// reads map data from this EOS Grid.
    void read_dimension(Array * a);

    /// remembers HDF5 dataset id.  
    void set_did(hid_t dset);

    /// remembers HDF5 datatype id.
    void set_tid(hid_t type);

};

#endif
