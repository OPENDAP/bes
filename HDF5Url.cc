
#ifdef __GNUG__
#pragma implementation
#endif

#include <assert.h>
#include <string>
#include <assert.h>
#include <ctype.h>

#define HAVE_CONFIG_H
#include "config_dap.h"
#include "HDF5Url.h"

Url *
NewUrl(const string &n)
{
    return new HDF5Url(n);
}

HDF5Url::HDF5Url(const string &n) : Url(n)
{
}

BaseType *
HDF5Url::ptr_duplicate()
{
    return new HDF5Url(*this);
}

bool
HDF5Url::read(const string &, int &error)
{
    error = 1;
    return false;
}


