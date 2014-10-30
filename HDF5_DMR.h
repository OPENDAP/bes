/////////////////////////////////////////////////////////////////////////////
//  This file is part of the hdf4 data handler for the OPeNDAP data server.
//
// Author:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) 2010-2012 The HDF Group
// The idea is borrowed from HDF4 OPeNDAP handler that is implemented by
// James Gallagher<jgallagher@opendap.org>

#ifndef HDF5_DMR_H_
#define HDF5_DMR_H_

#include "config_hdf5.h"


#include "hdf5.h"


//#define USE_DAP4 1
//#undef USE_DAP4
#ifdef USE_DAP4
#include <DMR.h>
#include <InternalErr.h>

using namespace libdap;

/**
 * This specialization of DMR is used to manage the 'resource' of the open
 * HDF4 dataset handle so that the BES will close that handle once the
 * framework is done working with the file. This provides a way for the
 * code in gdal_dds.cc to read binary objects from the file using the gdal
 * library and embed those in instances of Grid. Those Grid variables are
 * used later on (but during the same service request, so the binary data
 * are still valid). When the DMR is deleted by the BES, the HDF5DMR()
 * destructor closes the file.
 *
 * @todo Change DataDMR to DMR if we can... Doing that will enable the
 * handler to use this to close the library using this class. That is not
 * strictly needed, but it would make both the DMR and DataDMR responses
 * work the same way.
 */
class HDF5DMR : public libdap::DMR {
private:
    hid_t fileid;

    void m_duplicate(const HDF5DMR &src) 
    { 
        fileid = src.fileid; 
    }

public:
    HDF5DMR(libdap::DMR *dmr) : libdap::DMR(*dmr), fileid(-1) {}
    HDF5DMR(libdap::D4BaseTypeFactory *factory,const string &name):libdap::DMR(factory,name),fileid(-1) {}

    HDF5DMR(const HDF5DMR &rhs) : libdap::DMR(rhs) {
        m_duplicate(rhs);
    }

    HDF5DMR & operator= (const HDF5DMR &rhs) {
        if (this == &rhs)
            return *this;

        dynamic_cast<libdap::DMR &>(*this) = rhs;
        m_duplicate(rhs);

        return *this;
    }

    ~HDF5DMR() {

        if (fileid != -1)
            H5Fclose(fileid);

    }

    void setHDF5Dataset(const hid_t fileid_in) { 
        fileid = fileid_in;
    }

};

#endif
#endif



