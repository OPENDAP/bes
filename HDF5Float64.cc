
#ifdef __GNUG__
#pragma implementation
#endif

#include "config_hdf5.h"

#include <string>
#include <ctype.h>

#include "InternalErr.h"
#include "HDF5Float64.h"

HDF5Float64::HDF5Float64(const string & n):Float64(n)
{
    ty_id = -1;
    dset_id = -1;
}

BaseType *
HDF5Float64::ptr_duplicate()
{
    return new HDF5Float64(*this);
}

bool
HDF5Float64::read(const string & dataset)
{
    if (read_p())
	return false;

    if (return_type(ty_id) == "Float64") {
	double buf;
	dods_float64 flt64;
	char Msgi[256];

	if (get_data(dset_id, (void *) &buf, Msgi) < 0) {
	    throw InternalErr(__FILE__, __LINE__,
	      string("hdf5_dods server failed when getting float64 data\n")
			      + Msgi);
	}

	set_read_p(true);
	flt64 = (dods_float64) buf;
	val2buf(&flt64);
    }

    return false;
}

void
HDF5Float64::set_did(hid_t dset)
{
    dset_id = dset;
}

void
HDF5Float64::set_tid(hid_t type)
{
    ty_id = type;
}

hid_t
HDF5Float64::get_did()
{
    return dset_id;
}

hid_t
HDF5Float64::get_tid()
{
    return ty_id;
}
