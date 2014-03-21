/////////////////////////////////////////////////////////////////////////////
//  This file is part of the hdf4 data handler for the OPeNDAP data server.
//
// Author:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) 2010-2012 The HDF Group
// The idea is borrowed from HDF4 OPeNDAP handler that is implemented by
// James Gallagher<jgallagher@opendap.org>

#ifndef HDF4_DDS_H_
#define HDF4_DDS_H_

#include "config.h"


#include "hdf.h"
#include "mfhdf.h"

#ifdef USE_HDFEOS2_LIB
#include "HdfEosDef.h"
#endif


#include <DataDDS.h>
#include <InternalErr.h>

using namespace libdap;

/**
 * This specialization of DDS is used to manage the 'resource' of the open
 * HDF4 dataset handle so that the BES will close that handle once the
 * framework is done working with the file. This provides a way for the
 * code in gdal_dds.cc to read binary objects from the file using the gdal
 * library and embed those in instances of Grid. Those Grid variables are
 * used later on (but during the same service request, so the binary data
 * are still valid). When the DDS is deleted by the BES, the HDF4DDS()
 * destructor closes the file.
 *
 * @todo Change DataDDS to DDS if we can... Doing that will enable the
 * handler to use this to close the library using this class. That is not
 * strictly needed, but it would make both the DDS and DataDDS responses
 * work the same way.
 */
class HDF4DDS : public DataDDS {
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
    HDF4DDS(DataDDS *ddsIn) : DataDDS(*ddsIn), sdfd(-1),fileid(-1),gridfd(-1),swathfd(-1) {}

    HDF4DDS(const HDF4DDS &rhs) : DataDDS(rhs) {
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



