
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2003,2013 OPeNDAP, Inc.
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

#include <BaseType.h>
#include <Str.h>
#include <Array.h>
#include <Grid.h>
#include <Error.h>
#include <DDS.h>
#include <debug.h>
#include <util.h>

#include "GeoGridFunction.h"
#include "GridGeoConstraint.h"
#include "gse_parser.h"
#include "grid_utils.h"

using namespace libdap;

namespace functions {

/** The geogrid function returns the part of a Grid that includes a
 geographically specified rectangle. The arguments to the function are the
 name of a Grid, the left-top and right-bottom points of the rectangle and
 zero or more relational expressions of the sort that the grid function
 accepts. The constraints on the arguments are:<ul> <li>The Grid must have
 Latitude and Longitude map vectors. Those are discovered by looking for
 map vectors which satisfy enough of any one of a set of conventions to
 make the identification of those map vectors positive or by guessing
 which maps are which. The set of conventions supported is: COARDS, CF
 1.0, GDT and CSC (see
 http://www.unidata.ucar.edu/software/netcdf/conventions.html). If the
 geogrid guesses at the maps, it adds an attribute (geogrid_warning) which
 says so. (in version 1.1)</li> <li>The rectangle corner points are in
 Longitude-Latitude. Longitude may be given using -180 to 180 or 0 to 360.
 For data sources with global coverage, geogrid assumes that the Longitude
 axis is circular. For requests made using 0/359 notation, it assumes it
 is modulus 360. Requests made using -180/179 notation cannot use values
 outside that range.</li> <li>The notation used to specify the rectangular
 region determines the notation used in the longitude/latitude map vectors
 of the Grid returned by the function.</li> <li>There are no restrictions
 on the relational expressions beyond those for the grid() (see
 func_grid_select()) function.</li> </ul>

 @note The geogrid() function is implemented as a 'BaseType function'
 which means that there can be only one function per request and no other
 variables may be named in the request.

 @param argc The number of values in argv.
 @param argv An array of BaseType pointers which hold the arguments to be
 passed to geogrid. The arguments may be Strings, Integers, or Reals,
 subject to the above constraints.
 @param btpp A pointer to the return value; caller must delete.

 @return The constrained and read Grid, ready to be sent. */
void
function_geogrid(int argc, BaseType *argv[], DDS &, BaseType **btpp)
{
    string info =
    string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
    "<function name=\"geogrid\" version=\"1.2\" href=\"http://docs.opendap.org/index.php/Server_Side_Processing_Functions#geogrid\">\n"+
    "</function>";

    if (argc == 0) {
        Str *response = new Str("version");
        response->set_value(info);
        *btpp = response;
        return ;
    }

    // There are two main forms of this function, one that takes a Grid and one
    // that takes a Grid and two Arrays. The latter provides a way to explicitly
    // tell the function which maps contain lat and lon data. The remaining
    // arguments are the same for both versions, although that includes a
    // varying argument list.

    // Look at the types of the first three arguments to determine which of the
    // two forms were used to call this function.
    Grid *l_grid = 0;
    if (argc < 1 || !(l_grid = dynamic_cast < Grid * >(argv[0]->ptr_duplicate())))
	throw Error(malformed_expr,"The first argument to geogrid() must be a Grid variable!");

    // Both forms require at least this many args
    if (argc < 5)
        throw Error(malformed_expr,"Wrong number of arguments to geogrid() (expected at least 5 args). See geogrid() for more information.");

    bool grid_lat_lon_form;
    Array *l_lat = 0;
    Array *l_lon = 0;
    if (!(l_lat = dynamic_cast < Array * >(argv[1]))) //->ptr_duplicate())))
	grid_lat_lon_form = false;
    else if (!(l_lon = dynamic_cast < Array * >(argv[2]))) //->ptr_duplicate())))
	throw Error(malformed_expr,"When using the Grid, Lat, Lon form of geogrid() both the lat and lon maps must be given (lon map missing)!");
    else
	grid_lat_lon_form = true;

    if (grid_lat_lon_form && argc < 7)
        throw Error(malformed_expr,"Wrong number of arguments to geogrid() (expected at least 7 args). See geogrid() for more information.");

#if 0
    Grid *l_grid = dynamic_cast < Grid * >(argv[0]->ptr_duplicate());
    if (!l_grid)
        throw Error(malformed_expr,"The first argument to geogrid() must be a Grid variable!");
#endif
    // Read the maps. Do this before calling parse_gse_expression(). Avoid
    // reading the array until the constraints have been applied because it
    // might be really large.
    //
    // Trick: Some handlers build Grids from a combination of Array
    // variables and attributes. Those handlers (e.g., hdf4) use the send_p
    // property to determine which parts of the Grid to read *but they can
    // only read the maps from within Grid::read(), not the map's read()*.
    // Since the Grid's array does not have send_p set, it will not be read
    // by the call below to Grid::read().
    Grid::Map_iter i = l_grid->map_begin();
    while (i != l_grid->map_end())
        (*i++)->set_send_p(true);

    l_grid->read();
    // Calling read() above sets the read_p flag for the entire grid; clear it
    // for the grid's array so that later on the code will be sure to read it
    // under all circumstances.
    l_grid->get_array()->set_read_p(false);

    // Look for Grid Selection Expressions tacked onto the end of the BB
    // specification. If there are any, evaluate them before evaluating the BB.
    int min_arg_count = (grid_lat_lon_form) ? 7 : 5;
    if (argc > min_arg_count) {
        // argv[5..n] holds strings; each are little Grid Selection Expressions
        // to be parsed and evaluated.
        vector < GSEClause * > clauses;
        gse_arg *arg = new gse_arg(l_grid);
        for (int i = min_arg_count; i < argc; ++i) {
            parse_gse_expression(arg, argv[i]);
            clauses.push_back(arg->get_gsec());
        }
        delete arg;
        arg = 0;

        apply_grid_selection_expressions(l_grid, clauses);
    }

    try {
        // Build a GeoConstraint object. If there are no longitude/latitude
        // maps then this constructor throws Error.
        GridGeoConstraint gc(l_grid);

        // This sets the bounding box and modifies the maps to match the
        // notation of the box (0/359 or -180/179)
        int box_index_offset = (grid_lat_lon_form) ? 3 : 1;
        double top = extract_double_value(argv[box_index_offset]);
        double left = extract_double_value(argv[box_index_offset + 1]);
        double bottom = extract_double_value(argv[box_index_offset + 2]);
        double right = extract_double_value(argv[box_index_offset + 3]);
        gc.set_bounding_box(top, left, bottom, right);
        DBG(cerr << "geogrid: past bounding box set" << endl);

        // This also reads all of the data into the grid variable
        gc.apply_constraint_to_data();
        DBG(cerr << "geogrid: past apply constraint" << endl);

        // In this function the l_grid pointer is the same as the pointer returned
        // by this call. The caller of the function must free the pointer.
        *btpp = gc.get_constrained_grid();
        return;
    }
    catch (Error &e) {
        throw e;
    }
    catch (exception & e) {
        throw
        InternalErr(string
                ("A C++ exception was thrown from inside geogrid(): ")
                + e.what());
    }
}


/**
 * The passed DDS parameter dds is evaluated to see if it contains Grid objects whose semantics allow them
 * to be operated on by function_geogrid()
 *
 * @param dds The DDS to be evaluated.
 */
bool GeoGridFunction::canOperateOn(DDS &dds)
{
    bool usable = false;

    // Go find all the Grid variables.
	//vector<Grid *> *grids = new vector<Grid *>();
	vector<Grid*> grids;
	getGrids(dds, &grids);

	// Were there any?
    if(!grids.empty()){
    	// Apparently so...

    	// See if any one of them looks like suitable GeoGrid
    	vector<Grid *>::iterator git;
    	for(git=grids.begin(); !usable && git!=grids.end() ; git++){
    		Grid *grid = *git;
    		usable = isGeoGrid(grid);
    	}
    }
    //delete grids;

	return usable;
}

} // namesspace libdap
