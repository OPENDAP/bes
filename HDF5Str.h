
#ifndef _HDF5Str_h
#define _HDF5Str_h 1

#ifdef __GNUG__
#pragma interface
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <H5Ipublic.h>

#include <limits.h>
#ifndef STR_FLAG
#define STR_FLAG 1
#endif

#ifndef STR_NOFLAG
#define STR_NOFLAG 0
#endif

#include "Str.h"

extern "C"{int get_data(hid_t dset,void *buf,char *);}
extern "C"{size_t H5Tget_size(hid_t type_id);}
extern Str * NewStr(const string &n = "");

class HDF5Str: public Str {

private:
  hid_t dset_id;
  hid_t ty_id;
  int array_flag;
public:
    friend string return_type(hid_t datatype);   
    HDF5Str(const string &n = "");
    virtual ~HDF5Str() {}

    virtual BaseType *ptr_duplicate();
    
    virtual bool read(const string &dataset, int &error);
     void set_did(hid_t dset);
    void set_tid(hid_t type);
    void set_arrayflag(int flag);
    int get_arrayflag();
    hid_t get_did();
    hid_t get_tid(); 
};



#endif

