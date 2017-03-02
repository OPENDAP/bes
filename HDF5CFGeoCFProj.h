// This file is part of the hdf4 data handler for the OPeNDAP data server.

/////////////////////////////////////////////////////////////////////////////
#ifndef _HDF5CFGeoCFPROJ_H
#define _HDF5CFGeoCFPROJ_H

// DODS includes
#include <Byte.h>

using namespace libdap;

class HDF5CFGeoCFProj:public Byte {
  public:
    HDF5CFGeoCFProj(const string & varname, const string &datasetname);
    virtual ~ HDF5CFGeoCFProj();
    virtual BaseType *ptr_duplicate();
    virtual bool read();
};

#endif                          // _HDF5CFGeoCFPROJ_H

