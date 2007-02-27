
// -*- C++ -*-

#ifndef _HDF5Int32_h
#define _HDF5Int32_h 1

#ifdef __GNUG__
#pragma interface
#endif

#include <string>

#include <H5Ipublic.h>

#include "Int32.h"
#include "H5Git.h"

//extern "C" int get_data(hid_t dset,void *buf,char *);

class HDF5Int32: public Int32 {
private:
    hid_t dset_id;
    hid_t ty_id;

public:
    HDF5Int32(const string &n = "");
    virtual ~HDF5Int32() {}

    virtual BaseType *ptr_duplicate();
    
    virtual bool read(const string &dataset);
    
    void set_did(hid_t dset);
    void set_tid(hid_t type);
    hid_t get_did();
    hid_t get_tid();

    friend string return_type(hid_t datatype);   
};

#endif

