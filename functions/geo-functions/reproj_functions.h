
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
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

#ifndef _reproj_functions_h
#define _reproj_functions_h

// #include "BESAbstractModule.h"
#include <libdap/ServerFunction.h>
//#include <libdap/ServerFunctionsList.h>

namespace libdap {

struct SizeBox {
	int x_size;
	int y_size;

	SizeBox(int x, int y) : x_size(x), y_size(y) { }
	SizeBox(): x_size(0), y_size(0) { }
};

struct GeoBox {
	double top;		// Latitude
	double bottom;	// Lat
	double left;	// Lon
	double right;	// Lon

	GeoBox(double t, double b, double l, double r) : top(t), bottom(b), left(l), right(r) { }
	GeoBox() : top(0.0), bottom(0.0), left(0.0), right(0.0) { }
};

SizeBox get_size_box(Array *lat, Array *lon);
vector<double> get_geotransform_data(Array *lat, Array *lon, const SizeBox &size);

void function_swath2array(int argc, BaseType * argv[], DDS &, BaseType **btpp);
void function_swath2grid(int argc, BaseType * argv[], DDS &, BaseType **btpp);

class SwathToGrid: public ServerFunction {
public:
    SwathToGrid()
    {
        setName("swath2grid");
        setDescriptionString("This function echos back it's arguments as DAP data.");
        setUsageString("swath2grid(dataArray, latitudeArray, longitudeArray)");
        setRole("http://services.opendap.org/dap4/server-side-function/swath2grid");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2grid");
        setFunction(libdap::function_swath2grid);
        setVersion("1.0");
    }
    virtual ~SwathToGrid()
    {
    }

};

class SwathToArray: public ServerFunction {
public:
    SwathToArray()
    {
        setName("swath2array");
        setDescriptionString("This function echos back it's arguments as DAP data.");
        setUsageString("swath2array(dataArray, latitudeArray, longitudeArray)");
        setRole("http://services.opendap.org/dap4/server-side-function/swath2array");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#swath2array");
        setFunction(libdap::function_swath2array);
        setVersion("1.0");
    }
    virtual ~SwathToArray()
    {
    }

};

#if 0
class ReProjectionFunctions: public BESAbstractModule {
public:
    ReProjectionFunctions()
    {
        libdap::ServerFunctionsList::TheList()->add_function(new libdap::SwathToGrid());

        libdap::ServerFunctionsList::TheList()->add_function(new libdap::SwathToArray());

    }
    virtual ~ReProjectionFunctions()
    {
    }
    virtual void initialize(const string &modname);
    virtual void terminate(const string &modname);

    virtual void dump(ostream &strm) const;
};
#endif

} // namespace libdap

#endif // _reproj_functions_h
