
// -*- C++ -*-

// HDF5 sub-class implementation for HDF5Byte,...HDF5Grid.
// The files are patterned after the subcalssing examples 
// Test<type>.c,h files.
//

#ifndef _HDF5Array_h
#define _NCArray_h 1

#ifdef __GNUG__
#pragma interface
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <H5Ipublic.h>
//#include <H5Tpublic.h>
#include "Array.h"
#include "HDF5Str.h"

extern "C"{int get_data(hid_t dset,void *buf,char *);}
extern "C"{int get_slabdata(hid_t,int *,int*,int*,int,hsize_t,void *,char *);}
extern "C"{size_t H5Tget_size(hid_t type_id);}
extern "C" {herr_t H5Dclose(hid_t dataset_id);}
extern "C"{int get_strdata(hid_t,int,char*,char*,char*);}
extern "C"{int check_h5str(hid_t);}
extern Array * NewArray(const string &n = "", BaseType *v = 0);


class HDF5Array: public Array {

private:
  int num_dim;
  int num_elm;
  hid_t dset_id;
  hid_t ty_id;
  size_t memneed;

public:
friend  string return_type(hid_t datatype);  
    HDF5Array(const string &n = "", BaseType *v = 0);
    virtual ~HDF5Array();

    virtual BaseType *ptr_duplicate();

   virtual bool read(const string &dataset, int &error);
 int format_constraint(int *cor, int *step, int *edg);

  //   virtual bool read(String dataset, String var_name, String constraint);
//    virtual bool read_val(void *stuff);

  void set_did(hid_t dset);
  void set_tid(hid_t type);
  void set_memneed(size_t need);
  void set_numdim(int ndims);
  void set_numelm(int nelms);
  hid_t get_did();
  hid_t get_tid();
};

#endif




