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

#ifndef _MeshDataVariable_h
#define _MeshDataVariable_h 1

#include "LocationType.h"

namespace libdap {
class Array;
}

namespace ugrid {

class MeshDataVariable {

private:

    bool _initialized;

    /**
     * The DAP dataset variable that the user requested.
     */
    libdap::Array *meshDataVar;

    /*
     * Coordinate Dimension
     */
    libdap::Array::Dim_iter _coordinateDimension;

    /**
     * REQUIRED
     * The attribute mesh points to the mesh_topology variable containing the meta-data attributes
     * of the mesh on which the variable has been defined.
     */
    string meshName;

    /**
     * REQUIRED
     * The attribute location points to the (stagger) location within the mesh at which the
     * variable is defined. (face or node)
     */
    locationType myGridLocation;

    /**
     * OPTIONAL
     * The use of the coordinates attribute is copied from the CF-conventions.
     * It is used to map the values of variables defined on the unstructured
     * meshes directly to their location: latitude, longitude and optional elevation.
     *
     * The attribute node_coordinates contains a list of the whitespace separated names of
     * the auxiliary coordinate variables representing the node locations (latitude,
     * longitude, and optional elevation or other coordinates). These auxiliary coordinate
     * variables will have length nNodes or nFaces, depending on the value of the location attribute.
     *
     * It appears that the coordinates attribute is redundant since the coordinates could also be
     * obtained by using the appropriate coordinates definition in the mesh topology.
     *
     */
    //vector<string> *coordinateNames;
    //vector<Array *> *coordinateArrays;
public:

    MeshDataVariable();

    void setGridLocation(locationType loc)
    {
        myGridLocation = loc;
    }
    locationType getGridLocation() const
    {
        return myGridLocation;
    }

    void setMeshName(string mName)
    {
        meshName = mName;
    }
    string getMeshName() const
    {
        return meshName;
    }

    string getName() const
    {
        return meshDataVar->name();
    }

    libdap::Array *getDapArray() const
    {
        return meshDataVar;
    }

    libdap::Array::Dim_iter getLocationCoordinateDimension() const
    {
        return _coordinateDimension;
    }

    void setLocationCoordinateDimension(libdap::Array::Dim_iter cdim)
    {
        _coordinateDimension = cdim;
    }

    void init(libdap::Array *dapArray);
};

} // namespace ugrid

#endif // _MeshDataVariable_h
