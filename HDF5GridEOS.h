
#ifndef _HDF5GridEOS_h
#define _HDF5GridEOS_h 1

#ifdef _GNUG_
#pragma interface
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <H5Ipublic.h>
#include "Grid.h"

#include "H5Git.h"

// extern "C" int get_data(hid_t dset,void *buf,char *);

class HDF5GridEOS: public Grid {

private:
  hid_t dset_id;
  hid_t ty_id;
public:
  friend string print_type(hid_t datatype);  
  HDF5GridEOS(const string &n = "");
  virtual ~HDF5GridEOS();
    
  virtual BaseType *ptr_duplicate();

  hid_t         get_did();
  hid_t         get_tid();
  dods_float32* get_dimension_data(dods_float32* buf, int start, int stride, int stop, int count);
  virtual bool  read(const string &dataset);
  void          read_dimension(Array* a); // <hyokyung 2007.04. 6. 14:53:27>
  void          set_did(hid_t dset);
  void          set_tid(hid_t type);
};

#endif





