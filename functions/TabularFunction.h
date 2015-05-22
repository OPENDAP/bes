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
}

namespace functions {

class TabularFunction: public libdap::ServerFunction
{
private:
    friend class TabularFunctionTest;
    friend class Dap4_TabularFunctionTest;

    typedef std::vector< std::vector<libdap::BaseType*> *> SequenceValues;
    typedef std::vector<unsigned long> Shape;

    static void function_dap2_tabular(int argc, libdap::BaseType *argv[], libdap::DDS &dds, libdap::BaseType **btpp);

#if 0
    static void function_dap2_tabular_2(int argc, libdap::BaseType *argv[], libdap::DDS &, libdap::BaseType **btpp);
#endif
    // These are static so that the code does not have to make a TabularFunction
    // instance to access them. They are 'in' TabularFunction to control the name
    // space - they were static functions but that made it impossible to write
    // unit tests.
    static Shape array_shape(libdap::Array *a);
    static bool shape_matches(libdap::Array *a, const Shape &shape);
    static bool dep_indep_match(const Shape &dep_shape, const Shape &indep_shape);

    static unsigned long number_of_values(const Shape &shape);

    static void build_columns(unsigned long n, libdap::BaseType *btp, std::vector<libdap::Array*> &arrays, Shape &shape);

    static void read_values(const std::vector<libdap::Array*> &arrays);

    static void build_sequence_values(const std::vector<libdap::Array*> &arrays, SequenceValues &sv);
    static void combine_sequence_values(SequenceValues &dep, const SequenceValues &indep);
    static void add_index_column(const Shape &indep_shape, const Shape &dep_shape,
            std::vector<libdap::Array*> &dep_vars);

public:
    TabularFunction()
    {
        setName("tabular");
        setDescriptionString("The tabular() function transforms one or more arrays into a sequence.");
        setUsageString("tabular()");
        setRole("http://services.opendap.org/dap4/server-side-function/tabular");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#tabular");
        setFunction(TabularFunction::function_dap2_tabular);
        // FIXME setFunction(libdap::TabularFunction::function_dap4_tabular);
        setVersion("1.0");
    }

    virtual ~TabularFunction()
    {
    }
};

} // functions namespace
