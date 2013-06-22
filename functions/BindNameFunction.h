
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include <ServerFunction.h>

namespace libdap {

class BaseType;
class DDS;

/**
 * The linear_scale() function applies the familiar y = mx + b equation to data.
 */
void function_bind_name(int argc, BaseType *argv[], DDS &dds, BaseType **btpp) ;

/**
 * The LinearScaleFunction class encapsulates the linear_scale function 'function_linear_scale'
 * along with additional meta-data regarding its use and applicability.
 */
class BindNameFunction: public libdap::ServerFunction {
public:
	BindNameFunction()
    {
		setName("bind_name");
		setDescriptionString("The bind_name() function (re)names a DAP variable.");
		setUsageString("bind_name(name,variable)");
		setRole("http://services.opendap.org/dap4/server-side-function/bind_name");
		setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#bind_name");
		setFunction(libdap::function_bind_name);
		setVersion("1.0");
    }
    virtual ~BindNameFunction()
    {
    }
};

} // libdap namespace
