#ifdef _GNUG_
#pragma implementation
#endif

#include <assert.h>
#include <string>
#include <assert.h>
#include <ctype.h>

#define HAVE_CONFIG_H
#include "config_dap.h"
#include "HDF5Structure.h"

Structure *
NewStructure(const string &n)
{
    return new HDF5Structure(n);
}

BaseType *
HDF5Structure::ptr_duplicate()
{
    return new HDF5Structure(*this);
}

HDF5Structure::HDF5Structure(const string &n) : Structure(n)
{
  ty_id = -1;
  dset_id = -1;
}

HDF5Structure::~HDF5Structure()
{
}

bool
HDF5Structure::read(const string &, int &error)
{
    error = 1;
    return false;
}

void 
HDF5Structure::set_did(hid_t dset) {dset_id = dset;}
void 
HDF5Structure::set_tid(hid_t type) {ty_id = type;}
hid_t 
HDF5Structure::get_did() {return dset_id;}
hid_t
HDF5Structure::get_tid(){return ty_id;}
