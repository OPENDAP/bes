
// -*- C++ -*-


#ifndef _HDF5Int16_h
#define _HDF5Int16_h 1

#ifdef __GNUG__
#pragma interface
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <H5Ipublic.h>
#include "Int16.h"


extern "C"{int get_data(hid_t dset,void *buf,char *);}

extern Int16 * NewInt16(const string &n = "");

class HDF5Int16: public Int16 {
private:
  hid_t dset_id;
  hid_t ty_id;

public:
friend string return_type(hid_t datatype);
    HDF5Int16(const string &n = "");
    virtual ~HDF5Int16() {}

    virtual BaseType *ptr_duplicate();
    
    virtual bool read(const string &dataset, int &error);

   void set_did(hid_t dset);
   void set_tid(hid_t type);
   hid_t get_did();
   hid_t get_tid();
};



#endif




