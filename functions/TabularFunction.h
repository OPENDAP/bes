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

class TabularFunction: public libdap::ServerFunction
{
private:
    friend class TabularFunctionTest;
    friend class Dap4_TabularFunctionTest;

    typedef std::vector< std::vector<BaseType*> *> SequenceValues;

    static void function_dap2_tabular(int argc, BaseType *argv[], DDS &dds, BaseType **btpp);
    // static BaseType *function_dap4_tabular(D4RValueList *args, DMR &dmr);

    // This
    static void function_dap2_tabular_2(int argc, BaseType *argv[], DDS &, BaseType **btpp);

    // These are static so that the code does not have to make a TabularFunction
    // instance to access them. They are 'in' TabularFunction to control the name
    // space - they were static functions but that made it impossible to write
    // unit tests.
    static std::vector<unsigned long> array_shape(Array *a);
    static bool shape_matches(Array *a, std::vector<unsigned long> shape);
    static unsigned long number_of_values(std::vector<unsigned long> shape);
    static void build_columns(unsigned long n, BaseType *btp, std::vector<Array*> &arrays, std::vector<unsigned long> &shape);
    static void read_values(std::vector<Array*> arrays);
    static void build_sequence_values(std::vector<Array*> arrays, SequenceValues &sv);
    static void combine_sequence_values(SequenceValues &dep, SequenceValues &indep);
    static void add_index_columns(const std::vector<unsigned long> &indep_shape, const std::vector<unsigned long> &dep_shape,
            std::vector<Array*> &dep_vars);

public:
    TabularFunction()
    {
        setName("tabular");
        setDescriptionString("The tabular() function transforms one or more arrays into a sequence.");
        setUsageString("tabular()");
        setRole("http://services.opendap.org/dap4/server-side-function/tabular");
        setDocUrl("http://docs.opendap.org/index.php/Server_Side_Processing_Functions#tabular");
        setFunction(libdap::TabularFunction::function_dap2_tabular);
        // FIXME setFunction(libdap::TabularFunction::function_dap4_tabular);
        setVersion("1.0");
    }

    virtual ~TabularFunction()
    {
    }
};

} // libdap namespace
