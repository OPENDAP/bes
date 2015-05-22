
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Author:  Nathan Potter <npotter@opendap.org>
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

void function_geogrid(int argc, libdap::BaseType *argv[], libdap::DDS &dds, libdap::BaseType **btpp) ;

class GeoGridFunction: public libdap::ServerFunction {
public:
	GeoGridFunction()
    {
		setName("geogrid");
		setDescriptionString("Subsets a grid by the values of it's geo-located map variables.");
		setUsageString("geogrid(...)");
		setRole("http://services.opendap.org/dap4/server-side-function/geogrid");
		setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#geogrid");
		setFunction(function_geogrid);
		setVersion("1.2");
    }
    virtual ~GeoGridFunction()
    {
    }

    bool canOperateOn(libdap::DDS &dds);
};

} // functions namespace
