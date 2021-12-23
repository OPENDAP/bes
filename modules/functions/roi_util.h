// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2015 OPeNDAP, Inc.
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

#ifndef ROI_UTILS_H_
#define ROI_UTILS_H_

namespace libdap {
class BaseType;
class Array;
class Structure;
}

namespace functions {

// Defined here because prior to C++ 11, defining a container using a local
// type was not allowed.
struct slice {
	int start, stop;
	string name;
};

void roi_bbox_valid_slice(libdap::BaseType *btp);
unsigned int roi_valid_bbox(libdap::BaseType *btp);

void roi_bbox_get_slice_data(libdap::Array *bbox, unsigned int i, int &start, int &stop, string &name);

libdap::Structure *roi_bbox_build_slice(unsigned int start_value, unsigned int stop_value, const string &dim_name);

unique_ptr<libdap::Array> roi_bbox_build_empty_bbox(unsigned int num_dim, const string &bbox_name = "bbox");

}

#endif /* ROI_UTILS_H_ */
