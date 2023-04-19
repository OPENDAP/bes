// This file is part of the hdf5 data handler for the OPeNDAP data server.

/////////////////////////////////////////////////////////////////////////////
#ifndef _HDF5GeoCFPROJ_H
#define _HDF5GeoCFPROJ_H

// DODS includes
#include <libdap/Byte.h>


class HDF5GeoCFProj:public libdap::Byte {
  public:
    HDF5GeoCFProj(const std::string & varname, const std::string &datasetname);
    ~ HDF5GeoCFProj() override = default;
    libdap::BaseType *ptr_duplicate() override;
    bool read() override;
};

#endif                          // _HDF5GeoCFPROJ_H

