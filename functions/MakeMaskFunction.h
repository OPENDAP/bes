// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Authors: Nathan Potter <npotter@opendap.org>
//		James Gallagher <jgallagher@opendap.org>
//		Dan Holloway <dholloway@opendap.org>
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

typedef struct {
    int size;
    string name;
    int offset;
} MaskDIM;

namespace functions {

void function_make_dap2_mask(int argc, libdap::BaseType *argv[], libdap::DDS &dds, libdap::BaseType **btpp);

/**
 * The MakeMaskFunction class encapsulates the array builder function
 * implementations (function_make_dap2_mask() and function_make_dap4_mask())
 * along with additional meta-data regarding its use and applicability.
 */
class MakeMaskFunction: public libdap::ServerFunction
{
public:
    MakeMaskFunction()
    {
        setName("make_mask");
        setDescriptionString("The make_mask() function reads a number of dim_names, followed by a set of dim value tuples and builds a DAP Array object.");
        //setUsageString("make_mask(type,dim0,dim1,..dimN,dim0_value,dim1_value,...,dimN_value)");
        setRole("http://services.opendap.org/dap4/server-side-function/make_mask");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#make_mask");
        setFunction(function_make_dap2_mask);
        //setFunction(function_make_dap4_mask);
        setVersion("1.0");
    }
    virtual ~MakeMaskFunction()
    {
    }
};

} // functions namespace
