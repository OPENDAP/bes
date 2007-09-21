
// -*- C++ -*-

// HDF5 sub-class implementation for HDF5Byte,...HDF5Grid.
// The files are patterned after the subcalssing examples 
// Test<type>.c,h files.
//

#ifndef _hdf5array_h
#define _hdf5array_h 1

#ifdef __GNUG__
#pragma interface
#endif

#include <H5Ipublic.h>

#include "Array.h"
/*
  extern "C" {
  int get_data(hid_t dset,void *buf,char *);
  int get_slabdata(hid_t,int *,int*,int*,int,hsize_t,void *,char *);
  size_t H5Tget_size(hid_t type_id);
  herr_t H5Dclose(hid_t dataset_id);
  int get_strdata(hid_t,int,char*,char*,char*);
  int check_h5str(hid_t);
  }
*/
#include "H5Git.h"

class HDF5Array: public Array {
private:
  int d_num_dim;
  int d_num_elm;
  hid_t d_dset_id;
  hid_t d_ty_id;
  size_t d_memneed;

  
  int  format_constraint(int *cor, int *step, int *edg);
  int  linearize_multi_dimensions(int* start, int* stride, int* count, int* picks);
  hid_t mkstr(int size, H5T_str_t pad);
  
public:
  
  H5T_class_t d_type;
  
  HDF5Array(const string &n = "", BaseType *v = 0);
  virtual ~HDF5Array();

  virtual BaseType *ptr_duplicate();
  virtual bool read(const string &dataset);

  hid_t get_did();
  hid_t get_tid();
  
  bool read_vlen_string(hid_t d_dset_id, hid_t d_ty_id, int nelms, int* offset, int* step, int* count);
  
  void set_did(hid_t dset);
  void set_tid(hid_t type);
  void set_memneed(size_t need);
  void set_numdim(int ndims);
  void set_numelm(int nelms);
  
  friend  string return_type(hid_t datatype);  
};

#endif




