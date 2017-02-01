// This file is part of the GDAL OPeNDAP Adapter

// Copyright (c) 2012 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#ifndef GDAL_DDS_H_
#define GDAL_DDS_H_

#include "config.h"

#include <gdal.h>

#include <DDS.h>
#include <InternalErr.h>

using namespace libdap;

/**
 * This specialization of DDS is used to manage the 'resource' of the open
 * GDAL dataset handle so that the BES will close that handle once the
 * framework is done working with the file. This provides a way for the
 * code in gdal_dds.cc to read binary objects from the file using the gdal
 * library and embed those in instances of Grid. Those Grid variables are
 * used later on (but during the same service request, so the binary data
 * are still valid). When the DDS is deleted by the BES, the GDALDDS()
 * destructor closes the file.
 *
 * @todo Change DataDDS to DDS if we can... Doing that will enable the
 * handler to use this to close the library using this class. That is not
 * strictly needed, but it would make both the DDS and DataDDS responses
 * work the same way.
 */
class GDALDDS : public DDS {
private:
    GDALDatasetH d_hDS;

    void m_duplicate(const GDALDDS &src) { d_hDS = src.d_hDS; }

public:
    GDALDDS(DDS *ddsIn) : DDS(*ddsIn), d_hDS(0) {}
    GDALDDS(BaseTypeFactory *factory, const string &name) : DDS(factory, name), d_hDS(0) {}

    GDALDDS(const GDALDDS &rhs) : DDS(rhs) {
        m_duplicate(rhs);
    }

    GDALDDS & operator= (const GDALDDS &rhs) {
        if (this == &rhs)
            return *this;

        m_duplicate(rhs);

        return *this;
    }

    ~GDALDDS() {
        if (d_hDS)
            GDALClose(d_hDS);
    }

    void setGDALDataset(const GDALDatasetH &hDSIn) { d_hDS = hDSIn; }
    GDALDatasetH &GDALDataset() { return d_hDS; }
};


#endif /* GDAL_DDS_H_ */
