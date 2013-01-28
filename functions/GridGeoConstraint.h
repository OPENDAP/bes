
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2006 OPeNDAP, Inc.
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#ifndef _grid_geo_constraint_h
#define _grid_geo_constraint_h 1

#include <string>
#include <sstream>
#include <set>

#ifndef _geo_constraint_h
#include "GeoConstraint.h"
#endif

namespace libdap
{

// Defined in GeoConstraint; maybe move to util.cc/h?
extern bool unit_or_name_match(set < string > units, set < string > names,
			       const string & var_units,
			       const string & var_name);

/** Geographical constraint applied to a grid.
    @author James Gallagher */

class GridGeoConstraint : public GeoConstraint
{

private:
    // Specific to a Grid
    Grid *d_grid;               //< Constrain this Grid

    Array *d_latitude;          //< A pointer to the Grid's latitude map
    Array *d_longitude;         //< A pointer to the Grid's longitude map

    bool build_lat_lon_maps();
    bool build_lat_lon_maps(Array *lat, Array *lon);

    bool lat_lon_dimensions_ok();

    friend class GridGeoConstraintTest; // Unit tests

public:
    /** @name Constructors */
    //@{
    GridGeoConstraint(Grid *grid);
    GridGeoConstraint(Grid *grid, Array *lat, Array *lon);
    //@}

    virtual ~GridGeoConstraint()
    {}

    virtual void apply_constraint_to_data() ;

    virtual Grid *get_constrained_grid() const
    {
        return d_grid;
    }
};

} // namespace libdap

#endif // _grid_geo_constraint_h

