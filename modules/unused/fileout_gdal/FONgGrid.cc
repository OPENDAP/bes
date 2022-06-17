// FONgGrid.cc

// This file is part of BES GDAL File Out Module

// Copyright (c) 2012 OPeNDAP, Inc.
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
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

#include <algorithm>

#include <gdal.h>
#include <gdal_priv.h>
#include <ogr_spatialref.h>

#include <libdap/DDS.h>
#include <libdap/Grid.h>
// #include <ce_functions.h>
#include <libdap/util.h>

#include <BESInternalError.h>
#include <BESDebug.h>

#include "GeoTiffTransmitter.h"
#include "FONgTransform.h"
#include "FONgGrid.h"

using namespace libdap;

/** @brief Constructor for FONgGrid that takes a DAP Grid
 *
 * @param g A DAP BaseType that should be a grid
 */
FONgGrid::FONgGrid(Grid *g): d_grid(g), d_lat(0), d_lon(0)

{
    d_type = dods_grid_c;

    // Build sets of attribute values for easy searching.
    // Copied from GeoConstriant in libdap. That class is
    // abstract and didn't want to modify libdap's ABI for this hack.

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

/** @brief Destructor that cleans up the grid
 *
 * The DAP Grid instance does not belong to the FONgGrid instance, so it
 * is not deleted.
 */
FONgGrid::~FONgGrid()
{
}

/** This is used with find_if(). The GeoConstraint class holds a set of strings
 which are prefixes for variable names. Using the regular find() locates
 only the exact matches, using find_if() with this functor makes is easy
 to treat those set<string> objects as collections of prefixes. */
class is_prefix {
private:
    string s;
public:
    is_prefix(const string & in) :
        s(in)
    {
    }

    bool operator()(const string & prefix)
    {
        return s.find(prefix) == 0;
    }
};

/** Is this a latitude map? Use CF's long_name attribute first, then
 * drop into heuristics based on units names or common variable names.
 *
 * @param var_units The value of the 'unit' attribute
 * @param var_name The name of the variable
 * @param long_name The value of the long_name attribute
 *
 * @return true if there's a match, otherwise false
 * @see m_lon_unit_or_name_match()
 */
bool FONgGrid::m_lat_unit_or_name_match(const string &var_units, const string &var_name, const string &long_name)
{
    return (long_name == "latitude" || d_coards_lat_units.find(var_units) != d_coards_lat_units.end()
        || find_if(d_lat_names.begin(), d_lat_names.end(), is_prefix(var_name)) != d_lat_names.end());
}

bool FONgGrid::m_lon_unit_or_name_match(const string &var_units, const string &var_name, const string &long_name)
{
    return (long_name == "longitude" || d_coards_lon_units.find(var_units) != d_coards_lon_units.end()
        || find_if(d_lon_names.begin(), d_lon_names.end(), is_prefix(var_name)) != d_lon_names.end());
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
bool FONgGrid::find_lat_lon_maps()
{
    Grid::Map_iter m = d_grid->map_begin();

    // Assume that a Grid is correct and thus has exactly as many maps as its
    // array part has dimensions. Thus, don't bother to test the Grid's array
    // dimension iterator for '!= dim_end()'.
    Array::Dim_iter d = d_grid->get_array()->dim_begin();

    // The fields d_latitude and d_longitude may be initialized to null or they
    // may already contain pointers to the maps to use. In the latter case,
    // skip the heuristics used in this code. However, given that all this
    // method does is find the lat/lon maps, if they are given in the ctor,
    // This method will likely not be called at all.

    while (m != d_grid->map_end() && (!d_lat || !d_lon)) {
        string units_value = (*m)->get_attr_table().get_attr("units");
        units_value = remove_quotes(units_value);
        string long_name = (*m)->get_attr_table().get_attr("long_name");
        long_name = remove_quotes(long_name);
        string map_name = (*m)->name();

        // The 'units' attribute must match exactly; the name only needs to
        // match a prefix.
        if (!d_lat && m_lat_unit_or_name_match(units_value, map_name, long_name)) {
            // Set both d_latitude (a pointer to the real map vector) and
            // d_lat, a vector of the values represented as doubles. It's easier
            // to work with d_lat, but it's d_latitude that needs to be set
            // when constraining the grid. Also, record the grid variable's
            // dimension iterator so that it's easier to set the Grid's Array
            // (which also has to be constrained).
            d_lat = dynamic_cast<Array *>(*m);
            if (!d_lat) throw InternalErr(__FILE__, __LINE__, "Expected an array.");

            if (!d_lat->read_p()) d_lat->read();
        }

        if (!d_lon && m_lon_unit_or_name_match(units_value, map_name, long_name)) {
            d_lon = dynamic_cast<Array *>(*m);
            if (!d_lon) throw InternalErr(__FILE__, __LINE__, "Expected an array.");

            if (!d_lon->read_p()) d_lon->read();
        }

        ++m;
        ++d;
    }

    return d_lat && d_lon;
}

/** Extract the size (pixels), element data type and top-left and
 * bottom-right lat/lon corner points for the Grid. Also determine
 * if this is a 2D or 3D Grid and, in the latter case, ensure that
 * the first dimension is not lat or lon. In that case, the geotiff
 * will have N bands, where N is the number of elements from the
 * first dimension in the current selection.
 *
 */
void FONgGrid::extract_coordinates(FONgTransform &t)
{
    BESDEBUG("fong3", "Entering FONgGrid::extract_coordinates" << endl);

    double *lat = 0, *lon = 0;
    try {
        // Find the lat and lon maps for this Grid; throws Error
        // if the are not found.
        find_lat_lon_maps();

        // The array size
        t.set_height(d_lat->length());
        t.set_width(d_lon->length());

        lat = extract_double_array(d_lat);
        lon = extract_double_array(d_lon);

        //max latidude
        t.set_top(max (lat[0], lat[t.height()-1]));
        t.set_left(lon[0]);
        // min latitude
        t.set_bottom(min (lat[0], lat[t.height()-1]));
        t.set_right(lon[t.width() - 1]);

        // Read this from the 'missing_value' or '_FillValue' attributes
        string missing_value = d_grid->get_attr_table().get_attr("missing_value");
        if (missing_value.empty()) missing_value = d_grid->get_array()->get_attr_table().get_attr("missing_value");
        if (missing_value.empty()) missing_value = d_grid->get_attr_table().get_attr("_FillValue");
        if (missing_value.empty()) missing_value = d_grid->get_array()->get_attr_table().get_attr("_FillValue");

        BESDEBUG("fong3", "missing_value attribute: " << missing_value << endl);

        // NB: no_data_type() is 'none' by default
        if (!missing_value.empty()) {
            t.set_no_data(missing_value);
            if (t.no_data() < 0)
                t.set_no_data_type(FONgTransform::negative);
            else
                t.set_no_data_type(FONgTransform::positive);
        }

        t.geo_transform_set(true);

        t.set_num_bands(t.num_bands() + 1);
        t.push_var(this);

        delete[] lat;
        delete[] lon;
    }
    catch (Error &e) {
        delete[] lat;
        delete[] lon;

        throw;
    }
}

/** Use CF to determine if this is a 'Spherical Earth' datum */
static bool is_spherical(BaseType *btp)
{
    /* crs:grid_mapping_name = "latitude_longitude"
     crs:semi_major_axis = 6371000.0 ;
     crs:inverse_flattening = 0 ; */

    bool gmn = btp->get_attr_table().get_attr("grid_mapping_name") == "latitude_longitude";
    bool sma = btp->get_attr_table().get_attr("semi_major_axis") == "6371000.0";
    bool iflat = btp->get_attr_table().get_attr("inverse_flattening") == "0";

    return gmn && sma && iflat;
}

/** Use CF to determine if this is a 'WGS84' datum */
static bool is_wgs84(BaseType *btp)
{
    /*crs:grid_mapping_name = "latitude_longitude";
     crs:longitude_of_prime_meridian = 0.0 ;
     crs:semi_major_axis = 6378137.0 ;
     crs:inverse_flattening = 298.257223563 ; */

    bool gmn = btp->get_attr_table().get_attr("grid_mapping_name") == "latitude_longitude";
    bool lpm = btp->get_attr_table().get_attr("longitude_of_prime_meridian") == "0.0";
    bool sma = btp->get_attr_table().get_attr("semi_major_axis") == "6378137.0";
    bool iflat = btp->get_attr_table().get_attr("inverse_flattening") == "298.257223563";

    return gmn && lpm && sma && iflat;
}

/** @brief Set the projection information
 * For Grids, look for CF information. If it's not present, use the
 * default Geographic Coordinate system set in the bes/module conf
 * file. if it is present, look at the attributes and dope out a Well
 * Known Geographic coordinate string.
 */
string FONgGrid::get_projection(DDS *dds)
{
    // Here's the information about the CF and projections
    // http://cf-pcmdi.llnl.gov/documents/cf-conventions/1.4/cf-conventions.html#grid-mappings-and-projections
    // How this code looks for mapping information: Look for an
    // attribute named 'grid_mapping' and get it's value. This attribute
    // with be the name of a variable in the dataset, so get that
    // variable. Now, look at the attributes of that variable.
    string mapping_info = d_grid->get_attr_table().get_attr("grid_mapping");
    if (mapping_info.empty()) mapping_info = d_grid->get_array()->get_attr_table().get_attr("grid_mapping");

    string WK_GCS = GeoTiffTransmitter::default_gcs;

    if (!mapping_info.empty()) {
        // "WGS84": same as "EPSG:4326" but has no dependence on EPSG data files.
        // "WGS72": same as "EPSG:4322" but has no dependence on EPSG data files.
        // "NAD27": same as "EPSG:4267" but has no dependence on EPSG data files.
        // "NAD83": same as "EPSG:4269" but has no dependence on EPSG data files.
        // "EPSG:n": same as doing an ImportFromEPSG(n).

        // The mapping info is actually stored as attributes of an Int32 variable.
        BaseType *btp = dds->var(mapping_info);
        if (btp && btp->name() == "crs") {
            if (is_wgs84(btp))
                WK_GCS = "WGS84";
            else if (is_spherical(btp)) WK_GCS = "EPSG:4047";
        }
    }

    OGRSpatialReference srs;
    srs.SetWellKnownGeogCS(WK_GCS.c_str());
    char *srs_wkt = NULL;
    srs.exportToWkt(&srs_wkt);

    string wkt = srs_wkt;   // save the result

    CPLFree(srs_wkt);       // free memory alloc'd by GDAL

    return wkt;
}

double *FONgGrid::get_data()
{
    if (!d_grid->get_array()->read_p()) d_grid->get_array()->read();

    // This code assumes read() has been called.
    return extract_double_array(d_grid->get_array());
}

