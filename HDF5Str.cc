
#ifdef __GNUG__
#pragma implementation
#endif

#include "config_hdf5.h"

#include <string>
#include <ctype.h>

#include "InternalErr.h"
#include "HDF5Str.h"


Str *
NewStr(const string & n)
{
    return new HDF5Str(n);
}

HDF5Str::HDF5Str(const string & n):Str(n)
{
    ty_id = -1;
    dset_id = -1;
}

BaseType *
HDF5Str::ptr_duplicate()
{
    return new HDF5Str(*this);
}

bool
HDF5Str::read(const string & dataset)
{
    if (read_p())
	return false;

    if (array_flag == 1)
	return true;

    if (return_type(ty_id) == "String") {
	char Msgi[256];
	size_t size = H5Tget_size(ty_id);
	char *chr = new char[size + 1];

	if (get_data(dset_id, (void *) chr, Msgi) < 0) {
	    delete [] chr;
	    throw InternalErr(__FILE__, __LINE__,
	      string("hdf5_dods server failed when getting string data\n")
			      + Msgi);
	}

	set_read_p(true);
	string str = chr;

	val2buf(&str);
	delete[]chr;
    }

    return false;
}

void
HDF5Str::set_did(hid_t dset)
{
    dset_id = dset;
}

void
HDF5Str::set_tid(hid_t type)
{
    ty_id = type;
}

void
HDF5Str::set_arrayflag(int flag)
{
    array_flag = flag;
}

int
HDF5Str::get_arrayflag()
{
    return array_flag;
}

hid_t
HDF5Str::get_did()
{
    return dset_id;
}

hid_t
HDF5Str::get_tid()
{
    return ty_id;
}



