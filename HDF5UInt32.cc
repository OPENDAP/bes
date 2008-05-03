#include "config_hdf5.h"

#include <string>
#include <ctype.h>

#include "InternalErr.h"

#include "h5dds.h"
#include "HDF5UInt32.h"

HDF5UInt32::HDF5UInt32(const string & n):UInt32(n)
{
}

BaseType *HDF5UInt32::ptr_duplicate()
{

    return new HDF5UInt32(*this);
}

bool HDF5UInt32::read(const string & dataset)
{
    if (read_p())
        return false;

    if (return_type(ty_id) == "UInt32") {
	dods_uint32 buf;
	get_data(dset_id, (void *) &buf);
        set_read_p(true);
	set_value(buf);
    }

    return false;
}

void HDF5UInt32::set_did(hid_t dset)
{
    dset_id = dset;
}

void HDF5UInt32::set_tid(hid_t type)
{
    ty_id = type;
}

hid_t HDF5UInt32::get_did()
{
    return dset_id;
}

hid_t HDF5UInt32::get_tid()
{
    return ty_id;
}
