
#ifdef _GNUG_
#pragma implementation
#endif

#include <assert.h>
#include <string>
#include <assert.h>
#include <ctype.h>

#define HAVE_CONFIG_H
#include "config_dap.h"
#include "HDF5Sequence.h"

Sequence *
NewSequence(const string &n)
{
    return new HDF5Sequence(n);
}


BaseType *
HDF5Sequence::ptr_duplicate()
{
    return new HDF5Sequence(*this);
}


HDF5Sequence::HDF5Sequence(const string &n) : Sequence(n)
{
  ty_id = -1;
  dset_id = -1;
}

HDF5Sequence::~HDF5Sequence()
{
}

bool 
HDF5Sequence::read(const string &, int &error)
{
    error = 1;
    return false;
}

void 
HDF5Sequence::set_did(hid_t dset) {dset_id = dset;}
void 
HDF5Sequence::set_tid(hid_t type) {ty_id = type;}
hid_t 
HDF5Sequence::get_did() {return dset_id;}
hid_t
HDF5Sequence::get_tid(){return ty_id;}

