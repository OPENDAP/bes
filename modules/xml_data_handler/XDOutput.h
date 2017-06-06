
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

// An interface which can be used to print xml_data representations of DAP
// variables. 9/12/2001 jhrg

#ifndef _xdoutput_h
#define _xdoutput_h 1

#include <vector>

namespace libdap {
	class BaseType;
	class XMLWriter;
}

class XDOutput {

    friend class XDOutputTest;

protected:
    libdap::BaseType *d_redirect;

public:
    /**
     * Build an instance of XDOutput and set the redirect field to 'bt'
     * @param bt
     */
    XDOutput( libdap::BaseType *bt ) : d_redirect( bt ) { }

    /**
     * Build an instance of XDOutput. The redirect field is null.
     * @return
     */
    XDOutput() : d_redirect( 0 ) { }

    virtual ~XDOutput() {}

    virtual bool increment_state(vector<int> *state, const vector<int> &shape);

    virtual void start_xml_declaration(libdap::XMLWriter *writer, const char *element = 0);
    virtual void end_xml_declaration(libdap::XMLWriter *writer);

    virtual void print_xml_data(libdap::XMLWriter *writer, bool show_type);
};

#endif
