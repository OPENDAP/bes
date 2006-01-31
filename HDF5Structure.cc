
#ifdef _GNUG_
#pragma implementation
#endif

#include "config_hdf5.h"

#include <string>
#include <ctype.h>

#include "HDF5Structure.h"
#include "InternalErr.h"

BaseType *
HDF5Structure::ptr_duplicate()
{
    return new HDF5Structure(*this);
}

HDF5Structure::HDF5Structure(const string & n):Structure(n)
{
    ty_id = -1;
    dset_id = -1;
}

HDF5Structure::~HDF5Structure()
{
}

bool
HDF5Structure::read(const string &)
{
    throw InternalErr(__FILE__, __LINE__, 
		      "HDF5Structure::read(): Unimplemented method.");

    return false;
}

void
HDF5Structure::set_did(hid_t dset)
{
    dset_id = dset;
}

void
HDF5Structure::set_tid(hid_t type)
{
    ty_id = type;
}

hid_t
HDF5Structure::get_did()
{
    return dset_id;
}

hid_t
HDF5Structure::get_tid()
{
    return ty_id;
}
