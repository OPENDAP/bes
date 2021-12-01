
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

// Implementation for WWWArray. See WWWByte.cc
//
//  4/7/99 jhrg

#include "config.h"

static char rcsid[] not_used =
    { "$Id$" };

#include <iostream>
#include <sstream>
#include <string>

#include <libdap/InternalErr.h>
#include <libdap/escaping.h>

#include "WWWArray.h"
#include "WWWOutput.h"
#include "get_html_form.h"

using namespace dap_html_form;

BaseType *WWWArray::ptr_duplicate()
{
    return new WWWArray(*this);
}

WWWArray::WWWArray(const string & n, BaseType * v): Array(n, v), _redirect(0)
{
}

WWWArray::WWWArray(Array * bt) : Array( bt->name(), 0), _redirect(bt)
{
    BaseType *abt = basetype_to_wwwtype(bt->var());
    abt->set_attr_table(bt->get_attr_table());
    add_var(abt);
    // add_var makes a copy of the base type passed to it, so delete it here
    delete abt;

    // Copy the dimensions
    Dim_iter p = bt->dim_begin();
    while ( p != bt->dim_end() ) {
        append_dim(bt->dimension_size(p, true), bt->dimension_name(p));
        ++p;
    }
}

WWWArray::~WWWArray()
{
}

void
WWWArray::do_print_val( ostream &ss )
{
    const string fqn = get_fqn(this);
    ss << "<script type=\"text/javascript\">\n"
        << "<!--\n"
        << name_for_js_code(fqn) << " = new dods_var(\""
        << id2www_ce(fqn)
        << "\", \"" << name_for_js_code(fqn) << "\", 1);\n"
        << "DODS_URL.add_dods_var(" << name_for_js_code(fqn) << ");\n"
        << "// -->\n" << "</script>\n";

    // Slight formatting change: The typename is no longer bold. 10/28/08
    ss << "<b>"
        << "<input type=\"checkbox\" name=\"get_" <<
        name_for_js_code(fqn)
        << "\"\n" << "onclick=\"" << name_for_js_code(fqn)
        << ".handle_projection_change(get_"
        << name_for_js_code(fqn) << ") \"  onfocus=\"describe_projection()\">\n"
        << "<font size=\"+1\">" << name() << "</font></b>"
        << ": " << fancy_typename(this) << "<br>\n\n";

    Dim_iter p = dim_begin();
    for (int i = 0; p != dim_end(); ++i, ++p) {
        int size = dimension_size(p, true);
        string n = dimension_name(p);
        if (n != "")
            ss << n << ":";
        ss << "<input type=\"text\" name=\"" << name_for_js_code(fqn)
            << "_" << i
            << "\" size=8 onfocus=\"describe_index()\""
            << " onChange=\"DODS_URL.update_url()\">\n";
        ss << "<script type=\"text/javascript\">\n"
            << "<!--\n"
            << name_for_js_code(fqn) << ".add_dim(" << size << ");\n"
            << "// -->\n" << "</script>\n";
    }

    ss << "<br>\n\n";
}

void
WWWArray::print_val(FILE * os, string, bool /*print_decl_p */ )
{
    ostringstream ss ;
    do_print_val( ss ) ;
    fprintf(os, "%s", ss.str().c_str());
}

void
WWWArray::print_val(ostream &strm, string, bool /*print_decl_p */ )
{
    ostringstream ss ;
    do_print_val( strm ) ;
}

