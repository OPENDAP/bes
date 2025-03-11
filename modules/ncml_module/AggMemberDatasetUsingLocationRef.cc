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

#include "config.h"

#include "AggMemberDatasetUsingLocationRef.h"

#include "BESDataDDSResponse.h" // bes
#include <libdap/DDS.h> // libdap

#include "NCMLDebug.h" // ncml_module
#include "NCMLUtil.h" // ncml_module
#include "BESDebug.h"
#include "BESStopWatch.h"

using namespace std;

#define MODULE "ncml"
#define prolog string("AggMemberDatasetUsingLocationRef::").append(__func__).append("() - ")

namespace agg_util {

AggMemberDatasetUsingLocationRef::AggMemberDatasetUsingLocationRef(const std::string& locationToLoad,
    const agg_util::DDSLoader& loaderToUse) :
    AggMemberDatasetWithDimensionCacheBase(locationToLoad), _loader(loaderToUse), _pDataResponse(0)
{
}

AggMemberDatasetUsingLocationRef::~AggMemberDatasetUsingLocationRef()
{
    cleanup();
}

AggMemberDatasetUsingLocationRef::AggMemberDatasetUsingLocationRef(const AggMemberDatasetUsingLocationRef& proto) :
    RCObjectInterface(), AggMemberDatasetWithDimensionCacheBase(proto), _loader(proto._loader) // force a reload as needed for a copy
{
}

AggMemberDatasetUsingLocationRef&
AggMemberDatasetUsingLocationRef::operator=(const AggMemberDatasetUsingLocationRef& that)
{
    if (this != &that) {
        // clear out any old loaded stuff
        cleanup();
        // assign
        AggMemberDatasetWithDimensionCacheBase::operator=(that);
        copyRepFrom(that);
    }
    return *this;
}

const libdap::DDS*
AggMemberDatasetUsingLocationRef::getDDS()
{

    if (!_pDataResponse) {
        loadDDS();
    }
    DDS* pDDSRet = 0;
    if (_pDataResponse) {
        pDDSRet = _pDataResponse->get_dds();
    }
    return pDDSRet;
}

/////////////////////////////// Private Helpers ////////////////////////////////////
void AggMemberDatasetUsingLocationRef::loadDDS()
{
    BES_STOPWATCH_START(MODULE, prolog + "Timing");

    // We cannot load an empty location, so avoid the exception later.
    if (getLocation().empty()) {
        THROW_NCML_INTERNAL_ERROR("AggMemberDatasetUsingLocationRef():"
            " got empty location!  Cannot load!");
    }

    // Make a new response and store the raw ptr, noting that we need to delete it in dtor.
    unique_ptr<BESDapResponse> newResponse = agg_util::DDSLoader::makeResponseForType(DDSLoader::eRT_RequestDataDDS);

    // static_cast should work here, but I want to be sure since DataDDX is in the works...
    _pDataResponse = dynamic_cast<BESDataDDSResponse*>(newResponse.get());
    NCML_ASSERT_MSG(_pDataResponse,
        "AggMemberDatasetUsingLocationRef::loadDDS(): failed to get a BESDataDDSResponse back while loading location="
            + getLocation());

    // release after potential for exception to avoid double delete. Coverity reports
    // this as a leak, but the _loader.loadInto() method takes ownership. jhrg 2/7/17
    (void) newResponse.release();

    BESDEBUG("ncml", "Loading loadDDS for aggregation member location = " << getLocation() << endl);
    _loader.loadInto(getLocation(), DDSLoader::eRT_RequestDataDDS, _pDataResponse);
}

void AggMemberDatasetUsingLocationRef::cleanup() noexcept
{
    SAFE_DELETE(_pDataResponse);
}

void AggMemberDatasetUsingLocationRef::copyRepFrom(const AggMemberDatasetUsingLocationRef& rhs)
{
    _loader = rhs._loader;
    _pDataResponse = nullptr; // force this to be NULL... we want to reload if we get an assignment
}

}
