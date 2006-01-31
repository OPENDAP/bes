
#ifdef __GNUG__
#pragma implementation
#endif

#include "config_hdf5.h"

#include <string>
#include <ctype.h>

#include "InternalErr.h"
#include "HDF5UInt32.h"

HDF5UInt32::HDF5UInt32(const string & n):UInt32(n)
{
}

BaseType *
HDF5UInt32::ptr_duplicate()
{

    return new HDF5UInt32(*this);
}

bool
HDF5UInt32::read(const string & dataset)
{
    if (read_p())
	return false;

    if (return_type(ty_id) == "UInt32") {
	long buf;
	char Msgi[256];

	if (get_data(dset_id, (void *) &buf, Msgi) < 0) {
	    throw InternalErr(__FILE__, __LINE__,
      string("hdf5_dods server failed when getting unsigned int32 data\n")
			      + Msgi);
	}

	set_read_p(true);
	dods_uint32 uint32 = (dods_uint32) buf;
	val2buf(&uint32);
    }

    return false;
}

void
HDF5UInt32::set_did(hid_t dset)
{
    dset_id = dset;
}

void
HDF5UInt32::set_tid(hid_t type)
{
    ty_id = type;
}

hid_t
HDF5UInt32::get_did()
{
    return dset_id;
}

hid_t
HDF5UInt32::get_tid()
{
    return ty_id;
}
