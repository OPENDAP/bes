// This file is part of the GDAL OPeNDAP Adapter

// Copyright (c) 2004 OPeNDAP, Inc.
// Author: Frank Warmerdam <warmerdam@pobox.com>
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

#ifndef MODULES_GDAL_HANDLER_GDAL_UTILS_H_
#define MODULES_GDAL_HANDLER_GDAL_UTILS_H_

class GDALArray;

void gdal_read_dataset_variables(libdap::DDS *dds, const GDALDatasetH &hDS, const string &filename,bool include_attrs);
void gdal_read_dataset_attributes(libdap::DAS &das, const GDALDatasetH &hDS);

void gdal_read_dataset_variables(libdap::DMR *dmr, const GDALDatasetH &hDS, const std::string &filename);

void read_data_array(GDALArray *array, const GDALRasterBandH &hBand);
void read_map_array(libdap::Array *map, const GDALRasterBandH &hBand, const GDALDatasetH &hDS);

#endif /* MODULES_GDAL_HANDLER_GDAL_UTILS_H_ */
