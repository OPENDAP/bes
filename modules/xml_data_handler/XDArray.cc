
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

// Implementation for XDArray. See XDByte.cc
//
// 3/12/98 jhrg

#include "config.h"

#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>

using namespace std;

//#define DODS_DEBUG
#include <BESDebug.h>

#include <InternalErr.h>
#include <escaping.h>
#include <util.h>
#include <debug.h>

#include "XDArray.h"
#include "get_xml_data.h"

using namespace xml_data;

BaseType *
XDArray::ptr_duplicate()
{
    return new XDArray(*this);
}

XDArray::XDArray(const string &n, BaseType *v) : Array(n, v)
{
}

XDArray::XDArray( Array *bt )
    : Array(bt->name(), 0), XDOutput( bt )
{
    // By calling var() without any parameters we get back the template
    // itself, then we can add it to this Array as the template. By doing
    // this we set the parent as well, which is what we need.
    BaseType *abt = basetype_to_xd( bt->var() ) ;
    add_var( abt ) ;
    // add_var makes a copy of the base type passed, so delete the original
    delete abt ;

    // Copy the dimensions
    Dim_iter p = bt->dim_begin();
    while ( p != bt->dim_end() ) {
        append_dim(bt->dimension_size(p, true), bt->dimension_name(p));
        ++p;
    }

    // I'm not particularly happy with this constructor; we should be able to
    // use the libdap ctors like BaseType::BaseType(const BaseType &copy_from)
    // using that via the Array copy ctor won't let us use the
    // basetype_to_asciitype() factory class. jhrg 5/19/09
    set_send_p(bt->send_p());
}

XDArray::~XDArray()
{
}

/**
 * Print an array as a Map.
 * @note For use with XDGrid
 * @param writer
 * @param show_type Ignored
 */
void XDArray::print_xml_map_data(XMLWriter *writer, bool /*show_type*/) throw(InternalErr)
{
    if (var()->is_simple_type()) {
        if (dimensions(true) > 1) {
            // We might have n-dimensional maps someday...
            m_print_xml_array(writer, "Map");
        }
        else {
            m_print_xml_vector(writer, "Map");
        }
    }
    else {
	throw InternalErr(__FILE__, __LINE__, "A Map must be a simple type.");
    }
}

void XDArray::print_xml_data(XMLWriter *writer, bool /*show_type*/) throw(InternalErr)
{
    BESDEBUG("xd", "Entering XDArray::print_xml_data" << endl);

    if (var()->is_simple_type()) {
        if (dimensions(true) > 1)
            m_print_xml_array(writer, "Array");
        else
            m_print_xml_vector(writer, "Array");
    }
    else {
	m_print_xml_complex_array(writer, "Array");
    }
}

class PrintArrayDimXML : public unary_function<Array::dimension&, void>
{
    XMLWriter *d_writer;
    bool d_constrained;

public:
    PrintArrayDimXML(XMLWriter *writer, bool c)
            : d_writer(writer), d_constrained(c)
    {}

    void operator()(Array::dimension &d)
    {
	int size = d_constrained ? d.c_size : d.size;
	if (d.name.empty()) {
	    if (xmlTextWriterStartElement(d_writer->get_writer(),  (const xmlChar*) "dimension") < 0)
		throw InternalErr(__FILE__, __LINE__, "Could not write dimension element");
	    if (xmlTextWriterWriteFormatAttribute(d_writer->get_writer(), (const xmlChar*)"size", "%d", size) < 0)
		throw InternalErr(__FILE__, __LINE__, "Could not write attribute for size");
	    if (xmlTextWriterEndElement(d_writer->get_writer()) < 0)
		throw InternalErr(__FILE__, __LINE__, "Could not end dimension element");
	}
	else {
	    string id_name = id2xml(d.name);
	    if (xmlTextWriterStartElement(d_writer->get_writer(), (const xmlChar*) "dimension") < 0)
		throw InternalErr(__FILE__, __LINE__, "Could not write dimension element");
	    if (xmlTextWriterWriteAttribute(d_writer->get_writer(), (const xmlChar*) "name", (const xmlChar*)id_name.c_str()) < 0)
		throw InternalErr(__FILE__, __LINE__, "Could not write attribute for name");
	    if (xmlTextWriterWriteFormatAttribute(d_writer->get_writer(), (const xmlChar*) "size", "%d", size) < 0)
		throw InternalErr(__FILE__, __LINE__, "Could not write attribute for size");
	    if (xmlTextWriterEndElement(d_writer->get_writer()) < 0)
		throw InternalErr(__FILE__, __LINE__, "Could not end dimension element");
	}
    }
};

void XDArray::start_xml_declaration(XMLWriter *writer, const char *element)  throw(InternalErr)
{
    // Start the Array element (includes the name)
    if (xmlTextWriterStartElement(writer->get_writer(), (element != 0) ? (const xmlChar*)element : (const xmlChar*)"Array") < 0)
	throw InternalErr(__FILE__, __LINE__, "Could not write Array element '" + ((element != 0) ? string(element): string("Array")) + "' for " + name());
    if (xmlTextWriterWriteAttribute(writer->get_writer(), (const xmlChar*) "name", (const xmlChar*)name().c_str()) < 0)
	throw InternalErr(__FILE__, __LINE__, "Could not write attribute for " + name());

    // Start and End the Type element/s
    dynamic_cast<XDOutput&>(*var()).start_xml_declaration(writer);

    end_xml_declaration(writer);

    for_each(dim_begin(), dim_end(), PrintArrayDimXML(writer, true));
}

// Print out a values for a vector (one dimensional array) of simple types.
void XDArray::m_print_xml_vector(XMLWriter *writer, const char *element)
{
    BESDEBUG("xd", "Entering XDArray::m_print_xml_vector" << endl);

    start_xml_declaration(writer, element);

    // only one dimension
    int end = dimension_size(dim_begin(), true);
    m_print_xml_row(writer, 0, end);

    end_xml_declaration(writer);
}

void XDArray::m_print_xml_array(XMLWriter *writer, const char *element)
{
    BESDEBUG("xd", "Entering XDArray::m_print_xml_array" << endl);

    int dims = dimensions(true);
    if (dims <= 1)
        throw InternalErr(__FILE__, __LINE__, "Dimension count is <= 1 while printing multidimensional array.");

    start_xml_declaration(writer, element);

    // shape holds the maximum index value of all but the last dimension of
    // the array (not the size; each value is one less that the size).
    vector < int >shape = get_shape_vector(dims - 1);
    int rightmost_dim_size = get_nth_dim_size(dims - 1);

    // state holds the indexes of the current row being printed. For an N-dim
    // array, there are N-1 dims that are iterated over when printing (the
    // Nth dim is not printed explicitly. Instead it's the number of values
    // on the row).
    vector < int >state(dims - 1, 0);

    bool more_indices;
    int index = 0;
    do {
        for (int i = 0; i < dims - 1; ++i) {
	    if (xmlTextWriterStartElement(writer->get_writer(), (const xmlChar*) "dim") < 0)
		throw InternalErr(__FILE__, __LINE__, "Could not write Array element for " + name());
	    if (xmlTextWriterWriteFormatAttribute(writer->get_writer(), (const xmlChar*) "number", "%d", i) < 0)
		throw InternalErr(__FILE__, __LINE__, "Could not write number attribute for " + name() + ": " + long_to_string(i));
	    if (xmlTextWriterWriteFormatAttribute(writer->get_writer(), (const xmlChar*) "index",  "%d", state[i]) < 0)
		throw InternalErr(__FILE__, __LINE__, "Could not write index attribute for " + name());
	}

	index = m_print_xml_row(writer, index, rightmost_dim_size);

        for (int i = 0; i < dims - 1; ++i) {
	    if (xmlTextWriterEndElement(writer->get_writer()) < 0)
		throw InternalErr(__FILE__, __LINE__, "Could not end element for " + name());
	}

        more_indices = increment_state(&state, shape);
    } while (more_indices);

    end_xml_declaration(writer);
}

/** Print a single row of values for a N-dimensional array. Since we store
    N-dim arrays in vectors, #index# gives the starting point in that vector
    for this row and #number# is the number of values to print. The counter
    #index# is returned.

    @note This is called only for simple types.

    @note About 'redirect': The d_redirect field is used here because this is
    where actual values are needed. In this handler the data values have
    already been read and stored in the BaseType variables from the source
    (likely another handler such as the netCDF or HDF4 handler). Instead of
    allocating more memory to hold a copy of those data, the original BaseType
    objects are referenced using the d_redirect field. This field is also used
    in the simple types, which are all printed using XDOutput::print_xml_data().
    This is not done for complex types because those print data using either
    this method or the XDOutput version. But see XDSequence for an exception...

    @note That in testing instances of the various XDtypes are made directly,
    without any sibling BaseType. In those cases the XDtype variables are holding
    the data so we fool the code here.

    @param writer Write to this xml sink.
    @param index Print values starting from this point.
    @param number Print this many values.
    @return One past the last value printed (i.e., the index of the next
    row's first value).
    @see print\_array */
int XDArray::m_print_xml_row(XMLWriter *writer, int index, int number)
{
    Array *a = dynamic_cast<Array*>(d_redirect);
#if ENABLE_UNIT_TESTS
    if (!a)
	a = this;
#else
    if (!a)
	throw InternalErr(__FILE__, __LINE__, "d_redirect is null");
#endif

    BESDEBUG("xd", "Entering XDArray::m_print_xml_row" << endl);
    for (int i = 0; i < number; ++i) {
	// Must use the variable that holds the data here!
        BaseType *curr_var = basetype_to_xd(a->var(index++));
        dynamic_cast < XDOutput & >(*curr_var).print_xml_data(writer, false);
        // we're not saving curr_var for future use, so delete it here
        delete curr_var;
    }

    return index;
}

// Given a vector of indices, return the corresponding index.

int XDArray::m_get_index(vector < int >indices) throw(InternalErr)
{
    if (indices.size() != dimensions(true)) {
        throw InternalErr(__FILE__, __LINE__,
                          "Index vector is the wrong size!");
    }
    // suppose shape is [3][4][5][6] for x,y,z,t. The index is
    // t + z(6) + y(5 * 6) + x(4 * 5 *6).
    // Assume that indices[0] holds x, indices[1] holds y, ...

    // It's hard to work with Pixes
    vector < int >shape = get_shape_vector(indices.size());

    // We want to work from the rightmost index to the left
    reverse(indices.begin(), indices.end());
    reverse(shape.begin(), shape.end());

    vector < int >::iterator indices_iter = indices.begin();
    vector < int >::iterator shape_iter = shape.begin();

    int index = *indices_iter++;        // in the ex. above, this adds `t'
    int multiplier = 1;
    while (indices_iter != indices.end()) {
        multiplier *= *shape_iter++;
        index += multiplier * *indices_iter++;
    }

    return index;
}

// get_shape_vector and get_nth_dim_size are public because that are called
// from Grid. 9/14/2001 jhrg

/** Get the sizes of the first N dimensions of this array. This
    `shape vector' may be used in all sorts of output formatters.

     @return A vector describing the shape of the array. Each value
     contains the highest index value. To get the size, add one. */
vector < int > XDArray::get_shape_vector(size_t n) throw(InternalErr)
{
    if (n < 1 || n > dimensions(true)) {
        string msg = "Attempt to get ";
        msg += long_to_string(n) + " dimensions from " + name()
            + " which has only " + long_to_string(dimensions(true))
            + " dimensions.";

        throw InternalErr(__FILE__, __LINE__, msg);
    }

    vector < int >shape;
    Array::Dim_iter p = dim_begin();
    for (unsigned i = 0; i < n && p != dim_end(); ++i, ++p) {
        shape.push_back(dimension_size(p, true));
    }

    return shape;
}

/** Get the size of the Nth dimension. The first dimension is N == 0.
    @param n The index. Uses sero-based indexing.
    @return the size of the dimension. */
int XDArray::get_nth_dim_size(size_t n) throw(InternalErr)
{
    if (n > dimensions(true) - 1) {
        string msg = "Attempt to get dimension ";
        msg +=
            long_to_string(n + 1) + " from `" + name() +
            "' which has " + long_to_string(dimensions(true)) +
            " dimension(s).";
        throw InternalErr(__FILE__, __LINE__, msg);
    }

    return dimension_size(dim_begin() + n, true);
}

void XDArray::m_print_xml_complex_array(XMLWriter *writer, const char *element)
{
    start_xml_declaration(writer, element);

    int dims = dimensions(true);
    if (dims < 1)
        throw InternalErr(__FILE__, __LINE__, "Dimension count is < 1 while printing an array.");

    // shape holds the maximum index value of all but the last dimension of
    // the array (not the size; each value is one less that the size).
    vector < int >shape = get_shape_vector(dims);

    vector < int >state(dims, 0);

    bool more_indices = true;
    do {
        for (int i = 0; i < dims - 1; ++i) {
	    if (xmlTextWriterStartElement(writer->get_writer(), (const xmlChar*) "dim") < 0)
		throw InternalErr(__FILE__, __LINE__, "Could not write Array element for " + name());
	    if (xmlTextWriterWriteFormatAttribute(writer->get_writer(), (const xmlChar*) "number", "%d", i) < 0)
		throw InternalErr(__FILE__, __LINE__, "Could not write number attribute for " + name() + ": " + long_to_string(i));
	    if (xmlTextWriterWriteFormatAttribute(writer->get_writer(), (const xmlChar*) "index", "%d", state[i]) < 0)
		throw InternalErr(__FILE__, __LINE__, "Could not write index attribute for " + name());
	}

        BaseType *curr_var = basetype_to_xd(var(m_get_index(state)));
        dynamic_cast < XDOutput & >(*curr_var).print_xml_data(writer, true);
        // we are not saving curr_var for future reference, so delete it
        delete curr_var;

        for (int i = 0; i < dims - 1; ++i) {
	    if (xmlTextWriterEndElement(writer->get_writer()) < 0)
		throw InternalErr(__FILE__, __LINE__, "Could not end element for " + name());
	}

        more_indices = increment_state(&state, shape);

    } while (more_indices);

    end_xml_declaration(writer);
}
