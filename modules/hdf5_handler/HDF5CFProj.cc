// This file is part of the hdf5 data handler for the OPeNDAP data server.
// This is to support the HDF-EOS5 grid for the default option. 
// The grid mapping variable is declared here.
/////////////////////////////////////////////////////////////////////////////


#include "HDF5CFProj.h"

using namespace std;
using namespace libdap;


HDF5CFProj::HDF5CFProj(const string & n, const string &d ) : Byte(n, d)
{
}

BaseType *HDF5CFProj::ptr_duplicate()
{
    auto HDF5CFProj_unique = make_unique<HDF5CFProj>(*this);
    return HDF5CFProj_unique.release();
}

bool HDF5CFProj::read()
{
    // Just return a dummy value.
    char buf='p';
    set_read_p(true);
    set_value((dods_byte)buf);
    return true;
}

