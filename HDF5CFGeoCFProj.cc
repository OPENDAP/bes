// This file is part of the hdf5 data handler for the OPeNDAP data server.

/////////////////////////////////////////////////////////////////////////////


#include "HDF5CFGeoCFProj.h"



HDF5CFGeoCFProj::HDF5CFGeoCFProj(const string & n, const string &d ) : Byte(n, d)
{
}

HDF5CFGeoCFProj::~HDF5CFGeoCFProj()
{
}
BaseType *HDF5CFGeoCFProj::ptr_duplicate()
{
    return new HDF5CFGeoCFProj(*this);
}

bool HDF5CFGeoCFProj::read()
{
    // Just return a dummy value.
    char buf='p';
    set_read_p(true);
    set_value((dods_byte)buf);

    return true;

}

