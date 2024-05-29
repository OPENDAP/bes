// FONgType.h

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

#ifndef FONgType_h_
#define FONgType_h_ 1

#include <set>

#include <BESObj.h>

#include <libdap/Type.h>

class FONgTransform;

namespace libdap {
    class DDS;
    class Grid;
    class Array;
}

/** @brief A DAP BaseType with file out gdal information included
 *
 * This class represents a DAP BaseType with additional information
 * needed to write it out to a gdal file. Includes a reference to the
 * actual DAP BaseType being converted
 */
class FONgType: public BESObj {
protected:
    std::string d_name;
    libdap::Type d_type;

private:
    libdap::Grid *d_grid;
    libdap::Array *d_lat, *d_lon;

    // Sets of string values used to find stuff in attributes
    std::set<std::string> d_coards_lat_units;
    std::set<std::string> d_coards_lon_units;

    std::set<std::string> d_lat_names;
    std::set<std::string> d_lon_names;

    bool m_lat_unit_or_name_match(const std::string &var_units, const std::string &var_name, const std::string &long_name);
    bool m_lon_unit_or_name_match(const std::string &var_units, const std::string &var_name, const std::string &long_name);

public:
    FONgType(libdap::Grid *g);

    virtual ~FONgType() {}

    virtual std::string name() { return d_name; }
    virtual void set_name(const std::string &n) { d_name = n; }

    virtual libdap::Type type() { return d_type; }
    virtual void set_type(libdap::Type t) { d_type = t; }

    virtual void extract_coordinates(FONgTransform &t);

    /// Get the GDAL/OGC WKT projection string
    virtual std::string get_projection(libdap::DDS *dds);

    ///Get the data values for the band(s). Call must delete.
    virtual double *get_data();

    virtual void dump(std::ostream &) const {};

    libdap::Grid *grid() { return d_grid; }

    bool find_lat_lon_maps();

};

#endif // FONgType_h_

