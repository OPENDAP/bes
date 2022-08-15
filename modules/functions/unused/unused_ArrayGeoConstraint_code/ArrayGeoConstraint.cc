
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
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

// The Grid Selection Expression Clause class.


#include <functions/unused_ArrayGeoConstraint_code/ArrayGeoConstraint.h>
#include <functions/unused_ArrayGeoConstraint_code/ce_functions.h>
#include "config.h"

#include <cmath>
#include <iostream>
#include <sstream>

//#define DODS_DEBUG

#include <libdap/debug.h>
#include <libdap/dods-datatypes.h>
#include <libdap/Float64.h>

#include <libdap/Error.h>
#include <libdap/InternalErr.h>

using namespace std;

namespace libdap {

/** Private initialization code. */
void ArrayGeoConstraint::m_init()
{
    if (d_array->dimensions() < 2 || d_array->dimensions() > 3)
        throw Error("The geoarray() function works only with Arrays of two or three dimensions.");

    build_lat_lon_maps();
}

// In the other methods the ds_name parameter defaults to "" but that's not
// possible here. Remove ds_name
ArrayGeoConstraint::ArrayGeoConstraint(Array *array, double top, double left,
        double bottom, double right)
        : GeoConstraint(), d_array(array),
        d_extent(top, left, bottom, right), d_projection("plat-carre", "wgs84")

{
    m_init();
}

ArrayGeoConstraint::ArrayGeoConstraint(Array *array,
                                       double top, double left, double bottom, double right,
                                       const string &projection, const string &datum)
        : GeoConstraint(), d_array(array),
        d_extent(top, left, bottom, right), d_projection(projection, datum)

{
    m_init();
}

/** Build the longitude and latitude maps for this Array. This either builds
    the maps using the extent, projection and datum given or reads the maps
    from the data source somehow.

    The d_lon and d_lon_length (and matching lat) fields are modified.

    @note Rules used to find Maps:<ul><li>None, currently the extent must
    be given.</li>
    </ul>

    @return True if the maps are built, otherwise False */
bool ArrayGeoConstraint::build_lat_lon_maps()
{
    // Find the longitude dimension: Assume it is the rightmost
    set_longitude_rightmost(true);
    set_lon_dim(d_array->dim_begin() + (d_array->dimensions() - 1));

    int number_elements_longitude = d_array->dimension_size(get_lon_dim());
    double *lon_map = new double[number_elements_longitude];
    for (int i = 0; i < number_elements_longitude; ++i) {
        lon_map[i] = ((d_extent.d_right - d_extent.d_left) / (number_elements_longitude - 1)) * i + d_extent.d_left;
    }
    set_lon(lon_map);
    set_lon_size(number_elements_longitude);

    // Find the latitude dimension: Assume it is the next-rightmost
    set_lat_dim(d_array->dim_begin() + (d_array->dimensions() - 2));

    int number_elements_latitude = d_array->dimension_size(get_lat_dim());
    double *lat_map = new double[number_elements_latitude];
    for (int i = 0; i < number_elements_latitude; ++i) {
        lat_map[i] = ((d_extent.d_bottom - d_extent.d_top) / (number_elements_latitude - 1)) * i + d_extent.d_top;
    }
    set_lat(lat_map);
    set_lat_size(number_elements_latitude);

    return get_lat() && get_lon();
}

/** Are the latitude and longitude dimensions ordered so that this class can
    properly constrain the data?

    @note This version always returns true because geoarray() assumes the
    dimensions are ordered properly.

    @return Always returns True. */
bool
ArrayGeoConstraint::lat_lon_dimensions_ok()
{
    return true;
}

/** Once the bounding box is set, apply the constraint. If the data can be sent
    using Vector::serialize(), do so. If they cannot, read and organize the
    data so that Vector::serialize() will be able to send the data when asked
    to.

    How can it be that Vector::serialize() would not be able to read the data?
    If the longitude extent of the bounding box for the constraint wraps around
    the edge of the data/array, then two reads are required to get the data.
    This method performs those reads (using the constraints and the read()
    method so that the data server's type-specific and optimized code will be
    used to read actual data values) and then loads the combined result
    back into the object, marking it as having been read. Vector::serialize()
    will then see the object is loaded with data values, skip the regular read
    call and send all the data in the buffer. */
void ArrayGeoConstraint::apply_constraint_to_data()
{
    if (!is_bounding_box_set())
        throw InternalErr(
            "The Latitude and Longitude constraints must be set before calling\n\
            apply_constraint_to_data().");

    if (get_latitude_sense() == inverted) {
        int tmp = get_latitude_index_top();
        set_latitude_index_top(get_latitude_index_bottom());
        set_latitude_index_bottom(tmp);
    }

    // It's easy to flip the Latitude values; if the bottom index value
    // is before/above the top index, return an error explaining that.
    if (get_latitude_index_top() > get_latitude_index_bottom())
        throw Error("The upper and lower latitude indexes appear to be reversed. Please provide\nthe latitude bounding box numbers giving the northern-most latitude first.");

    d_array->add_constraint(get_lat_dim(),
                            get_latitude_index_top(), 1,
                            get_latitude_index_bottom());

    // Does the longitude constraint cross the edge of the longitude vector?
    // If so, reorder the data (array).
    if (get_longitude_index_left() > get_longitude_index_right()) {
        reorder_data_longitude_axis(*d_array, get_lon_dim());

        // Now the data are all in local storage

        // alter the indexes; the left index has now been moved to 0, and the right
        // index is now at lon_vector_length-left+right.
        set_longitude_index_right(get_lon_size() - get_longitude_index_left()
                                  + get_longitude_index_right());
        set_longitude_index_left(0);
    }
    // If the constraint used the -180/179 (neg_pos) notation, transform
    // the longitude map s it uses the -180/179 notation. Note that at this
    // point, d_longitude always uses the pos notation because of the earlier
    // conditional transformation.

    // Apply constraint; stride is always one
    d_array->add_constraint(get_lon_dim(),
                            get_longitude_index_left(), 1,
                            get_longitude_index_right());

    // Load the array if it has been read, which will be the case if
    // reorder_data_longitude_axis() has been called.
    if (get_array_data()) {

        int size = d_array->val2buf(get_array_data());

        if (size != get_array_data_size())
            throw InternalErr
            ("Expected data size not copied to the Grid's buffer.");
        d_array->set_read_p(true);
    }
    else {
        d_array->read();
    }
}

} // namespace libdap

