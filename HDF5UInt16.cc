#include "config_hdf5.h"

#include <string>
#include <ctype.h>

#include "InternalErr.h"

#include "h5dds.h"
#include "HDF5UInt16.h"

HDF5UInt16::HDF5UInt16(const string & n):UInt16(n)
{
    ty_id = -1;
    dset_id = -1;

}

BaseType *HDF5UInt16::ptr_duplicate()
{

    return new HDF5UInt16(*this);
}

bool HDF5UInt16::read(const string & dataset)
{
    if (read_p())
        return false;

    if (return_type(ty_id) == "UInt16") {
        dods_uint16 buf;
	get_data(dset_id, (void *) &buf);
        set_read_p(true);
	set_value(buf);
    }

    return false;
}

void HDF5UInt16::set_did(hid_t dset)
{
    dset_id = dset;
}

void HDF5UInt16::set_tid(hid_t type)
{
    ty_id = type;
}

hid_t HDF5UInt16::get_did()
{
    return dset_id;
}

hid_t HDF5UInt16::get_tid()
{
    return ty_id;
}
