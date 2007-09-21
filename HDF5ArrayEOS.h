// -*- C++ -*-

// HDF5 sub-class implementation for HDF5Byte,...HDF5Grid.
// The files are patterned after the subcalssing examples 
// Test<type>.c,h files.
//

#ifndef _hdf5eosarray_h
#define _hdf5eosarray_h 1

#ifdef __GNUG__
#pragma interface
#endif

#include <H5Ipublic.h>

#include "Array.h"
#include "H5Git.h"

class HDF5ArrayEOS: public Array {
  
private:
  int d_num_dim;
  int d_num_elm;
  hid_t d_dset_id;
  hid_t d_ty_id;
  size_t d_memneed;

  int  format_constraint(int *cor, int *step, int *edg);
  
public:
  HDF5ArrayEOS(const string &n = "", BaseType *v = 0);
  virtual ~HDF5ArrayEOS();

  virtual BaseType *ptr_duplicate();
  virtual bool read(const string &dataset);

  void set_memneed(size_t need);
  void set_numdim(int ndims);
  void set_numelm(int nelms);
  dods_float32* get_dimension_data(dods_float32* buf, int start, int stride, int stop, int count);  
  friend  string return_type(hid_t datatype);  
};

#endif




