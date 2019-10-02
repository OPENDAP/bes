// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2015 OPeNDAP, Inc.
// Authors: Dan Holloway <dholloway@opendap.org>
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

typedef struct 
{
    int size;
    std::string name;
    int offset;
} MaskDIM;

namespace functions {

// Added here so we can call it in unit tests
int find_value_index(double value, const std::vector<double> &map);
std::vector<int> find_value_indices(const std::vector<double> &values,
                                    const std::vector< std::vector<double> > &maps);
bool all_indices_valid(std::vector<int> indices);
template<typename T>
void make_mask_helper(const std::vector<libdap::Array*> dims, libdap::Array *tuples, std::vector<libdap::dods_byte> &mask);

void function_dap2_make_mask(int argc, libdap::BaseType *argv[], libdap::DDS &dds, libdap::BaseType **btpp);

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
        setFunction(function_dap2_make_mask);
        //setFunction(function_make_dap4_mask);
        setVersion("1.0");
    }
    virtual ~MakeMaskFunction()
    {
    }
};

} // functions namespace
