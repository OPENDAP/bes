
#ifndef _HDF5Float32_h
#define _HDF5Float32_h 1

#ifdef __GNUG__
#pragma interface
#endif

#include <string>

#include <H5Ipublic.h>

#include "Float32.h"

extern "C" int get_data(hid_t dset,void *buf,char *);

extern Float32 * NewFloat32(const string &n = "");

class HDF5Float32: public Float32 {
 private:
    hid_t dset_id;
    hid_t ty_id;

 public:
    HDF5Float32(const string &n = "");
    virtual ~HDF5Float32() {}

    virtual BaseType *ptr_duplicate();
    
    virtual bool read(const string &dataset);

    void set_did(hid_t dset);
    void set_tid(hid_t type);
    hid_t get_did();
    hid_t get_tid();  

    friend string return_type(hid_t datatype);   
};

#endif


