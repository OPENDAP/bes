

#ifdef __GNUG__
#pragma implementation
#endif

#include "config_hdf5.h"

#include <string>
#include <ctype.h>

#include "InternalErr.h"
#include "HDF5Int16.h"

Int16 *
NewInt16(const string & n)
{
    return new HDF5Int16(n);
}

HDF5Int16::HDF5Int16(const string & n):Int16(n)
{
}

BaseType *
HDF5Int16::ptr_duplicate()
{

    return new HDF5Int16(*this);
}

// ask experts about this interface.
bool
HDF5Int16::read(const string & dataset)
{
    if (read_p())
	return false;

    if (return_type(ty_id) == "Int16") {
	dods_int16 intg16;
	short buf;
	char Msgi[256];

	if (get_data(dset_id, (void *) &buf, Msgi) < 0) {
	    throw InternalErr(__FILE__, __LINE__,
	      string("hdf5_dods server failed when getting int16 data\n")
			      + Msgi);
	}

	set_read_p(true);
	intg16 = (dods_int16) buf;
	val2buf(&intg16);
    }

    return false;
}

void
HDF5Int16::set_did(hid_t dset)
{
    dset_id = dset;
}

void
HDF5Int16::set_tid(hid_t type)
{
    ty_id = type;
}

hid_t
HDF5Int16::get_did()
{
    return dset_id;
}

hid_t
HDF5Int16::get_tid()
{
    return ty_id;
}
