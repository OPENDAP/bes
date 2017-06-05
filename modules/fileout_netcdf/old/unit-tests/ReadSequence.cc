
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of asciival, software which can return an XML data
// representation of the data read from a DAP server.

// Copyright (c) 2010 OPeNDAP, Inc.
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

// Implementation for the class ReadStructure. See ReadByte.cc
//
// 3/12/98 jhrg

#include "config.h"

#include <iostream>
#include <string>

#include <InternalErr.h>
#include <util.h>
#include <debug.h>

#include "ReadSequence.h"

BaseType *
ReadSequence::ptr_duplicate()
{
    return new ReadSequence(*this);
}

ReadSequence::ReadSequence(const string &n) : Sequence(n)
{
}

ReadSequence::ReadSequence(Sequence * bt)
    : Sequence( bt->name() )
{
    // Let's make the alternative structure of Read types now so that we
    // don't have to do it on the fly.
    Vars_iter p = bt->var_begin();
    while (p != bt->var_end()) {
	BaseType *v = *p ;
        BaseType *new_bt = v->ptr_duplicate() ;
        add_var(new_bt);
        delete new_bt;
        p++;
    }

    BaseType::set_send_p(bt->send_p());
}

ReadSequence::~ReadSequence()
{
}

void
ReadSequence::intern_data(ConstraintEvaluator & eval, DDS & dds)
{
}

