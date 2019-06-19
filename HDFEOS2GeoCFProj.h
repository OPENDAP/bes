// This file is part of the hdf4 data handler for the OPeNDAP data server.

/////////////////////////////////////////////////////////////////////////////
#ifdef USE_HDFEOS2_LIB
#ifndef _HDFEOS2GEOCFPROJ_H
#define _HDFEOS2GEOCFPROJ_H

// DODS includes
#include <Byte.h>


class HDFEOS2GeoCFProj:public libdap::Byte {
  public:
    HDFEOS2GeoCFProj(const string & varname, const string &datasetname);
    virtual ~ HDFEOS2GeoCFProj();
    virtual libdap::BaseType *ptr_duplicate();
    virtual bool read();
};

#endif                          // _HDFEOS2GEOCFPROJ_H
#endif

