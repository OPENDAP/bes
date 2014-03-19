
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2013 OPeNDAP, Inc.
// Authors:  James Gallagher <jgallagher@opendap.org>
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

#include <cassert>

#include <sstream>
#include <vector>

#include <BaseType.h>
#include <Str.h>

#include <Error.h>
#include <DDS.h>
#include <DMR.h>
#include <D4RValue.h>

#include <debug.h>
#include <util.h>

#include <BESDebug.h>

#include "BindNameFunction.h"

namespace libdap {

string bind_name_info =
string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
"<function name=\"make_array\" version=\"1.0\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#bind_name\">\n" +
"</function>";



/** Bind a new name to a variable. The first argument to the function is
 * the new name and the second argument is the BaseType* to (re)name. This
 * function can be used to assign a name to an anonymous variable or change
 * the name of a variable. If the variable is already part of the dataset,
 * this function will make a copy and operate on that. In that case, the
 * function will also read values into the variable.
 *
 * @param argc A count of the arguments
 * @param argv An array of pointers to each argument, wrapped in a child of BaseType
 * @param btpp A pointer to the return value; caller must delete.
 * @return The newly (re)named variable.
 * @exception Error Thrown for a variety of errors.
 */
void
function_bind_name_dap2(int argc, BaseType * argv[], DDS &dds, BaseType **btpp)
{

    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(bind_name_info);
        *btpp = response;
        return;
    }

    // Check for two args or more. The first two must be strings.
    if (argc != 2)
    	throw Error(malformed_expr, "bind_name(name,variable) requires two arguments.");

    string name = extract_string_argument(argv[0]);
    BaseType *sourceVar = argv[1];

    // Don't allow renaming that will introduce namespace collisions.
    //
    // Check the DDS to see if a variable with name given as argv[0] already exists. If
    // so, return an error. This is complicated somewhat because the CE Evaluator will
    // have already looked and, if the string passed into the function matches a variable,
    // replaced that string with a BaseType* to the (already existing) variable. If not,
    // the CE Evaluator will make a DAP String variable with a value that is the string
    // passed into the function. So, either way argv[0] is a BaseType*. However, if it's
    // a variable in the dataset, its name() will be found by DDS::var().
    if (dds.var(argv[0]->name()))
    	throw Error(malformed_expr, "The name '" + name + "' is already in use.");


    // If the variable is the return value of a function, just pass it back. If it is
    // a variable in the dataset (i.e., present in the DDS), copy it because DDS deletes
    // all its variables and the function processing code also deletes all it's variables.
    // NB: Could use reference counting pointers to eliminate this copy... jhrg 6/24/13
    if (dds.var(sourceVar->name())) {
    	*btpp = sourceVar->ptr_duplicate();
    	if (!(*btpp)->read_p()) {
    		(*btpp)->read();
    		(*btpp)->set_read_p(true);
    	}
    	(*btpp)->set_send_p(true);
    	(*btpp)->set_name(name);
    }
    else {
    	sourceVar->set_name(name);

    	*btpp = sourceVar;
    }

    return;
}


BaseType *function_bind_name_dap4(D4RValueList *args, DMR &dmr){
    // DAP4 function porting information: in place of 'argc' use 'args.size()'
    if (args == 0 || args->size() == 0) {
        Str *response = new Str("info");
        response->set_value(bind_name_info);
        // DAP4 function porting: return a BaseType* instead of using the value-result parameter
        return response;
    }


    // Check for 2 arguments
    DBG(cerr << "args.size() = " << args.size() << endl);
    if (args->size() != 2)
        throw Error(malformed_expr,"bind_shape(shape,variable) requires two arguments.");

    BaseType *arg0 = args->get_rvalue(0)->value(dmr);
    string name = extract_string_argument(arg0);

    BaseType *sourceVar = args->get_rvalue(1)->value(dmr);

    BaseType *resultVar;

    // Don't allow renaming that will introduce namespace collisions.
    //
    // Check the DMR to see if a variable with name given as args[0] already exists. If
    // so, return an error. This is complicated somewhat because the CE Evaluator will
    // have already looked and, if the string passed into the function matches a variable,
    // replaced that string with a BaseType* to the (already existing) variable. If not,
    // the CE Evaluator will make a DAP String variable with a value that is the string
    // passed into the function. So, either way args[0] is a BaseType*. However, if it's
    // a variable in the dataset, its name() will be found by DMR::root()->var().
    if (dmr.root()->var(arg0->name()))
    	throw Error(malformed_expr, "The name '" + arg0->name() + "' is already in use.");


    // If the variable is the return value of a function, just pass it back. If it is
    // a variable in the dataset (i.e., present in the DDS), copy it because DDS deletes
    // all its variables and the function processing code also deletes all it's variables.
    // NB: Could use reference counting pointers to eliminate this copy... jhrg 6/24/13
    if (dmr.root()->var(sourceVar->name())) {
    	resultVar = sourceVar->ptr_duplicate();
    	if (!resultVar->read_p()) {
    		resultVar->read();
    		resultVar->set_read_p(true);
    	}
    	resultVar->set_send_p(true);
    	resultVar->set_name(name);
    }
    else {
    	resultVar = sourceVar;
    	resultVar->set_name(name);

    }
    return resultVar;


}


} // namesspace libdap
