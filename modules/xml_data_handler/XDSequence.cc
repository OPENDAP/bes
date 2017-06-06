
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

#include <iostream>
#include <string>

#include <BESDebug.h>

#include <InternalErr.h>
#include <util.h>
#include <debug.h>

#include "XDSequence.h"
#include "XDStructure.h"
#include "get_xml_data.h"

using std::endl ;
using namespace xml_data;
using namespace libdap;

BaseType *
XDSequence::ptr_duplicate()
{
    return new XDSequence(*this);
}

XDSequence::XDSequence(const string &n) : Sequence(n)
{
}

XDSequence::XDSequence(Sequence * bt)
    : Sequence( bt->name() ), XDOutput( bt )
{
    // Let's make the alternative structure of XD types now so that we
    // don't have to do it on the fly.
    Vars_iter p = bt->var_begin();
    while (p != bt->var_end()) {
        BaseType *new_bt = basetype_to_xd(*p);
        add_var(new_bt);
        delete new_bt;
        p++;
    }

    BaseType::set_send_p(bt->send_p());
}

XDSequence::~XDSequence()
{
}

int
XDSequence::length() const
{
    return -1;
}

// This specialization is different from the Sequence version only in that
// it tests '(*iter)->send_p()' before incrementing 'i' by
// '(*iter)->element_count(true)'.
int
XDSequence::element_count(bool leaves)
{
    if (!leaves)
        return d_vars.size();
    else {
        int i = 0;
        for (Vars_iter iter = d_vars.begin(); iter != d_vars.end(); iter++) {
            if ((*iter)->send_p())
                i += (*iter)->element_count(true);
        }
        return i;
    }
}
void
XDSequence::start_xml_declaration(XMLWriter *writer, const char *element)
{
    XDOutput::start_xml_declaration(writer);

    for (Vars_iter p = var_begin(); p != var_end(); ++p) {
        if ((*p)->send_p()) {
            dynamic_cast<XDOutput&>(**p).start_xml_declaration(writer, element);
            dynamic_cast<XDOutput&>(**p).end_xml_declaration(writer);
        }
    }
}

void
XDSequence::print_xml_data(XMLWriter *writer, bool show_type)
{
    // Forcing the use of the generic version prints just the <Structure>
    // element w/o the type information of the components. That will be printed
    // by the embedded print_xml_data calls.
    if (show_type)
	XDOutput::start_xml_declaration(writer);

    Sequence *seq = dynamic_cast<Sequence*>(d_redirect);
    if (!seq)
        seq = this;

    const int rows = seq->number_of_rows() /*- 1*/;
    const int elements = seq->element_count() /*- 1*/;

    // For each row of the Sequence...
    for (int i = 0; i < rows; ++i) {
	BESDEBUG("yd", "Working on the " << i << "th row" << endl);
	// Print the row information
	if (xmlTextWriterStartElement(writer->get_writer(), (const xmlChar*) "row") < 0)
	    throw InternalErr(__FILE__, __LINE__, "Could not write Array element for " + name());
	if (xmlTextWriterWriteFormatAttribute(writer->get_writer(), (const xmlChar*) "number", "%d", i) < 0)
	    throw InternalErr(__FILE__, __LINE__, "Could not write number attribute for " + name());

	// For each variable of the row...
	for (int j = 0; j < elements; ++j) {
	    BESDEBUG("yd", "Working on the " << j << "th field" << endl);
	    BaseType *bt_ptr = seq->var_value(i, j);
	    BaseType *abt_ptr = basetype_to_xd(bt_ptr);
	    dynamic_cast<XDOutput&>(*abt_ptr).print_xml_data(writer, true);
	    BESDEBUG("yd", "Back from print xml data." << endl);
	    // abt_ptr is not stored for future use, so delete it
	    delete abt_ptr;
	}

	// Close the row element
	if (xmlTextWriterEndElement(writer->get_writer()) < 0)
	    throw InternalErr(__FILE__, __LINE__, "Could not end element for " + name());
    }

    // End the <Structure> element
    if (show_type)
	end_xml_declaration(writer);
}
