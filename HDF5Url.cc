
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
}

BaseType *
HDF5Url::ptr_duplicate()
{
    return new HDF5Url(*this);
}

bool
HDF5Url::read(const string &)
{
    throw InternalErr(__FILE__, __LINE__, 
		      "HDF5Url::read(): Unimplemented method.");

    return false;
}
