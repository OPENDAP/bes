//////////////////////////////////////////////////////////////////////////////
// This file is part of the "NcML Module" project, a BES module designed
// to allow NcML files to be used to be used as a wrapper to add
// AIS to existing datasets of any format.
//
// Copyright (c) 2010 OPeNDAP, Inc.
// Author: Michael Johnson  <m.johnson@opendap.org>
//
// For more information, please also see the main website: http://opendap.org/
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
// Please see the files COPYING and COPYRIGHT for more information on the GLPL.
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
/////////////////////////////////////////////////////////////////////////////
#ifndef __AGG_UTIL__AGG_MEMBER_DATASET_USING_LOCATION_REF_H__
#define __AGG_UTIL__AGG_MEMBER_DATASET_USING_LOCATION_REF_H__

#include "AggMemberDatasetWithDimensionCacheBase.h"
#include "DDSLoader.h"
#include <string>

class BESDataDDSResponse;

namespace libdap {
class DDS;
}

using libdap::DDS;

namespace agg_util {

/**
 * class AggMemberDatasetUsingLocationRef:
 * Concrete subclass of AggMemberDataset for lazy-loading
 * a location (file) if the DDS for the given dataset is needed.
 *
 * Note: assignment and copy construction do not copy any loaded
 * DDS, merely the location.  Therefore, if getDDS() is
 * used for one of these and modified, a copy of this will see
 * the ORIGINAL dataset (will reload it) and NOT any changes to
 * the version it copied from!
 *
 * TODO Consider if we want to change the above by using an
 * external reference count on the loaded object state...
 */
class AggMemberDatasetUsingLocationRef: public agg_util::AggMemberDatasetWithDimensionCacheBase {
public:

    AggMemberDatasetUsingLocationRef(const std::string& locationToLoad, const agg_util::DDSLoader& loaderToUse);

    AggMemberDatasetUsingLocationRef(const AggMemberDatasetUsingLocationRef& proto);
    AggMemberDatasetUsingLocationRef& operator=(const AggMemberDatasetUsingLocationRef& rhs);

    virtual ~AggMemberDatasetUsingLocationRef();

    /** If not loaded yet, loads the DDS response,
     * then returns it.
     * @return the DDS for the location.
     */
    virtual const libdap::DDS* getDDS();

private:
    // helpers

    /** Load the given location as a data response so that we have a valid DDS
     * for streaming data from */
    void loadDDS();

    /** copy the local data rep from rhs */
    void copyRepFrom(const AggMemberDatasetUsingLocationRef& rhs);

    void cleanup() throw ();

private:
    DDSLoader _loader; // for loading
    BESDataDDSResponse* _pDataResponse; // holds our loaded DDS

};
// class AggMemberDatasetUsingLocationRef

}

#endif /* __AGG_UTIL__AGG_MEMBER_DATASET_USING_LOCATION_REF_H__ */
