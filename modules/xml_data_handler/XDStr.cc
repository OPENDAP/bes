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

// Implementation for XDStr. See XDByte.cc
//
// 3/12/98 jhrg

#include "config.h"

// #include <string>

#include <libdap/InternalErr.h>
#include <BESDebug.h>

#include "XDStr.h"

using namespace libdap;

BaseType *
XDStr::ptr_duplicate()
{
	return new XDStr(*this);
}

void XDStr::print_xml_data(XMLWriter *writer, bool show_type)
{
	BESDEBUG("xd", "Entering XDStr::print_xml_data" << endl);

	Str *s = dynamic_cast<Str*>(d_redirect);
#if ENABLE_UNIT_TESTS
	if (!s) s = dynamic_cast<Str *>(this);
#else
	if (!s)
	throw InternalErr(__FILE__, __LINE__, "d_redirect is null.");
#endif

	if (show_type) start_xml_declaration(writer);

	// Write the element for the value, then the value
	BESDEBUG("xd", "XDStr::print_xml_data, value = '" << s->value() << "'." << endl);
	if (xmlTextWriterWriteElement(writer->get_writer(), (const xmlChar*) "value", (const xmlChar*) s->value().c_str())
			< 0) throw InternalErr(__FILE__, __LINE__, "Could not write value element for " + s->name());

	if (show_type) end_xml_declaration(writer);
}
