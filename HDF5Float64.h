

#ifndef _HDF5Float64_h
#define _HDF5Float64_h 1

#ifdef __GNUG__
#pragma interface
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <H5Ipublic.h>
#include "Float64.h"

#include "H5Git.h"

// extern "C" int get_data(hid_t dset,void *buf,char *);

class HDF5Float64: public Float64 {

private:
  hid_t dset_id;
  hid_t ty_id;

public:
friend string return_type(hid_t datatype);   
    HDF5Float64(const string &n = "");
    virtual ~HDF5Float64() {}
    virtual BaseType *ptr_duplicate();
    virtual bool read(const string &dataset);
    void set_did(hid_t dset);
    void set_tid(hid_t type);
    hid_t get_did();
    hid_t get_tid();  
};



#endif

