#ifdef __GNUG__
#pragma implementation
#endif

#include <assert.h>
#include <string>
#include <assert.h>
#include <ctype.h>

#define HAVE_CONFIG_H
#include "config_dap.h"

#include "HDF5Grid.h"

Grid *
NewGrid(const string &n)
{
    return new HDF5Grid(n);
}

// protected

BaseType *
HDF5Grid::ptr_duplicate()
{
    return new HDF5Grid(*this);
}

// public

HDF5Grid::HDF5Grid(const string &n) : Grid(n)
{
  ty_id = -1;
  dset_id = -1;
}

HDF5Grid::~HDF5Grid()
{
}

bool
HDF5Grid::read(const string &dataset, int &error)
{
   bool status;

    if (read_p()) // nothing to do
        return false;

    // read array elements
    if (!(status = array_var()->read(dataset, error))) 
        return status;

    // read maps elements
    for (Pix p = first_map_var(); p; next_map_var(p))
	if(!(status = map_var(p)->read(dataset, error)))
            break;

    if( status ) 
	set_read_p(true);

    return status;
 
}

void 
HDF5Grid::set_did(hid_t dset) {dset_id = dset;}
void 
HDF5Grid::set_tid(hid_t type) {ty_id = type;}
hid_t 
HDF5Grid::get_did() {return dset_id;}
hid_t
HDF5Grid::get_tid(){return ty_id;}

