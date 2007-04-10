#ifdef __GNUG__
#pragma implementation
#endif

#include "config_hdf5.h"

#include <string>
#include <ctype.h>

#include "HDF5Grid.h"

// protected

BaseType *
HDF5Grid::ptr_duplicate()
{
    return new HDF5Grid(*this);
}

// public

HDF5Grid::HDF5Grid(const string & n):Grid(n)
{
    ty_id = -1;
    dset_id = -1;
}

HDF5Grid::~HDF5Grid()
{
}

bool
HDF5Grid::read(const string & dataset)
{
    if (read_p())		// nothing to do
	return false;

    // read array elements
    array_var()->read(dataset);
    // read maps elements
    Map_iter p = map_begin();
    while (p != map_end()) {
        (*p)->read(dataset);
        ++p;
    }
#if 0
    for (Pix p = first_map_var(); p; next_map_var(p))
	map_var(p)->read(dataset);
#endif

    set_read_p(true);

    return false;
}

void
HDF5Grid::set_did(hid_t dset)
{
    dset_id = dset;
}

void
HDF5Grid::set_tid(hid_t type)
{
    ty_id = type;
}

hid_t
HDF5Grid::get_did()
{
    return dset_id;
}

hid_t
HDF5Grid::get_tid()
{
    return ty_id;
}
