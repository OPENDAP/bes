

#ifdef __GNUG__
#pragma implementation
#endif

#include "config_hdf5.h"

#include <string>
#include <ctype.h>

#include "InternalErr.h"
#include "HDF5Int32.h"

HDF5Int32::HDF5Int32(const string & n):Int32(n)
{
    ty_id = -1;
    dset_id = -1;
}

BaseType *
HDF5Int32::ptr_duplicate()
{

    return new HDF5Int32(*this);
}

bool
HDF5Int32::read(const string & dataset)
{
    if (read_p())
	return false;

    if (return_type(ty_id) == "Int32") {
	char Msgi[256];
	int buf;

	if (get_data(dset_id, (void *) &buf, Msgi) < 0) {
	    throw InternalErr(__FILE__, __LINE__,
	      string("hdf5_dods server failed when getting int32 data\n")
			      + Msgi);
	}

	set_read_p(true);
	dods_int32 intg32 = (dods_int32) buf;
	val2buf(&intg32);
    }

    return false;
}

void
HDF5Int32::set_did(hid_t dset)
{
    dset_id = dset;
}

void
HDF5Int32::set_tid(hid_t type)
{
    ty_id = type;
}

hid_t
HDF5Int32::get_did()
{
    return dset_id;
}

hid_t
HDF5Int32::get_tid()
{
    return ty_id;
}
