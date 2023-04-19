// This file is part of the hdf5 data handler for the OPeNDAP data server.

/////////////////////////////////////////////////////////////////////////////


#include "HDF5GeoCFProj.h"

using namespace std;
using namespace libdap;


HDF5GeoCFProj::HDF5GeoCFProj(const string & n, const string &d ) : Byte(n, d)
{
}

BaseType *HDF5GeoCFProj::ptr_duplicate()
{
    return new HDF5GeoCFProj(*this);
}

bool HDF5GeoCFProj::read()
{
    // Just return a dummy value.
    char buf='p';
    set_read_p(true);
    set_value((dods_byte)buf);

    return true;

}

