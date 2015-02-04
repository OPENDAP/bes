// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of the BES, A C++ implementation of the OPeNDAP
// Hyrax data server

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

#include <ServerFunction.h>

namespace libdap {

class BaseType;
class Array;
class DDS;

std::vector<long long>compute_array_shape(Array *a);
bool array_shape_matches(Array *a, std::vector<long long> shape);

void function_dap2_tabular(int argc, BaseType *argv[], DDS &dds, BaseType **btpp);
BaseType *function_dap4_tabular(D4RValueList *args, DMR &dmr);

class TabularFunction: public libdap::ServerFunction
{
public:
    TabularFunction()
    {
        setName("tabular");
        setDescriptionString("The tabular() function transforms one or more arrays into a sequence.");
        setUsageString("tabular()");
        setRole("http://services.opendap.org/dap4/server-side-function/tabular");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#tabular");
        setFunction(libdap::function_dap2_tabular);
        setFunction(libdap::function_dap4_tabular);
        setVersion("1.0");
    }
    virtual ~TabularFunction()
    {
    }
};

} // libdap namespace
