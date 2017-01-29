
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

// Implementation for AsciiByte. See the comments in AsciiByte.h
//
// 3/12/98 jhrg

#include "config.h"

#include <algorithm>
#include <iostream>

#include "BaseType.h"
#include "debug.h"

#include "AsciiOutput.h"
#include "get_ascii.h"

using namespace dap_asciival;

string AsciiOutput::get_full_name()
{
    BaseType *this_btp = dynamic_cast < BaseType * >(this);
    if (!this_btp)
        throw InternalErr(__FILE__, __LINE__,
                          "Instance of AsciiOuput must also be a BaseType.");
    BaseType *btp = _redirect;
    if (!btp)
        btp = this_btp;

    BaseType *btp2 = this_btp->get_parent();
    if (!btp2)
        return btp->name();     // Must be top-level node/variable.
    else
        return dynamic_cast < AsciiOutput * >(btp2)->get_full_name()
            + "." + btp->name();
}

/** @brief Print values as ASCII
    Prints the values of \e this in ASCII suitable for import into a
    spreadsheet. This version prints only the values of simple types; other
    types such as Array specialize this method (see AsciiArray::print_ascii()).
    @param strm Output stream for values
    @print_name Name of this variable to include in the ASCII output. */
void AsciiOutput::print_ascii(ostream &strm,
                              bool print_name) throw(InternalErr)
{
    BaseType *BTptr = _redirect;
    if (!BTptr) {
        BTptr = dynamic_cast < BaseType * >(this);
    }

    if (!BTptr)
        throw InternalErr(__FILE__, __LINE__,
                          "An instance of AsciiOutput failed to cast to BaseType.");

    if (print_name)
        strm << get_full_name() << ", " ;

    BTptr->print_val(strm, "", false);
}

// This code implements simple modulo arithmetic. The vector shape contains
// the maximum count value for each dimension, state contains the current
// state. For example, if shape holds 10, 20 then when state holds 0, 20
// calling this method will increment state to 1, 0. For this example,
// calling the method with state equal to 10, 20 will reset state to 0, 0 and
// the return value will be false.
bool AsciiOutput::increment_state(vector < int >*state,
                                  const vector < int >&shape)
{
    DBG(cerr << "Entering increment_state" << endl);

    vector < int >::reverse_iterator state_riter;
    vector < int >::const_reverse_iterator shape_riter;
    for (state_riter = state->rbegin(), shape_riter = shape.rbegin();
         state_riter < state->rend(); state_riter++, shape_riter++) {
        if (*state_riter == *shape_riter - 1) {
            *state_riter = 0;
        } else {
            *state_riter = *state_riter + 1;

            DBG(cerr << "Returning state:";
                for_each(state->begin(), state->end(), print < int >);
                cerr << endl);

            return true;
        }
    }

    DBG(cerr << "Returning state without change:";
        for_each(state->begin(), state->end(), print < int >);
        cerr << endl);

    return false;
}
