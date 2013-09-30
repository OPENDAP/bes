
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

// The Grid Selection Expression Clause class.


#include "config.h"

#include <cstring>
#include <cmath>
#include <iostream>
#include <sstream>
#include <algorithm>  //  for find_if

//#define DODS_DEBUG
//#define DODS_DEBUG2

#include <Float64.h>
#include <Error.h>
#include <InternalErr.h>
#include <dods-datatypes.h>
#include <util.h>
#include <debug.h>

#include "GeoConstraint.h"

using namespace std;

namespace libdap {

/** This is used with find_if(). The GeoConstraint class holds a set of strings
    which are prefixes for variable names. Using the regular find() locates
    only the exact matches, using find_if() with this functor makes is easy
    to treat those set<string> objects as collections of prefixes. */
class is_prefix
{
private:
    string s;
public:
    is_prefix(const string & in): s(in)
    {}

    bool operator()(const string & prefix)
    {
        return s.find(prefix) == 0;
    }
};

/** Look in the containers which hold the units attributes and variable name
    prefixes which are considered as identifying a vector as being a latitude
    or longitude vector.

    @param units A container with a bunch of units attribute values.
    @param names A container with a bunch of variable name prefixes.
    @param var_units The value of the 'units' attribute for this variable.
    @param var_name The name of the variable.
    @return True if the units_value matches any of the accepted attributes
    exactly or if the name_value starts with any of the accepted prefixes */
bool
unit_or_name_match(set < string > units, set < string > names,
                       const string & var_units, const string & var_name)
{
    return (units.find(var_units) != units.end()
            || find_if(names.begin(), names.end(),
                       is_prefix(var_name)) != names.end());
}

/** A private method that determines if the longitude part of the bounding
    box uses 0/359 or -180/179 notation. This class only supports latitude
    constraints which use 90/-90 notation, so there's no need to figure out
    what sort of notation they use.

    @note This function assumes that if one of the two values is
    negative, then the notation is or the -180/179 form, otherwise not.
    If the user asks for 30 degrees to 50 degrees (or 50 to 30,
    for that matter), there's no real way to tell which notation they are
    using.

    @param left The left side of the bounding box, in degrees
    @param right The right side of the bounding box
    @return The notation (pos or neg_pos) */
GeoConstraint::Notation
GeoConstraint::categorize_notation(const double left,
                                   const double right) const
{
    return (left < 0 || right < 0) ? neg_pos : pos;
}

/* A private method to translate the longitude constraint from -180/179
   notation to 0/359 notation.

   About the two notations:
   0                          180                       360
   |---------------------------|-------------------------|
   0                        180,-180                     0

   so in the neg-pos notation (using the name I give it in this class) both
   180 and -180 are the same longitude. And in the pos notation 0 and 360 are
   the same.

   @param left Value-result parameter; the left side of the bounding box
   @parm right Value-result parameter; the right side of the bounding box */
void
GeoConstraint::transform_constraint_to_pos_notation(double &left,
        double &right) const
{
    if (left < 0)
	left += 360 ;

    if (right < 0)
	right += 360;
}

/** Given that the Grid has a longitude map that uses the 'neg_pos' notation,
    transform it to the 'pos' notation. This method modifies the d_lon array.

    @note: About the two notations:
   0                          180                       360
   |---------------------------|-------------------------|
   0                        180,-180                     0
*/
void GeoConstraint::transform_longitude_to_pos_notation()
{
    // Assume earlier logic is correct (since the test is expensive)
    // for each value, add 180
    // Longitude could be represented using any of the numeric types
    for (int i = 0; i < d_lon_length; ++i)
	if (d_lon[i] < 0)
	    d_lon[i] += 360;
}

/** Given that the Grid has a longitude map that uses the 'pos' notation,
    transform it to the 'neg_pos' notation. This method modifies the
    d_lon array.

    @note: About the two notations:
   0                          180                       360
   |---------------------------|-------------------------|
   0                        180,-180                     0
*/
void GeoConstraint::transform_longitude_to_neg_pos_notation()
{
    for (int i = 0; i < d_lon_length; ++i)
	if (d_lon[i] > 180)
	    d_lon[i] -= 360;
}

bool GeoConstraint::is_bounding_box_valid(const double left, const double top,
        const double right, const double bottom) const
{
    if ((left < d_lon[0] && right < d_lon[0])
        || (left > d_lon[d_lon_length-1] && right > d_lon[d_lon_length-1]))
        return false;

    if (d_latitude_sense == normal) {
        // When sense is normal, the largest values are at the origin.
        if ((top > d_lat[0] && bottom > d_lat[0])
            || (top < d_lat[d_lat_length-1] && bottom < d_lat[d_lat_length-1]))
            return false;
    }
    else {
        if ((top < d_lat[0] && bottom < d_lat[0])
            || (top > d_lat[d_lat_length-1] && bottom > d_lat[d_lat_length-1]))
            return false;
    }

    return true;
}

/** Scan from the left to the right, and the right to the left, looking
    for the left and right bounding box edges, respectively.

    @param left The left edge of the bounding box
    @param right The right edge
    @param longitude_index_left Value-result parameter that holds the index
    in the grid's longitude map of the left bounding box edge. Uses a closed
    interval for the test.
    @param  longitude_index_right Value-result parameter for the right edge
    index. */
void GeoConstraint::find_longitude_indeces(double left, double right,
        int &longitude_index_left, int &longitude_index_right) const
{
    // Use all values 'modulo 360' to take into account the cases where the
    // constraint and/or the longitude vector are given using values greater
    // than 360 (i.e., when 380 is used to mean 20).
    double t_left = fmod(left, 360.0);
    double t_right = fmod(right, 360.0);

    // Find the place where 'longitude starts.' That is, what value of the
    // index 'i' corresponds to the smallest value of d_lon. Why we do this:
    // Some data sources use offset longitude axes so that the 'seam' is
    // shifted to a place other than the date line.
    int i = 0;
    int lon_origin_index = 0;
    double smallest_lon = fmod(d_lon[0], 360.0);
    while (i < d_lon_length) {
	double curent_lon_value = fmod(d_lon[i], 360.0);
        if (smallest_lon > curent_lon_value) {
            smallest_lon = curent_lon_value;
            lon_origin_index = i;
        }
        ++i;
    }

    DBG2(cerr << "lon_origin_index: " << lon_origin_index << endl);

    // Scan from the index of the smallest value looking for the place where
    // the value is greater than or equal to the left most point of the bounding
    // box.
    i = lon_origin_index;
    while (fmod(d_lon[i], 360.0) < t_left) {
        ++i;
        i = i % d_lon_length;

        // If we cycle completely through all the values/indices, throw
        if (i == lon_origin_index)
            throw Error("geogrid: Could not find an index for the longitude value '" + double_to_string(left) + "'");
    }

    if (fmod(d_lon[i], 360.0) == t_left)
        longitude_index_left = i;
    else
        longitude_index_left = (i - 1) > 0 ? i - 1 : 0;

    DBG2(cerr << "longitude_index_left: " << longitude_index_left << endl);

    // Assume the vector is circular --> the largest value is next to the
    // smallest.
    int largest_lon_index = (lon_origin_index - 1 + d_lon_length) % d_lon_length;
    i = largest_lon_index;
    while (fmod(d_lon[i], 360.0) > t_right) {
        // This is like modulus but for 'counting down'
        i = (i == 0) ? d_lon_length - 1 : i - 1;
        if (i == largest_lon_index)
            throw Error("geogrid: Could not find an index for the longitude value '" + double_to_string(right) + "'");
    }

    if (fmod(d_lon[i], 360.0) == t_right)
        longitude_index_right = i;
    else
        longitude_index_right = (i + 1) < d_lon_length - 1 ? i + 1 : d_lon_length - 1;

    DBG2(cerr << "longitude_index_right: " << longitude_index_right << endl);
}

/** Scan from the top to the bottom, and the bottom to the top, looking
    for the top and bottom bounding box edges, respectively.

    @param top The top edge of the bounding box
    @param bottom The bottom edge
    @param sense Does the array/grid store data with larger latitudes at
    the starting indices or are the latitude 'upside down?'
    @param latitude_index_top Value-result parameter that holds the index
    in the grid's latitude map of the top bounding box edge. Uses a closed
    interval for the test.
    @param  latitude_index_bottom Value-result parameter for the bottom edge
    index. */
void GeoConstraint::find_latitude_indeces(double top, double bottom,
        LatitudeSense sense,
        int &latitude_index_top,
        int &latitude_index_bottom) const
{
    int i, j;

    if (sense == normal) {
	i = 0;
        while (i < d_lat_length - 1 && top < d_lat[i])
            ++i;

        j = d_lat_length - 1;
        while (j > 0 && bottom > d_lat[j])
            --j;

        if (d_lat[i] == top)
            latitude_index_top = i;
        else
            latitude_index_top = (i - 1) > 0 ? i - 1 : 0;

        if (d_lat[j] == bottom)
            latitude_index_bottom = j;
        else
            latitude_index_bottom =
                (j + 1) < d_lat_length - 1 ? j + 1 : d_lat_length - 1;
    }
    else {
        i = d_lat_length - 1;
        while (i > 0 && d_lat[i] > top)
            --i;

        j = 0;
        while (j < d_lat_length - 1 && d_lat[j] < bottom)
            ++j;

        if (d_lat[i] == top)
            latitude_index_top = i;
        else
            latitude_index_top = (i + 1) < d_lat_length - 1 ? i + 1 : d_lat_length - 1;

        if (d_lat[j] == bottom)
            latitude_index_bottom = j;
        else
            latitude_index_bottom = (j - 1) > 0 ? j - 1 : 0;
    }
}

/** Take a look at the latitude vector values and record whether the world is
    normal or upside down.
    @return normal or inverted. */
GeoConstraint::LatitudeSense GeoConstraint::categorize_latitude() const
{
    return d_lat[0] >= d_lat[d_lat_length - 1] ? normal : inverted;
}

// Use 'index' as the pivot point. Move the points behind index to the front of
// the vector and those points in front of and at index to the rear.
static void
swap_vector_ends(char *dest, char *src, int len, int index, int elem_sz)
{
    memcpy(dest, src + index * elem_sz, (len - index) * elem_sz);

    memcpy(dest + (len - index) * elem_sz, src, index * elem_sz);
}

template<class T>
static void transpose(std::vector<std::vector<T> > a,
	std::vector<std::vector<T> > b, int width, int height)
{
    for (int i = 0; i < width; i++) {
	for (int j = 0; j < height; j++) {
	    b[j][i] = a[i][j];
	}
    }
}

/** Given a vector of doubles, transpose the elements. Use this to flip the
 * latitude vector for a Grid that stores the southern latitudes at the top
 * instead of the bottom.
 *
 * @param src A pointer to the vector
 * @param length The number of elements in the vector
 */
void GeoConstraint::transpose_vector(double *src, const int length)
{
    double *tmp = new double[length];

    int i = 0, j = length-1;
    while (i < length)
	tmp[j--] = src[i++];

    memcpy(src, tmp,length * sizeof(double));

    delete[] tmp;
}

static int
count_size_except_latitude_and_longitude(Array &a)
{
    if (a.dim_end() - a.dim_begin() <= 2)	// < 2 is really an error...
	return 1;

    int size = 1;
    for (Array::Dim_iter i = a.dim_begin(); (i + 2) != a.dim_end(); ++i)
        size *= a.dimension_size(i, true);

    return size;
}

void GeoConstraint::flip_latitude_within_array(Array &a, int lat_length,
	int lon_length)
{
    if (!d_array_data) {
	a.read();
	d_array_data = static_cast<char*>(a.value());
	d_array_data_size = a.width(true);	// Bytes not elements
    }

    int size = count_size_except_latitude_and_longitude(a);
    // char *tmp_data = new char[d_array_data_size];
    vector<char> tmp_data(d_array_data_size);
    int array_elem_size = a.var()->width(true);
    int lat_lon_size = (d_array_data_size / size);

    DBG(cerr << "lat, lon_length: " << lat_length << ", " << lon_length << endl);
    DBG(cerr << "size: " << size << endl);
    DBG(cerr << "d_array_data_size: " << d_array_data_size << endl);
    DBG(cerr << "array_elem_size: " << array_elem_size<< endl);
    DBG(cerr << "lat_lon_size: " << lat_lon_size<< endl);

    for (int i = 0; i < size; ++i) {
	int lat = 0;
	int s_lat = lat_length - 1;
	// lon_length is the element size; memcpy() needs the number of bytes
	int lon_size = array_elem_size * lon_length;
	int offset = i * lat_lon_size;
	while (s_lat > -1)
	    memcpy(&tmp_data[0] + offset + (lat++ * lon_size),
		    d_array_data + offset + (s_lat-- * lon_size),
		    lon_size);
    }

    memcpy(d_array_data, &tmp_data[0], d_array_data_size);
}

/** Reorder the elements in the longitude map so that the longitude constraint no
    longer crosses the edge of the map's storage. The d_lon field is
    modified.

    @note The d_lon vector always has double values regardless of the type
    of d_longitude.

    @param longitude_index_left The left edge of the bounding box. */
void GeoConstraint::reorder_longitude_map(int longitude_index_left)
{
    double *tmp_lon = new double[d_lon_length];

    swap_vector_ends((char *) tmp_lon, (char *) d_lon, d_lon_length,
                     longitude_index_left, sizeof(double));

    memcpy(d_lon, tmp_lon, d_lon_length * sizeof(double));

    delete[]tmp_lon;
}

static int
count_dimensions_except_longitude(Array &a)
{
    int size = 1;
    for (Array::Dim_iter i = a.dim_begin(); (i + 1) != a.dim_end(); ++i)
        size *= a.dimension_size(i, true);

    return size;
}

/** Reorder the data values relative to the longitude axis so that the
    reordered longitude map (see GeoConstraint::reorder_longitude_map())
    and the data values match.

    @note This should be called with the Array that contains the d_lon_dim
    Array::Dim_iter.

    @note This method must set the d_array_data and d_array_data_size
    fields. If those are set, apply_constraint_to_data() will use those
    values.

    @note First set all the other constraints, including the latitude and
    then make this call. Other constraints, besides latitude, will be simple
    range constraints. Latitude might require that values be flipped, but
    that can be done _after_ the longitude reordering takes place.

    @todo Fix this code so that it works with latitude as the rightmost map */
void GeoConstraint::reorder_data_longitude_axis(Array &a, Array::Dim_iter lon_dim)
{

    if (!is_longitude_rightmost())
        throw Error("This grid does not have Longitude as its rightmost dimension, the geogrid()\ndoes not support constraints that wrap around the edges of this type of grid.");

    DBG(cerr << "Constraint for the left half: " << get_longitude_index_left()
        << ", " << get_lon_length() - 1 << endl);

    // Build a constraint for the left part and get those values
    a.add_constraint(lon_dim, get_longitude_index_left(), 1,
                     get_lon_length() - 1);
    a.set_read_p(false);
    a.read();
    DBG2(a.print_val(stderr));

    // Save the left-hand data to local storage
    int left_size = a.width(true);		// width() == length() * element size
    char *left_data = (char*)a.value();	// value() allocates and copies

    // Build a constraint for the 'right' part, which goes from the left edge
    // of the array to the right index and read those data.
    // (Don't call a.clear_constraint() since that will clear the constraint on
    // all the dimensions while add_constraint() will replace a constraint on
    // the given dimension).

    DBG(cerr << "Constraint for the right half: " << 0
        << ", " << get_longitude_index_right() << endl);

    a.add_constraint(lon_dim, 0, 1, get_longitude_index_right());
    a.set_read_p(false);
    a.read();
    DBG2(a.print_val(stderr));

    char *right_data = (char*)a.value();
    int right_size = a.width(true);

    // Make one big lump O'data
    d_array_data_size = left_size + right_size;
    d_array_data = new char[d_array_data_size];

    // Assume COARDS conventions are being followed: lon varies fastest.
    // These *_elements variables are actually elements * bytes/element since
    // memcpy() uses bytes.
    int elem_size = a.var()->width(true);
    int left_row_size = (get_lon_length() - get_longitude_index_left()) * elem_size;
    int right_row_size = (get_longitude_index_right() + 1) * elem_size;
    int total_bytes_per_row = left_row_size + right_row_size;

    DBG2(cerr << "elem_size: " << elem_size << "; left & right size: "
	    << left_row_size << ", " << right_row_size << endl);

    // This will work for any number of dimension so long as longitude is the
    // right-most array dimension.
    int rows_to_copy = count_dimensions_except_longitude(a);
    for (int i = 0; i < rows_to_copy; ++i) {
	DBG(cerr << "Copying " << i << "th row" << endl);
	DBG(cerr << "left memcpy: " << *(float *)(left_data + (left_row_size * i)) << endl);

        memcpy(d_array_data + (total_bytes_per_row * i),
               left_data + (left_row_size * i),
               left_row_size);

        DBG(cerr << "right memcpy: " << *(float *)(right_data + (right_row_size * i)) << endl);

        memcpy(d_array_data + (total_bytes_per_row * i) + left_row_size,
               right_data + (right_row_size * i),
               right_row_size);
    }

    delete[]left_data;
    delete[]right_data;
}

/** @brief Initialize GeoConstraint.
 */
GeoConstraint::GeoConstraint()
        : d_array_data(0), d_array_data_size(0),
        d_lat(0), d_lon(0),
        d_bounding_box_set(false),
        d_longitude_rightmost(false),
        d_longitude_notation(unknown_notation),
        d_latitude_sense(unknown_sense)
{
    // Build sets of attribute values for easy searching. Maybe overkill???
    d_coards_lat_units.insert("degrees_north");
    d_coards_lat_units.insert("degree_north");
    d_coards_lat_units.insert("degree_N");
    d_coards_lat_units.insert("degrees_N");

    d_coards_lon_units.insert("degrees_east");
    d_coards_lon_units.insert("degree_east");
    d_coards_lon_units.insert("degrees_E");
    d_coards_lon_units.insert("degree_E");

    d_lat_names.insert("COADSY");
    d_lat_names.insert("lat");
    d_lat_names.insert("Lat");
    d_lat_names.insert("LAT");

    d_lon_names.insert("COADSX");
    d_lon_names.insert("lon");
    d_lon_names.insert("Lon");
    d_lon_names.insert("LON");
}

/** Set the bounding box for this constraint. After calling this method the
    object has values for the indexes for the latitude and longitude extent
    as well as the sense of the latitude (south pole at the top or bottom of
    the Array or Grid). These are used by the apply_constraint_to_data()
    method to actually constrain the data.

    @param left The left side of the bounding box.
    @param right The right side
    @param top The top
    @param bottom The bottom */
void GeoConstraint::set_bounding_box(double top, double left,
                                     double bottom, double right)
{
    // Ensure this method is called only once. What about pthreads?
    // The method Array::reset_constraint() might make this so it could be
    // called more than once. jhrg 8/30/06
    if (d_bounding_box_set)
        throw Error("It is not possible to register more than one geographical constraint on a variable.");

    // Record the 'sense' of the latitude for use here and later on.
    d_latitude_sense = categorize_latitude();
#if 0
    if (d_latitude_sense == inverted)
	throw Error("geogrid() does not currently work with inverted data (data where the north pole is at a negative latitude value).");
#endif

    // Categorize the notation used by the bounding box (0/359 or -180/179).
    d_longitude_notation = categorize_notation(left, right);

    // If the notation uses -180/179, transform the request to 0/359 notation.
    if (d_longitude_notation == neg_pos)
        transform_constraint_to_pos_notation(left, right);

    // If the grid uses -180/179, transform it to 0/359 as well. This will make
    // subsequent logic easier and adds only a few extra operations, even with
    // large maps.
    Notation longitude_notation =
        categorize_notation(d_lon[0], d_lon[d_lon_length - 1]);

    if (longitude_notation == neg_pos)
        transform_longitude_to_pos_notation();

    if (!is_bounding_box_valid(left, top, right, bottom))
        throw Error("The bounding box does not intersect any data within this Grid or Array. The\ngeographical extent of these data are from latitude "
		    + double_to_string(d_lat[0]) + " to "
		    + double_to_string(d_lat[d_lat_length-1])
		    + "\nand longitude " + double_to_string(d_lon[0])
		    + " to " + double_to_string(d_lon[d_lon_length-1])
		    + " while the bounding box provided was latitude "
		    + double_to_string(top) + " to "
		    + double_to_string(bottom) + "\nand longitude "
		    + double_to_string(left) + " to "
		    + double_to_string(right));

    // This is simpler than the longitude case because there's no need to
    // test for several notations, no need to accommodate them in the return,
    // no modulo arithmetic for the axis and no need to account for a
    // constraint with two disconnected parts to be joined.
    find_latitude_indeces(top, bottom, d_latitude_sense,
                          d_latitude_index_top, d_latitude_index_bottom);


    // Find the longitude map indexes that correspond to the bounding box.
    find_longitude_indeces(left, right, d_longitude_index_left,
                           d_longitude_index_right);

    DBG(cerr << "Bounding box (tlbr): " << d_latitude_index_top << ", "
        << d_longitude_index_left << ", "
        << d_latitude_index_bottom << ", "
        << d_longitude_index_right << endl);

    d_bounding_box_set = true;
}

} // namespace libdap
