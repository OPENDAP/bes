////////////////////////////////////////////////////////////////////////////////
//
// This file is part of h5_dap_handler, A C++ implementation of the DAP handler
// for HDF5 data.
//
//
// Copyright (c) 2007 HDF Group, Inc.
//
// Copyright (c) 2005 OPeNDAP, Inc.
//
// All rights reserved.
////////////////////////////////////////////////////////////////////////////////
#ifndef _HDF5Url_h
#define _HDF5Url_h 1

#ifdef __GNUG__
#pragma interface
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <H5Ipublic.h>

#include "Url.h"
#include "H5Git.h"
////////////////////////////////////////////////////////////////////////////////
/// A HDF5Url class.
/// This class generates DAP URL type.
///
/// There is a no URL data type in HDF5 so this class is not used in .
/// \author James Gallagher <jgallagher@opendap.org>
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
///
/// \see String
////////////////////////////////////////////////////////////////////////////////
class HDF5Url: public Url {
  
private:
  hid_t dset_id;
  hid_t ty_id;
  
public:
  
  /// Constructor
  HDF5Url(const string &n = "");
  virtual ~HDF5Url() {}

  /// Clone this instance.
  /// 
  /// Allocate a new instance and copy *this into it. This method must perform a deep copy.
  /// \return A newly allocated copy of this class      
  virtual BaseType *ptr_duplicate();
  
  /// Reads HDF5 reference data into local buffer as a string
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

