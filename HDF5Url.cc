
#include "config_hdf5.h"

#include <string>
#include <ctype.h>

#include "HDF5Url.h"
#include "InternalErr.h"

HDF5Url::HDF5Url(const string &n, const string &d) : Url(n, d)
{
    ty_id = -1;
    dset_id = -1;
}

BaseType *HDF5Url::ptr_duplicate()
{
    return new HDF5Url(*this);
}

bool HDF5Url::read()
{
    hobj_ref_t rbuf;

    if (H5Dread(dset_id, H5T_STD_REF_OBJ, H5S_ALL, H5S_ALL, H5P_DEFAULT, 
		&rbuf) < 0) {
	throw InternalErr(__FILE__, __LINE__, "H5Dread() failed.");
    }

    hid_t did_r = H5Rdereference(dset_id, H5R_OBJECT, &rbuf);
    char name[DODS_NAMELEN];
    if (did_r < 0){
	throw InternalErr(__FILE__, __LINE__, "H5Rdereference() failed.");
    }
    if (H5Iget_name(did_r, name, DODS_NAMELEN) < 0){
	throw InternalErr(__FILE__, __LINE__, "Unable to retrieve the name of the object.");
    }
    string reference = name;
    set_value(reference);

    return false;
}

void HDF5Url::set_did(hid_t dset)
{
    dset_id = dset;
}

void HDF5Url::set_tid(hid_t type)
{
    ty_id = type;
}

hid_t HDF5Url::get_did()
{
    return dset_id;
}

hid_t HDF5Url::get_tid()
{
    return ty_id;
}
