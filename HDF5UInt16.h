
#ifndef _HDF5UInt16_h
#define _HDF5UInt16_h 1

#ifdef __GNUG__
#pragma interface
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <H5Ipublic.h>
#include "UInt16.h"

#
extern "C"{int get_data(hid_t dset,void *buf,char *);}

extern UInt16 * NewUInt16(const string &n = "");

class HDF5UInt16: public UInt16 {
private:
  hid_t dset_id;
  hid_t ty_id;

public:
friend string return_type(hid_t datatype); 
    HDF5UInt16(const string &n = "");
    virtual ~HDF5UInt16() {}
    virtual BaseType *ptr_duplicate();
    virtual bool read(const string &dataset, int &error);

  void set_did(hid_t dset);
  void set_tid(hid_t type);
  hid_t get_did();
  hid_t get_tid();
};

#endif

