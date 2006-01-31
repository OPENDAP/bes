
#ifdef __GNUG__
#pragma implementation
#endif

#include "config_hdf5.h"

#include <string>
#include <ctype.h>

#include "InternalErr.h"
#include "HDF5Float32.h"

HDF5Float32::HDF5Float32(const string & n):Float32(n)
{
    ty_id = -1;
    dset_id = -1;
}

BaseType *
HDF5Float32::ptr_duplicate()
{
    return new HDF5Float32(*this);
}

bool
HDF5Float32::read(const string & dataset)
{
    if (read_p())
	return false;

    if (return_type(ty_id) == "Float32") {
	float buf;
	dods_float32 flt32;
	char Msgi[256];

	if (get_data(dset_id, (void *) &buf, Msgi) < 0) {
	    throw InternalErr(__FILE__, __LINE__,
	      string("hdf5_dods server failed when getting float32 data\n")
			      + Msgi);
	}

	set_read_p(true);
	flt32 = (dods_float32) buf;
	val2buf(&flt32);
    }

    return false;
}

void
HDF5Float32::set_did(hid_t dset)
{
    dset_id = dset;
}

void
HDF5Float32::set_tid(hid_t type)
{
    ty_id = type;
}

hid_t
HDF5Float32::get_did()
{
    return dset_id;
}

hid_t
HDF5Float32::get_tid()
{
    return ty_id;
}
