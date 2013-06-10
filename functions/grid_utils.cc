// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: Nathan Potter <npotter@opendap.org>
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

#include <BaseType.h>
#include <Structure.h>
#include <Grid.h>
#include <util.h>

#include <BESDebug.h>

#include "grid_utils.h"
#include "gse_parser.h"
#include "GSEClause.h"
#include "GridGeoConstraint.h"

int gse_parse(void *arg);
void gse_restart(FILE * in);

// Glue routines declared in gse.lex
// void gse_switch_to_buffer(void *new_buffer);
void gse_delete_buffer(void *buffer);
void *gse_string(const char *yy_str);

using namespace libdap;

namespace libdap {

/**
 * Recursively traverses the BaseType bt (if its a conrtuctor type) and collects pointers to all of the Grid and places said pointers
 * into the vector parameter 'grids'. If the BaseType parameter bt is an instance of Grid the it is placed in the vector.
 * @param bt The BaseType to evaluate
 * @param grids A vector into which to place a pointer to every Grid.
 */
void getGrids(BaseType *bt, vector<Grid *> *grids) {
	Type type = bt->type();

	//BESDEBUG( "libdap::getGrids()","libdap::getGrids() - Searching for Grid variables. Current variable: "<< bt->name() << "  type: "<< bt->type_name() << endl);

	switch(type){

		case dods_grid_c:
		{
			// Yay! It's a Grid!
			Grid *grid = &dynamic_cast<Grid&>(*bt);
			grids->push_back(grid);
			break;
		}
		case dods_array_c:
		{
			// It's an Array - but of what? Check the template variable.
			Array &array = dynamic_cast<Array&>(*bt);
			BaseType *arrayTemplate = array.var("",true,0);
			getGrids(arrayTemplate, grids);
			break;
		}
#if 0
		// Sequences can contain only simple types; maybe that's a design flaw...
		case dods_sequence_c:
		{
			// It's an Sequence - but of what? Check each variable in the Sequence.
			Sequence &seq = dynamic_cast<Sequence&>(*bt);
			for(Sequence::Vars_iter i=seq.var_begin(); i!=seq.var_begin(); i++){
				BaseType *sbt = *i;
				getGrids(sbt, grids);
			}
			break;
		}
#endif
		case dods_structure_c:
		{
			// It's an Structure - but of what? Check each variable in the Structure.
			Structure &s = dynamic_cast<Structure&>(*bt);
			for(Structure::Vars_iter i=s.var_begin(); i!=s.var_begin(); i++){
				BaseType *sbt = *i;
				getGrids(sbt, grids);
			}
			break;
		}
		default:
			break;
	}

}



/**
 * Recursively traverses the DDS and collects pointers to all of the Grids and places said pointers
 * into the vector parameter 'grids'.
 * @param dds The dds to search
 * @param grids A vector into which to place a pointer to every Grid in the DDS.
 */
void getGrids(DDS &dds, vector<Grid *> *grids){
	//BESDEBUG( "libdap::getGrids()","libdap::getGrids() - Searching for Grid variables in DDS." << endl);
    for (DDS::Vars_iter i=dds.var_begin(); i!=dds.var_end(); i++) {
        BaseType *bt = *i;
        getGrids(bt, grids);
    }
}



/**
 * Evaluates a Grid to see if has suitable semantics for use with function_geogrid
 * @param grid the Grid to evaluate.
 */
bool isGeoGrid(Grid *grid){


	//BESDEBUG( "libdap::isGeoGrid()","libdap::isGeoGrid() - Checking coordinate map semantics for Grid: "<< grid->name() << endl);

    bool foundUsableMetadata = true;

    Grid::Map_iter m = grid->map_begin();


	//BESDEBUG( "libdap::isGeoGrid()","libdap::isGeoGrid() - Attempting to instantiate a GridGeoConstraint using Grid "<< grid->name() << endl);
	try {
		GridGeoConstraint gc(grid);
	} catch (Error *e) {
		//BESDEBUG( "libdap::isGeoGrid()","libdap::isGeoGrid() - Failed to instantiate a GridGeoConstraint using Grid "<< grid->name() << endl);
		foundUsableMetadata = false;
	}

    if(foundUsableMetadata){
		//BESDEBUG( "libdap::isGeoGrid()","libdap::isGeoGrid() - Grid "<< grid->name() << " MATCHES the semantics of a GeoGrid"<< endl);
    }
    else {
    	//BESDEBUG( "libdap::isGeoGrid()","libdap::isGeoGrid() - Grid "<< grid->name() << " DOES NOT match the semantics of a GeoGrid"<< endl);
    }
	return foundUsableMetadata;
}

void parse_gse_expression(gse_arg * arg, BaseType * expr)
{
    gse_restart(0); // Restart the scanner.
    void *cls = gse_string(extract_string_argument(expr).c_str());
    // gse_switch_to_buffer(cls); // Get set to scan the string.
    bool status = gse_parse((void *) arg) == 0;
    gse_delete_buffer(cls);
    if (!status)
        throw Error(malformed_expr, "Error parsing grid selection.");
}

void apply_grid_selection_expr(Grid *grid, GSEClause *clause)
{
    // Basic plan: For each map, look at each clause and set start and stop
    // to be the intersection of the ranges in those clauses.
    Grid::Map_iter map_i = grid->map_begin();
    while (map_i != grid->map_end() && (*map_i)->name() != clause->get_map_name())
        ++map_i;

    if (map_i == grid->map_end())
        throw Error(malformed_expr,"The map vector '" + clause->get_map_name()
                + "' is not in the grid '" + grid->name() + "'.");

    // Use pointer arith & the rule that map order must match array dim order
    Array::Dim_iter grid_dim = (grid->get_array()->dim_begin() + (map_i - grid->map_begin()));

    Array *map = dynamic_cast < Array * >((*map_i));
    if (!map)
        throw InternalErr(__FILE__, __LINE__, "Expected an Array");
    int start = max(map->dimension_start(map->dim_begin()), clause->get_start());
    int stop = min(map->dimension_stop(map->dim_begin()), clause->get_stop());

    if (start > stop) {
        ostringstream msg;
        msg
                << "The expressions passed to grid() do not result in an inclusive \n"
                << "subset of '" << clause->get_map_name()
                << "'. The map's values range " << "from "
                << clause->get_map_min_value() << " to "
                << clause->get_map_max_value() << ".";
        throw Error(malformed_expr,msg.str());
    }

    BESDEBUG("GeoGrid", "Setting constraint on " << map->name() << "[" << start << ":" << stop << "]" << endl);

    // Stride is always one.
    map->add_constraint(map->dim_begin(), start, 1, stop);
    grid->get_array()->add_constraint(grid_dim, start, 1, stop);
}

void apply_grid_selection_expressions(Grid * grid, vector < GSEClause * >clauses)
{
    vector < GSEClause * >::iterator clause_i = clauses.begin();
    while (clause_i != clauses.end())
        apply_grid_selection_expr(grid, *clause_i++);

    grid->set_read_p(false);
}

} //namespace libdap
