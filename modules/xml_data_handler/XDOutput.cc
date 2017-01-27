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

#include "config.h"

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#include <iostream>
#include <sstream>

#include <vector>
#include <algorithm>
#include <iterator>

//#define DODS_DEBUG

#include <BaseType.h>
#include <debug.h>

#include <BESInternalFatalError.h>
#include <BESDebug.h>

#include "XDOutput.h"
#include "get_xml_data.h"

//using namespace xml_data;
using namespace std;
using namespace libdap;

void XDOutput::start_xml_declaration(XMLWriter *writer, const char *element)
{
    BaseType *btp = dynamic_cast<BaseType *>(this);
    if (!btp)
        throw InternalErr(__FILE__, __LINE__, "Expected a BaseType instance");

    if (xmlTextWriterStartElement(writer->get_writer(),
            (element != 0) ? (const xmlChar*) element : (const xmlChar*) btp->type_name().c_str()) < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not write element for " + btp->name());

    if (xmlTextWriterWriteAttribute(writer->get_writer(), (const xmlChar*) "name",
            (const xmlChar*) (btp->name().c_str())) < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not write attribute 'name' for " + btp->name());
}

void XDOutput::end_xml_declaration(XMLWriter *writer)
{
    BaseType *btp = dynamic_cast<BaseType *>(this);
    if (!btp)
        throw InternalErr(__FILE__, __LINE__, "Expected a BaseType instance");

    if (xmlTextWriterEndElement(writer->get_writer()) < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not end element for " + btp->name());
}

void XDOutput::print_xml_data(XMLWriter *writer, bool show_type)
{
    BESDEBUG("xd", "Entering XDOutput::print_xml_data" << endl);

    BaseType *btp = d_redirect;
    if (!btp) btp = dynamic_cast<BaseType *>(this);
    if (!btp) throw BESInternalFatalError("Expected a valid BaseType instance", __FILE__, __LINE__);

    if (show_type)
        start_xml_declaration(writer);

    // Write the element for the value, then the value
    ostringstream oss;
    btp->print_val(oss, "", false);
    BESDEBUG("xd", "XDOutput::print_xml_data, value = '" << oss.str() << "'." << endl);
    if (xmlTextWriterWriteElement(writer->get_writer(), (const xmlChar*) "value", (const xmlChar*) oss.str().c_str())
            < 0)
        throw InternalErr(__FILE__, __LINE__, "Could not write value element for " + btp->name());

    if (show_type)
        end_xml_declaration(writer);
}

/** Increment #state# to the next value given #shape#. This method
 uses simple modulo arithmetic to provide a way to iterate over all
 combinations of dimensions of an Array or Grid. The vector #shape#
 holds the maximum sizes of each of N dimensions. The vector #state#
 holds the current index values of those N dimensions. Calling this
 method increments #state# to the next dimension, varying the
 right-most fastest.

 To print DODS Array and Grid objects according to the DAP 2.0
 specification, #state# and #shape# should be vectors of length N-1
 for an object of dimension N.

 For example, if shape holds 10, 20 then when state holds 0, 20
 calling this method will increment state to 1, 0. For this example,
 calling the method with state equal to 10, 20 will reset state to 0, 0 and
 the return value will be false.

 @param state A pointer to the current state vector, a value-result parameter
 @param share A reference to a vector of the dimension sizes.

 @return True if there are more states, false if not. */
bool XDOutput::increment_state(vector<int>*state, const vector<int>&shape)
{

    DBG(cerr << "Entering increment_state" << endl);

    vector<int>::reverse_iterator state_riter;
    vector<int>::const_reverse_iterator shape_riter;
    for (state_riter = state->rbegin(), shape_riter = shape.rbegin(); state_riter < state->rend();
            state_riter++, shape_riter++) {
        if (*state_riter == *shape_riter - 1) {
            *state_riter = 0;
        }
        else {
            *state_riter = *state_riter + 1;
#if 0
            cerr << "New value of state: ";
            copy(state->begin(), state->end(),
                    ostream_iterator<int>(cerr, ", "));
            cerr << endl;
#endif
            return true;
        }
    }

    return false;
}
