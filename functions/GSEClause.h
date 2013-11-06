
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

// (c) COPYRIGHT URI/MIT 1999
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      jhrg,jimg       James Gallagher <jgallagher@gso.uri.edu>

// The Grid Selection Expression Clause class.

#ifndef _gseclause_h
#define _gseclause_h 1

#include <string>
#include <sstream>

#include <BaseType.h>
#include <Array.h>
#include <Grid.h>

namespace libdap
{

enum relop {
    dods_nop_op,
    dods_greater_op,
    dods_greater_equal_op,
    dods_less_op,
    dods_less_equal_op,
    dods_equal_op,
    dods_not_equal_op
};

/** Holds the results of parsing one of the Grid Selection Expression
    clauses. The Grid selection function takes a set of clauses as arguments
    and must create one instance of this class for each of those clauses. The
    GridSelectionExpr class holds N instances of this class.

    @author James Gallagher
    @see GridSelectionExpr */

class GSEClause
{
private:
    Array *d_map;
    // _value1, 2 and _op1, 2 hold the first and second operators and
    // operands. For a clause like `var op value' only _op1 and _value1 have
    // valid information. For a clause like `value op var op value' the
    // second operator and operand are on _op2 and _value2. 1/19/99 jhrg
    double d_value1, d_value2;
    relop d_op1, d_op2;
    int d_start;
    int d_stop;

    string d_map_min_value, d_map_max_value;

    GSEClause();  // Hidden default constructor.

    GSEClause(const GSEClause &param); // Hide
    GSEClause &operator=(GSEClause &rhs); // Hide

    template<class T> void set_start_stop();
    template<class T> void set_map_min_max_value(T min, T max);

    void compute_indices();

public:
    /** @name Constructors */
    //@{
    GSEClause(Grid *grid, const string &map, const double value,
              const relop op);

    GSEClause(Grid *grid, const string &map, const double value1,
              const relop op1, const double value2, const relop op2);
    //@}

    virtual ~GSEClause() {
    	delete d_map; d_map = 0;
    }
    
    bool OK() const;

    /** @name Accessors */
    //@{
    Array *get_map() const;

    string get_map_name() const;

    int get_start() const;

    int get_stop() const;

    string get_map_min_value() const;

    string get_map_max_value() const;
    //@}

    /** @name Mutators */
    //@{
    void set_map(Array *map);

    void set_start(int start);

    void set_stop(int stop);
    //@}
};

} // namespace libdap

#endif // _gseclause_h

