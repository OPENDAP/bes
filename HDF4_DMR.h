/////////////////////////////////////////////////////////////////////////////
//  This file is part of the hdf4 data handler for the OPeNDAP data server.
//
// Author:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) 2010-2014 The HDF Group
// The idea is borrowed from GDAL OPeNDAP handler that is implemented by
// James Gallagher<jgallagher@opendap.org>

#ifndef HDF4_DMR_H_
#define HDF4_DMR_H_

#include "config.h"

#include "hdf.h"
#include "mfhdf.h"

#ifdef USE_HDFEOS2_LIB
#include "HdfEosDef.h"
#endif

//#define USE_DAP4 1
//#undef USE_DAP4
#ifdef USE_DAP4
#include <DMR.h>
#include <InternalErr.h>

using namespace libdap;

/**
 * This specialization of DMR is used to manage the 'resource' of the open
 * HDF4 file handle so that the BES will close that handle once the
 * framework is done working with the file. This provides a way for the
 * code in HDF4RequestHandler.cc to read data of HDF4 and HDF-EOS2 objects.
 * when HDF4/HDF-EOS2 file IDs are opened to fetch information to build DDS 
 * and DAS, these file IDs are kept to access data. In this way, 
 * multiple file open/close calls can be reduced to speed up the access
 * performance. This works well when using file netCDF module with an
 * HDF-EOS2 or HDF4 file that have many variables.
 * When the DMR is deleted by the BES, the HDF4DMR()
 * destructor closes the file.
 *
 */
class HDF4DMR : public libdap::DMR {
private:
    int sdfd;
    int fileid;
    int gridfd;
    int swathfd;

    void m_duplicate(const HDF4DMR &src) 
    { 
        sdfd = src.sdfd; 
        fileid = src.fileid;
        gridfd = src.gridfd;
        swathfd = src.swathfd;
    }

public:
    HDF4DMR(libdap::DMR *dmr) : libdap::DMR(*dmr), sdfd(-1),fileid(-1),gridfd(-1),swathfd(-1) {}
    HDF4DMR(libdap::D4BaseTypeFactory *factory,const string &name):libdap::DMR(factory,name),sdfd(-1),fileid(-1),gridfd(-1),swathfd(-1) {}

    HDF4DMR(const HDF4DMR &rhs) : libdap::DMR(rhs) {
        m_duplicate(rhs);
    }

    HDF4DMR & operator= (const HDF4DMR &rhs) {
        if (this == &rhs)
            return *this;

        dynamic_cast<libdap::DMR &>(*this) = rhs;
        m_duplicate(rhs);

        return *this;
    }

    ~HDF4DMR() {

        if (sdfd != -1)
            SDend(sdfd);
        if (fileid != -1)
            Hclose(fileid);

#ifdef USE_HDFEOS2_LIB
        if (gridfd != -1)
            GDclose(gridfd);
        if (swathfd != -1)
            SWclose(swathfd);
#endif
    }

    void setHDF4Dataset(const int sdfd_in, const int fileid_in, const int gridfd_in, const int swathfd_in ) { 
        sdfd = sdfd_in; 
        fileid = fileid_in;
        gridfd = gridfd_in;
        swathfd = swathfd_in;
    }

    void setHDF4Dataset(const int sdfd_in,const int fileid_in) {
        sdfd = sdfd_in;
        fileid = fileid_in;
    }
};
#endif

#endif



