
#ifndef _HDF5Structure_h
#define _HDF5Structure_h 1

#ifdef _GNUG_
#pragma interface
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <H5Ipublic.h>
#include "Structure.h"

extern "C" int get_data(hid_t dset,void *buf,char *);

extern Structure * NewStructure(const string &n = "");

class HDF5Structure: public Structure {
private:
  hid_t dset_id;
  hid_t ty_id;
public:
friend string print_type(hid_t datatype);   
    HDF5Structure(const string &n = "");
    virtual ~HDF5Structure();

    virtual BaseType *ptr_duplicate();

    virtual bool read(const string &dataset);

    void set_did(hid_t dset);
    void set_tid(hid_t type);
    hid_t get_did();
    hid_t get_tid();
};

#endif
