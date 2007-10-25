
#ifdef __GNUG__
#pragma implementation
#endif

#include "config_hdf5.h"

#include <string>
#include <ctype.h>

#include "HDF5Url.h"
#include "InternalErr.h"

HDF5Url::HDF5Url(const string & n):Url(n)
{
  ty_id = -1;
  dset_id = -1;
}

BaseType *
HDF5Url::ptr_duplicate()
{
    return new HDF5Url(*this);
}

bool
HDF5Url::read(const string &)
{
  string str[1];
  hobj_ref_t *rbuf; // buffer to read
  rbuf = (hobj_ref_t *)malloc(sizeof(hobj_ref_t));
  herr_t status = H5Dread(dset_id, H5T_STD_REF_OBJ, H5S_ALL, H5S_ALL,
			      H5P_DEFAULT, rbuf);
  hid_t did_r =  H5Rdereference(dset_id, H5R_OBJECT, &rbuf[0]);
  char buf2[DODS_NAMELEN];
  int name_size = H5Iget_name(did_r, (char*)buf2, DODS_NAMELEN);
  str[0] = buf2;
  set_read_p(true);
  val2buf(&str);
  return false;
  
}

void
HDF5Url::set_did(hid_t dset)
{
    dset_id = dset;
}

void
HDF5Url::set_tid(hid_t type)
{
    ty_id = type;
}

hid_t
HDF5Url::get_did()
{
    return dset_id;
}

hid_t
HDF5Url::get_tid()
{
    return ty_id;
}

