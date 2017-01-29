
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

// (c) COPYRIGHT URI/MIT 2001
// Please read the full copyright statement in the file COPYRIGHT_URI.
//
// Authors:
//      jhrg,jimg       James Gallagher <jgallagher@gso.uri.edu>

// An interface which can be used to print ascii representations of DAP
// variables. 9/12/2001 jhrg

#ifndef _asciioutput_h
#define _asciioutput_h 1

#include <stdio.h>
#include <vector>

#include <InternalErr.h>
#include <BaseType.h>

using namespace std;
using namespace libdap;

class AsciiOutput {

    friend class AsciiOutputTest;

protected:
    BaseType *_redirect ;
public:
    AsciiOutput( BaseType *bt ) : _redirect( bt ) {}
    AsciiOutput() : _redirect( 0 ) {}
    virtual ~AsciiOutput() {}

    /** Get the fully qualified name of this object. Names of nested
	constructor types are separated by dots (.).

	@return The fully qualified name of this object. */
    string get_full_name();

    /** Print an ASCII representation for an instance of BaseType's children.
	This version prints the suggested output only for simple types.
	Complex types should overload this with their own definition.

	The caller of this method is responsible for adding a trialing comma
	where appropriate.

	@param strm Write to this stream.
	@param print_name If True, write the name of the variable, a comma
	and then the value. If False, simply write the value. */
    virtual void print_ascii(ostream &strm, bool print_name = true)
	throw(InternalErr);

    /** Increment #state# to the next value given #shape#. This method
	uses simple modulo arithmetic to provide a way to iterate over all
	combinations of dimensions of an Array or Grid. The vector #shape#
	holds the maximum sizes of each of N dimensions. The vector #state#
	holds the current index values of those N dimensions. Calling this
	method increments #state# to the next dimension, varying the
	rightmost fastest.

	To print DODS Array and Grid objects according to the DAP 2.0
	specification, #state# and #shape# should be vectors of length N-1
	for an object of dimension N.
	@return True if there are more states, false if not. */
    bool increment_state(vector<int> *state, const vector<int> &shape);
};

#endif
