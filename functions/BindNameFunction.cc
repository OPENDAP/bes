
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

#include <cassert>

#include <sstream>
#include <vector>

#include <BaseType.h>
#include <Str.h>

#include <Error.h>
#include <DDS.h>

#include <debug.h>
#include <util.h>

#include <BESDebug.h>

#include "BindNameFunction.h"

namespace libdap {

/** Bind a new name to a variable.
 *
 * @note Unlike most server functions, this does not set send_p or read_p.
 * If this is used u=in an expression like bind_name(make_array(...)) make_array()
 * will set send_p & read_p. This means bind_name() can be used to change the name
 * of a variable in a DDS and pass a reference to it into a function where the
 * second function will decide if the value needs to be read and/or sent.
 *
 * @param argc A count of the arguments
 * @param argv An array of pointers to each argument, wrapped in a child of BaseType
 * @param btpp A pointer to the return value; caller must delete.
 * @return The newly (re)named variable.
 * @exception Error Thrown for a variety of errors.
 */
void
function_bind_name(int argc, BaseType * argv[], DDS &dds, BaseType **btpp)
{
    string info =
    string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
    "<function name=\"make_array\" version=\"1.0\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#bind_name\">\n" +
    "</function>";

    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(info);
        *btpp = response;
        return;
    }

    // Check for two args or more. The first two must be strings.
    if (argc != 2)
    	throw Error(malformed_expr, "bind_name(name,variable) requires two arguments.");

    // Don't allow renaming that will introduce namespace collisions.
    //
    // Testing dds.var(argv[0]->name()) does a look up of the variable name. If it's
    // a DAP String, then the name of the variable will be 'dummy' and the _value_ will
    // the string passed in as the first argument to bind_name()
    if (dds.var(argv[0]->name()))
    	throw Error(malformed_expr, "The name '" + argv[0]->name() + "' is already in use.");

    string name = extract_string_argument(argv[0]);
    BESDEBUG("functions", "name: " << name << endl);

    if (!argv[1]->read_p()) {
    	argv[1]->read();
    	argv[1]->set_read_p(true);
    }

    argv[1]->set_send_p(true);
    argv[1]->set_name(name);

    // set_send_p and set_read_p intentionally not called

    // return the array
    *btpp = argv[1];
    return;
}

} // namesspace libdap
