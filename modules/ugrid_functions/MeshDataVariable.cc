// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003,2011,2012 OPeNDAP, Inc.
// Authors: Nathan Potter <ndp@opendap.org>
//          James Gallagher <jgallagher@opendap.org>
//          Scott Moe <smeest1@gmail.com>
//          Bill Howe <billhowe@cs.washington.edu>
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

#include "config.h"

#include <gridfields/array.h>

#include <Array.h>
#include <util.h>

#include <BESDebug.h>
#include <BESUtil.h>

#include "ugrid_utils.h"
#include "LocationType.h"
#include "MeshDataVariable.h"
#include "TwoDMeshTopology.h"

#ifdef NDEBUG
#undef BESDEBUG
#define BESDEBUG( x, y )
#endif

using namespace std;

namespace ugrid {

MeshDataVariable::MeshDataVariable()
{
    myGridLocation = node;
    meshDataVar = 0;
    _initialized = false;
}

static locationType determineLocationType(libdap::Array *rangeVar)
{

    string locationString = getAttributeValue(rangeVar, UGRID_LOCATION);
    BESDEBUG("ugrid", "determineLocationType() - UGRID_LOCATION: " << locationString << endl);

    if (locationString.empty()) {
        locationString = getAttributeValue(rangeVar, UGRID_GRID_LOCATION);
        BESDEBUG("ugrid", "determineLocationType() - UGRID_GRID_LOCATION: " << locationString << endl);
    }

    if (locationString.empty()) {
        string msg = "MeshDataVariable::determineLocation() - The range variable '" + rangeVar->name()
            + "' is missing the required attribute named '" +
            UGRID_LOCATION + "' and its alternate attribute named '" +
            UGRID_GRID_LOCATION + "'";
        BESDEBUG("ugrid", msg);
        throw Error(msg);
    }

    locationString = BESUtil::lowercase(locationString);

    if (locationString.compare(UGRID_NODE) == 0) {
        BESDEBUG("ugrid", "determineLocationType() - Location is node.  locationString: " << locationString << endl);
        return node;
    }

    if (locationString.compare(UGRID_EDGE) == 0) {
        BESDEBUG("ugrid", "determineLocationType() - Location is edge.  locationString: " << locationString << endl);
        return edge;
    }

    if (locationString.compare(UGRID_FACE) == 0) {
        BESDEBUG("ugrid", "determineLocationType() - Location is face.  locationString: " << locationString << endl);
        return face;
    }
    string msg = "determineLocation() - The range variable '" + rangeVar->name() + "' has a '" + UGRID_LOCATION
        + "' attribute with an unrecognized value of  '" + locationString + "' The acceptable values are: '"
        + UGRID_NODE + "', '" + UGRID_EDGE + "', and '" + UGRID_FACE + "'";
    BESDEBUG("ugrid", msg);
    throw Error(msg);

}

void MeshDataVariable::init(libdap::Array *rangeVar)
{
    if (_initialized) return;

    meshDataVar = rangeVar;
    BESDEBUG("ugrid",
        "MeshDataVariable::init() - The user submitted the range data array: " << rangeVar->name() << endl);

    locationType rank = determineLocationType(rangeVar);

    setGridLocation(rank);

    meshName = getAttributeValue(rangeVar, UGRID_MESH);
    if (meshName.empty()) {
        string msg = "MeshDataVariable::init() - The range variable '" + rangeVar->name()
            + "' is missing the required attribute named '" + UGRID_MESH + "' ";
        BESDEBUG("ugrid", msg);
        throw Error(msg);
    }

    BESDEBUG("ugrid",
        "MeshDataVariable::init() - Range data array '" << meshDataVar->name() << "' references the 'mesh' variable '" << meshName << "'" << endl);

    _initialized = true;
}

} // namespace gf3
