
#ifndef _HDF5Byte_h
#define _HDF5Byte_h 1

#ifdef __GNUG__
#pragma interface
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <H5Ipublic.h>
#include "Byte.h"
#
extern "C"{int get_data(hid_t dset,void *buf,char *);}

extern Byte * NewByte(const string &n = "");

class HDF5Byte: public Byte {

private:
  hid_t dset_id;
  hid_t ty_id;

public:
friend string return_type(hid_t datatype); 
    HDF5Byte(const string &n = "");
    virtual ~HDF5Byte() {}

    virtual BaseType *ptr_duplicate();

    virtual bool read(const string &dataset, int &error);

    void set_did(hid_t dset);
    void set_tid(hid_t type);
    hid_t get_did();
    hid_t get_tid();

};

#endif

