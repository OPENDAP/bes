
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of www_int, software which returns an HTML form which
// can be used to build a URL to access data from a DAP data server.

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

// Interface for the class WWWSequence. See WWWByte.h
//
// 4/7/99 jhrg

#ifndef _wwwsequence_h
#define _wwwsequence_h 1

#include <libdap/Sequence.h>

using namespace libdap ;

class WWWSequence: public Sequence {
private:
    bool is_simple_sequence();
    void print_header(ostream &os);

public:
    WWWSequence(const string &n);
    WWWSequence( Sequence *bt );
    virtual ~WWWSequence();

    virtual BaseType *ptr_duplicate();

    virtual int size() const;
#ifdef FILE_METHODS
    virtual void print_val(FILE *os, string space = "",
			   bool print_decl_p = true);
#endif
    virtual void print_val(ostream &strm, string space = "",
			   bool print_decl_p = true);
};

#endif
