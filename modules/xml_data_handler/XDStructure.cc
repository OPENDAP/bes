
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

// Authors:
//      jhrg,jimg       James Gallagher <jgallagher@gso.uri.edu>

// Implementation for the class XDStructure. See XDByte.cc
//
// 3/12/98 jhrg

#include "config.h"

#include <string>

#include <BESDebug.h>

#include <InternalErr.h>

#include "XDStructure.h"
#include "XDSequence.h"
#include "get_xml_data.h"

using namespace xml_data;
using namespace libdap;

BaseType *
XDStructure::ptr_duplicate()
{
    return new XDStructure(*this);
}

XDStructure::XDStructure(const string &n) : Structure(n)
{
}

XDStructure::XDStructure( Structure *bt )
    : Structure( bt->name() ), XDOutput( bt )
{
    // Let's make the alternative structure of XD types now so that we
    // don't have to do it on the fly. This will also set the parents of
    // each of the underlying vars of the structure.
    Vars_iter p = bt->var_begin();
    while (p != bt->var_end()) {
        BaseType *new_bt = basetype_to_xd(*p);
        add_var(new_bt);
        // add_var makes a copy of the base type passed to it, so delete
        // it here
        delete new_bt;
        p++;
    }

    BaseType::set_send_p(bt->send_p());
}

XDStructure::~XDStructure()
{
}

void
XDStructure::start_xml_declaration(XMLWriter *writer, const char *element)
{
    XDOutput::start_xml_declaration(writer, element);

    for (Vars_iter p = var_begin(); p != var_end(); ++p) {
        if ((*p)->send_p()) {
            dynamic_cast<XDOutput&>(**p).start_xml_declaration(writer, element);
            dynamic_cast<XDOutput&>(**p).end_xml_declaration(writer);
        }
    }
}

void
XDStructure::print_xml_data(XMLWriter *writer, bool show_type)
{
    // Forcing the use of the generic version prints just the <Structure>
    // element w/o the type information of the components. That will be printed
    // by the embedded print_xml_data calls.
    if (show_type)
	XDOutput::start_xml_declaration(writer);

    for (Vars_iter p = var_begin(); p != var_end(); ++p) {
        if ((*p)->send_p()) {
            dynamic_cast<XDOutput&> (*(*p)).print_xml_data(writer, show_type);
        }
    }

    // End the <Structure> element
    if (show_type)
	end_xml_declaration(writer);
}
