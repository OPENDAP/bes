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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

#include "config.h"

#include <libdap/BaseType.h>
#include <libdap/Structure.h>
#include <libdap/Grid.h>
#include <libdap/Array.h>
#include <libdap/D4Maps.h>
#include <libdap/D4Group.h>
#include <libdap/DMR.h>
#include <libdap/util.h>

#include <BESDebug.h>

#include "grid_utils.h"
#include "gse_parser.h"
#include "GSEClause.h"
#include "GridGeoConstraint.h"

using namespace libdap;

int gse_parse(functions::gse_arg *arg);
void gse_restart(FILE * in);

// Glue routines declared in gse.lex
void gse_delete_buffer(void *buffer);
void *gse_string(const char *yy_str);

namespace functions {

/**
 * Recursively traverses the BaseType bt (if it's a constructor type) and collects pointers to all of the Grid and places said pointers
 * into the vector parameter 'grids'. If the BaseType parameter bt is an instance of Grid the it is placed in the vector.
 * @param bt The BaseType to evaluate
 * @param grids A vector into which to place a pointer to every Grid.
 */
void get_grids(BaseType *bt, vector<Grid *> *grids)
{
	switch (bt->type()) {

	case dods_grid_c: {
		// Yay! It's a Grid!
		grids->push_back(static_cast<Grid*>(bt));
		break;
	}
	case dods_structure_c: {
		// It's an Structure - but of what? Check each variable in the Structure.
		Structure &s = static_cast<Structure&>(*bt);
		for (Structure::Vars_iter i = s.var_begin(); i != s.var_begin(); i++) {
			get_grids(*i, grids);
		}
		break;
	}
	// Grids cannot be members of Array or Sequence in DAP2. jhrg 6/10/13
	case dods_array_c:
	case dods_sequence_c:
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
void get_grids(DDS &dds, vector<Grid *> *grids)
{
	for (DDS::Vars_iter i = dds.var_begin(); i != dds.var_end(); i++) {
		get_grids(*i, grids);
	}
}

/**
 * Evaluates a Grid to see if has suitable semantics for use with function_geogrid.
 *
 * @note Add an 'is' method to GeoGrid so that this code stop relying on an exception
 * thrown by GridGeoConstraint's ctor.
 *
 * @param grid the Grid to evaluate.
 * @return True if the grid will work with the GeoGrid function, otherwise false.
 */
bool is_geo_grid(Grid *grid)
{
	try {
		GridGeoConstraint gc(grid);
	}
	catch (Error *e) {
		return false;
	}

	return true;
}

void parse_gse_expression(gse_arg *arg, BaseType *expr)
{
    gse_restart(0); // Restart the scanner.
    void *cls = gse_string(extract_string_argument(expr).c_str());
    // gse_switch_to_buffer(cls); // Get set to scan the string.
    bool status = gse_parse(arg) == 0;
    gse_delete_buffer(cls);
    if (!status)
        throw Error(malformed_expr, "Error parsing grid selection.");
}

static void apply_grid_selection_expr(Grid *grid, GSEClause *clause)
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

/**
 * Recursively traverses the BaseType bt (if its a constructor type) and collects pointers to all of the Arrays and places said pointers
 * into the vector parameter 'coverages'. If the BaseType parameter bt is an instance of Array the it is placed in the vector.
 * @param bt The BaseType to evaluate
 * @param grids A vector into which to place a pointer to every Array.
 */
void get_coverages(BaseType *bt, vector<Array *> *coverages)
{
    switch (bt->type()) {

        case dods_array_c: {
            if ( static_cast<Array *>(bt)->is_dap2_grid() ) {
                // Yay! It's a Grid!
                coverages->push_back(static_cast<Array *>(bt));
            }
            break;
        }
        case dods_structure_c: {
            // It's a Structure - but of what? Check each variable in the Structure.
            Structure &s = static_cast<Structure&>(*bt);
            for (Structure::Vars_iter i = s.var_begin(); i != s.var_begin(); i++) {
                get_coverages(*i, coverages);
            }
            break;
        }

        case dods_group_c: {
            // It's a Group - but of what? Check each variable in the Group.
            D4Group &g = static_cast<D4Group&>(*bt);
            for (D4Group::Vars_iter i = g.var_begin(); i != g.var_begin(); i++) {
                get_coverages(*i, coverages);
            }
            break;
        }

        // Only an Array can be a Coverage in DAP4.
        default:
            break;
    }
}

/**
* Recursively traverses the DMR and collects pointers to all of the Arrays and places said pointers
* into the vector parameter 'coverages'.
* @param dds The dmr to search
* @param grids A vector into which to place a pointer to every Array in the DMR.
*/
void get_coverages(DMR &dmr, vector<Array *> *coverages)
{
    D4Group *root = dmr.root();

    // variables
    Constructor::Vars_iter v = root->var_begin();
    while (v != root->var_end()) {
        get_coverages(*v, coverages);
        ++v;
    }
    // groups
    D4Group::groupsIter g = root->grp_begin();
    while (g != root->grp_end())
        get_coverages((*g++),coverages);
}

static void apply_grid_selection_expr(Array *coverage, GSEClause *clause)
{
    // Basic plan: For each map, look at each clause and set start and stop
    // to be the intersection of the ranges in those clauses.
    D4Maps *d4_maps = coverage->maps();
    D4Maps::D4MapsIter miter = d4_maps->map_begin();
    while (miter != d4_maps->map_end() && (*miter)->name() != clause->get_map()->FQN())
        ++miter;

    if (miter == d4_maps->map_end())
        throw Error(malformed_expr,"The map vector '" + clause->get_map_name()
                                   + "' is not in the array '" + coverage->name() + "'.");

    D4Map *d4_map = (*miter);

    // Use pointer arith & the rule that map order must match array dim order

    Array::Dim_iter dim_i = coverage->dim_begin();
    while (dim_i != coverage->dim_end() && coverage->dimension_D4dim(dim_i)->fully_qualified_name() != clause->get_map()->FQN())
        ++dim_i;

    if (dim_i == coverage->dim_end())
        throw Error(malformed_expr,"The map vector '" + clause->get_map_name()
                                   + "' is not a dimension in the array '" + coverage->name() + "'.");

    Array::Dim_iter coverage_dim = (coverage->dim_begin() + (dim_i - coverage->dim_begin()));

    Array *map = const_cast<Array *>(d4_map->array());
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

    coverage->dimension_D4dim(coverage_dim)->set_constraint(start, 1, stop);
    coverage->add_constraint(coverage_dim,coverage->dimension_D4dim(coverage_dim));
}

void apply_grid_selection_expressions(Array * coverage, vector < GSEClause * >clauses)
{
    vector < GSEClause * >::iterator clause_i = clauses.begin();
    while (clause_i != clauses.end())
        apply_grid_selection_expr(coverage, *clause_i++);

    coverage->set_read_p(false);
}

} //namespace libdap
