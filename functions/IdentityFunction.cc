// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Authors: Nathan Potter <npotter@opendap.org>
//         James Gallagher <jgallagher@opendap.org>
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

#include "config.h"

#include <sstream>

#include <BaseType.h>
#include <Float64.h>
#include <Str.h>
#include <Array.h>
#include <Grid.h>
#include "D4RValue.h"

#include <Error.h>
#include <DDS.h>

#include <debug.h>
#include <util.h>

#include "BESDebug.h"

#include "LinearScaleFunction.h"

using namespace libdap;

namespace functions {

string identity_info =
        string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
        + "<function name=\"identity\" version=\"1.0\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#identity\">\n"
                + "</function>";

/**
 * Return the argument passed to this function. Use this to include a variable
 * in a function result without modifying that variable.
 * @param argc Argument count
 * @param argv List of arguments
 * @param btpp Return value (a value-result parameter)
 */
void function_dap2_identity(int argc, BaseType * argv[], DDS &, BaseType **btpp)
{
    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(identity_info);
        *btpp = response;
        return;
    }

    *btpp = argv[0];
}

/**
 * Return the argument passed to this function. Use this to include a variable
 * in a function result without modifying that variable.
 * @param args List of arguments - this function takes one argument
 * @param dmr DMR reference for this request
 * @return The argument passed to this function
 */
BaseType *function_dap4_identity(D4RValueList *args, DMR &dmr)
{
    BESDEBUG("function", "function_dap4_identity()  BEGIN " << endl);

    // DAP4 function porting information: in place of 'argc' use 'args.size()'
    if (args == 0 || args->size() == 0) {
        Str *response = new Str("info");
        response->set_value(identity_info);
        // DAP4 function porting: return a BaseType* instead of using the value-result parameter
        return response;
    }

    return args->get_rvalue(0)->value(dmr);
}
} // namesspace functions
