
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003,2013 OPeNDAP, Inc.
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

#include <libdap/BaseType.h>
#include <libdap/Str.h>
#include <libdap/Array.h>
#include <libdap/Grid.h>
#include <libdap/D4Group.h>
#include <libdap/D4RValue.h>
#include <libdap/D4Maps.h>

#include <libdap/Error.h>
#include <libdap/DDS.h>
#include <libdap/DMR.h>
#include <libdap/debug.h>
#include <libdap/util.h>

#include <BESDebug.h>

#include "GridFunction.h"
#include "gse_parser.h"
#include "grid_utils.h"

using namespace libdap;

namespace functions {

    string grid_info =
            string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
            + "<function name=\"grid\" version=\"1.0b1\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#grid\">\n"
            + "</function>";


    /** The grid function uses a set of relational expressions to form a selection
 within a Grid variable based on the values in the Grid's map vectors.
 Thus, if a Grid has a 'temperature' map which ranges from 0.0 to 32.0
 degrees, it's possible to request the values of the Grid that fall between
 10.5 and 12.5 degrees without knowing to which array indexes those values
 correspond. The function takes one or more arguments:<ul>
 <li>The name of a Grid.</li>
 <li>Zero or more strings which hold relational expressions of the form:<ul>
 <li><code>&lt;map var&gt; &lt;relop&gt; &lt;constant&gt;</code></li>
 <li><code>&lt;constant&gt; &lt;relop&gt; &lt;map var&gt; &lt;relop&gt;
 &lt;constant&gt;</code></li>
 </ul></li>
 </ul>

 Each of the relation expressions is applied to the Grid and the result is
 returned.

 @note Since this is a function and one of the arguments is the grid, the
 grid is read (using the Grid::read() method) at the time the argument list
 is built.

 @param argc The number of values in argv.
 @param argv An array of BaseType pointers which hold the arguments to be
 passed to geogrid. The arguments may be Strings, Integers, or Reals, subject
 to the above constraints.
 @param btpp A pointer to the return value; caller must delete.

 @see geogrid() (func_geogrid_select) A function which has logic specific
 to longitude/latitude selection. */
void
function_dap2_grid(int argc, BaseType *argv[], DDS &, BaseType **btpp)
{
    DBG(cerr << "Entering function_grid..." << endl);
    BESDEBUG("function", "function_dap2_grid()  BEGIN " << endl);

    if (argc == 0) {
        Str *response = new Str("info");
        response->set_value(grid_info);
        *btpp = response;
        return;
    }

    Grid *original_grid = dynamic_cast < Grid * >(argv[0]);
    if (!original_grid)
        throw Error(malformed_expr,"The first argument to grid() must be a Grid variable!");

    // Duplicate the grid; ResponseBuilder::send_data() will delete the variable
    // after serializing it.
    BaseType *btp = original_grid->ptr_duplicate();
    Grid *l_grid = dynamic_cast < Grid * >(btp);
    if (!l_grid) {
    	delete btp;
        throw InternalErr(__FILE__, __LINE__, "Expected a Grid.");
    }

    DBG(cerr << "grid: past initialization code" << endl);

    // Read the maps. Do this before calling parse_gse_expression(). Avoid
    // reading the array until the constraints have been applied because it
    // might be large.

    BESDEBUG("functions", "original_grid: read_p: " << original_grid->read_p() << endl);
    BESDEBUG("functions", "l_grid: read_p: " << l_grid->read_p() << endl);

    BESDEBUG("functions", "original_grid->array_(): read_p: " << original_grid->array_var()->read_p() << endl);
    BESDEBUG("functions", "l_grid->array+var(): read_p: " << l_grid->array_var()->read_p() << endl);

    // This version makes sure to set the send_p flags which is needed for
    // the hdf4 handler (and is what should be done in general).
    Grid::Map_iter i = l_grid->map_begin();
    while (i != l_grid->map_end())
        (*i++)->set_send_p(true);

    l_grid->read();

    DBG(cerr << "grid: past map read" << endl);

    // argv[1..n] holds strings; each are little expressions to be parsed.
    // When each expression is parsed, the parser makes a new instance of
    // GSEClause. GSEClause checks to make sure the named map really exists
    // in the Grid and that the range of values given makes sense.
    vector < GSEClause * > clauses;
    gse_arg *arg = new gse_arg(l_grid); // unique_ptr here
    for (int i = 1; i < argc; ++i) {
        parse_gse_expression(arg, argv[i]);
        clauses.push_back(arg->get_gsec());
    }
    delete arg;
    arg = 0;

    apply_grid_selection_expressions(l_grid, clauses);

    DBG(cerr << "grid: past gse application" << endl);

    l_grid->get_array()->set_send_p(true);

    l_grid->read();

    // Make a new grid here and copy just the parts of the Grid
    // that are in the current projection - this means reading
    // the array slicing information, extracting the correct
    // values and building destination arrays with just those
    // values.

    *btpp = l_grid;
    return;
}

/**
 * The passed DDS parameter dds is evaluated to see if it contains Grid objects.
 *
 * @param dds The DDS to be evaluated.
 */
bool GridFunction::canOperateOn(DDS &dds)
{
	vector<Grid *> grids;
	get_grids(dds, &grids);

	return !grids.empty();
}


/** The grid function uses a set of relational expressions to form a selection
 within a Grid variable based on the values in the Grid's map vectors.
 Thus, if a Grid has a 'temperature' map which ranges from 0.0 to 32.0
 degrees, it's possible to request the values of the Grid that fall between
 10.5 and 12.5 degrees without knowing to which array indexes those values
 correspond. The function takes one or more arguments:<ul>
 <li>The name of a Grid.</li>
 <li>Zero or more strings which hold relational expressions of the form:<ul>
 <li><code>&lt;map var&gt; &lt;relop&gt; &lt;constant&gt;</code></li>
 <li><code>&lt;constant&gt; &lt;relop&gt; &lt;map var&gt; &lt;relop&gt;
 &lt;constant&gt;</code></li>
 </ul></li>
 </ul>

 Each of the relation expressions is applied to the Grid and the result is
 returned.

 @note Since this is a function and one of the arguments is the grid, the
 grid is read (using the Grid::read() method) at the time the argument list
 is built.

 @note The main difference between this function and the DAP2
 version is to use args->size() in place of argc and
 args->get_rvalue(n)->value(dmr) in place of argv[n].

 @note Not yet implemented.

 @see function_dap2_grid
 */
BaseType *function_dap4_grid(D4RValueList *args, DMR &dmr)
{
    BESDEBUG("function", "function_dap4_grid()  BEGIN " << endl);

    // DAP4 function porting information: in place of 'argc' use 'args.size()'
    if (args == 0 || args->size() < 2) {
        Str *response = new Str("info");
        response->set_value(grid_info);
        // DAP4 function porting: return a BaseType* instead of using the value-result parameter
        return response;
    }

    BaseType *a_btp = args->get_rvalue(0)->value(dmr);
    Array *original_array = dynamic_cast < Array * >(a_btp);
    if (!original_array) {
        delete a_btp;
        throw InternalErr(__FILE__, __LINE__, "Expected an Array.");
    }

    // Duplicate the array; ResponseBuilder::send_data() will delete the variable
    // after serializing it.
    BaseType *btp = original_array->ptr_duplicate();
    Array *l_array = dynamic_cast < Array * >(btp);
    if (!l_array) {
        delete btp;
        throw InternalErr(__FILE__, __LINE__, "Expected an Array.");
    }

    DBG(cerr << "array: past initialization code" << endl);

    // Read the maps. Do this before calling parse_gse_expression(). Avoid
    // reading the array until the constraints have been applied because it
    // might be large.

    BESDEBUG("functions", "original_array: read_p: " << original_array->read_p() << endl);
    BESDEBUG("functions", "l_array: read_p: " << l_array->read_p() << endl);

    // Basic plan: For each map, set the send_p flag and read the map
    D4Maps *d4_maps = l_array->maps();
    D4Maps::D4MapsIter miter = d4_maps->map_begin();
    while (miter != d4_maps->map_end()) {
        D4Map *d4_map = (*miter);
        Array *map = const_cast<Array *>(d4_map->array());
        map->set_send_p(true);
        map->read();
        ++miter;
    }

    DBG(cerr << "array: past map read" << endl);

    // argv[1..n] holds strings; each are little expressions to be parsed.
    // When each expression is parsed, the parser makes a new instance of
    // GSEClause. GSEClause checks to make sure the named map really exists
    // in the Grid and that the range of values given makes sense.
    vector < GSEClause * > clauses;
    gse_arg *arg = new gse_arg(l_array); // unique_ptr here
    for (unsigned int i = 1; i < args->size(); ++i) {
        string relop = extract_string_argument(args->get_rvalue(i)->value(dmr));
        parse_gse_expression(arg, args->get_rvalue(i)->value(dmr));
        clauses.push_back(arg->get_gsec());
    }
    delete arg;
    arg = 0;

    apply_grid_selection_expressions(l_array, clauses);

    DBG(cerr << "array: past gse application" << endl);

    // Make a new array here and copy just the parts of the Array
    // that are in the current projection - this means reading
    // the array slicing information, extracting the correct
    // values and building destination arrays with just those
    // values.

    // Build the return value(s) - this means make copies of the Map arrays
    D4Group *dapResult = new D4Group("grid_result");

    // Set this container's parent ot the root D4Group
    dapResult->set_parent(dmr.root());

    // Basic plan: Add the new array to the destination D4Group, and clear read_p flag.
    l_array->set_read_p(false);
    dapResult->add_var_nocopy(l_array);

    // Basic plan: Add D4Dimensions to the destination D4Group; copy all dims to the parent group.
    D4Dimensions *grp_d4_dims = dapResult->dims();

    Array *g_array = dynamic_cast<Array *>(dapResult->find_var(l_array->name()));
    // Basic plan: For each D4Dimension in the array, add it to the destination D4Group
    Array::Dim_iter dim_i = g_array->dim_begin();
    while (dim_i != g_array->dim_end()) {
        D4Dimension *d4_dim = g_array->dimension_D4dim(dim_i);
        grp_d4_dims->add_dim_nocopy(d4_dim);
        ++dim_i;
    }

    // Basic plan: For each map in the array, add it to the destination structure and clear the read_p flag
    d4_maps = l_array->maps();
    miter = d4_maps->map_begin();
    while (miter != d4_maps->map_end()) {
        D4Map *d4_map = (*miter);
        Array *map = const_cast<Array *>(d4_map->array());
        map->set_read_p(false);
        dapResult->add_var_nocopy(map);
        ++miter;
    }

    // Basic plan: Mark the Structure for sending and read the data.
    dapResult->set_send_p(true);
    dapResult->read();

    return dapResult;
}

/**
* The passed DMR parameter dmr is evaluated to see if it contains DAP4 Arrays that conform with DAP2 Grid objects.
*
* @param dmr The DMR to be evaluated.
*/
bool GridFunction::canOperateOn(DMR &dmr)
{
    vector<Array *> coverages;
    get_coverages(dmr, &coverages);

    return !coverages.empty();
}

} // namespace functions
