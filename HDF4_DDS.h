/////////////////////////////////////////////////////////////////////////////
//  This file is part of the hdf4 data handler for the OPeNDAP data server.
//
// Author:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) 2010-2014 The HDF Group
// The idea is borrowed from GDAL OPeNDAP handler that is implemented by
// James Gallagher<jgallagher@opendap.org>

#ifndef HDF4_DDS_H_
#define HDF4_DDS_H_

#include "config.h"

#include "hdf.h"
#include "mfhdf.h"

#ifdef USE_HDFEOS2_LIB
#include "HdfEosDef.h"
#endif

#include <DDS.h>
#include <InternalErr.h>


/**
 * This specialization of DDS is used to manage the 'resource' of the open
 * HDF4 dataset handle so that the BES will close that handle once the
 * framework is done working with the file. This provides a way for the
 * code in HDF4RequestHandler.cc to read data of HDF4 and HDF-EOS2 objects.
 * when HDF4/HDF-EOS2 file IDs are opened to fetch information to build DDS 
 * and DAS, these file IDs are kept to access data. In this way, 
 * multiple file open/close calls can be reduced to speed up the access
 * performance. This works well when using file netCDF module with an
 * HDF-EOS2 or HDF4 file that have many variables.
 * When the DDS is deleted by the BES, the HDF4DDS()
 * destructor closes the file.
 *
 */
class HDF4DDS : public libdap::DDS {
private:
    int sdfd;
    int fileid;
    int gridfd;
    int swathfd;

    void m_duplicate(const HDF4DDS &src) 
    { 
        sdfd = src.sdfd; 
        fileid = src.fileid;
        gridfd = src.gridfd;
        swathfd = src.swathfd;
    }

public:
    explicit HDF4DDS(libdap::DDS *ddsIn) : libdap::DDS(*ddsIn), sdfd(-1),fileid(-1),gridfd(-1),swathfd(-1) {}

    HDF4DDS(const HDF4DDS &rhs) : libdap::DDS(rhs) {
        m_duplicate(rhs);
    }

    HDF4DDS & operator= (const HDF4DDS &rhs) {
        if (this == &rhs)
            return *this;

        m_duplicate(rhs);

        return *this;
    }

    ~HDF4DDS() {

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



