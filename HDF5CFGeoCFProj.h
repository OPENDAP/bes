// This file is part of the hdf5 data handler for the OPeNDAP data server.

/////////////////////////////////////////////////////////////////////////////
#ifndef _HDF5CFGeoCFPROJ_H
#define _HDF5CFGeoCFPROJ_H

// DODS includes
#include <Byte.h>


class HDF5CFGeoCFProj:public libdap::Byte {
  public:
    HDF5CFGeoCFProj(const std::string & varname, const std::string &datasetname);
    virtual ~ HDF5CFGeoCFProj();
    virtual libdap::BaseType *ptr_duplicate();
    virtual bool read();
};

#endif                          // _HDF5CFGeoCFPROJ_H

