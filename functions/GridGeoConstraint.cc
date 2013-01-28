
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// The Grid Selection Expression Clause class.


#include "config.h"

#include <cmath>

#include <iostream>
#include <sstream>

//#define DODS_DEBUG

#include "debug.h"
#include "dods-datatypes.h"
#include "GridGeoConstraint.h"
#include "Float64.h"

#include "Error.h"
#include "InternalErr.h"
#include "ce_functions.h"
#include "util.h"

using namespace std;

namespace libdap {

/** @brief Initialize GeoConstraint with a Grid.

    @param grid Set the GeoConstraint to use this Grid variable. It is the
    caller's responsibility to ensure that the value \e grid is a valid Grid
    variable.
 */
GridGeoConstraint::GridGeoConstraint(Grid *grid)
        : GeoConstraint(), d_grid(grid), d_latitude(0), d_longitude(0)
{
    if (d_grid->get_array()->dimensions() < 2
        || d_grid->get_array()->dimensions() > 3)
        throw Error("The geogrid() function works only with Grids of two or three dimensions.");

    // Is this Grid a geo-referenced grid? Throw Error if not.
    if (!build_lat_lon_maps())
        throw Error(string("The grid '") + d_grid->name()
                    + "' does not have identifiable latitude/longitude map vectors.");

    if (!lat_lon_dimensions_ok())
        throw Error("The geogrid() function will only work when the Grid's Longitude and Latitude\nmaps are the rightmost dimensions.");
}

GridGeoConstraint::GridGeoConstraint(Grid *grid, Array *lat, Array *lon)
        : GeoConstraint(), d_grid(grid), d_latitude(0), d_longitude(0)
{
    if (d_grid->get_array()->dimensions() < 2
        || d_grid->get_array()->dimensions() > 3)
        throw Error("The geogrid() function works only with Grids of two or three dimensions.");

    // Is this Grid a geo-referenced grid? Throw Error if not.
    if (!build_lat_lon_maps(lat, lon))
        throw Error(string("The grid '") + d_grid->name()
                    + "' does not have valid latitude/longitude map vectors.");


    if (!lat_lon_dimensions_ok())
        throw Error("The geogrid() function will only work when the Grid's Longitude and Latitude\nmaps are the rightmost dimensions.");
}

/** A private method called by the constructor that searches for latitude
    and longitude map vectors. This method returns false if either map
    cannot be found. It assumes that the d_grid and d_dds fields are set.

    The d_longitude, d_lon, d_lon_length and d_lon_grid_dim (and matching
    lat) fields are modified.

    @note Rules used to find Maps:<ul>
    <li>Latitude: If the Map has a units attribute of "degrees_north",
    "degree_north", "degree_N", or "degrees_N"</li>
    <li>Longitude: If the map has a units attribute of "degrees_east"
    (eastward positive), "degree_east", "degree_E", or "degrees_E"</li>
    </ul>

    @return True if the maps are found, otherwise False */
bool GridGeoConstraint::build_lat_lon_maps()
{
    Grid::Map_iter m = d_grid->map_begin();

    // Assume that a Grid is correct and thus has exactly as many maps as its
    // array part has dimensions. Thus don't bother to test the Grid's array
    // dimension iterator for '!= dim_end()'.
    Array::Dim_iter d = d_grid->get_array()->dim_begin();

    // The fields d_latitude and d_longitude may be initialized to null or they
    // may already contain pointers to the maps to use. In the latter case,
    // skip the heuristics used in this code. However, given that all this
    // method does is find the lat/lon maps, if they are given in the ctor,
    // This method will likely not be called at all.
    while (m != d_grid->map_end() && (!d_latitude || !d_longitude)) {
        string units_value = (*m)->get_attr_table().get_attr("units");
        units_value = remove_quotes(units_value);
        string map_name = (*m)->name();

        // The 'units' attribute must match exactly; the name only needs to
        // match a prefix.
        if (!d_latitude
            && unit_or_name_match(get_coards_lat_units(), get_lat_names(),
                                  units_value, map_name)) {

            // Set both d_latitude (a pointer to the real map vector) and
            // d_lat, a vector of the values represented as doubles. It's easier
            // to work with d_lat, but it's d_latitude that needs to be set
            // when constraining the grid. Also, record the grid variable's
            // dimension iterator so that it's easier to set the Grid's Array
            // (which also has to be constrained).
            d_latitude = dynamic_cast < Array * >(*m);
            if (!d_latitude)
                throw InternalErr(__FILE__, __LINE__, "Expected an array.");
            if (!d_latitude->read_p())
                d_latitude->read();

            set_lat(extract_double_array(d_latitude));   // throws Error
            set_lat_length(d_latitude->length());

            set_lat_dim(d);
        }

        if (!d_longitude        // && !units_value.empty()
            && unit_or_name_match(get_coards_lon_units(), get_lon_names(),
                                  units_value, map_name)) {

            d_longitude = dynamic_cast < Array * >(*m);
            if (!d_longitude)
                throw InternalErr(__FILE__, __LINE__, "Expected an array.");
            if (!d_longitude->read_p())
                d_longitude->read();

            set_lon(extract_double_array(d_longitude));
            set_lon_length(d_longitude->length());

            set_lon_dim(d);

            if (m + 1 == d_grid->map_end())
            	set_longitude_rightmost(true);
        }

        ++m;
        ++d;
    }

    return get_lat() && get_lon();
}

/** A private method called by the constructor that checks to make sure the
    two arrays passed to the constructor are valid latitude and longitude
    maps. If so, a they are read in and the values are slurped up by this
    object. Then the Grid's dimension iterator is used to record a reference
    the the lat and lon dimension of the Grid itself.

    @return True if the maps are valid, otherwise False */
bool GridGeoConstraint::build_lat_lon_maps(Array *lat, Array *lon)
{
    Grid::Map_iter m = d_grid->map_begin();

    Array::Dim_iter d = d_grid->get_array()->dim_begin();

    while (m != d_grid->map_end() && (!d_latitude || !d_longitude)) {
	// Look for the Grid map that matches the variable passed as 'lat'
	if (!d_latitude && *m == lat) {

            d_latitude = lat;

            if (!d_latitude->read_p())
                d_latitude->read();

            set_lat(extract_double_array(d_latitude));   // throws Error
            set_lat_length(d_latitude->length());

            set_lat_dim(d);
        }

        if (!d_longitude && *m == lon) {

            d_longitude = lon;

            if (!d_longitude->read_p())
                d_longitude->read();

            set_lon(extract_double_array(d_longitude));
            set_lon_length(d_longitude->length());

            set_lon_dim(d);

            if (m + 1 == d_grid->map_end())
            	set_longitude_rightmost(true);
        }

        ++m;
        ++d;
    }

    return get_lat() && get_lon();
}

/** Are the latitude and longitude dimensions ordered so that this class can
    properly constrain the data? This method throws Error if lat and lon are
    not to two 'fastest-varying' (or 'rightmost') dimensions. It also sets the
    internal property \e longitude_rightmost if that's true.

    @note Called by the constructor once build_lat_lon_maps() has returned.

    @return True if the lat/lon maps are the two rightmost maps,
    false otherwise; modifies the \e longitude_rightmost property as aside
    effect. */
bool
GridGeoConstraint::lat_lon_dimensions_ok()
{
    // get the last two map iterators
    Grid::Map_riter rightmost = d_grid->map_rbegin();
    Grid::Map_riter next_rightmost = rightmost + 1;

    if (*rightmost == d_longitude && *next_rightmost == d_latitude)
        set_longitude_rightmost(true);
    else if (*rightmost == d_latitude && *next_rightmost == d_longitude)
        set_longitude_rightmost(false);
    else
        return false;

    return true;
}

/** Once the bounding box is set use this method to apply the constraint. This
    modifies the data values in the Grid so that the software in
    Vector::serialize() will work correctly. Vector::serialize() assumes that
    the BaseType::read() method is called \e after the projection is applied to
    the data. That is, the projection is applied, then data are read. but
    geogrid() first reads all the data values and then computes the projection.
    To make Vector::serialize() work, this method uses the projection
    information recorded in the Grid by set_bounding_box() to arrange data so
    that the information to be sent is all that is held by the Grid. Call this
    after applying any 'Grid selection expressions' of the sort that can be
    passed to the grid() function.

    @note Why do this here? The grid() function uses the standard logic in
    Vector and elsewhere to read data that's to be sent. The problem is that
    the data values need to be reordered using information only this object
    has. If this were implemented as a 'selection function' (i.e., if the code
    was run by ConstraintExpression::eval() then we might be able to better
    optimize how data are read, but in this case we have read all the data
    and may have already reorganized it). Set up the internal buffers so they
    hold the correct values and mark the Grid's array and lat/lon maps as
    read. */
void GridGeoConstraint::apply_constraint_to_data()
{
    if (!is_bounding_box_set())
        throw InternalErr("The Latitude and Longitude constraints must be set before calling apply_constraint_to_data().");

    Array::Dim_iter fd = d_latitude->dim_begin();

    if (get_latitude_sense() == inverted) {
        int tmp = get_latitude_index_top();
        set_latitude_index_top(get_latitude_index_bottom());
        set_latitude_index_bottom(tmp);
    }

    // It's easy to flip the Latitude values; if the bottom index value
    // is before/above the top index, return an error explaining that.
    if (get_latitude_index_top() > get_latitude_index_bottom())
        throw Error("The upper and lower latitude indices appear to be reversed. Please provide the latitude bounding box numbers giving the northern-most latitude first.");

    // Constrain the lat vector and lat dim of the array
    d_latitude->add_constraint(fd, get_latitude_index_top(), 1,
                               get_latitude_index_bottom());
    d_grid->get_array()->add_constraint(get_lat_dim(),
                                        get_latitude_index_top(), 1,
                                        get_latitude_index_bottom());

    // Does the longitude constraint cross the edge of the longitude vector?
    // If so, reorder the grid's data (array), longitude map vector and the
    // local vector of longitude data used for computation.
    if (get_longitude_index_left() > get_longitude_index_right()) {
        reorder_longitude_map(get_longitude_index_left());

        // If the longitude constraint is 'split', join the two parts, reload
        // the data into the Grid's Array and make sure the Array is marked as
        // already read. This should be true for the whole Grid, but if some
        // future modification changes that, the array will be covered here.
        // Note that the following method only reads the data out and stores
        // it in this object after joining the two parts. The method
        // apply_constraint_to_data() transfers the data back from the this
        // object to the DAP Grid variable.
        reorder_data_longitude_axis(*d_grid->get_array(), get_lon_dim());

        // Now that the data are all in local storage alter the indices; the
        // left index has now been moved to 0, and the right index is now
        // at lon_vector_length-left+right.
        set_longitude_index_right(get_lon_length() - get_longitude_index_left()
                                  + get_longitude_index_right());
        set_longitude_index_left(0);
    }

    // If the constraint used the -180/179 (neg_pos) notation, transform
    // the longitude map so it uses the -180/179 notation. Note that at this
    // point, d_longitude always uses the pos notation because of the earlier
    // conditional transformation.

    // Do this _before_ applying the constraint since set_array_using_double()
    // tests the array length using Vector::length() and that method returns
    // the length _as constrained_. We want to move all of the longitude
    // values from d_lon back into the map, not just the number that will be
    // sent (although an optimization might do this, it's hard to imagine
    // it would gain much).
    if (get_longitude_notation() == neg_pos) {
        transform_longitude_to_neg_pos_notation();
    }

    // Apply constraint; stride is always one and maps only have one dimension
    fd = d_longitude->dim_begin();
    d_longitude->add_constraint(fd, get_longitude_index_left(), 1,
                                get_longitude_index_right());

    d_grid->get_array()->add_constraint(get_lon_dim(),
                                        get_longitude_index_left(),
                                        1, get_longitude_index_right());

    // Transfer values from the local lat vector to the Grid's
    // Here test the sense of the latitude vector and invert the vector if the
    // sense is 'inverted' so that the top is always the northern-most value
    if (get_latitude_sense() == inverted) {
	DBG(cerr << "Inverted latitude sense" << endl);
	transpose_vector(get_lat() + get_latitude_index_top(),
		get_latitude_index_bottom() - get_latitude_index_top() + 1);
	// Now read the Array data and flip the latitudes.
	flip_latitude_within_array(*d_grid->get_array(),
		get_latitude_index_bottom() - get_latitude_index_top() + 1,
		get_longitude_index_right() - get_longitude_index_left() + 1);
    }

    set_array_using_double(d_latitude, get_lat() + get_latitude_index_top(),
                           get_latitude_index_bottom() - get_latitude_index_top() + 1);

    set_array_using_double(d_longitude, get_lon() + get_longitude_index_left(),
                           get_longitude_index_right() - get_longitude_index_left() + 1);

    // Look for any non-lat/lon maps and make sure they are read correctly
    Grid::Map_iter i = d_grid->map_begin();
    Grid::Map_iter end = d_grid->map_end();
    while (i != end) {
	if (*i != d_latitude && *i != d_longitude) {
	    if ((*i)->send_p()) {
		DBG(cerr << "reading grid map: " << (*i)->name() << endl);
		//(*i)->set_read_p(false);
		(*i)->read();
	    }
	}
	++i;
    }

    // ... and then the Grid's array if it has been read.
    if (get_array_data()) {
        int size = d_grid->get_array()->val2buf(get_array_data());

        if (size != get_array_data_size())
            throw InternalErr(__FILE__, __LINE__, "Expected data size not copied to the Grid's buffer.");

        d_grid->set_read_p(true);
    }
    else {
        d_grid->get_array()->read();
    }
}

} // namespace libdap
