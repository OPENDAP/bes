
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#ifndef _array_geo_constraint_h
#define _array_geo_constraint_h 1

#include <string>
#include <sstream>
#include <set>

#ifndef _geo_constraint_h
#include "GeoConstraint.h"
#endif

#ifndef _util_h
#include <libdap/util.h>
#endif

namespace libdap
{

extern bool unit_or_name_match(set < string > units, set < string > names,
			       const string & var_units,
			       const string & var_name);

/** Geographical constraint applied to an Array.

    @note This class assumes that the Longitude dimension varies fastest, as
    does the COARDS conventions.

    @author James Gallagher */

class ArrayGeoConstraint : public GeoConstraint
{

private:
    struct Extent
    {
        double d_left;
        double d_top;
        double d_right;
        double d_bottom;

        Extent() : d_left(0.0), d_top(0.0), d_right(0.0), d_bottom(0.0)
        {}
        Extent(double t, double l, double b, double r)
                : d_left(l), d_top(t), d_right(r), d_bottom(b)
        {}
    };

    struct Projection
    {
        string d_name;
        string d_datum;

        Projection()
        {}
        Projection(const string &n, const string &d)
                : d_name(n), d_datum(d)
        {
            downcase(d_name);
            if (d_name != "plat-carre")
                throw Error(
                    "geoarray(): Only the Plat-Carre projection is supported by this version of\n\
                    geoarray().");
            downcase(d_datum);
            if (d_datum != "wgs84")
                throw Error(
                    "geoarray(): Only the wgs84 datum is supported by this version of geoarray().");
        }
    };

    Array *d_array;

    Extent d_extent;
    Projection d_projection;

    bool build_lat_lon_maps();
    bool lat_lon_dimensions_ok();

    void m_init();

    friend class ArrayGeoConstraintTest; // Unit tests

public:
    /** @name Constructors */
    //@{
    ArrayGeoConstraint(Array *)
            : GeoConstraint(), d_array(0)
    {
        // See ce_finctions.cc:function_geoarray() to put this message in
        // context.
        throw Error(
            "Bummer. The five-argument version of geoarray() is not currently implemented.");
    }

    ArrayGeoConstraint(Array *array,
                       double left, double top, double right, double bottom);

    ArrayGeoConstraint(Array *array,
                       double left, double top, double right, double bottom,
                       const string &projection, const string &datum);
    //@}

    virtual ~ArrayGeoConstraint()
    {}

    virtual void apply_constraint_to_data();

    virtual Array *get_constrained_array() const
    {
        return d_array;
    }
};

} // namespace libdap

#endif // _array_geo_constraint_h
