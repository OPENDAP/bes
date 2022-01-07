// This file is part of the hdf4 data handler for the OPeNDAP data server.

/////////////////////////////////////////////////////////////////////////////

#ifdef USE_HDFEOS2_LIB

#include "config_hdf.h"

#include "HDFEOS2GeoCFProj.h"


using namespace libdap;
using namespace std;

HDFEOS2GeoCFProj::HDFEOS2GeoCFProj(const string & n, const string &d ) : Byte(n, d)
{
}

HDFEOS2GeoCFProj::~HDFEOS2GeoCFProj()
{
}
BaseType *HDFEOS2GeoCFProj::ptr_duplicate()
{
    return new HDFEOS2GeoCFProj(*this);
}

bool HDFEOS2GeoCFProj::read()
{
    // Just return a dummy value.
    char buf='p';
    set_read_p(true);
    set_value((dods_byte)buf);

    return true;

}

#endif
