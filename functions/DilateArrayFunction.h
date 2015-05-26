// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2015 OPeNDAP, Inc.
// Authors: James Gallagher <jgallagher@opendap.org>
//          Dan Holloway <dholloway@opendap.org>
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
#include <dods-datatypes.h>

namespace libdap {
class BaseType;
class Array;
class DDS;
}

namespace functions {

void function_dilate_dap2_array(int argc, libdap::BaseType *argv[], libdap::DDS &dds, libdap::BaseType **btpp);

/**
 * The DilateMaskFunction class encapsulates the array builder function
 * implementations along with additional meta-data regarding its use and
 * applicability.
 */
class DilateArrayFunction: public libdap::ServerFunction
{
public:
    DilateArrayFunction()
    {
        setName("dilate_array");
        setDescriptionString("The dilate_array() function applies a dilation graphics operation to a mask array.");
        setUsageString("dilate_array(mask, dilatin_size = 1)");
        setRole("http://services.opendap.org/dap4/server-side-function/dilate_array");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#dilate_array");
        setFunction(function_dilate_dap2_array);
	//        setFunction(function_dilate_mask_dap4_array);
        setVersion("1.0");
    }
    virtual ~DilateArrayFunction()
    {
    }
};

} // functions namespace
