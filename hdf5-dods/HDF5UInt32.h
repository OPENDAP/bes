

#ifndef _HDF5UInt32_h
#define _HDF5UInt32_h 1

#ifdef __GNUG__
#pragma interface
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <H5Ipublic.h>
#include "UInt32.h"

extern "C" int get_data(hid_t dset,void *buf,char*);

extern UInt32 * NewUInt32(const string &n = "");

class HDF5UInt32: public UInt32 {

private:
  hid_t dset_id;
  hid_t ty_id;

public:
    friend string return_type(hid_t datatype);
    HDF5UInt32(const string &n = "");
    virtual ~HDF5UInt32() {}

    virtual BaseType *ptr_duplicate();
    
    virtual bool read(const string &dataset);

    void set_did(hid_t dset);
    void set_tid(hid_t type);
    hid_t get_did();
    hid_t get_tid();
};

#endif

