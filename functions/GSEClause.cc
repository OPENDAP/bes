
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of libdap, A C++ implementation of the OPeNDAP Data
// Access Protocol.

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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.

// (c) COPYRIGHT URI/MIT 1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      jhrg,jimg       James Gallagher <jgallagher@gso.uri.edu>

// The Grid Selection Expression Clause class.


#include "config.h"

static char id[] not_used =
    {"$Id: GSEClause.cc 24370 2011-03-28 16:21:32Z jimg $"
    };

#include <iostream>
#include <sstream>

#include "dods-datatypes.h"
#include "Error.h"
#include "InternalErr.h"

#include "debug.h"
#include "GSEClause.h"
#include "parser.h"
#include "gse.tab.hh"

using namespace std;

int gse_parse(void *arg);
void gse_restart(FILE *in);

// Glue routines declared in gse.lex
void gse_switch_to_buffer(void *new_buffer);
void gse_delete_buffer(void * buffer);
void *gse_string(const char *yy_str);

namespace libdap {

// Private methods

GSEClause::GSEClause()
{
    throw InternalErr(__FILE__, __LINE__, "default ctor called for GSEClause");
}

GSEClause::GSEClause(const GSEClause &)
{
    throw InternalErr(__FILE__, __LINE__, "copy ctor called for GSEClause");
}

GSEClause &GSEClause::operator=(GSEClause &)
{
    throw InternalErr(__FILE__, __LINE__, "assigment called for GSEClause");
}

// For the comparisions here, we should use an epsilon to catch issues
// with floating point values. jhrg 01/12/06
template<class T>
static bool
compare(T elem, relop op, double value)
{
    switch (op) {
    case dods_greater_op:
        return elem > value;
    case dods_greater_equal_op:
        return elem >= value;
    case dods_less_op:
        return elem < value;
    case dods_less_equal_op:
        return elem <= value;
    case dods_equal_op:
        return elem == value;
    case dods_not_equal_op:
        return elem != value;
    case dods_nop_op:
        throw Error(malformed_expr, "Attempt to use NOP in Grid selection.");
    default:
        throw Error(malformed_expr, "Unknown relational operator in Grid selection.");
    }
}

// These values are used in error messages, hence the strings.
template<class T>
void
GSEClause::set_map_min_max_value(T min, T max)
{
    DBG(cerr << "Inside set map min max value " << min << ", " << max << endl);
    std::ostringstream oss1;
    oss1 << min;
    d_map_min_value = oss1.str();

    std::ostringstream oss2;
    oss2 << max;
    d_map_max_value = oss2.str();
}

// Read the map array, scan, set start and stop.
template<class T>
void
GSEClause::set_start_stop()
{
    T *vals = new T[d_map->length()];
    d_map->value(vals);

    // Set the map's max and min values for use in error messages (it's a lot
    // easier to do here, now, than later... 9/20/2001 jhrg)
    set_map_min_max_value<T>(vals[d_start], vals[d_stop]);

    // Starting at the current start point in the map (initially index position
    // zero), scan forward until the comparison is true. Set the new value
    // of d_start to that location. Note that each clause applies to exactly
    // one map. The 'i <= end' test keeps us from setting start _past_ the
    // end ;-)
    int i = d_start;
    int end = d_stop;
    while (i <= end && !compare<T>(vals[i], d_op1, d_value1))
        i++;

    d_start = i;

    // Now scan backward from the end. We scan all the way to the actual start
    // although it would probably work to stop at 'i >= d_start'.
    i = end;
    while (i >= 0 && !compare<T>(vals[i], d_op1, d_value1))
        i--;
    d_stop = i;

    // Every clause must have one operator but the second is optional since
    // the more complex form of a clause is optional. That is, the above two
    // loops took care of constraints like 'x < 7' but we need the following
    // for ones like '3 < x < 7'.
    if (d_op2 != dods_nop_op) {
        int i = d_start;
        int end = d_stop;
        while (i <= end && !compare<T>(vals[i], d_op2, d_value2))
            i++;

        d_start = i;

        i = end;
        while (i >= 0 && !compare<T>(vals[i], d_op2, d_value2))
            i--;

        d_stop = i;
    }
    
    delete[] vals;
}

void
GSEClause::compute_indices()
{
    switch (d_map->var()->type()) {
    case dods_byte_c:
        set_start_stop<dods_byte>();
        break;
    case dods_int16_c:
        set_start_stop<dods_int16>();
        break;
    case dods_uint16_c:
        set_start_stop<dods_uint16>();
        break;
    case dods_int32_c:
        set_start_stop<dods_int32>();
        break;
    case dods_uint32_c:
        set_start_stop<dods_uint32>();
        break;
    case dods_float32_c:
        set_start_stop<dods_float32>();
        break;
    case dods_float64_c:
        set_start_stop<dods_float64>();
        break;
    default:
        throw Error(malformed_expr,
                    "Grid selection using non-numeric map vectors is not supported");
    }

}

// Public methods

/** @brief Create an instance using discrete parameters. */
GSEClause::GSEClause(Grid *grid, const string &map, const double value,
                     const relop op)
        : d_map(0),
        d_value1(value), d_value2(0), d_op1(op), d_op2(dods_nop_op),
        d_map_min_value(""), d_map_max_value("")
{
    d_map = dynamic_cast<Array *>(grid->var(map));
    if (!d_map)
        throw Error(string("The map variable '") + map
                    + string("' does not exist in the grid '")
                    + grid->name() + string("'."));

    DBG(cerr << d_map->toString());

    // Initialize the start and stop indices.
    Array::Dim_iter iter = d_map->dim_begin();
    d_start = d_map->dimension_start(iter);
    d_stop = d_map->dimension_stop(iter);

    compute_indices();
}

/** @brief Create an instance using discrete parameters. */
GSEClause::GSEClause(Grid *grid, const string &map, const double value1,
                     const relop op1, const double value2, const relop op2)
        : d_map(0),
        d_value1(value1), d_value2(value2), d_op1(op1), d_op2(op2),
        d_map_min_value(""), d_map_max_value("")
{
    d_map = dynamic_cast<Array *>(grid->var(map));
    if (!d_map)
        throw Error(string("The map variable '") + map
                    + string("' does not exist in the grid '")
                    + grid->name() + string("'."));

    DBG(cerr << d_map->toString());

    // Initialize the start and stop indices.
    Array::Dim_iter iter = d_map->dim_begin();
    d_start = d_map->dimension_start(iter);
    d_stop = d_map->dimension_stop(iter);

    compute_indices();
}

/** Class invariant.
    @return True if the object is valid, otherwise False. */
bool
GSEClause::OK() const
{
    if (!d_map)
        return false;

    // More ...

    return true;
}

/** @brief Get a pointer to the map variable constrained by this clause.
    @return The Array object. */
Array *
GSEClause::get_map() const
{
    return d_map;
}

/** @brief Set the pointer to the map vector contrained by this clause.

    Note that this method also sets the name of the map vector.
    @return void */
void
GSEClause::set_map(Array *map)
{
    d_map = map;
}

/** @brief Get the name of the map variable constrained by this clause.
    @return The Array object's name. */
string
GSEClause::get_map_name() const
{
    return d_map->name();
}

/** @brief Get the starting index of the clause's map variable as
    constrained by this clause.
    @return The start index. */
int
GSEClause::get_start() const
{
    return d_start;
}

/** @brief Set the starting index.
    @return void */
void
GSEClause::set_start(int start)
{
    d_start = start;
}

/** @brief Get the stopping index of the clause's map variable as
    constrained by this clause.
    @return The stop index. */
int
GSEClause::get_stop() const
{
    DBG(cerr << "Returning stop index value of: " << d_stop << endl);
    return d_stop;
}

/** @brief Set the stopping index.
    @return void */
void
GSEClause::set_stop(int stop)
{
    d_stop = stop;
}

/** @brief Get the minimum map vector value.

    Useful in messages back to users.
    @return The minimum map vetor value. */
string
GSEClause::get_map_min_value() const
{
    return d_map_min_value;
}

/** @brief Get the maximum map vector value.

    Useful in messages back to users.
    @return The maximum map vetor value. */
string
GSEClause::get_map_max_value() const
{
    return d_map_max_value;
}

} // namespace libdap

