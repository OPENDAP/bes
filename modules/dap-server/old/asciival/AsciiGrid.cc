// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of asciival, software which can return an ASCII
// representation of the data read from a DAP server.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
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

// (c) COPYRIGHT URI/MIT 1998,2000
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      jhrg,jimg       James Gallagher <jgallagher@gso.uri.edu>

// implementation for AsciiGrid. See AsciiByte.
//
// 3/12/98 jhrg

#include "config.h"

#include <iostream>
#include <string>

using namespace std;

#include <InternalErr.h>

#include <BESDebug.h>

// #define DODS_DEBUG

#include "AsciiGrid.h"
#include "AsciiArray.h"
#include "debug.h"
#include "get_ascii.h"

using namespace dap_asciival;

BaseType *
AsciiGrid::ptr_duplicate()
{
    return new AsciiGrid(*this);
}

AsciiGrid::AsciiGrid(const string &n) :
    Grid(n)
{
}

AsciiGrid::AsciiGrid(Grid *grid) :
    Grid(grid->name()), AsciiOutput(grid)
{
    BaseType *bt = basetype_to_asciitype(grid->array_var());
    // Added qualification for 'array' which is libdap::Part::array, but adding
    // the full qualification results in a warning about the code being c++-11
    // specific. The line with no qualification on 'array' does not compile on
    // gcc 6. jhrg 2/4/16
    add_var(bt, libdap::array);
    // add_var makes a copy of the base type passed to it, so delete it here
    delete bt;
    bt = 0;

    Grid::Map_iter i = grid->map_begin();
    Grid::Map_iter e = grid->map_end();
    while (i != e) {
        bt = basetype_to_asciitype(*i);
        add_var(bt, maps);
        // add_var makes a copy of the base type passed to it, so delete it here
        delete bt;
        ++i;
    }

    BaseType::set_send_p(grid->send_p());
}

AsciiGrid::~AsciiGrid()
{
}

void AsciiGrid::print_ascii(ostream &strm, bool print_name)
        throw(InternalErr)
{
    BESDEBUG("ascii", "In AsciiGrid::print_ascii" << endl);

    Grid *g = dynamic_cast<Grid *> (_redirect);
    if (!g)
        g = this;

    // If the 'array' part of the Grid is not projected, then only maps are
    // to be printed and those should be printed like arrays in a structure.
    // Similarly, if any of the maps are not projected, then the maps and
    // array in the grid should be printed like arrays in a structure. The
    // general rule is that if everything in the Grid (all maps plus the array)
    // are projected, then print as a Grid, else print as if the Gird is a
    // Structure.
    if (projection_yields_grid()) {
        if (dynamic_cast<Array &> (*g->array_var()).dimensions(true) > 1)
            print_grid(strm, print_name);
        else
            print_vector(strm, print_name);
    }
    else {
        Map_iter m = map_begin();
        while (m != map_end()) {
            if ((*m)->send_p()) {
                dynamic_cast<AsciiArray&>(**m).print_ascii(strm, print_name);
                strm << "\n";
            }
            ++m;
        }

        if (array_var()->send_p()) {
            dynamic_cast<AsciiArray&>(*array_var()).print_ascii(strm, print_name);
            strm << "\n";
        }
    }
}

// Similar to AsciiArray's print_vector. Print a Grid that has only one
// dimension. To fit the spec we can call print_ascii() on the map vector and
// then the array (which has only one dimension). This is a special case; if
// a grid has two or more dimensions then we can't use the AsciiArray code.
//
// Note that for the variable to be considered a Grid, it has to have all its
// parts projected so there's no need to test send_p(). If anything is not part
// of the current projection, then the variable is sent as a Structure.
void AsciiGrid::print_vector(ostream &strm, bool print_name)
{
    BESDEBUG("ascii", "In AsciiGrid::print_vector" << endl);

    dynamic_cast<AsciiArray&> (**map_begin()).print_ascii(strm, print_name);

    strm << "\n";

    dynamic_cast<AsciiArray&> (*array_var()).print_ascii(strm, print_name);
}

void AsciiGrid::print_grid(ostream &strm, bool print_name)
{
    BESDEBUG("ascii", "In AsciiGrid::print_grid" << endl);

    Grid *g = dynamic_cast<Grid *> (_redirect);
    if (!g) {
        g = this;
    }
    // Grab the Grid's array
    Array *grid_array = dynamic_cast<Array *> (g->array_var());
    if (!grid_array) throw InternalErr(__FILE__, __LINE__, "Expected an Array");

    AsciiArray *a_grid_array = dynamic_cast<AsciiArray *> (array_var());
    if (!a_grid_array) throw InternalErr(__FILE__, __LINE__, "Expected an AsciiArray");

    AsciiOutput *ao_grid_array = dynamic_cast<AsciiOutput *> (a_grid_array);
    if (!ao_grid_array) throw InternalErr(__FILE__, __LINE__, "Expected an AsciiOutput");

    // Set up the shape and state vectors. Shape holds the shape of this
    // array, state holds the index of the current vector to print.
    int dims = grid_array->dimensions(true);
    if (dims <= 1)
        throw InternalErr(__FILE__, __LINE__,
                "Dimension count is <= 1 while printing multidimensional array.");

    // shape holds the maximum index value of each dimension of the array
    // (not the size; each value is one less that the size).
    vector<int> shape = a_grid_array->get_shape_vector(dims - 1);
    int rightmost_dim_size = a_grid_array->get_nth_dim_size(dims - 1);

    // state holds the indexes of the current row being printed. For an N-dim
    // array, there are N-1 dims that are iterated over when printing (the
    // Nth dim is not printed explicitly. Instead it's the number of values
    // on the row.
    vector<int> state(dims - 1, 0);

    // Now that we have the number of dims, get and print the rightmost map.
    // This is cumbersome; if we used the STL it would be much less so.
    // We are now using STL, so it isn't so cumbersome. pcw
    // By definition, a map is a vector. Print the rightmost map.
    dynamic_cast<AsciiArray &> (**(map_begin() + dims - 1)) .print_ascii(
            strm, print_name);
    strm << "\n";

    bool more_indices;
    int index = 0;
    do {
        // Print indices for all dimensions except the last one. Include the
        // name of the corresponding map vector and the *value* of this
        // index. Note that the successive elements of state give the indices
        // of each of the N-1 dimensions for the current row.
        string n = ao_grid_array->get_full_name();

        strm << n;

        vector<int>::iterator state_iter = state.begin();
        Grid::Map_iter p = g->map_begin();
        Grid::Map_iter ap = map_begin();
        while (state_iter != state.end()) {
            Array *map = dynamic_cast<Array *> (*p);
            if (!map) throw InternalErr(__FILE__, __LINE__, "Expected an Array");

            AsciiArray *amap = dynamic_cast<AsciiArray *> (*ap);
            if (!amap) throw InternalErr(__FILE__, __LINE__, "Expected an AsciiArray");

            AsciiOutput *aomap = dynamic_cast<AsciiOutput *> (amap);
            if (!aomap) throw InternalErr(__FILE__, __LINE__, "Expected an AsciiOutput");

            strm << "[" << aomap->get_full_name() << "=";
            BaseType *avar = basetype_to_asciitype(map->var(*state_iter));
            AsciiOutput &aovar = dynamic_cast<AsciiOutput &> (*avar);
            aovar.print_ascii(strm, false);
            // we aren't saving a var for future reference so need to delete
            delete avar;
            strm << "]";

            state_iter++;
            p++;
            ap++;
        }
        strm << ", ";

        index = a_grid_array->print_row(strm, index, rightmost_dim_size - 1);

        more_indices = increment_state(&state, shape);
        if (more_indices)
            strm << "\n";

    } while (more_indices);
}
