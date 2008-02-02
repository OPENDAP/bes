#ifndef _hdf5byte_h
#define _hdf5byte_h 1

#ifdef __GNUG__
#pragma interface
#endif

#include <string>

#include <H5Ipublic.h>

#include "Byte.h"
#include "H5Git.h"

using namespace libdap ;

/// A class for HDF5 byte type.
/// 
/// This class provides a way to map HDF5 byte to DAP Byte.
///
/// @author Hyo-Kyung Lee   (hyoklee@hdfgroup.org)
/// @author Kent Yang       (ymuqun@hdfgroup.org)
/// @author James Gallagher (jgallagher@opendap.org)
///
/// @see HDF5TypeFactory
class HDF5Byte: public Byte {

private:
  hid_t dset_id;
  hid_t ty_id;

public:
  /// Constructor
  HDF5Byte(const string &n = "");
  virtual ~HDF5Byte() {}
  
  /// Clone this instance.
  /// 
  /// Allocate a new instance and copy *this into it. This method must perform a deep copy.
  /// \return A newly allocated copy of this class  
  virtual BaseType *ptr_duplicate();

  /// Reads HDF5 byte data into local buffer
  virtual bool read(const string &dataset);
  
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

