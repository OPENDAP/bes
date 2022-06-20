// This file is part of the hdf4 data handler for the OPeNDAP data server.

/////////////////////////////////////////////////////////////////////////////
#ifdef USE_HDFEOS2_LIB
#ifndef _HDFEOS2GEOCFPROJ_H
#define _HDFEOS2GEOCFPROJ_H

// DODS includes
#include <libdap/Byte.h>


class HDFEOS2GeoCFProj:public libdap::Byte {
  public:
    HDFEOS2GeoCFProj(const string & varname, const string &datasetname);
    ~ HDFEOS2GeoCFProj() override = default;
    libdap::BaseType *ptr_duplicate() override;
    bool read() override;

};

#endif                          // _HDFEOS2GEOCFPROJ_H
#endif

