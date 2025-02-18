// This file is part of the hdf5 data handler for the OPeNDAP data server.

/////////////////////////////////////////////////////////////////////////////


#include "HDF5CFGeoCFProj.h"
#include <memory>

using namespace std;
using namespace libdap;


HDF5CFGeoCFProj::HDF5CFGeoCFProj(const string & n, const string &d ) : Byte(n, d)
{
}

BaseType *HDF5CFGeoCFProj::ptr_duplicate()
{
    auto HDF5CFGeoCFProj_unique = make_unique<HDF5CFGeoCFProj>(*this);
    return HDF5CFGeoCFProj_unique.release();
}

bool HDF5CFGeoCFProj::read()
{
    // Just return a dummy value.
    char buf='p';
    set_read_p(true);
    set_value((dods_byte)buf);

    return true;

}

