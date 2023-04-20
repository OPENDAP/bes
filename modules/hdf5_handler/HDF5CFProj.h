// This file is part of the hdf5 data handler for the OPeNDAP data server.

/////////////////////////////////////////////////////////////////////////////
#ifndef _HDF5CFPROJ_H
#define _HDF5CFPROJ_H

// DODS includes
#include <libdap/Byte.h>


class HDF5CFProj:public libdap::Byte {
  public:
    HDF5CFProj(const std::string & varname, const std::string &datasetname);
    ~ HDF5CFProj() override = default;
    libdap::BaseType *ptr_duplicate() override;
    bool read() override;
};

#endif                          // _HDF5CFPROJ_H

