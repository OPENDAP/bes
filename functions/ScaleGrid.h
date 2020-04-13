
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2016 OPeNDAP, Inc.
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

#ifndef _scale_util_h
#define _scale_util_h

#include <vector>
#include <string>
#include <memory>

#include <gdal.h>
#include <gdal_priv.h>

#include <ServerFunction.h>


namespace libdap {
class Array;
class Grid;
}

namespace functions {

struct SizeBox {
	int x_size;
	int y_size;

	SizeBox(int x, int y) : x_size(x), y_size(y) { }
	SizeBox(): x_size(0), y_size(0) { }
};

#if 0
struct GeoBox {
	double top;		// Latitude
	double bottom;	// Lat
	double left;	// Lon
	double right;	// Lon

	GeoBox(double t, double b, double l, double r) : top(t), bottom(b), left(l), right(r) { }
	GeoBox() : top(0.0), bottom(0.0), left(0.0), right(0.0) { }
};
#endif

SizeBox get_size_box(libdap::Array *x, libdap::Array *y);

std::vector<double> get_geotransform_data(libdap::Array *x, libdap::Array *y, bool test_maps = false);
// std::auto_ptr< std::vector<GDAL_GCP> >
std::vector<GDAL_GCP> get_gcp_data(libdap::Array *x, libdap::Array *y, int sample_x = 1, int sample_y = 1);

GDALDataType get_array_type(const libdap::Array *a);
void read_band_data(const libdap::Array *src, GDALRasterBand* band);
void add_band_data(const libdap::Array *src, GDALDataset* ds);

std::unique_ptr<GDALDataset> build_src_dataset(libdap::Array *data, libdap::Array *x, libdap::Array *y,
    const std::string &srs = "WGS84");
std::unique_ptr<GDALDataset> build_src_dataset_3D(libdap::Array *data, libdap::Array *t,libdap::Array *x, libdap::Array *y,
    const std::string &srs = "WGS84");

std::unique_ptr<GDALDataset> scale_dataset(std::unique_ptr<GDALDataset>& src, const SizeBox &size,
    const std::string &crs = "", const std::string &interp = "nearest");

std::unique_ptr<GDALDataset> scale_dataset_3D(std::unique_ptr<GDALDataset>& src, const SizeBox &size,
    const std::string &crs = "", const std::string &interp = "nearest");

libdap::Array *build_array_from_gdal_dataset(GDALDataset *dst, const libdap::Array *src);
libdap::Array *build_array_from_gdal_dataset_3D(GDALDataset *dst, const libdap::Array *src);
void build_maps_from_gdal_dataset(GDALDataset *dst, libdap::Array *x_map, libdap::Array *y_map, bool name_maps = false);
void build_maps_from_gdal_dataset_3D(GDALDataset *dst, libdap::Array *t, libdap::Array *t_map, libdap::Array *x_map, libdap::Array *y_map, bool name_maps = false);

libdap::Grid *scale_dap_grid(const libdap::Grid *src, const SizeBox &size, const std::string &dest_crs,
    const std::string &interp);
libdap::Grid *scale_dap_array(const libdap::Array *data, const libdap::Array *lon, const libdap::Array *lat,
    const SizeBox &size, const std::string &crs, const std::string &interp);
libdap::Grid *scale_dap_array_3D(const libdap::Array *data, const libdap::Array *t, const libdap::Array *lon, const libdap::Array *lat, const SizeBox &size,
    const std::string &crs, const std::string &interp);

void function_scale_grid(int argc, libdap::BaseType * argv[], libdap::DDS &, libdap::BaseType **btpp);
void function_scale_array(int argc, libdap::BaseType * argv[], libdap::DDS &, libdap::BaseType **btpp);
void function_scale_array_3D(int argc, libdap::BaseType * argv[], libdap::DDS &, libdap::BaseType **btpp);

class ScaleGrid: public libdap::ServerFunction {
public:
    ScaleGrid()
    {
        setName("scale_grid");
        setDescriptionString("Scale a DAP2 Grid");
        setUsageString("scale_grid(Grid, Y size, X size, CRS, Interpolation method)");
        setRole("http://services.opendap.org/dap4/server-side-function/scale_grid");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#scale_grid");
        setFunction(function_scale_grid);
        setVersion("1.0");
    }
    virtual ~ScaleGrid()
    {
    }

};

class ScaleArray: public libdap::ServerFunction {
public:
    ScaleArray()
    {
        setName("scale_array");
        setDescriptionString("Scale a DAP2 Array");
        setUsageString("scale_grid(Array data, Array lon, Array lat, Y size, X size, CRS, Interpolation method)");
        setRole("http://services.opendap.org/dap4/server-side-function/scale_array");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#scale_array");
        setFunction(function_scale_array);
        setVersion("1.0");
    }
    virtual ~ScaleArray()
    {
    }

};

class Scale3DArray: public libdap::ServerFunction {
public:
    Scale3DArray()
    {
        setName("scale_3D_array");
        setDescriptionString("Scale a DAP2 3D Array");
        setUsageString("scale_3D_grid(Array data, Array time, Array lon, Array lat, Y size, X size, CRS, Interpolation method)");
        setRole("http://services.opendap.org/dap4/server-side-function/scale_3D_array");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#scale_3D_array");
        setFunction(function_scale_array_3D);
        setVersion("1.0");
    }
    virtual ~Scale3DArray()
    {
    }

};

} // namespace functions

#endif // _scale_util_h
