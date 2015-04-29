
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

#ifndef _geo_constraint_h
#define _geo_constraint_h 1

#include <string>
#include <sstream>
#include <set>

namespace libdap {
class BaseType;
class Array;
class Grid;
}

namespace functions
{

/** Encapsulate the logic needed to handle geographical constraints
    when they are applied to DAP Grid (and some Array) variables.

    This class will apply a longitude/latitude bounding box to a Grid
    that is a 'geo-referenced' Grid. That is, it follows the COARDS/CF
    conventions. This may be relaxed...

    If the longitude range of the constraint crosses the boundary of
    the data array so that the constraint creates two separate
    rectangles, this class will arrange to return the result as a
    single Grid. It will do this by rearranging the data before
    control is passed onto the constraint evaluator and serialization
    logic. Here's a diagram of how it works:

    Suppose a constraint for the longitude BB starts at the left edge
    of L and goes to the right edge of R:

    <pre>
       0.0       180.0       360.0 (longitude, in degrees)
        +----------------------+
        |xxxxxyyyyyyyyyyyyzzzzz|
        -----+            +-----
        |    |            |    |
        | R  |            | L  |
        |    |            |    |
        -----+            +-----
        |                      |
        +----------------------+
    </pre>

    For example, suppose the client provides a bounding box that
    starts at 200 degrees and ends at 80. This class will first copy
    the Left part to new storage and then copy the right part, thus
    'stitching together' the two halves of the constraint. The result
    looks like:

    <pre>
     80.0  360.0/0.0  180.0  ~200.0 (longitude, in degrees)
        +----------------------+
        |zzzzzxxxxxxyyyyyyyyyyy|
        -----++-----           |
        |    ||    |           |
        | L  || R  |           |
        |    ||    |           |
        -----++-----           |
        |                      |
        +----------------------+
    </pre>

    The changes are made in the Grid variable itself, so once this is
    done the Grid should not be re-read by the CE or serialization
    code.

    @author James Gallagher */

class GeoConstraint
{
public:
    /** The longitude extents of the constraint bounding box can be expressed
        two ways: using a 0/359 notation and using a -180/179 notation. I call
        the 0/359 notation 'pos' and the -180/179 notation 'neg_pos'. */
    enum Notation {
        unknown_notation,
        pos,
        neg_pos
    };

    /** Most of the time, latitude starts at the top of an array with
        positive values and ends up at the bottom with negative ones.
        But sometimes... the world is upside down. */
    enum LatitudeSense {
        unknown_sense,
        normal,
        inverted
    };

private:
    char *d_array_data;    	//< Holds the Grid's data values
    int d_array_data_size;	//< Total size (bytes) of the array data

    double *d_lat;              //< Holds the latitude values
    double *d_lon;              //< Holds the longitude values
    int d_lat_length;           //< Elements (not bytes) in the latitude vector
    int d_lon_length;           //< ... longitude vector

    // These four are indexes of the constraint
    int d_latitude_index_top;
    int d_latitude_index_bottom;
    int d_longitude_index_left;
    int d_longitude_index_right;

    bool d_bounding_box_set;    //< Has the bounding box been set?
    bool d_longitude_rightmost; //< Is longitude the rightmost dimension?

    Notation d_longitude_notation;
    LatitudeSense d_latitude_sense;

    libdap::Array::Dim_iter d_lon_dim;  //< References the longitude dimension
    libdap::Array::Dim_iter d_lat_dim;  //< References the latitude dimension

    // Sets of string values used to find stuff in attributes
    set<string> d_coards_lat_units;
    set<string> d_coards_lon_units;

    set<string> d_lat_names;
    set<string> d_lon_names;

    // Hide these three automatically provided methods
    GeoConstraint(const GeoConstraint &param);
    GeoConstraint &operator=(GeoConstraint &rhs);

protected:
    /** A protected method that searches for latitude and longitude
        map vectors and sets six key internal fields. This method
        returns false if either map cannot be found.

        The d_lon, d_lon_length and d_lon_dim (and matching lat)
        fields <em>must be set</em> by this method.

        @return True if the maps are found, otherwise False */
    virtual bool build_lat_lon_maps() = 0;

    /** Are the latitude and longitude dimensions ordered so that this
	class can properly constrain the data? This method throws
	Error if lat and lon are not to two 'fastest-varying' (or
	'rightmost) dimensions. It sets the internal property \e
	longitude_rightmost if that's true.

	@note Called by the constructor once build_lat_lon_maps() has returned.

	@return True if the lat/lon maps are the two rightmost maps,
	false otherwise*/
    virtual bool lat_lon_dimensions_ok() = 0;

    Notation categorize_notation(const double left, const double right) const;
    void transform_constraint_to_pos_notation(double &left, double &right) const;
    virtual void transform_longitude_to_pos_notation();
    virtual void transform_longitude_to_neg_pos_notation();
    virtual bool is_bounding_box_valid(const double left, const double top,
					const double right, const double bottom) const;
    void find_longitude_indeces(double left, double right,
                                int &longitude_index_left,
                                int &longitude_index_right) const;

    virtual void transpose_vector(double *src, const int length);
    virtual void reorder_longitude_map(int longitude_index_left);

    virtual LatitudeSense categorize_latitude() const;
    void find_latitude_indeces(double top, double bottom, LatitudeSense sense,
                               int &latitude_index_top,
                               int &latitude_index_bottom) const;

    virtual void reorder_data_longitude_axis(libdap::Array &a,libdap:: Array::Dim_iter lon_dim);
    virtual void flip_latitude_within_array(libdap::Array &a, int lat_length, int lon_length);

    friend class GridGeoConstraintTest; // Unit tests

public:
    /** @name Constructors */
    //@{
    GeoConstraint();
    //@}

    virtual ~GeoConstraint()
    {
        delete [] d_lat; d_lat = 0;
        delete [] d_lon; d_lon = 0;
        delete [] d_array_data; d_array_data = 0;
    }

    /** @name Accessors/Mutators */
    //@{
    // These are set in reorder_data_longitude_axis()
    char *get_array_data() const
    {
        return d_array_data;
    }
    int get_array_data_size() const
    {
        return d_array_data_size;
    }

    double *get_lat() const
    {
        return d_lat;
    }
    double *get_lon() const
    {
        return d_lon;
    }
    void set_lat(double *lat)
    {
        d_lat = lat;
    }
    void set_lon(double *lon)
    {
        d_lon = lon;
    }

    int get_lat_length() const
    {
        return d_lat_length;
    }
    int get_lon_length() const
    {
        return d_lon_length;
    }
    void set_lat_length(int len)
    {
        d_lat_length = len;
    }
    void set_lon_length(int len)
    {
        d_lon_length = len;
    }

    libdap::Array::Dim_iter get_lon_dim() const
    {
        return d_lon_dim;
    }
    libdap::Array::Dim_iter get_lat_dim() const
    {
        return d_lat_dim;
    }
    void set_lon_dim(libdap::Array::Dim_iter lon)
    {
        d_lon_dim = lon;
    }
    void set_lat_dim(libdap::Array::Dim_iter lat)
    {
        d_lat_dim = lat;
    }

    // These four are indexes of the constraint
    int get_latitude_index_top() const
    {
        return d_latitude_index_top;
    }
    int get_latitude_index_bottom() const
    {
        return d_latitude_index_bottom;
    }
    void set_latitude_index_top(int top)
    {
        d_latitude_index_top = top;
    }
    void set_latitude_index_bottom(int bottom)
    {
        d_latitude_index_bottom = bottom;
    }

    int get_longitude_index_left() const
    {
        return d_longitude_index_left;
    }
    int get_longitude_index_right() const
    {
        return d_longitude_index_right;
    }
    void set_longitude_index_left(int left)
    {
        d_longitude_index_left = left;
    }
    void set_longitude_index_right(int right)
    {
        d_longitude_index_right = right;
    }

    bool is_bounding_box_set() const
    {
        return d_bounding_box_set;
    }
    bool is_longitude_rightmost() const
    {
        return d_longitude_rightmost;
    }
    void set_longitude_rightmost(bool state)
    {
        d_longitude_rightmost = state;
    }

    Notation get_longitude_notation() const
    {
        return d_longitude_notation;
    }
    LatitudeSense get_latitude_sense() const
    {
        return d_latitude_sense;
    }
    void set_longitude_notation(Notation n)
    {
        d_longitude_notation = n;
    }
    void set_latitude_sense(LatitudeSense l)
    {
        d_latitude_sense = l;
    }

    set<string> get_coards_lat_units() const
        {
            return d_coards_lat_units;
        }
    set<string> get_coards_lon_units() const
        {
            return d_coards_lon_units;
        }

    set<string> get_lat_names() const
        {
            return d_lat_names;
        }
    set<string> get_lon_names() const
        {
            return d_lon_names;
        }
    //@}

    void set_bounding_box(double top, double left, double bottom, double right);

    /** @brief Once the bounding box is set use this method to apply
        the constraint. */
    virtual void apply_constraint_to_data() = 0;
};

} // namespace libdap

#endif // _geo_constraint_h

