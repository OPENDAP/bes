// This file is part of the hdf5 data handler for the OPeNDAP data server.

/////////////////////////////////////////////////////////////////////////////
#ifndef _HDF5CFGeoCFPROJ_H
#define _HDF5CFGeoCFPROJ_H

// DODS includes
#include <libdap/Byte.h>


class HDF5CFGeoCFProj:public libdap::Byte {
  public:
    HDF5CFGeoCFProj(const std::string & varname, const std::string &datasetname);
    ~ HDF5CFGeoCFProj() override = default;
    libdap::BaseType *ptr_duplicate() override;
    bool read() override;
};

#endif                          // _HDF5CFGeoCFPROJ_H

