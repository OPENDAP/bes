
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
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

#include <ServerFunction.h>

namespace libdap {

class BaseType;
class DDS;

void function_geogrid(int argc, BaseType *argv[], DDS &dds, BaseType **btpp) ;

class GeoGridFunction: public libdap::ServerFunction {
public:
	GeoGridFunction()
    {
		setName("geogrid");
		setDescriptionString("Subsets a grid by the values of it's geo-located map variables.");
		setUsageString("geogrid(...)");
		setRole("http://services.opendap.org/dap4/server-side-function/geogrid");
		setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#geogrid");
		setFunction(libdap::function_geogrid);
		setVersion("1.2");
    }
    virtual ~GeoGridFunction()
    {
    }

    bool canOperateOn(DDS &dds);
};

} // libdap namespace
