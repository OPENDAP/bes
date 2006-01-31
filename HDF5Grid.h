
#ifndef _HDF5Grid_h
#define _HDF5Grid_h 1

#ifdef _GNUG_
#pragma interface
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <H5Ipublic.h>
#include "Grid.h"

extern "C" int get_data(hid_t dset,void *buf,char *);

class HDF5Grid: public Grid {

private:
  hid_t dset_id;
  hid_t ty_id;
public:
friend string print_type(hid_t datatype);  
    HDF5Grid(const string &n = "");
    virtual ~HDF5Grid();
    
    virtual BaseType *ptr_duplicate();

        virtual bool read(const string &dataset);
//    virtual bool read(String dataset, String var_name, String constraint);
//    virtual bool read_val(void *stuff);

  void set_did(hid_t dset);
  void set_tid(hid_t type);
  hid_t get_did();
  hid_t get_tid();
};

#endif





