
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
 
// (c) COPYRIGHT URI/MIT 1995-1996,1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      jhrg,jimg       James Gallagher <jgallagher@gso.uri.edu>

// implementation for TestGrid. See TestByte.
//
// jhrg 1/13/95

#include "TestGrid.h"
#include "TestCommon.h"

extern int test_variable_sleep_interval;

void
TestGrid::_duplicate(const TestGrid &ts)
{
    d_series_values = ts.d_series_values;
}

BaseType *
TestGrid::ptr_duplicate()
{
    return new TestGrid(*this);
}

TestGrid::TestGrid(const TestGrid &rhs) : Grid(rhs), TestCommon(rhs)
{
    _duplicate(rhs);
}

void 
TestGrid::output_values(std::ostream &out)
{
    // value_written controls comma printing
    bool value_written = false;
    bool pyg = projection_yields_grid();
    if (pyg)
        out << "{  Array: " ;
    else //if (components(true) > 1)
        out << "{ " ;

    if (array_var()->send_p()) {
        array_var()->print_val(out, "", false);
        value_written = true;
    }
        
    if (pyg) {
        out << "  Maps: " ;
        // No comma needed if the 'Maps:' separator is written out
        value_written = false;
    }

    Map_citer i = map_begin();
    // Write the first (and maybe only) value.
    while(i != map_end() && !value_written) {
        if ((*i)->send_p()) {
            (*i++)->print_val(out, "", false);
            value_written = true;
        }
        else {
            ++i;
        }
    }
    // Each subsequent value will be preceded by a comma
    while(i != map_end()) {
        if ((*i)->send_p()) {
            out << ", ";
            (*i++)->print_val(out, "", false);
        }
        else {
            ++i;
        }
    }

    //if (pyg || components(true) > 1)
        out << " }" ;
}

TestGrid &
TestGrid::operator=(const TestGrid &rhs)
{
    if (this == &rhs)
	return *this;

    dynamic_cast<Grid &>(*this) = rhs; // run Constructor=

    _duplicate(rhs);

    return *this;
}


TestGrid::TestGrid(const string &n) : Grid(n), d_series_values(false)
{
}

TestGrid::TestGrid(const string &n, const string &d)
    : Grid(n, d), d_series_values(false)
{
}

TestGrid::~TestGrid()
{
}

bool TestGrid::read()
{
    if (read_p())
        return true;

    get_array()->read();

    for (Map_iter i = map_begin(); i != map_end(); i++) {
        if (!(*i)->read()) {
            return false;
        }
    }

    set_read_p(true);

    return true;
}

void
TestGrid::set_series_values(bool sv)
{
    Map_iter i = map_begin();
    while (i != map_end()) {
        dynamic_cast<TestCommon&>(*(*i)).set_series_values(sv);
        ++i;
    }
    
    dynamic_cast<TestCommon&>(*array_var()).set_series_values(sv);
    
    d_series_values = sv;
}
