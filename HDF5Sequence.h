
#ifndef _HDF5Sequence_h
#define _HDF5Sequence_h 1

#ifdef _GNUG_
#pragma interface
#endif

#include "Sequence.h"
#include "common.h"
#
extern "C"{int get_data(hid_t dset,void *buf,char *);}
extern Sequence * NewSequence(const string &n = "");

class HDF5Sequence: public Sequence {

private:
  hid_t dset_id;
  hid_t ty_id;
public:
friend string return_type(hid_t datatype);   
    HDF5Sequence(const string &n = "");
    virtual ~HDF5Sequence();

    virtual BaseType *ptr_duplicate();

    virtual bool read(const string &dataset, int &error);

    void set_did(hid_t dset);
    void set_tid(hid_t type);
     hid_t get_did();
     hid_t get_tid();
};

#endif
