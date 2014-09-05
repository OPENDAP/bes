
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
#include <Array.h>
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
string bind_shape_info =
string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
"<function name=\"make_array\" version=\"1.0\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#bind_shape\">\n" +
"</function>";


vector<int> parse_dims(const string &shape); // defined in MakeArrayFunction.cc


BaseType *bind_shape_worker(string shape, BaseType *btp){

    // string shape = extract_string_argument(argv[0]);
    vector<int> dims = parse_dims(shape);

    Array *array = dynamic_cast<Array*>(btp);
    if (!array)
    	throw Error(malformed_expr, "bind_shape() requires an Array as its second argument.");

    unsigned long vector_size = array->length();
	DBG(cerr << "bind_shape_worker() - vector_size: " << long_to_string(vector_size) << endl);


    array->clear_all_dims();

    unsigned long number_of_elements = 1;
    vector<int>::iterator i = dims.begin();
    while (i != dims.end()) {
    	int dimSize = *i;
    	number_of_elements *= dimSize;
        if(array->is_dap4()){
        	DBG(cerr << "bind_shape_worker() - Adding DAP4 dimension." << endl);

        	// FIXME - I think this creates a memory leak because the D4Dimension will never be deleted
        	// by the current implementation of Array which only has a weak pointer to the D4Dimension.
        	//
        	// NB: The likely fix is to find the Group that holds this variable and add the new
        	// D4Dimension to its D4Dimensions object. That will enure it is deleted. jhrg 8/26/14
        	D4Dimension *this_dim = new D4Dimension("",dimSize);
        	array->append_dim(this_dim);
        }
        else
        {
        	DBG(cerr << "bind_shape_worker() - Adding DAP2 dimension." << endl);
        	array->append_dim(dimSize);
        }
        i++;
    }
    DBG(cerr << "bind_shape_worker() - number_of_elements: " << long_to_string(number_of_elements) << endl);

    if (number_of_elements != vector_size)
    	throw Error(malformed_expr, "bind_shape(): The product of the new dimensions must match the size of the Array's internal storage vector.");


    return array;


}

/** Bind a shape to a DAP2 Array that is a vector. The product of the dimension
 * sizes must match the number of elements in the vector. This function takes
 * two arguments: A shape expression and a BaseType* to the DAP2 Array that holds
 * the data. In practice, the Array can already have a shape (it's a vector, so
 * that is a shape, e.g.) and this function simply changes that shape. The shape
 * expression is the C bracket notation for array size and is parsed by this
 * function.
 *
 * @param argc A count of the arguments
 * @param argv An array of pointers to each argument, wrapped in a child of BaseType
 * @param btpp A pointer to the return value; caller must delete.
 * @return The newly (re)named variable.
 * @exception Error Thrown for a variety of errors.
 */
void
function_bind_shape_dap2(int argc, BaseType * argv[], DDS &, BaseType **btpp)
{

    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(bind_shape_info);
        *btpp = response;
        return;
    }

    // Check for two args or more. The first two must be strings.
    if (argc != 2)
    	throw Error(malformed_expr, "bind_shape(shape,variable) requires two arguments.");

    string shape = extract_string_argument(argv[0]);

    BaseType *btp = argv[1];

    *btpp = bind_shape_worker(shape, btp);

    return;
}

/** Bind a shape to a DAP4 Array that is a vector. The product of the dimension
 * sizes must match the number of elements in the vector. This function takes
 * two arguments: A shape expression and a BaseType* to the DAP2 Array that holds
 * the data. In practice, the Array can already have a shape (it's a vector, so
 * that is a shape, e.g.) and this function simply changes that shape. The shape
 * expression is the C bracket notation for array size and is parsed by this
 * function.
 *
 * @param args The DAP4 function arguments list
 * @param dmr The DMR for the dataset in question
 * @return A pointer to the return value; caller must delete.
 * @exception Error Thrown for a variety of errors.
 */

BaseType *function_bind_shape_dap4(D4RValueList *args, DMR &dmr){


    // DAP4 function porting information: in place of 'argc' use 'args.size()'
    if (args == 0 || args->size() == 0) {
        Str *response = new Str("info");
        response->set_value(bind_shape_info);
        // DAP4 function porting: return a BaseType* instead of using the value-result parameter
        return response;
    }


    // Check for 2 arguments
    DBG(cerr << "args.size() = " << args.size() << endl);
    if (args->size() != 2)
        throw Error(malformed_expr,"bind_shape(shape,variable) requires two arguments.");

    string shape = extract_string_argument(args->get_rvalue(0)->value(dmr));

    BaseType *btp = args->get_rvalue(1)->value(dmr);

    return bind_shape_worker(shape, btp);

}


} // namesspace libdap
