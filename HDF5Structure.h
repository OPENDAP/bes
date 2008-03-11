#ifndef _HDF5Structure_h
#define _HDF5Structure_h 1

#ifdef _GNUG_
#pragma interface
#endif

#include <string>
#include <H5Ipublic.h>
#include "Structure.h"
#include "H5Git.h"

using namespace libdap;

/// A HDF5Structure class.
/// This class converts HDF5 compound type into DAP structure.
///
/// @author James Gallagher
/// @author Hyo-Kyung Lee
///
/// @see Structure 
class HDF5Structure:public Structure {
  private:
    hid_t dset_id;
    hid_t ty_id;
    int array_index;
    int array_size;             // size constrained by constraint expression
    int array_entire_size;      // entire size in case of array of structure

  public:
    /// Constructor
     HDF5Structure(const string & n = "");
     virtual ~ HDF5Structure();

    /// Assignment operator for dynamic cast into generic Structure.
     HDF5Structure & operator=(const HDF5Structure & rhs);

    /// Clone this instance.
    /// 
    /// Allocate a new instance and copy *this into it. This method must perform a deep copy.
    /// \return A newly allocated copy of this class  
    virtual BaseType *ptr_duplicate();

    /// Reads HDF5 structure data by calling each member's read method in this structure.
    virtual bool read(const string & dataset);

    /// See return_type function defined in h5dds.cc.  
    friend string return_type(hid_t datatype);

    /// returns HDF5 dataset id.  
    hid_t get_did();
    /// returns HDF5 datatype id.
    hid_t get_tid();

    /// returns the array index of this Structure if it's a part of array of structures.
    int get_array_index();
    /// returns the array size for subsetting if it's a part of array of structures.
    int get_array_size();
    /// returns the entire array size of this Structure if it's a part of array of structures.
    int get_entire_array_size();

    /// remembers HDF5 datatype id.  
    void set_did(hid_t dset);
    /// remembers HDF5 datatype id.
    void set_tid(hid_t type);

    /// remembers the array index of this Structure if it's a part of array of structures.
    void set_array_index(int i);
    /// remembers the array size for subsetting if it's a part of array of structures.
    void set_array_size(int i);
    /// returns the entire array size of this Structure if it's a part of array of structures.  
    void set_entire_array_size(int i);


};

#endif
