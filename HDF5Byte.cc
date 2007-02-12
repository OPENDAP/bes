
#ifdef __GNUG__
#pragma implementation
#endif

#include "config_hdf5.h"

#include <string>
#include <ctype.h>

#include "InternalErr.h"

#include "h5dds.h"
#include "HDF5Byte.h"

HDF5Byte::HDF5Byte(const string & n) : Byte(n)
{
    ty_id = -1;
    dset_id = -1;
}

BaseType *
HDF5Byte::ptr_duplicate()
{
    return new HDF5Byte(*this);
}

bool
HDF5Byte::read(const string & dataset)
{
    if (read_p())
	return false;

    if (return_type(ty_id) == "Byte") {
	dods_byte Dbyte;
	char Msgi[256];

	if (get_data(dset_id, (void *) &Dbyte, Msgi) < 0) {
	    throw InternalErr(__FILE__, __LINE__,
	      string("hdf5_dods server failed when getting one byte data\n"));

	}

	set_read_p(true);
	val2buf(&Dbyte);
    }

    return false;
}

void
HDF5Byte::set_did(hid_t dset)
{
    dset_id = dset;
}

void
HDF5Byte::set_tid(hid_t type)
{
    ty_id = type;
}

hid_t
HDF5Byte::get_did()
{
    return dset_id;
}

hid_t
HDF5Byte::get_tid()
{
    return ty_id;
}
