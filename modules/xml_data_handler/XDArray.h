
// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of xml_data_handler, software which can return an XML data
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

// Interface definition for XDArray. See XDByte.h for more information
//
// 3/12/98 jhrg

#ifndef _xdarray_h
#define _xdarray_h 1

#include <Array.h>

#include "XDOutput.h"

using namespace libdap ;

class XDArray: public Array, public XDOutput {
private:
    int m_get_index(vector<int> indices) throw(InternalErr);

    void m_print_xml_vector(XMLWriter *writer, const char *element);
    void m_print_xml_array(XMLWriter *writer, const char *element);
    void m_print_xml_complex_array(XMLWriter *writer, const char *element);

    int m_print_xml_row(XMLWriter *writer, int index, int number);

    friend class XDArrayTest;

public:
    XDArray(const string &n, BaseType *v);
    XDArray( Array *bt ) ;
    virtual ~XDArray();

    virtual BaseType *ptr_duplicate();

    int get_nth_dim_size(size_t n) throw(InternalErr);

    vector<int> get_shape_vector(size_t n) throw(InternalErr);

    virtual void start_xml_declaration(XMLWriter *writer, const char *element = 0)  throw(InternalErr);

    void print_xml_map_data(XMLWriter *writer, bool show_type) throw(InternalErr);

    void print_xml_data(XMLWriter *writer, bool show_type) throw(InternalErr);

};

#endif
