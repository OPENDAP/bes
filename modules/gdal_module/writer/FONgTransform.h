// FONgTransform.h

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

#ifndef FONgTransfrom_h_
#define FONgTransfrom_h_ 1

//#include <cstdlib>

class GDALDataset;
class BESDataHandlerInterface;
class FONgGrid;

/** @brief Transformation object that converts an OPeNDAP DataDDS to a
 * GeoTiff file
 *
 * This class transforms variables in a DataDDS object to a GeoTiff file. For
 * more information on the transformation please refer to the OpeNDAP
 * documents wiki.
 */
class FONgTransform : public BESObj
{
public:
    typedef enum { none, negative, positive } no_data_type_t;

private:
    GDALDataset *d_dest;

    libdap::DDS *d_dds;

    string d_localfile;

    vector<FONgGrid *> d_fong_vars;

    // used when there is more than one variable; this is possible
    // when returning a GMLJP2 response but not a GeoTiff.
    bool d_geo_transform_set;

    // Collect data here
    double d_width, d_height, d_top, d_left, d_bottom, d_right;
    double d_no_data;
    no_data_type_t d_no_data_type;

    // Put GeoTransform info here
    double d_gt[6];

    int d_num_bands;

#if 0
    void m_scale_data(double *data);
#endif

    bool effectively_two_D(FONgGrid *fbtp);

public:
    FONgTransform(libdap::DDS *dds, libdap::ConstraintEvaluator &evaluator, const string &localfile);
    virtual ~FONgTransform();

    virtual void transform_to_geotiff();
    virtual void transform_to_jpeg2000();

    bool is_geo_transform_set() { return d_geo_transform_set; }
    void geo_transform_set(bool state) { d_geo_transform_set = state; }

    double no_data() { return d_no_data; }
    void set_no_data(const string &nd) { d_no_data = strtod(nd.c_str(), NULL); }

    void set_no_data_type(no_data_type_t t) { d_no_data_type = t; }
    no_data_type_t no_data_type() { return d_no_data_type; }

    int num_bands() { return d_num_bands; }
    void set_num_bands(int n) { d_num_bands = n; }

    void push_var(FONgGrid *v) { d_fong_vars.push_back(v); }
    int num_var() { return d_fong_vars.size(); }

    FONgGrid *var(int i) { return d_fong_vars.at(i); }

    // Image/band height and width in pixels
    virtual void set_width(int width) { d_width = width; }
    virtual void set_height(int height) { d_height = height; }

    virtual int width() { return d_width; }
    virtual int height() { return d_height; }

    // The top-left corner of the top-left pixel
    virtual void set_top(int top) { d_top = top; }
    virtual void set_left(int left) { d_left = left; }

    virtual double top() { return d_top; }
    virtual double left() { return d_left; }

    // The top-left corner of the top-left pixel
    virtual void set_bottom(int top) { d_bottom = top; }
    virtual void set_right(int left) { d_right = left; }

    virtual double bottom() { return d_bottom; }
    virtual double right() { return d_right; }

    virtual double *geo_transform();

    virtual void dump(ostream &) const {}
};

#endif // FONgTransfrom_h_

