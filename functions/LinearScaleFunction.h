
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Authors: Nathan Potter <npotter@opendap.org>
//		James Gallagher <jgallagher@opendap.org>
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

#include <ServerFunction.h>

namespace libdap {
class BaseType;
class DDS;
}

namespace functions {

/**
 * The linear_scale() function applies the familiar y = mx + b equation to data.
 */
void function_dap2_linear_scale(int argc, libdap::BaseType *argv[], libdap::DDS &dds, libdap::BaseType **btpp) ;

/**
 * The linear_scale() function applies the familiar y = mx + b equation to data.
 */
libdap::BaseType *function_dap4_linear_scale(libdap::D4RValueList *args, libdap::DMR &dmr);

/**
 * The LinearScaleFunction class encapsulates the linear_scale function 'function_linear_scale'
 * along with additional meta-data regarding its use and applicability.
 */
class LinearScaleFunction: public libdap::ServerFunction {
public:
	LinearScaleFunction()
    {
		setName("linear_scale");
		setDescriptionString("The linear_scale() function applies the familiar y = mx + b equation to data.");
		setUsageString("linear_scale(var) | linear_scale(var,scale_factor,add_offset) | linear_scale(var,scale_factor,add_offset,missing_value)");
		setRole("http://services.opendap.org/dap4/server-side-function/linear-scale");
		setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#linear_scale");
		setFunction(function_dap2_linear_scale);
		setFunction(function_dap4_linear_scale);
		setVersion("1.0b1");
    }
    virtual ~LinearScaleFunction()
    {
    }
};

} // functions namespace
